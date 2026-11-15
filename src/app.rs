// Location: src/app.rs
// Function: Top-level eframe/egui Dopamine Desktop application orchestrating
// frameless window chrome, audio engine, catalog/playlists/queue controllers,
// session interconnect, dynamic accent themes, and modal workflows.
// All Rights Reserved isgaar

use std::path::Path;
use std::time::{Duration, Instant};

use egui::{
    ColorImage, Context, CornerRadius, Frame, Key, Pos2, Rect, Stroke, TextureHandle,
    TextureOptions, Vec2, ViewportCommand,
};

use crate::audio::AudioPlayer;
use crate::core::config::{ProjectConfig, SourceKind};
use crate::core::global_menu::{GlobalMenuAction, GlobalMenuService, GlobalMenuSnapshot};
use crate::core::lrc::{parse_lrc, LyricLine};
use crate::core::settings::{SettingsData, SettingsStore};
use crate::core::single_instance::SingleInstanceGuard;
use crate::data::model::Track;
use crate::data::remote::DopamineApiClient;
use crate::features::interconnect::{InterconnectAction, SessionPlaybackController};
use crate::features::library::LibraryController;
use crate::features::playlists::PlaylistController;
use crate::features::queue::QueueController;
use crate::session::controller::{SessionController, SessionEvent};
use crate::session::store::SessionStore;
use crate::shared::colors::ThemePalette;
use crate::shared::i18n::StringCatalog;
use crate::ui::chrome::{handle_frameless_resize, render_title_bar, TitleBarAction};
use crate::ui::modals::{render_lyrics_drawer, render_modals, ModalAction, ModalState};
use crate::ui::player_bar::{render_player_bar, PlayerBarAction};
use crate::ui::sidebar::{render_sidebar, SidebarAction};
use crate::ui::theme::{apply_palette, ease_out_cubic, install_fonts, RADIUS_MD};
use crate::ui::views::{render_content_area, ContentAction, ViewUiState};
use crate::ui::visualizer::{load_presets, render_visualizer, VisualizerPreset};

const APP_ICON_PNG: &[u8] = include_bytes!("shared/assets/app-icon.png");

pub struct DopamineApp {
    config: ProjectConfig,
    settings_store: SettingsStore,
    settings: SettingsData,
    base_palette: ThemePalette,
    palette: ThemePalette,
    strings: StringCatalog,
    single_instance: Option<SingleInstanceGuard>,
    session: SessionController,
    player: AudioPlayer,
    library: LibraryController,
    queue: QueueController,
    playlists: PlaylistController,
    interconnect: SessionPlaybackController,
    presets: Vec<VisualizerPreset>,
    active_view: String,
    view_state: ViewUiState,
    modal_state: ModalState,
    show_recent_popup: bool,
    show_lyrics: bool,
    show_visualizer: bool,
    app_icon_tex: Option<TextureHandle>,
    lyrics_lines: Vec<LyricLine>,
    lyrics_track_id: i64,
    sleep_deadline: Option<Instant>,
    global_menu: GlobalMenuService,
}

impl DopamineApp {
    pub fn new(
        cc: &eframe::CreationContext<'_>,
        single_instance: Option<SingleInstanceGuard>,
        initial_uri: Option<String>,
    ) -> Self {
        install_fonts(&cc.egui_ctx);

        let settings_store = SettingsStore::default();
        let settings = settings_store.load();
        let base_palette = ThemePalette::load(&settings.theme);
        let palette = base_palette.clone();
        apply_palette(&cc.egui_ctx, &palette);

        let strings = StringCatalog::new(&settings.language);
        let app_icon_tex = load_app_icon(&cc.egui_ctx);

        let mut config = ProjectConfig::default();
        apply_settings_to_config(&settings, &mut config);

        let mut session = SessionController::new(SessionStore::default());
        if let Some(ref uri) = initial_uri {
            let _ = session.handle_uri(uri);
        }

        if config.source_type == SourceKind::Server
            && !session.server_url.is_empty()
            && settings.server_url.trim().is_empty()
        {
            let _ = config.set_api_url(&session.server_url);
        }

        let player = AudioPlayer::spawn(settings.volume, settings.equalizer.clone());
        player.update_audio_options(
            settings.volume_stabilizer,
            settings.crossfade_enabled,
            settings.crossfade_sec,
            settings.skip_silence,
            settings.buffer_ms,
        );

        let mut modal_state = ModalState::default();
        if settings.is_first_run {
            modal_state.open_wizard(&settings);
        }

        let queue = QueueController::new();
        let initial_menu_snapshot = GlobalMenuSnapshot {
            language: settings.language.clone(),
            is_playing: false,
            is_shuffle: queue.is_shuffle,
            is_repeat: queue.is_repeat,
            show_lyrics: false,
            show_visualizer: false,
            is_logged_in: session.user.is_logged_in,
        };
        let global_menu = GlobalMenuService::start(cc, initial_menu_snapshot);

        let mut app = Self {
            config,
            settings_store,
            settings,
            base_palette,
            palette,
            strings,
            single_instance,
            session,
            player,
            library: LibraryController::new(),
            queue,
            playlists: PlaylistController::new(),
            interconnect: SessionPlaybackController::new(),
            presets: load_presets(),
            active_view: String::from("library"),
            view_state: ViewUiState::new(),
            modal_state,
            show_recent_popup: false,
            show_lyrics: false,
            show_visualizer: false,
            app_icon_tex,
            lyrics_lines: Vec::new(),
            lyrics_track_id: 0,
            sleep_deadline: None,
            global_menu,
        };

        if app.session.user.is_logged_in {
            app.interconnect.start();
        }
        if app.config.is_source_configured() {
            app.reload_catalog();
        }
        app
    }

    fn api_client(&self) -> Option<DopamineApiClient> {
        if self.config.source_type != SourceKind::Server || self.config.api_url.trim().is_empty() {
            return None;
        }
        let mut client = DopamineApiClient::new(&self.config.api_url);
        if self.session.user.is_logged_in && !self.session.user.token.is_empty() {
            client = client.with_token(&self.session.user.token);
        }
        Some(client)
    }

    fn local_scan_dir(&self) -> Option<String> {
        match self.config.source_type {
            SourceKind::Local if !self.config.local_path.trim().is_empty() => {
                Some(self.config.local_path.clone())
            }
            SourceKind::Smb => self.config.resolve_smb_mount_path(),
            _ => None,
        }
    }

    fn reload_catalog(&mut self) {
        let client = self.api_client();
        let local = self.local_scan_dir();
        self.library
            .load_tracks(client.as_ref(), local.as_deref(), &self.settings, 1);
        self.library.load_recent(client.as_ref());
        self.playlists.load_all(client.as_ref());
    }

    fn start_playing_track(&mut self, track: Track, start_sec: f64, auto_play: bool) {
        let token = if self.session.user.is_logged_in && !self.session.user.token.is_empty() {
            Some(self.session.user.token.clone())
        } else {
            None
        };
        self.player.load_track(track, token, start_sec, auto_play);
        if auto_play {
            self.interconnect.notify_local_play_started();
        }
    }

    fn apply_theme_and_accent(&mut self, ctx: &Context) {
        let mut next_palette = self.base_palette.clone();
        if self.settings.dynamic_accent {
            if let Some(track) = self.queue.current_track().or(self.player.current_track.as_ref()) {
                if let Some(url) = track.cover_url.as_ref() {
                    if let Some(&accent) = self.library.cover_accents.get(url) {
                        next_palette.primary = accent;
                        next_palette.accent = accent;
                        next_palette.primary_dim = ThemePalette::with_alpha(accent, 0.22);
                    }
                }
            }
        }
        self.palette = next_palette;
        apply_palette(ctx, &self.palette);
    }

    fn refresh_lyrics_for_current_track(&mut self) {
        let Some(track) = self.queue.current_track().or(self.player.current_track.as_ref()) else {
            self.lyrics_lines.clear();
            self.lyrics_track_id = 0;
            return;
        };
        if self.lyrics_track_id == track.id {
            return;
        }
        self.lyrics_track_id = track.id;
        self.lyrics_lines.clear();

        if !track.path.is_empty() {
            let lrc_path = Path::new(&track.path).with_extension("lrc");
            if let Ok(raw) = std::fs::read_to_string(&lrc_path) {
                self.lyrics_lines = parse_lrc(&raw);
            }
        }
    }

    fn request_visible_covers(&mut self) {
        let client = self.api_client();
        let mut urls: Vec<String> = Vec::new();
        if let Some(t) = self.queue.current_track() {
            if let Some(ref u) = t.cover_url {
                urls.push(u.clone());
            }
        }
        for t in self.library.tracks.iter().take(60) {
            if let Some(ref u) = t.cover_url {
                urls.push(u.clone());
            }
        }
        for t in self.library.recent_tracks.iter().take(12) {
            if let Some(ref u) = t.cover_url {
                urls.push(u.clone());
            }
        }
        for url in urls {
            let _ = self.library.request_cover(Some(&url), client.as_ref());
        }
    }

    fn process_background_events(&mut self, ctx: &Context) {
        if let Some(ref guard) = self.single_instance {
            for msg in guard.poll_messages() {
                if msg == "RAISE" {
                    ctx.send_viewport_cmd(ViewportCommand::Focus);
                } else {
                    let _ = self.session.handle_uri(&msg);
                }
            }
        }

        for ev in self.session.drain_events() {
            match ev {
                SessionEvent::ServerAdopted(url) => {
                    self.config.set_server_source(&url);
                    self.settings.mode = String::from("server");
                    self.settings.server_url = url;
                    self.settings_store.save(&self.settings);
                    self.reload_catalog();
                }
                SessionEvent::LoginSucceeded(_) => {
                    self.interconnect.start();
                    self.reload_catalog();
                }
                SessionEvent::LoggedOut => {
                    self.interconnect.stop();
                    self.reload_catalog();
                }
                SessionEvent::SessionWindowRequested => {
                    self.modal_state.open_session(&self.session);
                }
                SessionEvent::WindowRaiseRequested => {
                    ctx.send_viewport_cmd(ViewportCommand::Focus);
                }
                SessionEvent::StateChanged => {}
            }
        }

        if let Some(_dl_path) = self.library.poll(ctx, &mut self.settings) {
            self.settings_store.save(&self.settings);
        }

        let client = self.api_client();
        self.queue.poll(&self.settings);
        self.playlists.poll(client.as_ref());

        // Advance when track completes naturally
        if self.player.take_finished() {
            let client_opt = self.api_client();
            if let Some(next_track) = self.queue.next(client_opt.as_ref(), &mut self.settings) {
                self.start_playing_track(next_track, 0.0, true);
            }
        }

        // Sleep timer expiry
        if let Some(deadline) = self.sleep_deadline {
            if Instant::now() >= deadline {
                self.sleep_deadline = None;
                self.player.pause();
            }
        }

        // Interconnect sync tick
        let queue_ids: Vec<i64> = self.queue.queue.iter().map(|t| t.id).collect();
        let current_track = self.queue.current_track().cloned();
        let actions = self.interconnect.tick(
            client.as_ref(),
            self.player.is_playing(),
            current_track.as_ref(),
            self.player.position_sec(),
            self.player.volume,
            &queue_ids,
        );
        for act in actions {
            match act {
                InterconnectAction::PauseLocal => {
                    self.player.pause();
                }
                InterconnectAction::TakeoverDesktop {
                    tracks,
                    current_index,
                    position_sec,
                    auto_play,
                } => {
                    if let Some(list) = tracks {
                        self.queue.apply_session_queue(list, current_index);
                    }
                    if let Some(track) = self.queue.current_track().cloned() {
                        self.start_playing_track(track, position_sec, auto_play);
                    }
                }
                InterconnectAction::RemoteSeek(pos) => {
                    self.player.seek(pos);
                }
                InterconnectAction::RemoteVolume(vol) => {
                    self.player.set_volume(vol);
                    self.settings.volume = vol;
                }
            }
        }

        self.request_visible_covers();
        self.refresh_lyrics_for_current_track();
        self.apply_theme_and_accent(ctx);

        self.global_menu.update_snapshot(self.current_menu_snapshot());
        for act in self.global_menu.poll_actions() {
            self.dispatch_global_menu_action(act, ctx);
        }
    }

    fn current_menu_snapshot(&self) -> GlobalMenuSnapshot {
        let is_playing = if self.interconnect.is_controlling_android {
            self.interconnect.remote_playing
        } else {
            self.player.is_playing()
        };
        GlobalMenuSnapshot {
            language: self.settings.language.clone(),
            is_playing,
            is_shuffle: self.queue.is_shuffle,
            is_repeat: self.queue.is_repeat,
            show_lyrics: self.show_lyrics,
            show_visualizer: self.show_visualizer,
            is_logged_in: self.session.user.is_logged_in,
        }
    }

    fn dispatch_global_menu_action(&mut self, action: GlobalMenuAction, ctx: &Context) {
        match action {
            GlobalMenuAction::OpenView(view) => {
                self.active_view = view;
                self.view_state.selected_playlist = None;
                self.view_state.selected_group_key = None;
            }
            GlobalMenuAction::PlayPause => self.toggle_play_or_remote(),
            GlobalMenuAction::NextTrack => self.next_track(),
            GlobalMenuAction::PreviousTrack => self.previous_track(),
            GlobalMenuAction::ToggleShuffle => {
                self.queue.is_shuffle = !self.queue.is_shuffle;
            }
            GlobalMenuAction::ToggleRepeat => {
                self.queue.is_repeat = !self.queue.is_repeat;
            }
            GlobalMenuAction::ToggleLyrics => {
                self.show_lyrics = !self.show_lyrics;
            }
            GlobalMenuAction::ToggleVisualizer => {
                self.show_visualizer = !self.show_visualizer;
            }
            GlobalMenuAction::NewPlaylist => {
                self.modal_state.show_create_playlist = true;
            }
            GlobalMenuAction::RefreshLibrary => {
                self.reload_catalog();
            }
            GlobalMenuAction::OpenEqualizer => {
                self.modal_state.show_equalizer = true;
            }
            GlobalMenuAction::OpenSleepTimer => {
                self.modal_state.show_sleep_timer = true;
            }
            GlobalMenuAction::OpenSession => {
                self.modal_state.open_session(&self.session);
            }
            GlobalMenuAction::OpenSettings => {
                self.modal_state.open_settings(&self.settings);
            }
            GlobalMenuAction::OpenAbout => {
                self.modal_state.show_about = true;
            }
            GlobalMenuAction::Quit => {
                ctx.send_viewport_cmd(ViewportCommand::Close);
            }
        }
    }

    fn handle_shortcuts(&mut self, ctx: &Context) {
        if ctx.wants_keyboard_input() {
            return;
        }
        ctx.input(|i| {
            if i.key_pressed(Key::Space) {
                self.toggle_play_or_remote();
            } else if i.modifiers.alt && i.key_pressed(Key::ArrowRight) {
                self.next_track();
            } else if i.modifiers.alt && i.key_pressed(Key::ArrowLeft) {
                self.previous_track();
            }
        });
    }

    fn toggle_play_or_remote(&mut self) {
        if self.interconnect.is_controlling_android {
            let client = self.api_client();
            let qids: Vec<i64> = self.queue.queue.iter().map(|t| t.id).collect();
            self.interconnect.toggle_remote_play(client.as_ref(), &qids);
        } else if self.player.current_track.is_some() {
            self.player.toggle_play();
        } else if let Some(first) = self.library.tracks.first().cloned() {
            let list = self.library.tracks.clone();
            let client = self.api_client();
            if let Some(track) = self.queue.play_from_library(&list, 0, client.as_ref(), &mut self.settings) {
                self.start_playing_track(track, 0.0, true);
            } else {
                self.start_playing_track(first, 0.0, true);
            }
        }
    }

    fn next_track(&mut self) {
        let client = self.api_client();
        if let Some(track) = self.queue.next(client.as_ref(), &mut self.settings) {
            self.start_playing_track(track, 0.0, true);
        }
    }

    fn previous_track(&mut self) {
        if self.player.position_sec() > 4.0 {
            self.player.seek(0.0);
            return;
        }
        let client = self.api_client();
        if let Some(track) = self.queue.previous(client.as_ref(), &mut self.settings) {
            self.start_playing_track(track, 0.0, true);
        }
    }

    fn apply_saved_settings(&mut self, next: SettingsData) {
        let source_changed = self.settings.mode != next.mode
            || self.settings.server_url != next.server_url
            || self.settings.folder_path != next.folder_path
            || self.settings.smb_host != next.smb_host
            || self.settings.smb_share != next.smb_share;

        self.settings = next;
        self.base_palette = ThemePalette::load(&self.settings.theme);
        self.strings.set_language(&self.settings.language);
        apply_settings_to_config(&self.settings, &mut self.config);
        self.player.update_equalizer(self.settings.equalizer.clone());
        self.player.update_audio_options(
            self.settings.volume_stabilizer,
            self.settings.crossfade_enabled,
            self.settings.crossfade_sec,
            self.settings.skip_silence,
            self.settings.buffer_ms,
        );
        self.settings_store.save(&self.settings);

        if source_changed {
            self.reload_catalog();
        }
    }
}

impl eframe::App for DopamineApp {
    fn clear_color(&self, _visuals: &egui::Visuals) -> [f32; 4] {
        [0.0, 0.0, 0.0, 0.0]
    }

    fn update(&mut self, ctx: &Context, _frame: &mut eframe::Frame) {
        self.process_background_events(ctx);
        self.handle_shortcuts(ctx);

        let is_playing_local = self.player.is_playing();
        if is_playing_local || self.show_visualizer {
            ctx.request_repaint_after(Duration::from_millis(16));
        } else {
            ctx.request_repaint_after(Duration::from_millis(200));
        }

        let is_maximized = ctx.input(|i| i.viewport().maximized.unwrap_or(false));
        let shell_radius = if is_maximized {
            CornerRadius::ZERO
        } else {
            CornerRadius::same(RADIUS_MD as u8)
        };

        egui::CentralPanel::default()
            .frame(Frame::NONE.fill(egui::Color32::TRANSPARENT))
            .show(ctx, |ui| {
                let outer_rect = ui.max_rect();
                ui.painter()
                    .rect_filled(outer_rect, shell_radius, self.palette.bg_darkest);
                if !is_maximized {
                    ui.painter().rect_stroke(
                        outer_rect,
                        shell_radius,
                        Stroke::new(1.0_f32, self.palette.border_dark),
                        egui::StrokeKind::Inside,
                    );
                }

                let mut sidebar_action = SidebarAction::None;
                let mut content_action = ContentAction::None;
                let mut lyrics_seek: Option<f64> = None;

                let title_h = 56.0_f32;
                let player_h = 74.0_f32;
                let title_rect = Rect::from_min_size(
                    outer_rect.min,
                    Vec2::new(outer_rect.width(), title_h),
                );
                let player_rect = Rect::from_min_max(
                    Pos2::new(
                        outer_rect.left(),
                        (outer_rect.bottom() - player_h).max(title_rect.bottom()),
                    ),
                    outer_rect.max,
                );
                let work_rect = Rect::from_min_max(
                    Pos2::new(outer_rect.left(), title_rect.bottom()),
                    Pos2::new(outer_rect.right(), player_rect.top()),
                );

                // 1. Top TitleBar (56px)
                let mut search_draft = self.library.search_query.clone();
                let endpoint = self.config.active_endpoint_label();
                let user_initial = self.session.user.initial();
                let username = self.session.user.display();
                let menu_entries = self.global_menu.entries();
                let mut title_action = render_title_bar(
                    ui,
                    ctx,
                    title_rect,
                    &self.palette,
                    &self.strings,
                    self.app_icon_tex.as_ref(),
                    &menu_entries,
                    self.settings.is_first_run,
                    is_maximized,
                    self.session.online
                        || self.playlists.is_server_online
                        || self.config.source_type != SourceKind::Server,
                    &endpoint,
                    &user_initial,
                    &username,
                    self.session.user.is_logged_in,
                    &mut search_draft,
                    &self.settings.recent_searches,
                    &mut self.show_recent_popup,
                );
                if search_draft != self.library.search_query {
                    title_action = TitleBarAction::SearchChanged(search_draft);
                }

                // 2. Central workspace (Sidebar + Content + optional LyricsDrawer)
                if work_rect.height() > 24.0 && work_rect.width() > 160.0 {
                    let effective_pls = self.playlists.effective_playlists(&self.settings);
                    let target_sidebar_w = if self.settings.sidebar_collapsed {
                        72.0
                    } else {
                        self.settings.sidebar_width.clamp(190.0, 320.0)
                    }
                    .min((work_rect.width() - 260.0).max(72.0));
                    let sidebar_w = ctx.animate_value_with_time(
                        egui::Id::new("anim_sidebar_width"),
                        target_sidebar_w,
                        0.18,
                    );
                    if (sidebar_w - target_sidebar_w).abs() > 0.3 {
                        ctx.request_repaint();
                    }

                    let lyrics_t = ease_out_cubic(ctx.animate_bool_with_time(
                        egui::Id::new("anim_lyrics_drawer"),
                        self.show_lyrics,
                        0.22,
                    ));
                    if lyrics_t > 0.0 && lyrics_t < 1.0 {
                        ctx.request_repaint();
                    }
                    let max_lyrics_w =
                        324.0_f32.min((work_rect.width() - sidebar_w - 220.0).max(0.0));
                    let lyrics_w = max_lyrics_w * lyrics_t;

                    let sidebar_rect = Rect::from_min_max(
                        work_rect.min,
                        Pos2::new(work_rect.left() + sidebar_w, work_rect.bottom()),
                    );
                    let content_rect = Rect::from_min_max(
                        Pos2::new(sidebar_rect.right(), work_rect.top()),
                        Pos2::new(
                            (work_rect.right() - lyrics_w).max(sidebar_rect.right() + 120.0),
                            work_rect.bottom(),
                        ),
                    );
                    let lyrics_rect = Rect::from_min_max(
                        Pos2::new(content_rect.right(), work_rect.top()),
                        work_rect.max,
                    );

                    sidebar_action = render_sidebar(
                        ui,
                        sidebar_rect,
                        &self.palette,
                        &self.strings,
                        &self.active_view,
                        &mut self.settings.sidebar_width,
                        &mut self.settings.sidebar_collapsed,
                        &effective_pls,
                        self.playlists.liked_ids.len(),
                    );

                    let viz_t = ease_out_cubic(ctx.animate_bool_with_time(
                        egui::Id::new("anim_visualizer_drawer"),
                        self.show_visualizer && self.settings.visualizer_enabled,
                        0.22,
                    ));
                    if viz_t > 0.0 && viz_t < 1.0 {
                        ctx.request_repaint();
                    }

                    ui.allocate_new_ui(egui::UiBuilder::new().max_rect(content_rect), |ui| {
                        ui.set_min_size(content_rect.size());
                        ui.vertical(|ui| {
                            if viz_t > 0.01 {
                                let viz_h = (190.0_f32.min(ui.available_height() * 0.42)) * viz_t;
                                let (viz_rect, _) = ui.allocate_exact_size(
                                    Vec2::new((ui.available_width() - 16.0).max(120.0), viz_h),
                                    egui::Sense::hover(),
                                );
                                if viz_rect.height() > 28.0 {
                                    ui.scope(|ui| {
                                        ui.multiply_opacity(viz_t);
                                        let spectrum = self.player.spectrum_snapshot();
                                        render_visualizer(
                                            ui,
                                            viz_rect.shrink2(Vec2::new(8.0, 6.0)),
                                            &spectrum,
                                            self.settings.visualizer_preset,
                                            is_playing_local,
                                            &self.palette,
                                        );
                                    });
                                }
                            }

                            let cur_track_id = self
                                .queue
                                .current_track()
                                .or(self.player.current_track.as_ref())
                                .map(|t| t.id);

                            content_action = render_content_area(
                                ui,
                                &self.palette,
                                &self.strings,
                                &self.active_view,
                                &mut self.view_state,
                                &self.library,
                                &self.queue,
                                &self.playlists,
                                cur_track_id,
                                is_playing_local,
                                &self.library.covers,
                                self.config.is_source_configured(),
                                &self.settings,
                            );
                        });
                    });

                    if lyrics_t > 0.02 && lyrics_rect.width() > 80.0 {
                        let cur_track = self
                            .queue
                            .current_track()
                            .or(self.player.current_track.as_ref());
                        let pos = if self.interconnect.is_controlling_android {
                            self.interconnect.remote_position_sec
                        } else {
                            self.player.position_sec()
                        };
                        ui.allocate_new_ui(egui::UiBuilder::new().max_rect(lyrics_rect), |ui| {
                            ui.set_min_size(lyrics_rect.size());
                            ui.multiply_opacity(lyrics_t);
                            lyrics_seek = render_lyrics_drawer(
                                ui,
                                &self.palette,
                                &self.strings,
                                cur_track,
                                &self.lyrics_lines,
                                pos,
                            );
                        });
                    }
                }

                // 3. Bottom PlayerBar (74px)
                let active_track = self
                    .queue
                    .current_track()
                    .or(self.player.current_track.as_ref());
                let active_cover = active_track
                    .and_then(|t| t.cover_url.as_ref())
                    .and_then(|u| self.library.covers.get(u))
                    .and_then(|o| o.as_ref());
                let pos_sec = if self.interconnect.is_controlling_android {
                    self.interconnect.remote_position_sec
                } else {
                    self.player.position_sec()
                };
                let dur_sec = active_track
                    .map(|t| {
                        if t.duration > 0.0 {
                            t.duration
                        } else {
                            self.player.duration_sec()
                        }
                    })
                    .unwrap_or(0.0);
                let playing = if self.interconnect.is_controlling_android {
                    self.interconnect.remote_playing
                } else {
                    is_playing_local
                };
                let volume = if self.interconnect.is_controlling_android {
                    (self.interconnect.remote_volume_pct as f32 / 100.0).clamp(0.0, 1.0)
                } else {
                    self.player.volume
                };
                let liked = active_track
                    .map(|t| self.playlists.is_liked(t.id))
                    .unwrap_or(false);

                let player_action = render_player_bar(
                    ui,
                    player_rect,
                    is_maximized,
                    &self.palette,
                    &self.strings,
                    active_track,
                    active_cover,
                    playing,
                    pos_sec,
                    dur_sec,
                    volume,
                    self.queue.is_shuffle,
                    self.queue.is_repeat,
                    liked,
                    self.interconnect.is_controlling_android,
                    self.interconnect.has_session,
                    self.show_lyrics,
                    self.show_visualizer,
                );

                if !is_maximized {
                    handle_frameless_resize(ctx, is_maximized);
                }

                self.dispatch_title_action(title_action, ctx);
                self.dispatch_sidebar_action(sidebar_action);
                self.dispatch_content_action(content_action);
                self.dispatch_player_action(player_action);
                if let Some(target_sec) = lyrics_seek {
                    self.player.seek(target_sec);
                }
            });

        let sleep_rem = self
            .sleep_deadline
            .and_then(|d| d.checked_duration_since(Instant::now()))
            .map(|dur| dur.as_secs());
        let cur_track = self
            .queue
            .current_track()
            .or(self.player.current_track.as_ref())
            .cloned();
        let pls = self.playlists.effective_playlists(&self.settings);

        let modal_action = render_modals(
            ctx,
            &self.palette,
            &self.strings,
            &mut self.modal_state,
            &mut self.settings,
            &self.session,
            cur_track.as_ref(),
            &pls,
            &self.presets,
            sleep_rem,
        );
        self.dispatch_modal_action(modal_action, ctx);
    }
}

impl DopamineApp {
    fn dispatch_title_action(&mut self, action: TitleBarAction, ctx: &Context) {
        let client = self.api_client();
        let local = self.local_scan_dir();
        match action {
            TitleBarAction::None => {}
            TitleBarAction::OpenSession => self.modal_state.open_session(&self.session),
            TitleBarAction::OpenSettings => self.modal_state.open_settings(&self.settings),
            TitleBarAction::SearchChanged(q) => {
                self.library
                    .set_search_query(&q, client.as_ref(), local.as_deref(), &self.settings);
                if !q.trim().is_empty() {
                    self.settings_store.record_recent_search(&mut self.settings, &q);
                }
            }
            TitleBarAction::ClearRecentSearches => {
                self.settings.recent_searches.clear();
                self.settings_store.save(&self.settings);
            }
            TitleBarAction::SelectRecentSearch(q) => {
                self.show_recent_popup = false;
                self.library
                    .set_search_query(&q, client.as_ref(), local.as_deref(), &self.settings);
            }
            TitleBarAction::MenuAction(act) => {
                self.dispatch_global_menu_action(act, ctx);
            }
        }
    }

    fn dispatch_sidebar_action(&mut self, action: SidebarAction) {
        match action {
            SidebarAction::None => {}
            SidebarAction::SelectView(view) => {
                self.active_view = view;
                self.view_state.selected_playlist = None;
                self.view_state.selected_group_key = None;
            }
            SidebarAction::NewPlaylist => {
                self.modal_state.show_create_playlist = true;
            }
            SidebarAction::OpenPlaylist(pl) => {
                self.active_view = String::from("playlists");
                self.view_state.selected_playlist = Some(pl);
            }
            SidebarAction::OpenEqualizer => {
                self.modal_state.show_equalizer = true;
            }
            SidebarAction::OpenSleepTimer => {
                self.modal_state.show_sleep_timer = true;
            }
            SidebarAction::OpenSettings => {
                self.modal_state.open_settings(&self.settings);
            }
            SidebarAction::OpenAbout => {
                self.modal_state.show_about = true;
            }
            SidebarAction::WidthCommitted => {
                self.settings_store.save(&self.settings);
            }
        }
    }

    fn dispatch_content_action(&mut self, action: ContentAction) {
        let client = self.api_client();
        match action {
            ContentAction::None => {}
            ContentAction::SelectLibraryTab(tab) => {
                self.view_state.active_library_tab = tab;
                self.view_state.selected_group_key = None;
            }
            ContentAction::PlayFromList(list, idx) => {
                if let Some(track) =
                    self.queue
                        .play_from_library(&list, idx, client.as_ref(), &mut self.settings)
                {
                    self.start_playing_track(track, 0.0, true);
                }
            }
            ContentAction::PlayNow(track) => {
                if let Some(t) =
                    self.queue
                        .play_now(track, client.as_ref(), &mut self.settings)
                {
                    self.start_playing_track(t, 0.0, true);
                }
            }
            ContentAction::PlayNext(track) => {
                if let Some(t) = self.queue.play_next(track) {
                    self.start_playing_track(t, 0.0, true);
                }
            }
            ContentAction::AddToQueue(track) => {
                self.queue.append_to_queue(track);
            }
            ContentAction::StartMix(track) => {
                if let Some(t) =
                    self.queue
                        .start_mix_track(track, client.as_ref(), &mut self.settings)
                {
                    self.start_playing_track(t, 0.0, true);
                }
            }
            ContentAction::ToggleLike(id) => {
                self.playlists.toggle_like(id, client.as_ref());
            }
            ContentAction::OpenAddToPlaylist(track) => {
                self.modal_state.add_to_playlist_track = Some(track);
            }
            ContentAction::DownloadTrack(track) => {
                self.library
                    .download_track(&track, client.as_ref(), &self.settings);
            }
            ContentAction::InspectTrack(track) => {
                self.modal_state.inspect_track = Some(track);
            }
            ContentAction::LoadMore => {
                let next_page = self.library.page + 1;
                let local = self.local_scan_dir();
                self.library
                    .load_tracks(client.as_ref(), local.as_deref(), &self.settings, next_page);
            }
            ContentAction::RefreshLibrary => self.reload_catalog(),
            ContentAction::OpenSettings => self.modal_state.open_settings(&self.settings),
            ContentAction::OpenWizard => self.modal_state.open_wizard(&self.settings),
            ContentAction::QueueJumpTo(idx) => {
                if let Some(t) =
                    self.queue
                        .play_at_index(idx, client.as_ref(), &mut self.settings)
                {
                    self.start_playing_track(t, 0.0, true);
                }
            }
            ContentAction::QueueClear => {
                self.queue.clear_queue();
                self.player.stop();
            }
            ContentAction::NewPlaylist => {
                self.modal_state.show_create_playlist = true;
            }
            ContentAction::DeletePlaylist(id) => {
                self.playlists.delete_playlist(id, client.as_ref());
                self.view_state.selected_playlist = None;
            }
            ContentAction::SelectPlaylist(pl) => {
                self.view_state.selected_playlist = pl;
            }
            ContentAction::SelectGroup(key) => {
                self.view_state.selected_group_key = key;
            }
        }
    }

    fn dispatch_player_action(&mut self, action: PlayerBarAction) {
        let client = self.api_client();
        match action {
            PlayerBarAction::None => {}
            PlayerBarAction::TogglePlay => self.toggle_play_or_remote(),
            PlayerBarAction::Next => self.next_track(),
            PlayerBarAction::Previous => self.previous_track(),
            PlayerBarAction::ToggleShuffle => {
                self.queue.is_shuffle = !self.queue.is_shuffle;
            }
            PlayerBarAction::ToggleRepeat => {
                self.queue.is_repeat = !self.queue.is_repeat;
            }
            PlayerBarAction::Seek(pos) => {
                if self.interconnect.is_controlling_android {
                    let qids: Vec<i64> = self.queue.queue.iter().map(|t| t.id).collect();
                    self.interconnect.remote_seek(client.as_ref(), pos, &qids);
                } else {
                    self.player.seek(pos);
                }
            }
            PlayerBarAction::SetVolume(vol) => {
                if self.interconnect.is_controlling_android {
                    let pct = (vol * 100.0).round() as i32;
                    let qids: Vec<i64> = self.queue.queue.iter().map(|t| t.id).collect();
                    self.interconnect.remote_set_volume(client.as_ref(), pct, &qids);
                } else {
                    self.player.set_volume(vol);
                    self.settings.volume = vol;
                    self.settings_store.save(&self.settings);
                }
            }
            PlayerBarAction::ToggleLike(id) => {
                self.playlists.toggle_like(id, client.as_ref());
            }
            PlayerBarAction::OpenQueue => {
                self.active_view = String::from("queue");
            }
            PlayerBarAction::OpenEqualizer => {
                self.modal_state.show_equalizer = true;
            }
            PlayerBarAction::ToggleLyrics => {
                self.show_lyrics = !self.show_lyrics;
            }
            PlayerBarAction::ToggleVisualizer => {
                self.show_visualizer = !self.show_visualizer;
            }
            PlayerBarAction::OpenQualityDialog => {
                self.modal_state.show_quality_dialog = true;
            }
            PlayerBarAction::SwitchDevice => {
                if self.interconnect.is_controlling_android {
                    let pos = self.interconnect.remote_position_sec;
                    let _ = self.interconnect.switch_to_desktop(client.as_ref());
                    if let Some(t) = self.queue.current_track().cloned() {
                        self.start_playing_track(t, pos, true);
                    }
                } else {
                    let cur_id = self.queue.current_track().map(|t| t.id).unwrap_or(0);
                    let pos = self.player.position_sec();
                    let qids: Vec<i64> = self.queue.queue.iter().map(|t| t.id).collect();
                    let vol = self.player.volume;
                    self.player.pause();
                    let _ = self
                        .interconnect
                        .switch_to_android(client.as_ref(), cur_id, pos, vol, &qids);
                }
            }
        }
    }

    fn dispatch_modal_action(&mut self, action: ModalAction, ctx: &Context) {
        let client = self.api_client();
        match action {
            ModalAction::None => {}
            ModalAction::WizardFinished(next) | ModalAction::SettingsSaved(next) => {
                self.apply_saved_settings(next);
            }
            ModalAction::SettingsDraftThemeChanged(theme_id) => {
                self.base_palette = ThemePalette::load(&theme_id);
                self.apply_theme_and_accent(ctx);
            }
            ModalAction::EqualizerChanged(eq) => {
                self.settings.equalizer = eq.clone();
                self.player.update_equalizer(eq);
                self.settings_store.save(&self.settings);
            }
            ModalAction::SessionConnectServer(url) => {
                self.session.connect_server(&url);
            }
            ModalAction::SessionLogin {
                server_url,
                username,
                password,
            } => {
                self.session.login(&server_url, &username, &password);
            }
            ModalAction::SessionLogout => {
                self.session.logout();
            }
            ModalAction::CreatePlaylist(name) => {
                self.playlists.create_playlist(&name, client.as_ref());
            }
            ModalAction::AddTrackToPlaylist {
                playlist_id,
                track_id,
            } => {
                self.playlists
                    .add_track_to_playlist(playlist_id, track_id, client.as_ref());
            }
            ModalAction::SetSleepTimerMinutes(mins) => {
                self.sleep_deadline = mins.map(|m| Instant::now() + Duration::from_secs(m * 60));
                self.modal_state.show_sleep_timer = false;
            }
            ModalAction::SeekTo(sec) => {
                self.player.seek(sec);
            }
            ModalAction::OpenSettings => {
                self.modal_state.open_settings(&self.settings);
            }
            ModalAction::OpenSession => {
                self.modal_state.open_session(&self.session);
            }
        }
    }
}

fn apply_settings_to_config(settings: &SettingsData, config: &mut ProjectConfig) {
    match settings.mode.as_str() {
        "local" => config.set_local_source(&settings.folder_path),
        "smb" => config.set_smb_source(
            &settings.smb_host,
            &settings.smb_share,
            &settings.smb_folder,
            &settings.smb_user,
        ),
        _ => config.set_server_source(&settings.server_url),
    }
}

fn load_app_icon(ctx: &Context) -> Option<TextureHandle> {
    let dyn_img = image::load_from_memory(APP_ICON_PNG).ok()?;
    let rgba = dyn_img.into_rgba8();
    let (w, h) = rgba.dimensions();
    let img = ColorImage::from_rgba_unmultiplied([w as usize, h as usize], rgba.as_raw());
    Some(ctx.load_texture("dopamine_app_icon", img, TextureOptions::LINEAR))
}
