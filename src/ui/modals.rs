// Location: src/ui/modals.rs
// Function: All modal windows and drawers: FirstRunWizard, SettingsWindow,
// EqualizerWindow, SessionWindow, AudioQualityDialog, TrackTechnicalDialog,
// AddToPlaylistDialog, CreatePlaylistDialog, SleepTimerDialog, AboutWindow,
// and LyricsDrawer.
// All Rights Reserved isgaar

use egui::{
    Align, Color32, Context, CornerRadius, Frame, Key, Layout, Margin, Pos2, Rect, RichText,
    ScrollArea, Sense, Slider, Stroke, Ui, Vec2, ViewportBuilder, ViewportClass, ViewportCommand,
};

use crate::core::lrc::{active_line_index, LyricLine};
use crate::core::settings::{EqualizerSettings, SettingsData};
use crate::data::model::{Playlist, Track};
use crate::session::controller::SessionController;
use crate::shared::colors::{ThemePalette, THEME_IDS};
use crate::shared::i18n::StringCatalog;
use crate::ui::chrome::handle_frameless_resize;
use crate::ui::theme::{
    bold_text, fidelity_color, format_duration, icon_glyph, icon_text, lerp_color, RADIUS_FULL,
    RADIUS_MD, RADIUS_SM,
};
use crate::ui::visualizer::VisualizerPreset;

const EQ_FREQS: [&str; 12] = [
    "31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "12k", "16k", "20k",
];

#[derive(Debug, Clone)]
pub enum ModalAction {
    None,
    WizardFinished(SettingsData),
    SettingsSaved(SettingsData),
    SettingsDraftThemeChanged(String),
    EqualizerChanged(EqualizerSettings),
    SessionConnectServer(String),
    SessionLogin {
        server_url: String,
        username: String,
        password: String,
    },
    SessionLogout,
    CreatePlaylist(String),
    AddTrackToPlaylist {
        playlist_id: i64,
        track_id: i64,
    },
    SetSleepTimerMinutes(Option<u64>),
    SeekTo(f64),
    OpenSettings,
    OpenSession,
}

#[derive(Debug, Clone)]
pub struct ModalState {
    pub show_wizard: bool,
    pub wizard_step: usize,
    pub wizard_draft: SettingsData,

    pub show_settings: bool,
    pub settings_section: &'static str,
    pub settings_draft: SettingsData,

    pub show_equalizer: bool,
    pub eq_tab: &'static str,

    pub show_session: bool,
    pub session_server_input: String,
    pub session_username_input: String,
    pub session_password_input: String,

    pub show_quality_dialog: bool,
    pub inspect_track: Option<Track>,
    pub add_to_playlist_track: Option<Track>,

    pub show_create_playlist: bool,
    pub new_playlist_name: String,

    pub show_sleep_timer: bool,
    pub show_about: bool,
}

impl Default for ModalState {
    fn default() -> Self {
        let base = SettingsData::default();
        Self {
            show_wizard: false,
            wizard_step: 0,
            wizard_draft: base.clone(),
            show_settings: false,
            settings_section: "source",
            settings_draft: base,
            show_equalizer: false,
            eq_tab: "bands",
            show_session: false,
            session_server_input: String::new(),
            session_username_input: String::new(),
            session_password_input: String::new(),
            show_quality_dialog: false,
            inspect_track: None,
            add_to_playlist_track: None,
            show_create_playlist: false,
            new_playlist_name: String::new(),
            show_sleep_timer: false,
            show_about: false,
        }
    }
}

impl ModalState {
    pub fn open_settings(&mut self, current: &SettingsData) {
        self.settings_draft = current.clone();
        self.show_settings = true;
    }

    pub fn open_wizard(&mut self, current: &SettingsData) {
        self.wizard_draft = current.clone();
        self.wizard_step = 0;
        self.show_wizard = true;
    }

    pub fn open_session(&mut self, session: &SessionController) {
        if self.session_server_input.is_empty() {
            self.session_server_input = if !session.server_draft.is_empty() {
                session.server_draft.clone()
            } else {
                session.server_url.clone()
            };
        }
        self.show_session = true;
    }
}

struct SecondaryWindowSpec<'a> {
    id_salt: &'a str,
    title: &'a str,
    icon: &'a str,
    size: Vec2,
    resizable: bool,
}

fn compute_centered_pos(parent_ctx: &Context, size: Vec2) -> Option<Pos2> {
    parent_ctx.input(|i| {
        let vp = i.viewport();
        if let Some(outer) = vp.outer_rect {
            return Some(Pos2::new(
                (outer.center().x - size.x * 0.5).max(0.0),
                (outer.center().y - size.y * 0.5).max(0.0),
            ));
        }
        if let Some(mon) = vp.monitor_size {
            if mon.x > 1.0 && mon.y > 1.0 {
                return Some(Pos2::new(
                    ((mon.x - size.x) * 0.5).max(0.0),
                    ((mon.y - size.y) * 0.5).max(0.0),
                ));
            }
        }
        None
    })
}

fn render_secondary_window(
    parent_ctx: &Context,
    palette: &ThemePalette,
    spec: &SecondaryWindowSpec<'_>,
    open: &mut bool,
    mut body: impl FnMut(&mut Ui, &mut bool),
) {
    if !*open {
        return;
    }

    let pos_key = egui::Id::new(("dopamine_sec_pos", spec.id_salt));
    let cached_pos = parent_ctx.data_mut(|d| d.get_temp::<Option<Pos2>>(pos_key));
    let initial_pos = match cached_pos {
        Some(pos) => pos,
        None => {
            let computed = compute_centered_pos(parent_ctx, spec.size);
            parent_ctx.data_mut(|d| d.insert_temp(pos_key, computed));
            computed
        }
    };

    let clean_title = spec.title.trim_end_matches([' ', '·']);
    let vp_id = egui::ViewportId::from_hash_of(("dopamine_secondary", spec.id_salt));
    let mut builder = ViewportBuilder::default()
        .with_title(clean_title)
        .with_app_id("com.isgaar.dopamine")
        .with_inner_size(spec.size)
        .with_min_inner_size(Vec2::new(
            (spec.size.x * 0.85).max(280.0),
            (spec.size.y * 0.80).max(160.0),
        ))
        .with_resizable(spec.resizable)
        .with_decorations(false)
        .with_transparent(true)
        .with_window_type(egui::X11WindowType::Dialog);
    if let Some(pos) = initial_pos {
        builder = builder.with_position(pos);
    }

    let mut still_open = *open;
    let close_btn_id = egui::Id::new(("dopamine_sec_close", spec.id_salt));
    let header_drag_id = egui::Id::new(("dopamine_sec_drag", spec.id_salt));
    let centered_flag_id = egui::Id::new(("dopamine_sec_centered", spec.id_salt));
    let already_centered = parent_ctx.data_mut(|d| d.get_temp::<bool>(centered_flag_id).unwrap_or(false));

    parent_ctx.show_viewport_immediate(vp_id, builder, |vp_ctx, class| {
        if class == ViewportClass::Embedded {
            let mut win_open = true;
            egui::Window::new(clean_title)
                .id(egui::Id::new(("dopamine_emb", spec.id_salt)))
                .open(&mut win_open)
                .collapsible(false)
                .resizable(spec.resizable)
                .anchor(egui::Align2::CENTER_CENTER, Vec2::ZERO)
                .fixed_size(spec.size)
                .show(vp_ctx, |ui| {
                    body(ui, &mut still_open);
                });
            if !win_open {
                still_open = false;
            }
            return;
        }

        if initial_pos.is_none() && !already_centered {
            if let Some(cmd) = ViewportCommand::center_on_screen(vp_ctx) {
                vp_ctx.send_viewport_cmd(cmd);
                vp_ctx.data_mut(|d| d.insert_temp(centered_flag_id, true));
            }
        }

        if spec.resizable {
            handle_frameless_resize(vp_ctx, false);
        }

        let close_requested = vp_ctx.input(|i| {
            i.viewport().close_requested() || i.key_pressed(Key::Escape)
        });
        if close_requested {
            still_open = false;
        }

        egui::CentralPanel::default()
            .frame(Frame::NONE)
            .show(vp_ctx, |ui| {
                let outer = ui.max_rect();
                Frame::new()
                    .fill(palette.bg_dark)
                    .stroke(Stroke::new(1.0_f32, palette.border_highlight))
                    .corner_radius(CornerRadius::same(14))
                    .inner_margin(Margin::same(0))
                    .show(ui, |ui| {
                        ui.set_min_size(outer.size());
                        ui.set_max_size(outer.size());

                        let header_h = 42.0;
                        let (header_rect, _) = ui.allocate_exact_size(
                            Vec2::new(ui.available_width(), header_h),
                            Sense::hover(),
                        );

                        ui.painter().line_segment(
                            [
                                Pos2::new(header_rect.left() + 1.0, header_rect.bottom()),
                                Pos2::new(header_rect.right() - 1.0, header_rect.bottom()),
                            ],
                            Stroke::new(1.0_f32, palette.border_dark),
                        );

                        let close_size = Vec2::splat(26.0);
                        let close_rect = Rect::from_center_size(
                            Pos2::new(header_rect.right() - 22.0, header_rect.center().y),
                            close_size,
                        );
                        let drag_rect = Rect::from_min_max(
                            header_rect.min,
                            Pos2::new(close_rect.left() - 8.0, header_rect.bottom()),
                        );

                        let drag_resp =
                            ui.interact(drag_rect, header_drag_id, Sense::click_and_drag());
                        if drag_resp.drag_started_by(egui::PointerButton::Primary) {
                            vp_ctx.send_viewport_cmd(ViewportCommand::StartDrag);
                        }

                        // Badge + title on left
                        let badge_rect = Rect::from_center_size(
                            Pos2::new(header_rect.left() + 24.0, header_rect.center().y),
                            Vec2::splat(22.0),
                        );
                        ui.painter()
                            .rect_filled(badge_rect, CornerRadius::same(6), palette.bg_surface);
                        ui.painter().text(
                            badge_rect.center(),
                            egui::Align2::CENTER_CENTER,
                            icon_glyph(spec.icon),
                            egui::FontId::new(12.5, egui::FontFamily::Name("lucide".into())),
                            palette.primary,
                        );
                        ui.painter().text(
                            Pos2::new(badge_rect.right() + 10.0, header_rect.center().y),
                            egui::Align2::LEFT_CENTER,
                            clean_title,
                            egui::FontId::new(13.5, egui::FontFamily::Name("LumpySansBold".into())),
                            palette.text_light,
                        );

                        // Close button on right
                        let close_resp = ui.interact(close_rect, close_btn_id, Sense::click());
                        let close_hov = close_resp.hovered();
                        let close_t =
                            vp_ctx.animate_bool_with_time(close_btn_id.with("h"), close_hov, 0.14);
                        ui.painter().circle_filled(
                            close_rect.center(),
                            13.0,
                            lerp_color(palette.bg_surface, palette.danger, close_t),
                        );
                        ui.painter().text(
                            close_rect.center(),
                            egui::Align2::CENTER_CENTER,
                            icon_glyph("x"),
                            egui::FontId::new(12.5, egui::FontFamily::Name("lucide".into())),
                            lerp_color(palette.text_muted, Color32::WHITE, close_t),
                        );
                        if close_resp.clicked() {
                            still_open = false;
                        }

                        // Window content body with generous padding and exact bounds
                        let body_avail_h = (ui.available_height() - 1.0).max(80.0);
                        ui.allocate_ui_with_layout(
                            Vec2::new(ui.available_width(), body_avail_h),
                            Layout::top_down(Align::Min),
                            |ui| {
                                Frame::NONE
                                    .inner_margin(Margin::symmetric(18, 14))
                                    .show(ui, |ui| {
                                        let inner_w = ui.available_width().max(120.0);
                                        let inner_h = ui.available_height().max(80.0);
                                        ui.set_min_size(Vec2::new(inner_w, inner_h));
                                        ui.set_max_size(Vec2::new(inner_w, inner_h));
                                        body(ui, &mut still_open);
                                    });
                            },
                        );
                    });
            });

        if !still_open {
            vp_ctx.send_viewport_cmd(ViewportCommand::Close);
        }
    });

    if !still_open {
        *open = false;
        parent_ctx.data_mut(|d| {
            d.remove::<Option<Pos2>>(pos_key);
            d.remove::<bool>(centered_flag_id);
        });
        parent_ctx.request_repaint();
    }
}

#[allow(clippy::too_many_arguments)]
pub fn render_modals(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
    settings: &mut SettingsData,
    session: &SessionController,
    current_track: Option<&Track>,
    playlists: &[Playlist],
    presets: &[VisualizerPreset],
    sleep_remaining_sec: Option<u64>,
) -> ModalAction {
    let mut action = ModalAction::None;

    if state.show_wizard {
        let wiz_action = render_first_run_wizard(ctx, palette, strings, state);
        if !matches!(wiz_action, ModalAction::None) {
            action = wiz_action;
        }
    }

    if state.show_settings {
        let s_action = render_settings_window(ctx, palette, strings, state, presets);
        if !matches!(s_action, ModalAction::None) {
            action = s_action;
        }
    }

    if state.show_equalizer {
        let eq_action = render_equalizer_window(ctx, palette, strings, state, &mut settings.equalizer);
        if !matches!(eq_action, ModalAction::None) {
            action = eq_action;
        }
    }

    if state.show_session {
        let ses_action = render_session_window(ctx, palette, strings, state, session);
        if !matches!(ses_action, ModalAction::None) {
            action = ses_action;
        }
    }

    if state.show_quality_dialog {
        render_audio_quality_dialog(ctx, palette, strings, &mut state.show_quality_dialog, current_track, settings);
    }

    if let Some(track) = state.inspect_track.clone() {
        let mut open = true;
        render_track_technical_dialog(ctx, palette, strings, &mut open, &track);
        if !open {
            state.inspect_track = None;
        }
    }

    if let Some(track) = state.add_to_playlist_track.clone() {
        let mut open = true;
        let pl_action = render_add_to_playlist_dialog(
            ctx,
            palette,
            strings,
            &mut open,
            &track,
            playlists,
        );
        if !matches!(pl_action, ModalAction::None) {
            action = pl_action;
            open = false;
        }
        if !open {
            state.add_to_playlist_track = None;
        }
    }

    if state.show_create_playlist {
        let cp_action = render_create_playlist_dialog(ctx, palette, strings, state);
        if !matches!(cp_action, ModalAction::None) {
            action = cp_action;
        }
    }

    if state.show_sleep_timer {
        let st_action = render_sleep_timer_dialog(ctx, palette, strings, &mut state.show_sleep_timer, sleep_remaining_sec);
        if !matches!(st_action, ModalAction::None) {
            action = st_action;
        }
    }

    if state.show_about {
        render_about_window(ctx, palette, strings, &mut state.show_about);
    }

    action
}

pub fn render_lyrics_drawer(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    track: Option<&Track>,
    lines: &[LyricLine],
    position_sec: f64,
) -> Option<f64> {
    let mut seek_target = None;
    let panel_w = ui.available_width().min(320.0).max(80.0);
    ui.allocate_ui(Vec2::new(panel_w, ui.available_height()), |ui| {
        Frame::new()
            .fill(palette.bg_dark)
            .stroke(Stroke::new(1.0_f32, palette.border_dark))
            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
            .inner_margin(Margin::same(16))
            .show(ui, |ui| {
                ui.set_height((ui.available_height()).max(120.0));
                ui.horizontal(|ui| {
                    ui.label(icon_text("music", 15.0, palette.primary));
                    ui.label(bold_text(
                        strings.get_or("lyrics_title", "Letras"),
                        15.0,
                        palette.text_light,
                    ));
                });
                if let Some(t) = track {
                    ui.label(
                        RichText::new(format!("{} · {}", t.title, t.artist))
                            .size(11.5)
                            .color(palette.text_muted),
                    );
                }
                ui.add_space(10.0);
                ui.separator();
                ui.add_space(10.0);

                if lines.is_empty() {
                    ui.centered_and_justified(|ui| {
                        ui.label(
                            RichText::new(strings.get_or("lyrics_empty", "Sin letras sincronizadas para esta pista"))
                                .size(12.5)
                                .color(palette.text_muted),
                        );
                    });
                } else {
                    let active_idx = active_line_index(lines, position_sec);
                    ScrollArea::vertical()
                        .id_salt("lyrics_scroll")
                        .auto_shrink([false, false])
                        .show(ui, |ui| {
                            for (idx, line) in lines.iter().enumerate() {
                                let active = active_idx == Some(idx);
                                let text = if active {
                                    bold_text(&line.text, 15.0, palette.primary)
                                } else {
                                    RichText::new(&line.text)
                                        .size(13.5)
                                        .color(palette.text_muted)
                                };
                                let resp = ui.add(
                                    egui::Label::new(text).sense(Sense::click()).wrap(),
                                );
                                if active {
                                    resp.scroll_to_me(Some(Align::Center));
                                }
                                if resp.clicked() {
                                    seek_target = Some(line.time_sec);
                                }
                                ui.add_space(8.0);
                            }
                        });
                }
            });
    });
    seek_target
}

fn render_first_run_wizard(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
) -> ModalAction {
    let mut action = ModalAction::None;
    let mut open = state.show_wizard;
    let spec = SecondaryWindowSpec {
        id_salt: "wizard",
        title: strings.get_or("wizard_title", "Bienvenido a Dopamine"),
        icon: "sparkles",
        size: Vec2::new(520.0, 460.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, &mut open, |ui, win_open| {
        ui.label(
            RichText::new(strings.get("app_tagline"))
                .size(12.5)
                .color(palette.text_muted),
        );
        ui.add_space(10.0);

        let scroll_h = (ui.available_height() - 52.0).max(160.0);
        ScrollArea::vertical()
            .id_salt("wizard_body")
            .max_height(scroll_h)
            .auto_shrink([false, false])
            .show(ui, |ui| {
                let field_w = (ui.available_width() - 8.0).max(200.0);
                if state.wizard_step == 0 {
                    ui.label(bold_text(
                        strings.get_or("wizard_choose_source", "Selecciona tu fuente de musica"),
                        14.5,
                        palette.text_light,
                    ));
                    ui.add_space(10.0);

                    let modes = [
                        ("server", "server", strings.get("settings_mode_server")),
                        ("local", "folder", strings.get("settings_mode_local")),
                        ("smb", "wifi", strings.get("settings_mode_smb")),
                    ];
                    for (id, icon, label) in modes {
                        let selected = state.wizard_draft.mode == id;
                        let resp = Frame::new()
                            .fill(if selected {
                                palette.bg_selected
                            } else {
                                palette.bg_surface
                            })
                            .stroke(Stroke::new(
                                1.0_f32,
                                if selected {
                                    palette.primary
                                } else {
                                    palette.border_dark
                                },
                            ))
                            .corner_radius(CornerRadius::same(RADIUS_SM as u8))
                            .inner_margin(Margin::same(10))
                            .show(ui, |ui| {
                                ui.set_width((field_w - 20.0).max(160.0));
                                ui.horizontal(|ui| {
                                    ui.label(icon_text(
                                        icon,
                                        16.0,
                                        if selected {
                                            palette.primary
                                        } else {
                                            palette.text_light
                                        },
                                    ));
                                    ui.label(bold_text(label, 13.0, palette.text_light));
                                });
                            })
                            .response
                            .interact(Sense::click());
                        if resp.clicked() {
                            state.wizard_draft.mode = id.to_string();
                        }
                        ui.add_space(6.0);
                    }

                    ui.add_space(8.0);
                    match state.wizard_draft.mode.as_str() {
                        "server" => {
                            ui.label(strings.get("settings_server_url"));
                            ui.add(
                                egui::TextEdit::singleline(&mut state.wizard_draft.server_url)
                                    .desired_width(field_w),
                            );
                        }
                        "local" => {
                            ui.label(strings.get("settings_local_folder"));
                            ui.add(
                                egui::TextEdit::singleline(&mut state.wizard_draft.folder_path)
                                    .desired_width(field_w),
                            );
                        }
                        "smb" => {
                            ui.label(strings.get("settings_smb_host"));
                            ui.add(
                                egui::TextEdit::singleline(&mut state.wizard_draft.smb_host)
                                    .desired_width(field_w),
                            );
                            ui.label(strings.get("settings_smb_share"));
                            ui.add(
                                egui::TextEdit::singleline(&mut state.wizard_draft.smb_share)
                                    .desired_width(field_w),
                            );
                            ui.label(strings.get("settings_smb_folder"));
                            ui.add(
                                egui::TextEdit::singleline(&mut state.wizard_draft.smb_folder)
                                    .desired_width(field_w),
                            );
                        }
                        _ => {}
                    }
                } else {
                    ui.label(bold_text(
                        strings.get("settings_nav_appearance"),
                        14.5,
                        palette.text_light,
                    ));
                    ui.add_space(10.0);
                    ui.horizontal(|ui| {
                        ui.label(strings.get("settings_language"));
                        egui::ComboBox::from_id_salt("wizard_lang")
                            .selected_text(if state.wizard_draft.language == "en" {
                                "English"
                            } else {
                                "Espanol"
                            })
                            .show_ui(ui, |ui| {
                                ui.selectable_value(
                                    &mut state.wizard_draft.language,
                                    String::from("es"),
                                    "Espanol",
                                );
                                ui.selectable_value(
                                    &mut state.wizard_draft.language,
                                    String::from("en"),
                                    "English",
                                );
                            });
                    });
                    ui.add_space(10.0);
                    ui.label(strings.get("settings_theme"));
                    ui.horizontal_wrapped(|ui| {
                        for id in THEME_IDS {
                            if ui
                                .selectable_label(state.wizard_draft.theme == id, id)
                                .clicked()
                            {
                                state.wizard_draft.theme = id.to_string();
                                action = ModalAction::SettingsDraftThemeChanged(id.to_string());
                            }
                        }
                    });
                }
            });

        ui.add_space(8.0);
        ui.separator();
        ui.add_space(8.0);
        ui.horizontal(|ui| {
            if state.wizard_step > 0
                && ui
                    .button(strings.get_or("wizard_back", "Atras"))
                    .clicked()
            {
                state.wizard_step = 0;
            }
            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                if state.wizard_step == 0 {
                    if ui
                        .button(strings.get_or("wizard_next", "Siguiente"))
                        .clicked()
                    {
                        state.wizard_step = 1;
                    }
                } else if ui
                    .button(strings.get_or("wizard_finish", "Comenzar"))
                    .clicked()
                {
                    state.wizard_draft.is_first_run = false;
                    *win_open = false;
                    action = ModalAction::WizardFinished(state.wizard_draft.clone());
                }
                if ui
                    .button(strings.get_or("wizard_skip", "Omitir"))
                    .clicked()
                {
                    state.wizard_draft.is_first_run = false;
                    *win_open = false;
                    action = ModalAction::WizardFinished(state.wizard_draft.clone());
                }
            });
        });
    });

    state.show_wizard = open;
    action
}

fn render_settings_window(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
    presets: &[VisualizerPreset],
) -> ModalAction {
    let mut action = ModalAction::None;
    let mut open = state.show_settings;
    let spec = SecondaryWindowSpec {
        id_salt: "settings",
        title: strings.get("settings_title"),
        icon: "settings",
        size: Vec2::new(740.0, 540.0),
        resizable: true,
    };

    render_secondary_window(ctx, palette, &spec, &mut open, |ui, win_open| {
        let total_w = ui.available_width();
        let total_h = ui.available_height();
        let nav_w = 185.0;
        let body_w = (total_w - nav_w - 18.0).max(240.0);

        ui.horizontal_top(|ui| {
            // Left section nav
            ui.allocate_ui_with_layout(
                Vec2::new(nav_w, total_h),
                Layout::top_down(Align::Min),
                |ui| {
                    let sections: [(&'static str, &str, &str); 5] = [
                        ("source", "server", strings.get("settings_nav_source")),
                        ("audio", "audio-lines", strings.get("settings_nav_audio")),
                        ("appearance", "palette", strings.get("settings_nav_appearance")),
                        ("covers", "image", strings.get("settings_nav_covers")),
                        ("system", "settings", strings.get("settings_nav_system")),
                    ];
                    for (id, icon, label) in sections {
                        let sel = state.settings_section == id;
                        let resp = Frame::new()
                            .fill(if sel {
                                palette.primary
                            } else {
                                Color32::TRANSPARENT
                            })
                            .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
                            .inner_margin(Margin::symmetric(12, 8))
                            .show(ui, |ui| {
                                ui.set_width((nav_w - 28.0).max(120.0));
                                ui.horizontal(|ui| {
                                    ui.label(icon_text(
                                        icon,
                                        14.0,
                                        if sel {
                                            palette.active_fg()
                                        } else {
                                            palette.text_muted
                                        },
                                    ));
                                    ui.label(RichText::new(label).size(12.5).color(if sel {
                                        palette.active_fg()
                                    } else {
                                        palette.text_light
                                    }));
                                });
                            })
                            .response
                            .interact(Sense::click());
                        if resp.clicked() {
                            state.settings_section = id;
                        }
                        ui.add_space(4.0);
                    }

                    ui.add_space(12.0);
                    ui.separator();
                    ui.add_space(8.0);
                    if ui
                        .add_sized(
                            Vec2::new(nav_w - 4.0, 30.0),
                            egui::Button::new(strings.get("session_title")),
                        )
                        .clicked()
                    {
                        action = ModalAction::OpenSession;
                    }
                },
            );

            ui.separator();

            // Right settings body + pinned footer
            ui.allocate_ui_with_layout(
                Vec2::new(body_w, total_h),
                Layout::top_down(Align::Min),
                |ui| {
                    let scroll_h = (total_h - 48.0).max(160.0);
                    let input_w = (body_w - 28.0).max(180.0);
                    ScrollArea::vertical()
                        .id_salt("settings_body")
                        .max_height(scroll_h)
                        .auto_shrink([false, false])
                        .show(ui, |ui| {
                            ui.set_width(input_w);
                            let d = &mut state.settings_draft;
                            match state.settings_section {
                                "source" => {
                                    ui.label(bold_text(
                                        strings.get("settings_nav_source"),
                                        15.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(8.0);
                                    ui.horizontal_wrapped(|ui| {
                                        ui.radio_value(&mut d.mode, String::from("server"), strings.get("settings_mode_server"));
                                        ui.radio_value(&mut d.mode, String::from("local"), strings.get("settings_mode_local"));
                                        ui.radio_value(&mut d.mode, String::from("smb"), strings.get("settings_mode_smb"));
                                    });
                                    ui.add_space(10.0);
                                    ui.label(strings.get("settings_server_url"));
                                    ui.add(egui::TextEdit::singleline(&mut d.server_url).desired_width(input_w));
                                    ui.add_space(6.0);
                                    ui.label(strings.get("settings_local_folder"));
                                    ui.add(egui::TextEdit::singleline(&mut d.folder_path).desired_width(input_w));
                                    ui.add_space(6.0);
                                    ui.label(strings.get("settings_smb_host"));
                                    ui.add(egui::TextEdit::singleline(&mut d.smb_host).desired_width(input_w));
                                    ui.add_space(4.0);
                                    ui.label(strings.get("settings_smb_share"));
                                    ui.add(egui::TextEdit::singleline(&mut d.smb_share).desired_width(input_w));
                                    ui.add_space(4.0);
                                    ui.label(strings.get("settings_smb_folder"));
                                    ui.add(egui::TextEdit::singleline(&mut d.smb_folder).desired_width(input_w));
                                    ui.add_space(4.0);
                                    ui.label(strings.get("settings_smb_user"));
                                    ui.add(egui::TextEdit::singleline(&mut d.smb_user).desired_width(input_w));
                                    ui.add_space(6.0);
                                    ui.checkbox(&mut d.save_credentials, strings.get("settings_smb_save"));
                                }
                                "audio" => {
                                    ui.label(bold_text(
                                        strings.get("settings_nav_audio"),
                                        15.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(8.0);
                                    ui.checkbox(&mut d.gapless, strings.get("settings_gapless"));
                                    ui.checkbox(&mut d.crossfade_enabled, strings.get("settings_crossfade"));
                                    if d.crossfade_enabled {
                                        ui.add(Slider::new(&mut d.crossfade_sec, 0.5..=12.0).text("Crossfade (s)"));
                                    }
                                    ui.checkbox(&mut d.fade_on_pause, strings.get("settings_fade_pause"));
                                    ui.checkbox(&mut d.skip_silence, strings.get("settings_skip_silence"));
                                    ui.checkbox(&mut d.volume_stabilizer, strings.get("settings_volume_stabilizer"));
                                    ui.checkbox(&mut d.visualizer_enabled, strings.get("settings_visualizer"));
                                    ui.add_space(4.0);
                                    ui.horizontal(|ui| {
                                        ui.label(strings.get_or("settings_visualizer_preset", "Preset"));
                                        let current_name = presets
                                            .iter()
                                            .find(|p| p.id == d.visualizer_preset)
                                            .map(|p| p.name.as_str())
                                            .unwrap_or("WonderWave");
                                        egui::ComboBox::from_id_salt("viz_preset_combo")
                                            .selected_text(current_name)
                                            .show_ui(ui, |ui| {
                                                for preset in presets {
                                                    ui.selectable_value(
                                                        &mut d.visualizer_preset,
                                                        preset.id,
                                                        &preset.name,
                                                    );
                                                }
                                            });
                                    });
                                    ui.add_space(4.0);
                                    let mut buf = d.buffer_ms as f32;
                                    if ui.add(Slider::new(&mut buf, 100.0..=1500.0).text(strings.get("settings_buffer_ms"))).changed() {
                                        d.buffer_ms = buf.round() as u32;
                                    }
                                }
                                "appearance" => {
                                    ui.label(bold_text(
                                        strings.get("settings_nav_appearance"),
                                        15.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(8.0);
                                    ui.label(strings.get("settings_theme"));
                                    ui.horizontal_wrapped(|ui| {
                                        for id in THEME_IDS {
                                            if ui.selectable_label(d.theme == id, id).clicked() {
                                                d.theme = id.to_string();
                                                action = ModalAction::SettingsDraftThemeChanged(id.to_string());
                                            }
                                        }
                                    });
                                    ui.add_space(8.0);
                                    ui.checkbox(&mut d.dynamic_accent, strings.get("settings_dynamic_accent"));
                                    ui.add_space(8.0);
                                    ui.horizontal(|ui| {
                                        ui.label(strings.get("settings_language"));
                                        egui::ComboBox::from_id_salt("settings_lang_combo")
                                            .selected_text(if d.language == "en" { "English" } else { "Espanol" })
                                            .show_ui(ui, |ui| {
                                                ui.selectable_value(&mut d.language, String::from("es"), "Espanol");
                                                ui.selectable_value(&mut d.language, String::from("en"), "English");
                                            });
                                    });
                                }
                                "covers" => {
                                    ui.label(bold_text(
                                        strings.get("settings_nav_covers"),
                                        15.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(8.0);
                                    ui.checkbox(&mut d.cover_carousel, strings.get("settings_cover_carousel"));
                                    ui.checkbox(&mut d.download_art, strings.get("settings_download_art"));
                                    ui.checkbox(&mut d.covers_wifi_only, strings.get("settings_covers_wifi"));
                                    ui.checkbox(&mut d.prefer_folder_art, strings.get("settings_prefer_folder_art"));
                                    ui.checkbox(&mut d.cover_no_ext, strings.get("settings_cover_no_ext"));
                                    ui.checkbox(&mut d.auto_lrc, strings.get("settings_auto_lrc"));
                                }
                                _ => {
                                    ui.label(bold_text(
                                        strings.get("settings_nav_system"),
                                        15.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(8.0);
                                    ui.checkbox(&mut d.scan_startup, strings.get("settings_scan_startup"));
                                    ui.checkbox(&mut d.scan_notify, strings.get("settings_scan_notify"));
                                    ui.checkbox(&mut d.skip_nomedia, strings.get("settings_skip_nomedia"));
                                    ui.checkbox(&mut d.skip_mp4, strings.get("settings_skip_mp4"));
                                    ui.checkbox(&mut d.skip_ogg, strings.get("settings_skip_ogg"));
                                    ui.checkbox(&mut d.media_keys, strings.get("settings_media_keys"));
                                    ui.checkbox(&mut d.power_saving, strings.get("settings_power_saving"));
                                    let mut short_sec = d.skip_short_sec as f32;
                                    if ui.add(Slider::new(&mut short_sec, 0.0..=60.0).text(strings.get("settings_skip_short"))).changed() {
                                        d.skip_short_sec = short_sec.round() as u32;
                                    }
                                }
                            }
                        });

                    ui.add_space(6.0);
                    ui.separator();
                    ui.add_space(6.0);
                    ui.horizontal(|ui| {
                        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                            if ui.button(strings.get("settings_save")).clicked() {
                                action = ModalAction::SettingsSaved(state.settings_draft.clone());
                                *win_open = false;
                            }
                            if ui.button(strings.get("settings_discard")).clicked() {
                                *win_open = false;
                            }
                        });
                    });
                },
            );
        });
    });

    state.show_settings = open;
    action
}

fn render_equalizer_window(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
    eq: &mut EqualizerSettings,
) -> ModalAction {
    let mut action = ModalAction::None;
    let mut open = state.show_equalizer;
    let mut changed = false;
    let spec = SecondaryWindowSpec {
        id_salt: "equalizer",
        title: strings.get("eq_title"),
        icon: "sliders",
        size: Vec2::new(680.0, 450.0),
        resizable: true,
    };

    render_secondary_window(ctx, palette, &spec, &mut open, |ui, _| {
        ui.horizontal(|ui| {
            for (id, label) in [
                ("bands", strings.get("eq_tab_bands")),
                ("fx1", strings.get("eq_tab_effects1")),
                ("fx2", strings.get("eq_tab_effects2")),
            ] {
                if ui.selectable_label(state.eq_tab == id, label).clicked() {
                    state.eq_tab = id;
                }
            }
            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                if ui.button(strings.get("eq_reset")).clicked() {
                    *eq = EqualizerSettings::default();
                    changed = true;
                }
            });
        });
        ui.separator();
        ui.add_space(8.0);

        match state.eq_tab {
            "fx1" => {
                changed |= ui.add(Slider::new(&mut eq.balance, -1.0..=1.0).text(strings.get("eq_balance"))).changed();
                changed |= ui.add(Slider::new(&mut eq.z_bass, 0.0..=1.0).text(strings.get("eq_z_bass"))).changed();
                changed |= ui.add(Slider::new(&mut eq.compressor, 0.0..=1.0).text(strings.get("eq_compressor"))).changed();
                changed |= ui.add(Slider::new(&mut eq.z_mids, 0.0..=1.0).text(strings.get("eq_z_mids"))).changed();
                changed |= ui.add(Slider::new(&mut eq.z_treble, 0.0..=1.0).text(strings.get("eq_z_treble"))).changed();
                changed |= ui.add(Slider::new(&mut eq.pitch_semitones, -12.0..=12.0).text(strings.get("eq_pitch"))).changed();
                changed |= ui.add(Slider::new(&mut eq.speed, 0.5..=2.0).text(strings.get("eq_speed"))).changed();
                changed |= ui.checkbox(&mut eq.agc_enabled, strings.get("eq_agc")).changed();
            }
            "fx2" => {
                changed |= ui.checkbox(&mut eq.echo_enabled, strings.get("eq_echo")).changed();
                changed |= ui.add(Slider::new(&mut eq.echo_gain, 0.0..=1.0).text("Echo Gain")).changed();
                changed |= ui.checkbox(&mut eq.autowah_enabled, strings.get("eq_autowah")).changed();
                changed |= ui.add(Slider::new(&mut eq.autowah_gain, 0.0..=1.0).text("AutoWah Gain")).changed();
                changed |= ui.checkbox(&mut eq.reverb_enabled, strings.get("eq_reverb")).changed();
                changed |= ui.add(Slider::new(&mut eq.reverb_gain, 0.0..=1.0).text("Reverb Gain")).changed();
                changed |= ui.checkbox(&mut eq.flanger_enabled, strings.get("eq_flanger")).changed();
            }
            _ => {
                ui.horizontal(|ui| {
                    changed |= ui.checkbox(&mut eq.limiter_enabled, strings.get("eq_limiter")).changed();
                    ui.add_space(16.0);
                    changed |= ui.add(Slider::new(&mut eq.preamp_db, -15.0..=15.0).text(strings.get("eq_preamp"))).changed();
                });
                ui.add_space(12.0);
                ui.spacing_mut().slider_width = (ui.available_height() - 60.0).max(160.0);
                let col_w = ((ui.available_width() - 44.0) / 12.0).max(34.0);
                ui.horizontal(|ui| {
                    for (idx, freq) in EQ_FREQS.iter().enumerate() {
                        ui.allocate_ui_with_layout(
                            Vec2::new(col_w, ui.available_height()),
                            Layout::top_down(Align::Center),
                            |ui| {
                                let resp = ui.add(
                                    Slider::new(&mut eq.bands_db[idx], -15.0..=15.0)
                                        .vertical()
                                        .show_value(false),
                                );
                                if resp.changed() {
                                    changed = true;
                                }
                                ui.label(
                                    RichText::new(format!("{:+.0}", eq.bands_db[idx]))
                                        .size(10.0)
                                        .color(palette.primary),
                                );
                                ui.label(
                                    RichText::new(*freq).size(10.0).color(palette.text_muted),
                                );
                            },
                        );
                    }
                });
            }
        }
    });

    if changed {
        action = ModalAction::EqualizerChanged(eq.clone());
    }
    state.show_equalizer = open;
    action
}

fn render_session_window(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
    session: &SessionController,
) -> ModalAction {
    let mut action = ModalAction::None;
    let mut open = state.show_session;
    let spec = SecondaryWindowSpec {
        id_salt: "session",
        title: strings.get("session_title"),
        icon: "user",
        size: Vec2::new(440.0, 360.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, &mut open, |ui, _| {
        let field_w = (ui.available_width() - 8.0).max(200.0);
        if session.user.is_logged_in {
            Frame::new()
                .fill(palette.bg_surface)
                .stroke(Stroke::new(1.0_f32, palette.primary_dim))
                .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                .inner_margin(Margin::same(14))
                .show(ui, |ui| {
                    ui.set_width((field_w - 28.0).max(160.0));
                    ui.horizontal(|ui| {
                        ui.label(icon_text("shield-check", 24.0, palette.success));
                        ui.vertical(|ui| {
                            ui.label(bold_text(&session.user.username, 15.0, palette.text_light));
                            ui.label(
                                RichText::new(&session.user.email)
                                    .size(12.0)
                                    .color(palette.text_muted),
                            );
                            ui.label(
                                RichText::new(&session.server_host)
                                    .size(11.0)
                                    .color(palette.text_dim),
                            );
                        });
                    });
                });
            ui.add_space(12.0);
            if ui.button(strings.get("session_logout_button")).clicked() {
                action = ModalAction::SessionLogout;
            }
        } else {
            ui.label(strings.get("session_server_label"));
            ui.horizontal(|ui| {
                ui.add(
                    egui::TextEdit::singleline(&mut state.session_server_input)
                        .desired_width((field_w - 110.0).max(150.0)),
                );
                if ui.button(strings.get("session_connect_button")).clicked() {
                    action = ModalAction::SessionConnectServer(state.session_server_input.clone());
                }
            });
            ui.add_space(10.0);
            ui.label(strings.get("session_username_label"));
            ui.add(
                egui::TextEdit::singleline(&mut state.session_username_input)
                    .desired_width(field_w),
            );
            ui.add_space(6.0);
            ui.label(strings.get("session_password_label"));
            ui.add(
                egui::TextEdit::singleline(&mut state.session_password_input)
                    .password(true)
                    .desired_width(field_w),
            );
            ui.add_space(12.0);
            if ui
                .add_enabled(
                    !session.busy,
                    egui::Button::new(strings.get("session_login_button")),
                )
                .clicked()
            {
                action = ModalAction::SessionLogin {
                    server_url: state.session_server_input.clone(),
                    username: state.session_username_input.clone(),
                    password: state.session_password_input.clone(),
                };
            }
        }

        if !session.status_key.is_empty() {
            ui.add_space(8.0);
            ui.label(
                RichText::new(strings.get(&session.status_key))
                    .size(11.5)
                    .color(palette.text_muted),
            );
        }
        if !session.error_key.is_empty() || !session.error_detail.is_empty() {
            ui.add_space(4.0);
            let msg = if !session.error_detail.is_empty() {
                session.error_detail.clone()
            } else {
                strings.get(&session.error_key).to_string()
            };
            ui.label(RichText::new(msg).size(11.5).color(palette.danger));
        }
    });

    state.show_session = open;
    action
}

fn render_audio_quality_dialog(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    open: &mut bool,
    track: Option<&Track>,
    settings: &SettingsData,
) {
    let spec = SecondaryWindowSpec {
        id_salt: "audio_quality",
        title: strings.get("audio_quality_title"),
        icon: "audio-lines",
        size: Vec2::new(400.0, 265.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, open, |ui, _| {
        if let Some(t) = track {
            let col = fidelity_color(&t.fidelity_tier, palette);
            ui.horizontal(|ui| {
                ui.label(bold_text(&t.fidelity_tier, 18.0, col));
                ui.label(bold_text(&t.title, 14.0, palette.text_light));
            });
            ui.add_space(8.0);
            ui.label(format!("{}: {}", strings.get("audio_quality_format"), t.format.to_uppercase()));
            ui.label(format!("{}: {} Hz", strings.get("audio_quality_sample_rate"), t.sample_rate));
            ui.label(format!("{}: {}-bit", strings.get("audio_quality_bit_depth"), t.bit_depth));
            if t.bitrate > 0 {
                ui.label(format!("{}: {} kbps", strings.get("audio_quality_bitrate"), t.bitrate));
            }
            ui.add_space(6.0);
            ui.separator();
            ui.add_space(6.0);
            ui.label(format!(
                "DSP: 12-Band EQ + Limiter ({}) · Stabilizer ({})",
                if settings.equalizer.limiter_enabled { "ON" } else { "OFF" },
                if settings.volume_stabilizer { "ON" } else { "OFF" }
            ));
        } else {
            ui.label(strings.get("empty_no_tracks_title"));
        }
    });
}

fn render_track_technical_dialog(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    open: &mut bool,
    track: &Track,
) {
    let spec = SecondaryWindowSpec {
        id_salt: "track_info",
        title: strings.get_or("ctx_track_info", "Detalles tecnicos"),
        icon: "info",
        size: Vec2::new(440.0, 340.0),
        resizable: true,
    };

    render_secondary_window(ctx, palette, &spec, open, |ui, _| {
        ui.label(bold_text(&track.title, 15.0, palette.text_light));
        ui.label(RichText::new(format!("{} · {}", track.artist, track.album)).color(palette.text_muted));
        ui.add_space(6.0);
        ui.separator();
        ui.add_space(6.0);
        ScrollArea::vertical()
            .id_salt("track_info_scroll")
            .auto_shrink([false, false])
            .show(ui, |ui| {
                ui.label(format!("ID: {}", track.id));
                ui.label(format!("Format: {}", track.quality_details));
                ui.label(format!("Duration: {}", format_duration(track.duration)));
                if !track.genre.is_empty() {
                    ui.label(format!("Genre: {}", track.genre));
                }
                if track.bpm > 0.0 {
                    ui.label(format!("BPM: {:.1}", track.bpm));
                }
                if !track.musical_key.is_empty() {
                    ui.label(format!("Key: {}", track.musical_key));
                }
                if !track.path.is_empty() {
                    ui.add(egui::Label::new(format!("Path: {}", track.path)).wrap());
                }
            });
    });
}

fn render_add_to_playlist_dialog(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    open: &mut bool,
    track: &Track,
    playlists: &[Playlist],
) -> ModalAction {
    let mut action = ModalAction::None;
    let spec = SecondaryWindowSpec {
        id_salt: "add_to_playlist",
        title: strings.get("ctx_add_to_playlist"),
        icon: "list-music",
        size: Vec2::new(360.0, 310.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, open, |ui, win_open| {
        ui.label(bold_text(&track.title, 13.5, palette.text_light));
        ui.add_space(8.0);
        ScrollArea::vertical()
            .max_height((ui.available_height() - 8.0).max(120.0))
            .auto_shrink([false, false])
            .show(ui, |ui| {
                let btn_w = (ui.available_width() - 8.0).max(140.0);
                for pl in playlists.iter().filter(|p| p.id > 0) {
                    if ui
                        .add_sized(Vec2::new(btn_w, 30.0), egui::Button::new(&pl.name))
                        .clicked()
                    {
                        action = ModalAction::AddTrackToPlaylist {
                            playlist_id: pl.id,
                            track_id: track.id,
                        };
                        *win_open = false;
                    }
                    ui.add_space(4.0);
                }
            });
    });

    action
}

fn render_create_playlist_dialog(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    state: &mut ModalState,
) -> ModalAction {
    let mut action = ModalAction::None;
    let mut open = state.show_create_playlist;
    let spec = SecondaryWindowSpec {
        id_salt: "create_playlist",
        title: strings.get("pl_create_title"),
        icon: "plus",
        size: Vec2::new(360.0, 185.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, &mut open, |ui, win_open| {
        let field_w = (ui.available_width() - 8.0).max(180.0);
        ui.label(RichText::new(strings.get("pl_name_label")).color(palette.text_muted));
        ui.add_space(4.0);
        ui.add(
            egui::TextEdit::singleline(&mut state.new_playlist_name)
                .desired_width(field_w),
        );
        ui.add_space(12.0);
        ui.horizontal(|ui| {
            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                if ui.button(strings.get("pl_create_confirm")).clicked()
                    && !state.new_playlist_name.trim().is_empty()
                {
                    action = ModalAction::CreatePlaylist(state.new_playlist_name.trim().to_string());
                    state.new_playlist_name.clear();
                    *win_open = false;
                }
                if ui.button(strings.get("pl_cancel")).clicked() {
                    *win_open = false;
                }
            });
        });
    });

    state.show_create_playlist = open;
    action
}

fn render_sleep_timer_dialog(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    open: &mut bool,
    remaining_sec: Option<u64>,
) -> ModalAction {
    let mut action = ModalAction::None;
    let spec = SecondaryWindowSpec {
        id_salt: "sleep_timer",
        title: strings.get("nav_timer"),
        icon: "clock",
        size: Vec2::new(340.0, 220.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, open, |ui, win_open| {
        if let Some(rem) = remaining_sec {
            ui.label(bold_text(
                format!("{}: {}", strings.get_or("timer_remaining", "Restante"), format_duration(rem as f64)),
                14.0,
                palette.primary,
            ));
            ui.add_space(8.0);
        }
        ui.horizontal_wrapped(|ui| {
            for mins in [15_u64, 30, 45, 60] {
                if ui.button(format!("{mins} min")).clicked() {
                    action = ModalAction::SetSleepTimerMinutes(Some(mins));
                    *win_open = false;
                }
            }
        });
        ui.add_space(10.0);
        if remaining_sec.is_some()
            && ui.button(strings.get_or("timer_cancel", "Desactivar")).clicked()
        {
            action = ModalAction::SetSleepTimerMinutes(None);
            *win_open = false;
        }
    });

    action
}

fn render_about_window(
    ctx: &Context,
    palette: &ThemePalette,
    strings: &StringCatalog,
    open: &mut bool,
) {
    let spec = SecondaryWindowSpec {
        id_salt: "about",
        title: strings.get("nav_about"),
        icon: "music",
        size: Vec2::new(420.0, 260.0),
        resizable: false,
    };

    render_secondary_window(ctx, palette, &spec, open, |ui, _| {
        ui.label(bold_text("Dopamine Desktop", 18.0, palette.primary));
        ui.label(RichText::new(strings.get("app_tagline")).color(palette.text_muted));
        ui.add_space(8.0);
        ui.separator();
        ui.add_space(8.0);
        ui.label("Engine: 100% Pure Rust · egui/eframe · Symphonia · DSP 32-bit float");
        ui.label("Audio Output: PulseAudio / PipeWire (libpulse-simple)");
        ui.add_space(8.0);
        ui.label(
            RichText::new("All Rights Reserved isgaar")
                .size(11.5)
                .color(palette.text_muted),
        );
    });
}
