// Location: src/ui/sidebar.rs
// Function: Interactive resizable SideNav rail with drag splitter (160..500px),
// snap-collapse (<120px), double-click toggle, capsule nav pills, and tools.
// All Rights Reserved isgaar

use egui::{
    Align, Color32, CornerRadius, CursorIcon, Frame, Layout, Margin, Pos2, Rect, RichText,
    ScrollArea, Sense, Stroke, Ui, Vec2,
};

use crate::data::model::Playlist;
use crate::shared::colors::ThemePalette;
use crate::shared::i18n::StringCatalog;
use crate::ui::theme::{
    animate_hover_and_active, bold_text, icon_glyph, icon_text, lerp_color, RADIUS_FULL,
    RADIUS_MD, SPACE_SM,
};

#[derive(Debug, Clone)]
pub enum SidebarAction {
    None,
    SelectView(String),
    NewPlaylist,
    OpenPlaylist(Playlist),
    OpenEqualizer,
    OpenSleepTimer,
    OpenSettings,
    OpenAbout,
    WidthCommitted,
}

#[allow(clippy::too_many_arguments)]
pub fn render_sidebar(
    ui: &mut Ui,
    panel_rect: Rect,
    palette: &ThemePalette,
    strings: &StringCatalog,
    active_view: &str,
    sidebar_width: &mut f32,
    sidebar_collapsed: &mut bool,
    playlists: &[Playlist],
    liked_count: usize,
) -> SidebarAction {
    let mut action = SidebarAction::None;

    let card_rect = panel_rect.shrink2(Vec2::new(6.0, 6.0));
    ui.painter().rect_filled(
        card_rect,
        CornerRadius::same(RADIUS_MD as u8),
        palette.bg_dark,
    );
    ui.painter().rect_stroke(
        card_rect,
        CornerRadius::same(RADIUS_MD as u8),
        Stroke::new(1.0_f32, palette.border_dark),
        egui::StrokeKind::Inside,
    );

    let expand_t = ((panel_rect.width() - 72.0) / 80.0).clamp(0.0, 1.0);
    let compact_mode = panel_rect.width() < 120.0;

    let inner_rect = card_rect.shrink(10.0);
    let mut nav_ui = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(inner_rect)
            .layout(Layout::top_down(Align::Min)),
    );

    // Header + collapse button
    nav_ui.horizontal(|ui| {
        if !compact_mode {
            ui.scope(|ui| {
                ui.multiply_opacity(expand_t);
                ui.label(bold_text(
                    strings.get("nav_section_library"),
                    11.0,
                    palette.text_muted,
                ));
            });
        }
        ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
            let tip = if *sidebar_collapsed {
                strings.get("sidebar_expand")
            } else {
                strings.get("sidebar_collapse")
            };
            let (btn_rect, btn_resp) = ui.allocate_exact_size(Vec2::splat(24.0), Sense::click());
            let (h_t, _) = animate_hover_and_active(
                ui,
                ui.id().with("sb_collapse_btn"),
                btn_resp.hovered(),
                *sidebar_collapsed,
            );
            ui.painter().rect_filled(
                btn_rect,
                CornerRadius::same(6),
                lerp_color(Color32::TRANSPARENT, palette.bg_hover, h_t),
            );
            ui.painter().text(
                btn_rect.center(),
                egui::Align2::CENTER_CENTER,
                icon_glyph("panel-left"),
                egui::FontId::new(14.5, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
                lerp_color(palette.text_muted, palette.text_light, h_t),
            );
            if btn_resp.on_hover_text(tip).clicked() {
                *sidebar_collapsed = !*sidebar_collapsed;
                action = SidebarAction::WidthCommitted;
            }
        });
    });

    nav_ui.add_space(SPACE_SM);

    // Primary capsule nav pills with smooth sliding indicator
    let primary_items = [
        ("listen", "home", strings.get("nav_listen")),
        ("library", "library", strings.get("nav_local_library")),
        ("playlists", "list-music", strings.get("nav_playlists")),
        ("queue", "list", strings.get("nav_queue")),
    ];

    let pill_h = 34.0;
    let pill_gap = 4.0;
    let start_y = nav_ui.cursor().top();
    let pill_w = nav_ui.available_width();
    let selected_idx = primary_items
        .iter()
        .position(|(id, _, _)| *id == active_view);

    let indicator_alpha = nav_ui.ctx().animate_bool_with_time(
        ui.id().with("sb_pill_visible"),
        selected_idx.is_some(),
        0.16,
    );
    if let Some(idx) = selected_idx {
        let target_y = start_y + idx as f32 * (pill_h + pill_gap);
        let anim_y = nav_ui.ctx().animate_value_with_time(
            ui.id().with("sb_active_y"),
            target_y,
            0.20,
        );
        if (anim_y - target_y).abs() > 0.25 {
            nav_ui.ctx().request_repaint();
        }
        let slide_rect = Rect::from_min_size(
            Pos2::new(inner_rect.left(), anim_y),
            Vec2::new(pill_w, pill_h),
        );
        nav_ui.painter().rect_filled(
            slide_rect,
            CornerRadius::same(RADIUS_FULL as u8),
            ThemePalette::with_alpha(palette.primary, indicator_alpha),
        );
    } else {
        // Keep last Y synced even while Favorites or a tool is selected
        let _ = nav_ui.ctx().animate_value_with_time(
            ui.id().with("sb_active_y"),
            start_y,
            0.20,
        );
    }

    for (view_id, icon, label) in primary_items {
        let selected = active_view == view_id;
        if nav_pill(
            &mut nav_ui,
            palette,
            view_id,
            icon,
            label,
            selected,
            compact_mode,
            expand_t,
            true,
        ) {
            action = SidebarAction::SelectView(view_id.to_string());
        }
        nav_ui.add_space(pill_gap);
    }

    nav_ui.add_space(6.0);

    // Liked Music card with smooth hover & clean typography
    let fav_selected = active_view == "favorites";
    let fav_id = ui.id().with("sb_liked_card");
    let fav_hov = ui
        .ctx()
        .data(|d| d.get_temp::<bool>(fav_id.with("hov")))
        .unwrap_or(false);
    let (fav_h, fav_a) = animate_hover_and_active(&nav_ui, fav_id, fav_hov, fav_selected);
    let fav_bg = lerp_color(
        lerp_color(palette.bg_card, palette.bg_hover, fav_h),
        palette.primary_dim,
        fav_a,
    );
    let fav_stroke = lerp_color(
        lerp_color(palette.border_dark, palette.border_highlight, fav_h),
        palette.primary,
        fav_a,
    );
    let fav_resp = Frame::new()
        .fill(fav_bg)
        .stroke(Stroke::new(1.0_f32, fav_stroke))
        .corner_radius(CornerRadius::same(10))
        .inner_margin(Margin::symmetric(8, 8))
        .show(&mut nav_ui, |ui| {
            ui.set_width(ui.available_width());
            ui.horizontal(|ui| {
                ui.spacing_mut().item_spacing.x = 10.0;
                Frame::new()
                    .fill(palette.primary)
                    .corner_radius(CornerRadius::same(7))
                    .inner_margin(Margin::same(6))
                    .show(ui, |ui| {
                        ui.label(icon_text("heart", 14.0, palette.active_fg()));
                    });
                if !compact_mode {
                    ui.vertical(|ui| {
                        ui.multiply_opacity(expand_t);
                        ui.spacing_mut().item_spacing.y = 2.0;
                        ui.add(
                            egui::Label::new(bold_text(
                                strings.get("nav_liked_title"),
                                11.5,
                                palette.text_light,
                            ))
                            .truncate(),
                        );
                        ui.add(
                            egui::Label::new(
                                RichText::new(format!(
                                    "{} · {}",
                                    liked_count,
                                    strings.get("nav_songs_suffix")
                                ))
                                .size(10.5)
                                .color(palette.text_muted),
                            )
                            .truncate(),
                        );
                    });
                }
            });
        })
        .response
        .interact(Sense::click());
    let fav_hovered = fav_resp.hovered();
    ui.ctx()
        .data_mut(|d| d.insert_temp(fav_id.with("hov"), fav_hovered));
    if fav_resp.clicked() {
        action = SidebarAction::SelectView(String::from("favorites"));
    }

    nav_ui.add_space(10.0);

    // Playlists header + new playlist button
    if !compact_mode {
        nav_ui.horizontal(|ui| {
            ui.multiply_opacity(expand_t);
            ui.label(bold_text(
                strings.get("nav_playlists"),
                11.0,
                palette.text_muted,
            ));
            ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                if ui
                    .add(
                        egui::Button::new(icon_text("plus", 14.0, palette.primary))
                            .frame(false),
                    )
                    .on_hover_text(strings.get("nav_new_playlist"))
                    .clicked()
                {
                    action = SidebarAction::NewPlaylist;
                }
            });
        });
        nav_ui.add_space(4.0);

        let bottom_tools_h = 166.0;
        let scroll_h = (nav_ui.available_height() - bottom_tools_h).max(40.0);
        ScrollArea::vertical()
            .id_salt("sidebar_playlists")
            .max_height(scroll_h)
            .show(&mut nav_ui, |ui| {
                ui.multiply_opacity(expand_t);
                if playlists.is_empty() {
                    ui.label(
                        RichText::new(strings.get("nav_playlists_hint"))
                            .size(11.0)
                            .color(palette.text_dim),
                    );
                } else {
                    for pl in playlists {
                        let pl_id = ui.id().with(("sb_pl", pl.id));
                        let pl_hov = ui
                            .ctx()
                            .data(|d| d.get_temp::<bool>(pl_id.with("hov")))
                            .unwrap_or(false);
                        let (h_t, _) = animate_hover_and_active(ui, pl_id, pl_hov, false);
                        let resp = Frame::new()
                            .fill(lerp_color(Color32::TRANSPARENT, palette.bg_hover, h_t))
                            .corner_radius(CornerRadius::same(8))
                            .inner_margin(Margin::symmetric(8, 5))
                            .show(ui, |ui| {
                                ui.set_width(ui.available_width());
                                ui.horizontal(|ui| {
                                    ui.label(icon_text("list-music", 13.0, palette.primary));
                                    ui.vertical(|ui| {
                                        ui.spacing_mut().item_spacing.y = 0.0;
                                        ui.add(
                                            egui::Label::new(
                                                RichText::new(&pl.name)
                                                    .size(12.0)
                                                    .color(palette.text_light),
                                            )
                                            .truncate(),
                                        );
                                        ui.label(
                                            RichText::new(format!(
                                                "{} {}",
                                                pl.total_tracks,
                                                strings.get("nav_songs_suffix")
                                            ))
                                            .size(10.0)
                                            .color(palette.text_muted),
                                        );
                                    });
                                });
                            })
                            .response
                            .interact(Sense::click());
                        let pl_hovered = resp.hovered();
                        ui.ctx()
                            .data_mut(|d| d.insert_temp(pl_id.with("hov"), pl_hovered));
                        if resp.clicked() {
                            action = SidebarAction::OpenPlaylist(pl.clone());
                        }
                    }
                }
            });
    } else {
        let spacer = (nav_ui.available_height() - 166.0).max(10.0);
        nav_ui.add_space(spacer);
    }

    // Bottom tools section
    nav_ui.separator();
    nav_ui.add_space(4.0);
    if nav_pill(
        &mut nav_ui,
        palette,
        "tool_eq",
        "sliders-horizontal",
        strings.get("nav_equalizer"),
        false,
        compact_mode,
        expand_t,
        false,
    ) {
        action = SidebarAction::OpenEqualizer;
    }
    if nav_pill(
        &mut nav_ui,
        palette,
        "tool_timer",
        "moon",
        strings.get("nav_sleep_timer"),
        false,
        compact_mode,
        expand_t,
        false,
    ) {
        action = SidebarAction::OpenSleepTimer;
    }
    if nav_pill(
        &mut nav_ui,
        palette,
        "tool_settings",
        "settings",
        strings.get("nav_settings"),
        false,
        compact_mode,
        expand_t,
        false,
    ) {
        action = SidebarAction::OpenSettings;
    }
    if nav_pill(
        &mut nav_ui,
        palette,
        "tool_about",
        "info",
        strings.get("nav_about"),
        false,
        compact_mode,
        expand_t,
        false,
    ) {
        action = SidebarAction::OpenAbout;
    }

    // Interactive Resizer splitter handle on the right edge
    let splitter_rect = Rect::from_min_max(
        Pos2::new(panel_rect.right() - 4.0, panel_rect.top() + 8.0),
        Pos2::new(panel_rect.right() + 4.0, panel_rect.bottom() - 8.0),
    );
    let split_resp = ui.interact(splitter_rect, ui.id().with("sidebar_resizer"), Sense::click_and_drag());
    if split_resp.hovered() || split_resp.dragged() {
        ui.ctx().set_cursor_icon(CursorIcon::ResizeHorizontal);
        ui.painter().line_segment(
            [
                Pos2::new(panel_rect.right(), panel_rect.top() + 12.0),
                Pos2::new(panel_rect.right(), panel_rect.bottom() - 12.0),
            ],
            Stroke::new(2.0_f32, palette.primary),
        );
    }
    if split_resp.double_clicked() {
        *sidebar_collapsed = !*sidebar_collapsed;
        action = SidebarAction::WidthCommitted;
    }
    if split_resp.dragged() {
        if let Some(pos) = ui.ctx().pointer_latest_pos() {
            let new_w = pos.x - panel_rect.left();
            if new_w < 120.0 {
                *sidebar_collapsed = true;
            } else {
                *sidebar_collapsed = false;
                *sidebar_width = new_w.clamp(160.0, 500.0);
            }
        }
    }
    if split_resp.drag_stopped() {
        action = SidebarAction::WidthCommitted;
    }

    action
}

#[allow(clippy::too_many_arguments)]
fn nav_pill(
    ui: &mut Ui,
    palette: &ThemePalette,
    item_id: &str,
    icon: &str,
    label: &str,
    selected: bool,
    collapsed: bool,
    expand_t: f32,
    uses_sliding_bg: bool,
) -> bool {
    let w = ui.available_width();
    let (rect, resp) = ui.allocate_exact_size(Vec2::new(w, 34.0), Sense::click());
    let (h_t, a_t) = animate_hover_and_active(
        ui,
        ui.id().with(("sb_nav", item_id)),
        resp.hovered(),
        selected,
    );

    if !uses_sliding_bg {
        let bg = lerp_color(
            Color32::TRANSPARENT,
            if selected {
                palette.primary
            } else {
                palette.bg_hover
            },
            if selected { a_t } else { h_t },
        );
        ui.painter()
            .rect_filled(rect, CornerRadius::same(RADIUS_FULL as u8), bg);
    } else if !selected && h_t > 0.01 {
        ui.painter().rect_filled(
            rect,
            CornerRadius::same(RADIUS_FULL as u8),
            ThemePalette::with_alpha(palette.bg_hover, h_t * 0.85),
        );
    }

    let icon_col = lerp_color(
        lerp_color(palette.text_muted, palette.text_light, h_t),
        palette.active_fg(),
        a_t,
    );
    let label_col = lerp_color(palette.text_light, palette.active_fg(), a_t);

    let icon_x = if collapsed {
        rect.center().x
    } else {
        rect.left() + 18.0
    };
    ui.painter().text(
        Pos2::new(icon_x, rect.center().y),
        egui::Align2::CENTER_CENTER,
        icon_glyph(icon),
        egui::FontId::new(15.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
        icon_col,
    );

    if !collapsed && expand_t > 0.08 {
        let text_col = Color32::from_rgba_unmultiplied(
            label_col.r(),
            label_col.g(),
            label_col.b(),
            ((label_col.a() as f32) * expand_t).round() as u8,
        );
        ui.painter().text(
            Pos2::new(rect.left() + 36.0, rect.center().y),
            egui::Align2::LEFT_CENTER,
            label,
            egui::FontId::new(
                12.5,
                egui::FontFamily::Name(std::sync::Arc::from("LumpySansBold")),
            ),
            text_col,
        );
    }

    resp.clicked()
}

