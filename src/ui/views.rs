// Location: src/ui/views.rs
// Function: Main content views: ContentTabs capsule bar, ListenView,
// LibraryView (Tracks + A-Z jump bar + context menus, Albums, Artists,
// Genres, Folders with drilldown), FavoritesView, PlaylistsView, QueueView,
// and EmptyStateView.
// All Rights Reserved isgaar

use std::collections::{HashMap, HashSet};

use egui::{
    Align, Color32, CornerRadius, Frame, Id, Layout, Margin, Pos2, Rect, RichText, ScrollArea,
    Sense, Stroke, TextureHandle, Ui, Vec2,
};

use crate::core::settings::SettingsData;
use crate::data::model::{Playlist, Track};
use crate::features::library::{CollectionGroup, LibraryController};
use crate::features::playlists::PlaylistController;
use crate::features::queue::QueueController;
use crate::shared::colors::ThemePalette;
use crate::shared::i18n::StringCatalog;
use crate::ui::theme::{
    animate_hover_and_active, begin_keyed_transition, bold_text, fidelity_color, format_duration,
    hash_transition_key, icon_glyph, icon_text, lerp_color, paint_animated_eq_bars, RADIUS_FULL,
    RADIUS_MD, RADIUS_SM, SPACE_MD, SPACE_SM,
};

#[derive(Debug, Clone)]
pub enum ContentAction {
    None,
    SelectLibraryTab(&'static str),
    PlayFromList(Vec<Track>, usize),
    PlayNow(Track),
    PlayNext(Track),
    AddToQueue(Track),
    StartMix(Track),
    ToggleLike(i64),
    OpenAddToPlaylist(Track),
    DownloadTrack(Track),
    InspectTrack(Track),
    LoadMore,
    RefreshLibrary,
    OpenSettings,
    OpenWizard,
    QueueJumpTo(usize),
    QueueClear,
    NewPlaylist,
    DeletePlaylist(i64),
    SelectPlaylist(Option<Playlist>),
    SelectGroup(Option<String>),
}

#[derive(Debug, Clone, Default)]
pub struct ViewUiState {
    pub active_library_tab: &'static str,
    pub selected_group_key: Option<String>,
    pub selected_playlist: Option<Playlist>,
    pub scroll_to_section: Option<String>,
}

impl ViewUiState {
    pub fn new() -> Self {
        Self {
            active_library_tab: "tracks",
            selected_group_key: None,
            selected_playlist: None,
            scroll_to_section: None,
        }
    }
}

pub fn render_content_area(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    active_view: &str,
    view_state: &mut ViewUiState,
    library: &LibraryController,
    queue: &QueueController,
    playlists: &PlaylistController,
    current_track_id: Option<i64>,
    is_playing: bool,
    covers: &HashMap<String, Option<TextureHandle>>,
    is_source_configured: bool,
    settings: &SettingsData,
) -> ContentAction {
    let mut action = ContentAction::None;

    if !is_source_configured && library.all_tracks.is_empty() {
        return render_empty_state(ui, palette, strings, true);
    }

    let view_key = hash_transition_key(&[active_view]);
    let view_tx = begin_keyed_transition(ui, ui.id().with("main_view_tx"), view_key, 0.20);
    let view_offset_y = (1.0 - view_tx) * 10.0;
    let base_rect = ui.available_rect_before_wrap();
    let shifted_rect = base_rect.translate(Vec2::new(0.0, view_offset_y));
    ui.allocate_rect(base_rect, Sense::hover());

    ui.allocate_new_ui(egui::UiBuilder::new().max_rect(shifted_rect), |ui| {
        ui.multiply_opacity(0.28 + 0.72 * view_tx);

        match active_view {
            "listen" => {
                action = render_listen_view(
                    ui,
                    palette,
                    strings,
                    library,
                    playlists,
                    current_track_id,
                    covers,
                );
            }
            "library" => {
                ui.add_space(SPACE_SM);
                let tab_action =
                    render_content_tabs(ui, palette, strings, view_state.active_library_tab);
                if !matches!(tab_action, ContentAction::None) {
                    action = tab_action;
                }
                ui.add_space(SPACE_SM);

                let tab_key = hash_transition_key(&[
                    view_state.active_library_tab,
                    view_state.selected_group_key.as_deref().unwrap_or(""),
                ]);
                let tab_tx =
                    begin_keyed_transition(ui, ui.id().with("lib_subtab_tx"), tab_key, 0.18);
                let tab_avail = ui.available_rect_before_wrap();
                let tab_rect = tab_avail.translate(Vec2::new((1.0 - tab_tx) * 14.0, 0.0));
                ui.allocate_rect(tab_avail, Sense::hover());

                let mut body_action = ContentAction::None;
                ui.allocate_new_ui(egui::UiBuilder::new().max_rect(tab_rect), |ui| {
                    ui.multiply_opacity(0.32 + 0.68 * tab_tx);
                    body_action = match view_state.active_library_tab {
                        "albums" => render_group_browser(
                            ui,
                            palette,
                            strings,
                            "albums",
                            &library.albums,
                            &view_state.selected_group_key,
                            &playlists.liked_ids,
                            current_track_id,
                            covers,
                        ),
                        "artists" => render_group_browser(
                            ui,
                            palette,
                            strings,
                            "artists",
                            &library.artists,
                            &view_state.selected_group_key,
                            &playlists.liked_ids,
                            current_track_id,
                            covers,
                        ),
                        "genres" => render_group_browser(
                            ui,
                            palette,
                            strings,
                            "genres",
                            &library.genres,
                            &view_state.selected_group_key,
                            &playlists.liked_ids,
                            current_track_id,
                            covers,
                        ),
                        "folders" => render_group_browser(
                            ui,
                            palette,
                            strings,
                            "folders",
                            &library.folders,
                            &view_state.selected_group_key,
                            &playlists.liked_ids,
                            current_track_id,
                            covers,
                        ),
                        _ => render_tracks_view(
                            ui,
                            palette,
                            strings,
                            library,
                            &playlists.liked_ids,
                            current_track_id,
                            is_playing,
                            covers,
                            &mut view_state.scroll_to_section,
                        ),
                    };
                });
                if matches!(action, ContentAction::None) {
                    action = body_action;
                }
            }
            "favorites" => {
                let fav_tracks: Vec<Track> = library
                    .all_tracks
                    .iter()
                    .filter(|t| playlists.liked_ids.contains(&t.id))
                    .cloned()
                    .collect();
                action = render_curated_track_list(
                    ui,
                    palette,
                    strings,
                    strings.get("nav_favorites"),
                    "heart",
                    &fav_tracks,
                    &playlists.liked_ids,
                    current_track_id,
                    is_playing,
                    covers,
                    strings.get("empty_favorites_subtitle"),
                );
            }
            "playlists" => {
                action = render_playlists_view(
                    ui,
                    palette,
                    strings,
                    playlists,
                    library,
                    &view_state.selected_playlist,
                    current_track_id,
                    is_playing,
                    covers,
                    settings,
                );
            }
            "queue" => {
                action = render_queue_view(
                    ui,
                    palette,
                    strings,
                    queue,
                    &playlists.liked_ids,
                    current_track_id,
                    is_playing,
                    covers,
                );
            }
            _ => {
                action = render_tracks_view(
                    ui,
                    palette,
                    strings,
                    library,
                    &playlists.liked_ids,
                    current_track_id,
                    is_playing,
                    covers,
                    &mut view_state.scroll_to_section,
                );
            }
        }
    });

    action
}

fn render_content_tabs(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    active_tab: &'static str,
) -> ContentAction {
    let mut action = ContentAction::None;
    let tabs: [(&'static str, &str, &str); 5] = [
        ("tracks", "music", strings.get("nav_tracks")),
        ("albums", "disc-album", strings.get("nav_albums")),
        ("artists", "users", strings.get("nav_artists")),
        ("genres", "tag", strings.get("nav_genres")),
        ("folders", "folder", strings.get("nav_folders")),
    ];

    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        Frame::new()
            .fill(ThemePalette::with_alpha(palette.bg_surface, 0.92))
            .stroke(Stroke::new(1.0_f32, palette.border_dark))
            .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
            .inner_margin(Margin::symmetric(6, 4))
            .show(ui, |ui| {
                // Measure exact tab widths first so the sliding active pill paints behind labels cleanly.
                let mut tab_widths = [0.0_f32; 5];
                let mut total_w = 0.0_f32;
                let gap = 4.0_f32;
                for (i, (_, _, label)) in tabs.iter().enumerate() {
                    let g = ui.painter().layout_no_wrap(
                        (*label).to_string(),
                        egui::FontId::new(
                            12.5,
                            egui::FontFamily::Name(std::sync::Arc::from("LumpySansBold")),
                        ),
                        palette.text_light,
                    );
                    tab_widths[i] = 12.0 + 14.0 + 6.0 + g.size().x + 12.0;
                    total_w += tab_widths[i];
                    if i + 1 < tabs.len() {
                        total_w += gap;
                    }
                }
                let pill_h = 28.0_f32;
                let (strip_rect, _) =
                    ui.allocate_exact_size(Vec2::new(total_w, pill_h), Sense::hover());

                let mut tab_rects = [Rect::NOTHING; 5];
                let mut cursor_x = strip_rect.left();
                let mut target_idx = 0usize;
                for (i, (id, _, _)) in tabs.iter().enumerate() {
                    let w = tab_widths[i];
                    tab_rects[i] =
                        Rect::from_min_size(Pos2::new(cursor_x, strip_rect.top()), Vec2::new(w, pill_h));
                    if *id == active_tab {
                        target_idx = i;
                    }
                    cursor_x += w + gap;
                }

                let target_x = tab_rects[target_idx].left() - strip_rect.left();
                let target_w = tab_rects[target_idx].width();
                let anim_x = ui.ctx().animate_value_with_time(
                    ui.id().with("lib_tab_slide_x"),
                    target_x,
                    0.20,
                );
                let anim_w = ui.ctx().animate_value_with_time(
                    ui.id().with("lib_tab_slide_w"),
                    target_w,
                    0.20,
                );
                if (anim_x - target_x).abs() > 0.25 || (anim_w - target_w).abs() > 0.25 {
                    ui.ctx().request_repaint();
                }

                let slide_rect = Rect::from_min_size(
                    Pos2::new(strip_rect.left() + anim_x, strip_rect.top()),
                    Vec2::new(anim_w, pill_h),
                );
                ui.painter().rect_filled(
                    slide_rect,
                    CornerRadius::same(RADIUS_FULL as u8),
                    palette.primary,
                );

                for (i, (id, icon, label)) in tabs.iter().enumerate() {
                    let rect = tab_rects[i];
                    let selected = active_tab == *id;
                    let resp = ui.interact(rect, ui.id().with(("lib_tab_btn", *id)), Sense::click());
                    let (h_t, a_t) = animate_hover_and_active(
                        ui,
                        ui.id().with(("lib_tab_anim", *id)),
                        resp.hovered(),
                        selected,
                    );
                    if !selected && h_t > 0.01 {
                        ui.painter().rect_filled(
                            rect,
                            CornerRadius::same(RADIUS_FULL as u8),
                            ThemePalette::with_alpha(palette.bg_hover, h_t * 0.8),
                        );
                    }
                    let fg = lerp_color(
                        lerp_color(palette.text_muted, palette.text_light, h_t),
                        palette.active_fg(),
                        a_t,
                    );
                    let icon_x = rect.left() + 19.0;
                    ui.painter().text(
                        Pos2::new(icon_x, rect.center().y),
                        egui::Align2::CENTER_CENTER,
                        icon_glyph(icon),
                        egui::FontId::new(13.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
                        fg,
                    );
                    let font = if selected {
                        egui::FontId::new(
                            12.5,
                            egui::FontFamily::Name(std::sync::Arc::from("LumpySansBold")),
                        )
                    } else {
                        egui::FontId::new(12.5, egui::FontFamily::Proportional)
                    };
                    ui.painter().text(
                        Pos2::new(rect.left() + 32.0, rect.center().y),
                        egui::Align2::LEFT_CENTER,
                        *label,
                        font,
                        fg,
                    );
                    if resp.clicked() {
                        action = ContentAction::SelectLibraryTab(id);
                    }
                }
            });
    });

    action
}

fn render_listen_view(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    library: &LibraryController,
    playlists: &PlaylistController,
    current_track_id: Option<i64>,
    covers: &HashMap<String, Option<TextureHandle>>,
) -> ContentAction {
    let mut action = ContentAction::None;

    if library.all_tracks.is_empty() && !library.is_loading {
        return render_empty_state(ui, palette, strings, false);
    }

    ScrollArea::vertical()
        .id_salt("listen_scroll")
        .auto_shrink([false, false])
        .show(ui, |ui| {
            ui.add_space(SPACE_MD);
            ui.horizontal(|ui| {
                ui.add_space(SPACE_MD);
                ui.vertical(|ui| {
                    ui.set_max_width((ui.available_width() - SPACE_MD * 2.0).max(200.0));

                    // Hero banner
                    Frame::new()
                        .fill(palette.bg_surface)
                        .stroke(Stroke::new(1.0_f32, palette.border_highlight))
                        .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                        .inner_margin(Margin::same(20))
                        .show(ui, |ui| {
                            ui.horizontal(|ui| {
                                ui.vertical(|ui| {
                                    ui.label(bold_text(
                                        strings.get("nav_listen"),
                                        22.0,
                                        palette.text_light,
                                    ));
                                    ui.add_space(4.0);
                                    let hours = library.total_duration_hours();
                                    ui.label(
                                        RichText::new(format!(
                                            "{} · {:.1} h",
                                            format_count_label(
                                                library.all_tracks.len(),
                                                strings.is_spanish()
                                            ),
                                            hours
                                        ))
                                        .size(12.5)
                                        .color(palette.text_muted),
                                    );
                                    ui.add_space(12.0);
                                    ui.horizontal(|ui| {
                                        if let Some(first) = library.all_tracks.first() {
                                            if pill_button(
                                                ui,
                                                palette,
                                                "play",
                                                strings.get("ctx_play_now"),
                                                true,
                                            ) {
                                                action = ContentAction::PlayFromList(
                                                    library.all_tracks.clone(),
                                                    0,
                                                );
                                            }
                                            ui.add_space(8.0);
                                            if pill_button(
                                                ui,
                                                palette,
                                                "sparkles",
                                                strings.get("ctx_start_mix"),
                                                false,
                                            ) {
                                                action = ContentAction::StartMix(first.clone());
                                            }
                                        }
                                    });
                                });
                            });
                        });

                    ui.add_space(20.0);

                    // Recent tracks strip
                    let recent_source = if !library.recent_tracks.is_empty() {
                        &library.recent_tracks
                    } else {
                        &library.all_tracks
                    };
                    if !recent_source.is_empty() {
                        ui.label(bold_text(
                            strings.get_or("listen_recent", "Recientes"),
                            16.0,
                            palette.text_light,
                        ));
                        ui.add_space(10.0);
                        ui.horizontal_wrapped(|ui| {
                            ui.spacing_mut().item_spacing = Vec2::new(12.0, 12.0);
                            for (idx, track) in recent_source.iter().take(8).enumerate() {
                                let card_w = 162.0;
                                let resp = Frame::new()
                                    .fill(if current_track_id == Some(track.id) {
                                        palette.bg_selected
                                    } else {
                                        palette.bg_card
                                    })
                                    .stroke(Stroke::new(1.0_f32, palette.border_dark))
                                    .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                                    .inner_margin(Margin::same(10))
                                    .show(ui, |ui| {
                                        ui.set_width(card_w - 20.0);
                                        let (cov_rect, _) = ui.allocate_exact_size(
                                            Vec2::splat(card_w - 20.0),
                                            Sense::hover(),
                                        );
                                        let tex = cover_for_track(track, covers);
                                        paint_cover_or_placeholder(
                                            ui,
                                            cov_rect,
                                            tex,
                                            palette,
                                            RADIUS_SM as u8,
                                        );
                                        ui.add_space(8.0);
                                        ui.label(
                                            bold_text(&track.title, 12.5, palette.text_light),
                                        );
                                        ui.label(
                                            RichText::new(&track.artist)
                                                .size(11.0)
                                                .color(palette.text_muted),
                                        );
                                    })
                                    .response
                                    .interact(Sense::click());

                                if resp.clicked() {
                                    action =
                                        ContentAction::PlayFromList(recent_source.to_vec(), idx);
                                }
                                let is_liked = playlists.liked_ids.contains(&track.id);
                                resp.context_menu(|ui| {
                                    render_track_context_menu(
                                        ui,
                                        track,
                                        is_liked,
                                        strings,
                                        palette,
                                        &mut action,
                                    );
                                });
                            }
                        });
                        ui.add_space(20.0);
                    }

                    // Featured albums
                    if !library.albums.is_empty() {
                        ui.label(bold_text(
                            strings.get("nav_albums"),
                            16.0,
                            palette.text_light,
                        ));
                        ui.add_space(10.0);
                        ui.horizontal_wrapped(|ui| {
                            ui.spacing_mut().item_spacing = Vec2::new(12.0, 12.0);
                            for album in library.albums.iter().take(8) {
                                let card_w = 162.0;
                                let resp = Frame::new()
                                    .fill(palette.bg_card)
                                    .stroke(Stroke::new(1.0_f32, palette.border_dark))
                                    .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                                    .inner_margin(Margin::same(10))
                                    .show(ui, |ui| {
                                        ui.set_width(card_w - 20.0);
                                        let (cov_rect, _) = ui.allocate_exact_size(
                                            Vec2::splat(card_w - 20.0),
                                            Sense::hover(),
                                        );
                                        let tex = album
                                            .cover_url
                                            .as_ref()
                                            .and_then(|u| covers.get(u))
                                            .and_then(|o| o.as_ref());
                                        paint_cover_or_placeholder(
                                            ui,
                                            cov_rect,
                                            tex,
                                            palette,
                                            RADIUS_SM as u8,
                                        );
                                        ui.add_space(8.0);
                                        ui.label(
                                            bold_text(&album.name, 12.5, palette.text_light),
                                        );
                                        ui.label(
                                            RichText::new(&album.subtitle)
                                                .size(11.0)
                                                .color(palette.text_muted),
                                        );
                                    })
                                    .response
                                    .interact(Sense::click());
                                if resp.clicked() {
                                    action = ContentAction::PlayFromList(album.tracks.clone(), 0);
                                }
                            }
                        });
                    }
                });
            });
            ui.add_space(SPACE_MD);
        });

    action
}

#[allow(clippy::too_many_arguments)]
fn render_tracks_view(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    library: &LibraryController,
    liked_ids: &HashSet<i64>,
    current_track_id: Option<i64>,
    is_playing: bool,
    covers: &HashMap<String, Option<TextureHandle>>,
    scroll_to_section: &mut Option<String>,
) -> ContentAction {
    let mut action = ContentAction::None;

    // Header summary bar
    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        ui.label(bold_text(
            strings.get("nav_tracks"),
            18.0,
            palette.text_light,
        ));
        ui.add_space(8.0);
        ui.label(
            RichText::new(format!(
                "{} · {:.1} h",
                format_count_label(library.tracks.len(), strings.is_spanish()),
                library.total_duration_hours().abs()
            ))
            .size(12.0)
            .color(palette.text_muted),
        );
        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
            ui.add_space(SPACE_MD);
            if ui
                .button(icon_text("refresh", 14.0, palette.text_light))
                .on_hover_text(strings.get_or("action_refresh", "Actualizar"))
                .clicked()
            {
                action = ContentAction::RefreshLibrary;
            }
            if !library.tracks.is_empty()
                && pill_button(
                    ui,
                    palette,
                    "play",
                    strings.get("ctx_play_now"),
                    false,
                )
            {
                action = ContentAction::PlayFromList(library.tracks.clone(), 0);
            }
        });
    });

    if !library.status_message.is_empty() && !library.tracks.is_empty() {
        let display_msg = if library.status_message.starts_with("err_")
            || library.status_message.starts_with("state_")
        {
            strings.get(&library.status_message)
        } else {
            &library.status_message
        };
        ui.add_space(4.0);
        ui.horizontal(|ui| {
            ui.add_space(SPACE_MD);
            Frame::new()
                .fill(palette.bg_surface)
                .stroke(Stroke::new(1.0_f32, palette.border_dark))
                .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
                .inner_margin(Margin::symmetric(10, 4))
                .show(ui, |ui| {
                    ui.label(RichText::new(display_msg).size(11.5).color(palette.text_muted));
                });
        });
    }

    if library.tracks.is_empty() && !library.is_loading {
        return render_empty_state(ui, palette, strings, false);
    }

    ui.add_space(6.0);

    // Collect available section markers for A-Z jump rail
    let mut active_sections: HashSet<String> = HashSet::new();
    for track in &library.tracks {
        active_sections.insert(track.section.clone());
    }

    let az_w = 24.0;
    let list_w = (ui.available_width() - az_w - 8.0).max(200.0);

    ui.horizontal_top(|ui| {
        ui.allocate_ui(Vec2::new(list_w, ui.available_height()), |ui| {
            ScrollArea::vertical()
                .id_salt("tracks_scroll")
                .auto_shrink([false, false])
                .show(ui, |ui| {
                    ui.set_width((ui.available_width() - 8.0).max(160.0));
                    let target_sec = scroll_to_section.take();
                    let mut scrolled_sec = false;

                    for (idx, track) in library.tracks.iter().enumerate() {
                        let is_current = current_track_id == Some(track.id);
                        let is_liked = liked_ids.contains(&track.id);
                        let tex = cover_for_track(track, covers);

                        let row_resp = render_track_row(
                            ui,
                            idx,
                            track,
                            tex,
                            is_current,
                            is_playing,
                            is_liked,
                            palette,
                            strings,
                            &library.tracks,
                            &mut action,
                        );

                        if !scrolled_sec {
                            if let Some(ref sec) = target_sec {
                                if track.section.eq_ignore_ascii_case(sec) {
                                    row_resp.scroll_to_me(Some(Align::TOP));
                                    scrolled_sec = true;
                                }
                            }
                        }
                    }

                    if library.has_more {
                        ui.add_space(10.0);
                        ui.centered_and_justified(|ui| {
                            if ui
                                .button(if library.is_loading {
                                    "..."
                                } else {
                                    strings.get_or("action_load_more", "Cargar mas")
                                })
                                .clicked()
                                && !library.is_loading
                            {
                                action = ContentAction::LoadMore;
                            }
                        });
                    }
                    ui.add_space(16.0);
                });
        });

        // A-Z jump rail
        ui.vertical(|ui| {
            ui.set_width(az_w);
            ui.spacing_mut().item_spacing.y = 0.0;
            let letters: [&str; 27] = [
                "#", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
                "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
            ];
            for letter in letters {
                let present = active_sections.contains(letter);
                let col = if present {
                    palette.text_muted
                } else {
                    ThemePalette::with_alpha(palette.text_dim, 0.35)
                };
                let resp = ui.add_sized(
                    Vec2::new(az_w - 2.0, 17.0),
                    egui::Button::new(RichText::new(letter).size(10.0).color(col)).frame(false),
                );
                if present && resp.clicked() {
                    *scroll_to_section = Some(letter.to_string());
                }
            }
        });
    });

    action
}

#[allow(clippy::too_many_arguments)]
fn render_group_browser(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    tab_kind: &str,
    groups: &[CollectionGroup],
    selected_key: &Option<String>,
    liked_ids: &HashSet<i64>,
    current_track_id: Option<i64>,
    covers: &HashMap<String, Option<TextureHandle>>,
) -> ContentAction {
    let mut action = ContentAction::None;

    if let Some(key) = selected_key {
        if let Some(group) = groups.iter().find(|g| &g.name == key) {
            ui.horizontal(|ui| {
                ui.add_space(SPACE_MD);
                if ui
                    .button(icon_text("left", 15.0, palette.text_light))
                    .clicked()
                {
                    action = ContentAction::SelectGroup(None);
                }
                ui.add_space(6.0);
                ui.label(bold_text(&group.name, 18.0, palette.text_light));
                ui.add_space(8.0);
                ui.label(
                    RichText::new(&group.subtitle)
                        .size(12.0)
                        .color(palette.text_muted),
                );
                ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                    ui.add_space(SPACE_MD);
                    if !group.tracks.is_empty()
                        && pill_button(
                            ui,
                            palette,
                            "play",
                            strings.get("ctx_play_now"),
                            true,
                        )
                    {
                        action = ContentAction::PlayFromList(group.tracks.clone(), 0);
                    }
                });
            });
            ui.add_space(8.0);
            ScrollArea::vertical()
                .id_salt(("group_detail", tab_kind, key))
                .auto_shrink([false, false])
                .show(ui, |ui| {
                    for (idx, track) in group.tracks.iter().enumerate() {
                        let is_current = current_track_id == Some(track.id);
                        let is_liked = liked_ids.contains(&track.id);
                        let tex = cover_for_track(track, covers);
                        render_track_row(
                            ui,
                            idx,
                            track,
                            tex,
                            is_current,
                            false,
                            is_liked,
                            palette,
                            strings,
                            &group.tracks,
                            &mut action,
                        );
                    }
                });
            return action;
        }
    }

    if groups.is_empty() {
        return render_empty_state(ui, palette, strings, false);
    }

    ScrollArea::vertical()
        .id_salt(("group_grid", tab_kind))
        .auto_shrink([false, false])
        .show(ui, |ui| {
            ui.add_space(6.0);
            ui.horizontal(|ui| {
                ui.add_space(SPACE_MD);
                ui.horizontal_wrapped(|ui| {
                    ui.spacing_mut().item_spacing = Vec2::new(12.0, 12.0);
                    for group in groups {
                        let card_w = 172.0;
                        let card_id = ui.id().with(("grp_card", tab_kind, &group.name));
                        let hov = ui
                            .ctx()
                            .data(|d| d.get_temp::<bool>(card_id.with("hov")))
                            .unwrap_or(false);
                        let (h_t, _) = animate_hover_and_active(ui, card_id, hov, false);
                        let resp = Frame::new()
                            .fill(lerp_color(palette.bg_card, palette.bg_hover, h_t))
                            .stroke(Stroke::new(
                                1.0_f32,
                                lerp_color(palette.border_dark, palette.border_highlight, h_t),
                            ))
                            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                            .inner_margin(Margin::same(10))
                            .show(ui, |ui| {
                                ui.set_width(card_w - 20.0);
                                let (cov_rect, _) = ui.allocate_exact_size(
                                    Vec2::splat(card_w - 20.0),
                                    Sense::hover(),
                                );
                                let tex = group
                                    .cover_url
                                    .as_ref()
                                    .and_then(|u| covers.get(u))
                                    .and_then(|o| o.as_ref());
                                paint_cover_or_placeholder(
                                    ui,
                                    cov_rect,
                                    tex,
                                    palette,
                                    RADIUS_SM as u8,
                                );
                                ui.add_space(8.0);
                                ui.label(bold_text(&group.name, 12.5, palette.text_light));
                                ui.label(
                                    RichText::new(&group.subtitle)
                                        .size(11.0)
                                        .color(palette.text_muted),
                                );
                            })
                            .response
                            .interact(Sense::click());
                        ui.ctx()
                            .data_mut(|d| d.insert_temp(card_id.with("hov"), resp.hovered()));
                        if resp.clicked() {
                            action = ContentAction::SelectGroup(Some(group.name.clone()));
                        }
                    }
                });
            });
            ui.add_space(SPACE_MD);
        });

    action
}

#[allow(clippy::too_many_arguments)]
fn render_curated_track_list(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    title: &str,
    icon: &str,
    tracks: &[Track],
    liked_ids: &HashSet<i64>,
    current_track_id: Option<i64>,
    is_playing: bool,
    covers: &HashMap<String, Option<TextureHandle>>,
    empty_hint: &str,
) -> ContentAction {
    let mut action = ContentAction::None;
    ui.add_space(SPACE_MD);
    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        ui.label(icon_text(icon, 18.0, palette.primary));
        ui.label(bold_text(title, 18.0, palette.text_light));
        ui.add_space(8.0);
        ui.label(
            RichText::new(format_count_label(tracks.len(), strings.is_spanish()))
                .size(12.0)
                .color(palette.text_muted),
        );
        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
            ui.add_space(SPACE_MD);
            if !tracks.is_empty()
                && pill_button(
                    ui,
                    palette,
                    "play",
                    strings.get("ctx_play_now"),
                    true,
                )
            {
                action = ContentAction::PlayFromList(tracks.to_vec(), 0);
            }
        });
    });
    ui.add_space(10.0);

    if tracks.is_empty() {
        ui.centered_and_justified(|ui| {
            ui.label(RichText::new(empty_hint).size(13.0).color(palette.text_muted));
        });
        return action;
    }

    ScrollArea::vertical()
        .id_salt(("curated_scroll", title))
        .auto_shrink([false, false])
        .show(ui, |ui| {
            for (idx, track) in tracks.iter().enumerate() {
                let is_current = current_track_id == Some(track.id);
                let is_liked = liked_ids.contains(&track.id);
                let tex = cover_for_track(track, covers);
                render_track_row(
                    ui,
                    idx,
                    track,
                    tex,
                    is_current,
                    is_playing,
                    is_liked,
                    palette,
                    strings,
                    tracks,
                    &mut action,
                );
            }
        });

    action
}

#[allow(clippy::too_many_arguments)]
fn render_playlists_view(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    playlists: &PlaylistController,
    library: &LibraryController,
    selected_playlist: &Option<Playlist>,
    current_track_id: Option<i64>,
    is_playing: bool,
    covers: &HashMap<String, Option<TextureHandle>>,
    settings: &SettingsData,
) -> ContentAction {
    let mut action = ContentAction::None;
    let list = playlists.effective_playlists(settings);

    if let Some(pl) = selected_playlist {
        let pl_tracks: Vec<Track> = if pl.id == -1 {
            library
                .all_tracks
                .iter()
                .filter(|t| playlists.liked_ids.contains(&t.id))
                .cloned()
                .collect()
        } else {
            pl.tracks.clone()
        };

        ui.add_space(SPACE_MD);
        ui.horizontal(|ui| {
            ui.add_space(SPACE_MD);
            if ui
                .button(icon_text("left", 15.0, palette.text_light))
                .clicked()
            {
                action = ContentAction::SelectPlaylist(None);
            }
            ui.label(bold_text(&pl.name, 18.0, palette.text_light));
            ui.add_space(8.0);
            ui.label(
                RichText::new(format_count_label(pl_tracks.len(), strings.is_spanish()))
                    .size(12.0)
                    .color(palette.text_muted),
            );
            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                ui.add_space(SPACE_MD);
                if pl.id > 0
                    && ui
                        .button(icon_text("close", 14.0, palette.danger))
                        .on_hover_text(strings.get("pl_delete"))
                        .clicked()
                {
                    action = ContentAction::DeletePlaylist(pl.id);
                }
                if !pl_tracks.is_empty()
                    && pill_button(
                        ui,
                        palette,
                        "play",
                        strings.get("ctx_play_now"),
                        true,
                    )
                {
                    action = ContentAction::PlayFromList(pl_tracks.clone(), 0);
                }
            });
        });
        ui.add_space(10.0);

        ScrollArea::vertical()
            .id_salt(("pl_tracks", pl.id))
            .auto_shrink([false, false])
            .show(ui, |ui| {
                for (idx, track) in pl_tracks.iter().enumerate() {
                    let is_current = current_track_id == Some(track.id);
                    let is_liked = playlists.liked_ids.contains(&track.id);
                    let tex = cover_for_track(track, covers);
                    render_track_row(
                        ui,
                        idx,
                        track,
                        tex,
                        is_current,
                        is_playing,
                        is_liked,
                        palette,
                        strings,
                        &pl_tracks,
                        &mut action,
                    );
                }
            });

        return action;
    }

    ui.add_space(SPACE_MD);
    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        ui.label(icon_text("list-music", 18.0, palette.primary));
        ui.label(bold_text(
            strings.get("nav_playlists"),
            18.0,
            palette.text_light,
        ));
        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
            ui.add_space(SPACE_MD);
            if pill_button(
                ui,
                palette,
                "plus",
                strings.get("nav_new_playlist"),
                true,
            ) {
                action = ContentAction::NewPlaylist;
            }
        });
    });
    ui.add_space(12.0);

    ScrollArea::vertical()
        .id_salt("playlists_grid")
        .auto_shrink([false, false])
        .show(ui, |ui| {
            ui.horizontal(|ui| {
                ui.add_space(SPACE_MD);
                ui.horizontal_wrapped(|ui| {
                    ui.spacing_mut().item_spacing = Vec2::new(12.0, 12.0);
                    for pl in list {
                        let card_id = ui.id().with(("pl_card", pl.id));
                        let hov = ui
                            .ctx()
                            .data(|d| d.get_temp::<bool>(card_id.with("hov")))
                            .unwrap_or(false);
                        let (h_t, _) = animate_hover_and_active(ui, card_id, hov, false);
                        let resp = Frame::new()
                            .fill(lerp_color(palette.bg_card, palette.bg_hover, h_t))
                            .stroke(Stroke::new(
                                1.0_f32,
                                lerp_color(palette.border_dark, palette.border_highlight, h_t),
                            ))
                            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                            .inner_margin(Margin::same(14))
                            .show(ui, |ui| {
                                ui.set_width(188.0);
                                ui.horizontal(|ui| {
                                    ui.label(icon_text("list-music", 20.0, palette.primary));
                                    ui.vertical(|ui| {
                                        ui.label(bold_text(&pl.name, 13.5, palette.text_light));
                                        ui.label(
                                            RichText::new(format_count_label(
                                                pl.total_tracks.max(pl.tracks.len()),
                                                strings.is_spanish(),
                                            ))
                                            .size(11.0)
                                            .color(palette.text_muted),
                                        );
                                    });
                                });
                            })
                            .response
                            .interact(Sense::click());
                        ui.ctx()
                            .data_mut(|d| d.insert_temp(card_id.with("hov"), resp.hovered()));
                        if resp.clicked() {
                            action = ContentAction::SelectPlaylist(Some(pl));
                        }
                    }
                });
            });
        });

    action
}

#[allow(clippy::too_many_arguments)]
fn render_queue_view(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    queue: &QueueController,
    liked_ids: &HashSet<i64>,
    current_track_id: Option<i64>,
    is_playing: bool,
    covers: &HashMap<String, Option<TextureHandle>>,
) -> ContentAction {
    let mut action = ContentAction::None;

    ui.add_space(SPACE_MD);
    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        ui.label(icon_text("list", 18.0, palette.primary));
        ui.label(bold_text(strings.get("nav_queue"), 18.0, palette.text_light));
        ui.add_space(8.0);
        if queue.is_mix_active {
            Frame::new()
                .fill(palette.primary_dim)
                .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
                .inner_margin(Margin::symmetric(10, 3))
                .show(ui, |ui| {
                    ui.label(
                        RichText::new(format!("Mix · {}", queue.mix_seed_title))
                            .size(11.0)
                            .color(palette.active_fg()),
                    );
                });
        }
        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
            ui.add_space(SPACE_MD);
            if !queue.queue.is_empty()
                && pill_button(
                    ui,
                    palette,
                    "close",
                    strings.get_or("queue_clear", "Vaciar"),
                    false,
                )
            {
                action = ContentAction::QueueClear;
            }
        });
    });
    ui.add_space(10.0);

    if queue.queue.is_empty() {
        ui.centered_and_justified(|ui| {
            ui.label(
                RichText::new(strings.get_or("empty_queue", "La cola esta vacia"))
                    .size(13.0)
                    .color(palette.text_muted),
            );
        });
        return action;
    }

    ScrollArea::vertical()
        .id_salt("queue_scroll")
        .auto_shrink([false, false])
        .show(ui, |ui| {
            for (idx, track) in queue.queue.iter().enumerate() {
                let is_current = queue.current_index == Some(idx)
                    || current_track_id == Some(track.id);
                let is_liked = liked_ids.contains(&track.id);
                let tex = cover_for_track(track, covers);
                let mut inner_action = ContentAction::None;
                let row_resp = render_track_row(
                    ui,
                    idx,
                    track,
                    tex,
                    is_current,
                    is_playing,
                    is_liked,
                    palette,
                    strings,
                    &queue.queue,
                    &mut inner_action,
                );
                if matches!(inner_action, ContentAction::PlayFromList(_, _)) || row_resp.clicked() {
                    action = ContentAction::QueueJumpTo(idx);
                } else if !matches!(inner_action, ContentAction::None) {
                    action = inner_action;
                }
            }
        });

    action
}

fn render_empty_state(
    ui: &mut Ui,
    palette: &ThemePalette,
    strings: &StringCatalog,
    needs_setup: bool,
) -> ContentAction {
    let mut action = ContentAction::None;
    let avail = ui.available_rect_before_wrap();
    let card_w = 420.0_f32.min((avail.width() - 32.0).max(260.0));
    let card_h = 224.0_f32.min((avail.height() - 24.0).max(180.0));
    let card_rect = Rect::from_center_size(avail.center(), Vec2::new(card_w, card_h));
    ui.allocate_rect(avail, egui::Sense::hover());

    ui.allocate_new_ui(egui::UiBuilder::new().max_rect(card_rect), |ui| {
        Frame::new()
            .fill(palette.bg_surface)
            .stroke(Stroke::new(1.0_f32, palette.border_dark))
            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
            .inner_margin(Margin::symmetric(28, 24))
            .show(ui, |ui| {
                ui.set_width(ui.available_width());
                ui.vertical_centered(|ui| {
                    let (icon_rect, _) = ui.allocate_exact_size(Vec2::splat(52.0), Sense::hover());
                    let pulse = 0.5 + 0.5 * (ui.ctx().input(|i| i.time) as f32 * 2.2).sin();
                    ui.painter().circle_filled(
                        icon_rect.center(),
                        22.0 + pulse * 5.0,
                        ThemePalette::with_alpha(palette.primary, 0.08 + 0.07 * (1.0 - pulse)),
                    );
                    ui.painter().circle_filled(
                        icon_rect.center(),
                        18.0,
                        ThemePalette::with_alpha(palette.primary_dim, 0.7),
                    );
                    ui.painter().text(
                        icon_rect.center(),
                        egui::Align2::CENTER_CENTER,
                        icon_glyph("disc-album"),
                        egui::FontId::new(28.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
                        palette.primary,
                    );
                    ui.add_space(10.0);
                    ui.label(bold_text(
                        if needs_setup {
                            strings.get("empty_no_source_title")
                        } else {
                            strings.get("empty_no_tracks_title")
                        },
                        16.0,
                        palette.text_light,
                    ));
                    ui.add_space(6.0);
                    ui.label(
                        RichText::new(if needs_setup {
                            strings.get("empty_no_source_subtitle")
                        } else {
                            strings.get("empty_no_tracks_subtitle")
                        })
                        .size(12.5)
                        .color(palette.text_muted),
                    );
                    ui.add_space(18.0);
                    ui.allocate_ui_with_layout(
                        Vec2::new(ui.available_width(), 34.0),
                        egui::Layout::left_to_right(Align::Center),
                        |ui| {
                            let row_w = 276.0_f32.min(ui.available_width());
                            let pad = ((ui.available_width() - row_w) * 0.5).max(0.0);
                            ui.add_space(pad);
                            if pill_button(
                                ui,
                                palette,
                                "server",
                                strings.get("settings_nav_source"),
                                true,
                            ) {
                                action = if needs_setup {
                                    ContentAction::OpenWizard
                                } else {
                                    ContentAction::OpenSettings
                                };
                            }
                            ui.add_space(8.0);
                            if pill_button(
                                ui,
                                palette,
                                "refresh",
                                strings.get_or("action_refresh", "Reintentar"),
                                false,
                            ) {
                                action = ContentAction::RefreshLibrary;
                            }
                        },
                    );
                });
            });
    });
    action
}

#[allow(clippy::too_many_arguments)]
fn render_track_row(
    ui: &mut Ui,
    index: usize,
    track: &Track,
    cover_tex: Option<&TextureHandle>,
    is_current: bool,
    is_playing: bool,
    is_liked: bool,
    palette: &ThemePalette,
    strings: &StringCatalog,
    source_slice: &[Track],
    action: &mut ContentAction,
) -> egui::Response {
    let row_h = 54.0;
    let avail_w = (ui.available_width() - 24.0).max(260.0);
    let mut click_resp: Option<egui::Response> = None;

    ui.horizontal(|ui| {
        ui.add_space(SPACE_MD);
        let row_id = ui.id().with(("track_row", track.id, index));
        let row_hovered = ui
            .ctx()
            .data(|d| d.get_temp::<bool>(row_id.with("hov")))
            .unwrap_or(false);
        let (h_t, c_t) = animate_hover_and_active(ui, row_id, row_hovered, is_current);
        let frame_fill = lerp_color(
            lerp_color(Color32::TRANSPARENT, palette.bg_hover, h_t * 0.72),
            palette.bg_selected,
            c_t,
        );
        let stroke = if c_t > 0.02 {
            Stroke::new(1.0_f32, ThemePalette::with_alpha(palette.primary, 0.5 * c_t))
        } else {
            Stroke::NONE
        };

        let frame_resp = Frame::new()
            .fill(frame_fill)
            .stroke(stroke)
            .corner_radius(CornerRadius::same(RADIUS_SM as u8))
            .inner_margin(Margin::symmetric(10, 6))
            .show(ui, |ui| {
                ui.set_width(avail_w);
                ui.set_height(row_h - 12.0);
                ui.horizontal_centered(|ui| {
                    // Index or animated active EQ indicator
                    let (idx_rect, _) = ui.allocate_exact_size(Vec2::new(28.0, 24.0), Sense::hover());
                    if is_current {
                        paint_animated_eq_bars(ui, idx_rect, palette.primary, is_playing);
                    } else if h_t > 0.45 {
                        ui.painter().text(
                            idx_rect.center(),
                            egui::Align2::CENTER_CENTER,
                            icon_glyph("play"),
                            egui::FontId::new(
                                13.0,
                                egui::FontFamily::Name(std::sync::Arc::from("lucide")),
                            ),
                            lerp_color(palette.text_muted, palette.text_light, h_t),
                        );
                    } else {
                        ui.painter().text(
                            idx_rect.center(),
                            egui::Align2::CENTER_CENTER,
                            format!("{}", index + 1),
                            egui::FontId::new(11.5, egui::FontFamily::Proportional),
                            palette.text_dim,
                        );
                    }

                    // Cover thumbnail
                    let (cov_rect, _) = ui.allocate_exact_size(Vec2::splat(38.0), Sense::hover());
                    paint_cover_or_placeholder(ui, cov_rect, cover_tex, palette, 6);
                    ui.add_space(10.0);

                    // Right-side controls first inside remaining area or fixed-width middle
                    let right_controls_w = 180.0;
                    let text_w = (ui.available_width() - right_controls_w).max(100.0);
                    ui.allocate_ui(Vec2::new(text_w, 38.0), |ui| {
                        ui.vertical(|ui| {
                            ui.spacing_mut().item_spacing.y = 1.0;
                            ui.label(bold_text(
                                &track.title,
                                13.0,
                                if is_current {
                                    palette.primary
                                } else {
                                    palette.text_light
                                },
                            ));
                            ui.label(
                                RichText::new(format!("{} · {}", track.artist, track.album))
                                    .size(11.5)
                                    .color(palette.text_muted),
                            );
                        });
                    });

                    ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                        ui.spacing_mut().item_spacing.x = 8.0;

                        // Overflow "..." menu button
                        let menu_btn = ui.button(RichText::new("...").size(13.0));
                        let menu_id = Id::new(("track_menu", track.id, index));
                        if menu_btn.clicked() {
                            ui.memory_mut(|m| m.toggle_popup(menu_id));
                        }
                        egui::popup_below_widget(
                            ui,
                            menu_id,
                            &menu_btn,
                            egui::PopupCloseBehavior::CloseOnClick,
                            |ui| {
                                render_track_context_menu(
                                    ui, track, is_liked, strings, palette, action,
                                );
                            },
                        );

                        // Duration
                        ui.label(
                            RichText::new(format_duration(track.duration))
                                .size(11.5)
                                .color(palette.text_muted),
                        );

                        // Favorite heart button
                        let heart_col = if is_liked {
                            palette.primary
                        } else {
                            palette.text_dim
                        };
                        if ui
                            .add(egui::Button::new(icon_text("heart", 14.0, heart_col)).frame(false))
                            .clicked()
                        {
                            *action = ContentAction::ToggleLike(track.id);
                        }

                        // Quality badge pill
                        let q_col = fidelity_color(&track.fidelity_tier, palette);
                        Frame::new()
                            .fill(ThemePalette::with_alpha(q_col, 0.14))
                            .stroke(Stroke::new(1.0_f32, ThemePalette::with_alpha(q_col, 0.55)))
                            .corner_radius(CornerRadius::same(4))
                            .inner_margin(Margin::symmetric(6, 2))
                            .show(ui, |ui| {
                                ui.label(bold_text(&track.fidelity_tier, 9.5, q_col));
                            })
                            .response
                            .on_hover_text(&track.quality_details);
                    });
                });
            })
            .response
            .interact(Sense::click());

        if frame_resp.clicked() {
            *action = ContentAction::PlayFromList(source_slice.to_vec(), index);
        }
        if c_t > 0.02 {
            let accent_h = (frame_resp.rect.height() - 14.0).max(8.0) * c_t;
            let bar = Rect::from_center_size(
                Pos2::new(frame_resp.rect.left() + 3.0, frame_resp.rect.center().y),
                Vec2::new(3.0, accent_h),
            );
            ui.painter()
                .rect_filled(bar, CornerRadius::same(2), palette.primary);
        }
        ui.ctx()
            .data_mut(|d| d.insert_temp(row_id.with("hov"), frame_resp.hovered()));
        frame_resp.context_menu(|ui| {
            render_track_context_menu(ui, track, is_liked, strings, palette, action);
        });
        click_resp = Some(frame_resp);
    });

    click_resp.expect("row built inside horizontal layout")
}

fn render_track_context_menu(
    ui: &mut Ui,
    track: &Track,
    is_liked: bool,
    strings: &StringCatalog,
    palette: &ThemePalette,
    action: &mut ContentAction,
) {
    ui.set_min_width(190.0);
    if menu_item(ui, "play", strings.get("ctx_play_now"), palette.text_light) {
        *action = ContentAction::PlayNow(track.clone());
        ui.close_menu();
    }
    if menu_item(ui, "next", strings.get("ctx_play_next"), palette.text_light) {
        *action = ContentAction::PlayNext(track.clone());
        ui.close_menu();
    }
    if menu_item(
        ui,
        "list",
        strings.get("ctx_add_to_queue"),
        palette.text_light,
    ) {
        *action = ContentAction::AddToQueue(track.clone());
        ui.close_menu();
    }
    if menu_item(
        ui,
        "sparkles",
        strings.get("ctx_start_mix"),
        palette.accent,
    ) {
        *action = ContentAction::StartMix(track.clone());
        ui.close_menu();
    }
    ui.separator();
    let like_label = if is_liked {
        strings.get("ctx_unlike")
    } else {
        strings.get("ctx_like")
    };
    if menu_item(
        ui,
        "heart",
        like_label,
        if is_liked {
            palette.primary
        } else {
            palette.text_light
        },
    ) {
        *action = ContentAction::ToggleLike(track.id);
        ui.close_menu();
    }
    if menu_item(
        ui,
        "plus",
        strings.get("ctx_add_to_playlist"),
        palette.text_light,
    ) {
        *action = ContentAction::OpenAddToPlaylist(track.clone());
        ui.close_menu();
    }
    if menu_item(
        ui,
        "folder",
        strings.get_or("ctx_download_offline", "Descargar offline"),
        palette.text_light,
    ) {
        *action = ContentAction::DownloadTrack(track.clone());
        ui.close_menu();
    }
    ui.separator();
    if menu_item(
        ui,
        "info",
        strings.get_or("ctx_track_info", "Detalles tecnicos"),
        palette.text_muted,
    ) {
        *action = ContentAction::InspectTrack(track.clone());
        ui.close_menu();
    }
}

fn menu_item(ui: &mut Ui, icon: &str, label: &str, color: Color32) -> bool {
    ui.horizontal(|ui| {
        ui.label(icon_text(icon, 13.0, color));
        ui.button(RichText::new(label).size(12.5).color(color))
            .clicked()
    })
    .inner
}

fn pill_button(
    ui: &mut Ui,
    palette: &ThemePalette,
    icon: &str,
    label: &str,
    primary: bool,
) -> bool {
    let btn_id = ui.id().with(("pill_btn", icon, label, primary));
    let hov = ui
        .ctx()
        .data(|d| d.get_temp::<bool>(btn_id.with("hov")))
        .unwrap_or(false);
    let (h_t, _) = animate_hover_and_active(ui, btn_id, hov, primary);
    let base_fill = if primary {
        palette.primary
    } else {
        palette.bg_card
    };
    let fill = if primary {
        lerp_color(base_fill, Color32::WHITE, h_t * 0.14)
    } else {
        lerp_color(base_fill, palette.bg_hover, h_t)
    };
    let fg = if primary {
        palette.active_fg()
    } else {
        palette.text_light
    };
    let stroke = if primary {
        Stroke::NONE
    } else {
        Stroke::new(
            1.0_f32,
            lerp_color(palette.border_dark, palette.border_highlight, h_t),
        )
    };
    let resp = Frame::new()
        .fill(fill)
        .stroke(stroke)
        .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
        .inner_margin(Margin::symmetric(14, 7))
        .show(ui, |ui| {
            ui.horizontal(|ui| {
                ui.spacing_mut().item_spacing.x = 6.0;
                ui.label(icon_text(icon, 13.0, fg));
                ui.label(bold_text(label, 12.5, fg));
            });
        })
        .response
        .interact(Sense::click());
    ui.ctx()
        .data_mut(|d| d.insert_temp(btn_id.with("hov"), resp.hovered()));
    resp.clicked()
}

pub fn cover_for_track<'a>(
    track: &Track,
    covers: &'a HashMap<String, Option<TextureHandle>>,
) -> Option<&'a TextureHandle> {
    track
        .cover_url
        .as_ref()
        .and_then(|u| covers.get(u))
        .and_then(|opt| opt.as_ref())
}

pub fn paint_cover_or_placeholder(
    ui: &mut Ui,
    rect: Rect,
    cover_tex: Option<&TextureHandle>,
    palette: &ThemePalette,
    radius: u8,
) {
    if let Some(tex) = cover_tex {
        ui.painter().image(
            tex.id(),
            rect,
            Rect::from_min_max(Pos2::ZERO, Pos2::new(1.0, 1.0)),
            Color32::WHITE,
        );
    } else {
        ui.painter()
            .rect_filled(rect, CornerRadius::same(radius), palette.bg_surface);
        ui.painter().rect_stroke(
            rect,
            CornerRadius::same(radius),
            Stroke::new(1.0_f32, palette.border_dark),
            egui::StrokeKind::Inside,
        );
        ui.painter().text(
            rect.center(),
            egui::Align2::CENTER_CENTER,
            crate::ui::theme::icon_glyph("music"),
            egui::FontId::new(
                (rect.width() * 0.42).clamp(14.0, 42.0),
                egui::FontFamily::Name(std::sync::Arc::from("lucide")),
            ),
            palette.text_dim,
        );
    }
}

fn format_count_label(count: usize, spanish: bool) -> String {
    if spanish {
        if count == 1 {
            String::from("1 pista")
        } else {
            format!("{count} pistas")
        }
    } else if count == 1 {
        String::from("1 track")
    } else {
        format!("{count} tracks")
    }
}