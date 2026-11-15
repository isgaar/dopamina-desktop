// Location: src/ui/chrome.rs
// Function: Frameless custom titlebar (56px) with app brand, ConnectionPill,
// centered search bar with recent-searches popup, avatar, and corner-flush
// window controls with drag-to-resize grips.
// All Rights Reserved isgaar

use egui::{
    Align, Color32, Context, CornerRadius, CursorIcon, Frame, Layout, Margin, Pos2, Rect,
    ResizeDirection, RichText, Sense, Stroke, TextureHandle, Ui, Vec2, ViewportCommand,
};

use crate::core::global_menu::{GlobalMenuAction, MenuEntry};
use crate::shared::colors::ThemePalette;
use crate::shared::i18n::StringCatalog;
use crate::ui::theme::{
    animate_hover_and_active, bold_text, icon_glyph, icon_text, lerp_color, RADIUS_FULL,
    RADIUS_MD, SPACE_MD, SPACE_SM,
};

pub enum TitleBarAction {
    None,
    OpenSession,
    OpenSettings,
    SearchChanged(String),
    ClearRecentSearches,
    SelectRecentSearch(String),
    MenuAction(GlobalMenuAction),
}

#[allow(clippy::too_many_arguments)]
pub fn render_title_bar(
    ui: &mut Ui,
    ctx: &Context,
    bar_rect: Rect,
    palette: &ThemePalette,
    strings: &StringCatalog,
    app_icon: Option<&TextureHandle>,
    menu_entries: &[MenuEntry],
    is_first_run: bool,
    is_maximized: bool,
    online: bool,
    endpoint: &str,
    user_initial: &str,
    username: &str,
    is_logged_in: bool,
    search_query: &mut String,
    recent_searches: &[String],
    show_recent_popup: &mut bool,
) -> TitleBarAction {
    let mut action = TitleBarAction::None;
    let bar_h = bar_rect.height();

    let bar_resp = ui.interact(
        bar_rect,
        ui.id().with("titlebar_drag"),
        Sense::click_and_drag(),
    );

    if bar_resp.drag_started() {
        ctx.send_viewport_cmd(ViewportCommand::StartDrag);
    }
    if bar_resp.double_clicked() && !is_first_run {
        ctx.send_viewport_cmd(ViewportCommand::Maximized(!is_maximized));
    }

    let top_rounding = if is_maximized { 0 } else { RADIUS_MD as u8 };
    ui.painter().rect_filled(
        bar_rect,
        CornerRadius {
            nw: top_rounding,
            ne: top_rounding,
            sw: 0,
            se: 0,
        },
        palette.bg_dark,
    );
    ui.painter().line_segment(
        [
            Pos2::new(bar_rect.left(), bar_rect.bottom()),
            Pos2::new(bar_rect.right(), bar_rect.bottom()),
        ],
        Stroke::new(1.0_f32, palette.border_dark),
    );

    let mut child = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(bar_rect)
            .layout(Layout::left_to_right(Align::Center)),
    );

    // Left: App Menu button + Brand + ConnectionPill
    child.add_space(SPACE_MD);
    if !menu_entries.is_empty() {
        let (menu_rect, menu_btn) = child.allocate_exact_size(Vec2::splat(30.0), Sense::click());
        let menu_popup_id = ui.id().with("app_global_menu_popup");
        let is_open = ui.memory(|m| m.is_popup_open(menu_popup_id));
        let (h_t, a_t) =
            animate_hover_and_active(&child, menu_popup_id.with("btn"), menu_btn.hovered(), is_open);
        let btn_bg = lerp_color(
            Color32::TRANSPARENT,
            if is_open {
                palette.primary_dim
            } else {
                palette.bg_hover
            },
            h_t.max(a_t),
        );
        child
            .painter()
            .rect_filled(menu_rect, CornerRadius::same(8), btn_bg);
        let icon_col = lerp_color(palette.text_muted, palette.primary, a_t.max(h_t * 0.5));
        child.painter().text(
            menu_rect.center(),
            egui::Align2::CENTER_CENTER,
            icon_glyph("list"),
            egui::FontId::new(15.5, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            icon_col,
        );
        let menu_btn = menu_btn.on_hover_text(strings.get("menu_app"));
        if menu_btn.clicked() {
            ui.memory_mut(|m| m.toggle_popup(menu_popup_id));
            if ui.memory(|m| !m.is_popup_open(menu_popup_id)) {
                ui.ctx().data_mut(|d| d.remove::<i32>(menu_popup_id.with("open_sub")));
            }
        }
        if !is_open {
            ui.ctx().data_mut(|d| d.remove::<i32>(menu_popup_id.with("open_sub")));
        }
        let open_t = ctx.animate_bool_with_time(menu_popup_id.with("anim"), is_open, 0.14);
        egui::popup_below_widget(
            &child,
            menu_popup_id,
            &menu_btn,
            egui::PopupCloseBehavior::CloseOnClick,
            |ui| {
                ui.set_min_width(216.0);
                ui.multiply_opacity(0.35 + 0.65 * open_t);
                configure_popup_visuals(ui, palette);
                render_popup_menu_tree(
                    ui,
                    menu_entries,
                    palette,
                    &mut action,
                    true,
                    menu_popup_id,
                );
            },
        );
        child.add_space(6.0);
    }
    if let Some(tex) = app_icon {
        child.add(
            egui::Image::new(tex)
                .fit_to_exact_size(Vec2::splat(28.0))
                .corner_radius(CornerRadius::same(7)),
        );
        child.add_space(SPACE_SM);
    }
    child.label(bold_text("Dopamine", 17.0, palette.text_light));
    child.add_space(10.0);

    if !is_first_run {
        let dot_color = if online {
            palette.success
        } else {
            palette.danger
        };
        let label_txt = if online && !endpoint.is_empty() {
            endpoint.to_string()
        } else if online {
            strings.get("session_status_connected").to_string()
        } else {
            strings.get("settings_status_disconnected").to_string()
        };
        let pill_id = ui.id().with("conn_pill");
        let hover_flag = ui
            .ctx()
            .data(|d| d.get_temp::<bool>(pill_id.with("hov")))
            .unwrap_or(false);
        let (pill_h, _) = animate_hover_and_active(&child, pill_id, hover_flag, false);
        let pill_resp = Frame::new()
            .fill(lerp_color(
                ThemePalette::with_alpha(palette.bg_surface, 0.88),
                palette.bg_hover,
                pill_h,
            ))
            .stroke(Stroke::new(
                1.0_f32,
                lerp_color(palette.border_dark, palette.border_highlight, pill_h),
            ))
            .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
            .inner_margin(Margin::symmetric(10, 4))
            .show(&mut child, |ui| {
                ui.horizontal(|ui| {
                    ui.spacing_mut().item_spacing.x = 6.0;
                    let (dot_rect, _) = ui.allocate_exact_size(Vec2::splat(7.0), Sense::hover());
                    if online {
                        let pulse = 0.5 + 0.5 * (ctx.input(|i| i.time) as f32 * 2.4).sin();
                        ui.painter().circle_filled(
                            dot_rect.center(),
                            4.5 + pulse * 1.5,
                            ThemePalette::with_alpha(dot_color, 0.22 * (1.0 - pulse * 0.4)),
                        );
                    }
                    ui.painter().circle_filled(dot_rect.center(), 3.5, dot_color);
                    ui.label(
                        RichText::new(label_txt)
                            .size(11.0)
                            .color(lerp_color(palette.text_muted, palette.text_light, pill_h)),
                    );
                });
            })
            .response
            .interact(Sense::click());
        let pill_hovered = pill_resp.hovered();
        ui.ctx()
            .data_mut(|d| d.insert_temp(pill_id.with("hov"), pill_hovered));
        if pill_resp.clicked() {
            action = TitleBarAction::OpenSession;
        }
    }

    // Right-aligned controls first via right_to_left inside remaining rect
    let rem_rect = Rect::from_min_max(
        Pos2::new(child.cursor().min.x, bar_rect.top()),
        bar_rect.max,
    );

    let mut right_ui = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(rem_rect)
            .layout(Layout::right_to_left(Align::Center)),
    );

    // Flush WindowControls on the far right (56px high, no right margin)
    let ctrl_w = 44.0;
    let close_rect = Rect::from_min_size(
        Pos2::new(rem_rect.right() - ctrl_w, rem_rect.top()),
        Vec2::new(ctrl_w, bar_h),
    );
    let max_rect = Rect::from_min_size(
        Pos2::new(close_rect.left() - ctrl_w, rem_rect.top()),
        Vec2::new(ctrl_w, bar_h),
    );
    let min_rect = Rect::from_min_size(
        Pos2::new(max_rect.left() - ctrl_w, rem_rect.top()),
        Vec2::new(ctrl_w, bar_h),
    );

    // Reserve exact width for 3 window buttons
    right_ui.allocate_exact_size(Vec2::new(ctrl_w * 3.0, bar_h), Sense::hover());

    let close_resp = ui.interact(close_rect, ui.id().with("win_close"), Sense::click());
    if close_resp.hovered() {
        ui.painter().rect_filled(
            close_rect,
            CornerRadius {
                nw: 0,
                ne: top_rounding,
                sw: 0,
                se: 0,
            },
            palette.danger,
        );
    }
    ui.painter().text(
        close_rect.center(),
        egui::Align2::CENTER_CENTER,
        crate::ui::theme::icon_glyph("x"),
        egui::FontId::new(15.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
        if close_resp.hovered() {
            Color32::WHITE
        } else {
            palette.text_light
        },
    );
    if close_resp.clicked() {
        ctx.send_viewport_cmd(ViewportCommand::Close);
    }

    let max_resp = ui.interact(max_rect, ui.id().with("win_max"), Sense::click());
    if max_resp.hovered() {
        ui.painter()
            .rect_filled(max_rect, CornerRadius::ZERO, palette.bg_hover);
    }
    ui.painter().text(
        max_rect.center(),
        egui::Align2::CENTER_CENTER,
        if is_maximized { "\u{e104}" } else { "\u{e16b}" },
        egui::FontId::new(13.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
        palette.text_light,
    );
    if max_resp.clicked() {
        ctx.send_viewport_cmd(ViewportCommand::Maximized(!is_maximized));
    }

    let min_resp = ui.interact(min_rect, ui.id().with("win_min"), Sense::click());
    if min_resp.hovered() {
        ui.painter()
            .rect_filled(min_rect, CornerRadius::ZERO, palette.bg_hover);
    }
    ui.painter().line_segment(
        [
            Pos2::new(min_rect.center().x - 5.0, min_rect.center().y),
            Pos2::new(min_rect.center().x + 5.0, min_rect.center().y),
        ],
        Stroke::new(1.5_f32, palette.text_light),
    );
    if min_resp.clicked() {
        ctx.send_viewport_cmd(ViewportCommand::Minimized(true));
    }

    if !is_first_run {
        right_ui.add_space(10.0);

        // Settings gear icon with smooth circular hover tint
        let (gear_rect, gear_resp) = right_ui.allocate_exact_size(Vec2::splat(30.0), Sense::click());
        let (gear_h, _) =
            animate_hover_and_active(&right_ui, ui.id().with("gear_btn"), gear_resp.hovered(), false);
        right_ui.painter().circle_filled(
            gear_rect.center(),
            14.0,
            lerp_color(Color32::TRANSPARENT, palette.bg_hover, gear_h),
        );
        right_ui.painter().text(
            gear_rect.center(),
            egui::Align2::CENTER_CENTER,
            icon_glyph("settings"),
            egui::FontId::new(15.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            lerp_color(palette.text_muted, palette.text_light, gear_h),
        );
        if gear_resp
            .on_hover_text(strings.get("nav_settings"))
            .clicked()
        {
            action = TitleBarAction::OpenSettings;
        }

        right_ui.add_space(6.0);

        // Compact Session avatar capsule (exact measured width, never expands in right_to_left)
        let avatar_label = if is_logged_in && !user_initial.is_empty() {
            user_initial.to_string()
        } else {
            String::from("?")
        };
        let avatar_fg = if is_logged_in {
            palette.active_fg()
        } else {
            palette.text_light
        };
        let initial_galley = right_ui.painter().layout_no_wrap(
            avatar_label.clone(),
            egui::FontId::new(
                11.5,
                egui::FontFamily::Name(std::sync::Arc::from("LumpySansBold")),
            ),
            avatar_fg,
        );
        let name_galley = if is_logged_in && !username.is_empty() {
            Some(right_ui.painter().layout_no_wrap(
                username.to_string(),
                egui::FontId::new(11.5, egui::FontFamily::Proportional),
                avatar_fg,
            ))
        } else {
            None
        };
        let pill_w = if let Some(ref ng) = name_galley {
            (10.0 + initial_galley.size().x + 6.0 + ng.size().x + 12.0).clamp(48.0, 160.0)
        } else {
            30.0
        };
        let (av_rect, av_resp) =
            right_ui.allocate_exact_size(Vec2::new(pill_w, 28.0), Sense::click());
        let (av_h, _) =
            animate_hover_and_active(&right_ui, ui.id().with("avatar_pill"), av_resp.hovered(), false);
        let av_base = if is_logged_in {
            palette.primary
        } else {
            palette.bg_surface
        };
        let av_fill = lerp_color(av_base, palette.bg_hover, if is_logged_in { av_h * 0.18 } else { av_h });
        right_ui.painter().rect_filled(
            av_rect,
            CornerRadius::same(14),
            av_fill,
        );
        right_ui.painter().rect_stroke(
            av_rect,
            CornerRadius::same(14),
            Stroke::new(
                1.0_f32,
                lerp_color(palette.border_dark, palette.border_highlight, av_h),
            ),
            egui::StrokeKind::Inside,
        );
        if let Some(ng) = name_galley {
            let badge_center = Pos2::new(av_rect.left() + 14.0, av_rect.center().y);
            right_ui.painter().circle_filled(
                badge_center,
                9.0,
                ThemePalette::with_alpha(avatar_fg, 0.18),
            );
            right_ui.painter().galley(
                Pos2::new(
                    badge_center.x - initial_galley.size().x * 0.5,
                    badge_center.y - initial_galley.size().y * 0.5,
                ),
                initial_galley,
                avatar_fg,
            );
            right_ui.painter().galley(
                Pos2::new(
                    badge_center.x + 13.0,
                    av_rect.center().y - ng.size().y * 0.5,
                ),
                ng,
                avatar_fg,
            );
        } else {
            right_ui.painter().galley(
                Pos2::new(
                    av_rect.center().x - initial_galley.size().x * 0.5,
                    av_rect.center().y - initial_galley.size().y * 0.5,
                ),
                initial_galley,
                avatar_fg,
            );
        }
        if av_resp
            .on_hover_text(strings.get("session_avatar_title"))
            .clicked()
        {
            action = TitleBarAction::OpenSession;
        }

        // Centered search box inside safe bounds between ConnectionPill and AvatarPill
        let left_bound = rem_rect.left() + 14.0;
        let right_bound = av_rect.left() - 14.0;
        let free_w = (right_bound - left_bound).max(0.0);
        if free_w >= 140.0 {
            let search_w = free_w.clamp(140.0, 390.0);
            let ideal_cx = bar_rect.center().x;
            let clamped_cx = ideal_cx.clamp(
                left_bound + search_w * 0.5,
                right_bound - search_w * 0.5,
            );
            let search_rect = Rect::from_center_size(
                Pos2::new(clamped_cx, bar_rect.center().y),
                Vec2::new(search_w, 34.0),
            );
            let mut search_ui = ui.new_child(
                egui::UiBuilder::new()
                    .max_rect(search_rect)
                    .layout(Layout::left_to_right(Align::Center)),
            );
            let search_id = ui.id().with("title_search_box");
            let is_focused = ui
                .ctx()
                .data(|d| d.get_temp::<bool>(search_id.with("foc")))
                .unwrap_or(false);
            let is_hovered = ui
                .ctx()
                .data(|d| d.get_temp::<bool>(search_id.with("hov")))
                .unwrap_or(false);
            let (s_h, s_f) = animate_hover_and_active(
                &search_ui,
                search_id,
                is_hovered,
                is_focused || *show_recent_popup,
            );
            let frame_resp = Frame::new()
                .fill(lerp_color(palette.bg_surface, palette.bg_card, s_h.max(s_f)))
                .stroke(Stroke::new(
                    1.0_f32 + s_f * 0.35,
                    lerp_color(
                        lerp_color(palette.border_dark, palette.border_highlight, s_h),
                        palette.primary,
                        s_f,
                    ),
                ))
                .corner_radius(CornerRadius::same(RADIUS_FULL as u8))
                .inner_margin(Margin::symmetric(12, 4))
                .show(&mut search_ui, |ui| {
                    ui.set_width((search_w - 24.0).max(80.0));
                    ui.horizontal(|ui| {
                        ui.label(icon_text(
                            "search",
                            14.0,
                            lerp_color(palette.text_muted, palette.primary, s_f),
                        ));
                        let edit = ui.add(
                            egui::TextEdit::singleline(search_query)
                                .hint_text(strings.get("search_placeholder"))
                                .frame(false)
                                .desired_width((search_w - 72.0).max(40.0)),
                        );
                        let edit_focused = edit.has_focus();
                        ui.ctx().data_mut(|d| {
                            d.insert_temp(search_id.with("foc"), edit_focused);
                        });
                        if edit.gained_focus() && !recent_searches.is_empty() {
                            *show_recent_popup = true;
                        }
                        if edit.changed() {
                            action = TitleBarAction::SearchChanged(search_query.clone());
                        }
                        if !search_query.is_empty()
                            && ui
                                .add(
                                    egui::Button::new(icon_text("x", 12.0, palette.text_muted))
                                        .frame(false),
                                )
                                .clicked()
                        {
                            search_query.clear();
                            action = TitleBarAction::SearchChanged(String::new());
                        }
                    });
                });
            let search_hovered = frame_resp.response.hovered();
            ui.ctx()
                .data_mut(|d| d.insert_temp(search_id.with("hov"), search_hovered));

            let recent_t = ctx.animate_bool_with_time(
                ui.id().with("recent_popup_anim"),
                *show_recent_popup && !recent_searches.is_empty(),
                0.15,
            );
            if recent_t > 0.01 && !recent_searches.is_empty() {
                let popup_id = ui.id().with("recent_searches_popup");
                let slide_y = (1.0 - recent_t) * -6.0;
                let popup_pos = Pos2::new(search_rect.left(), search_rect.bottom() + 6.0 + slide_y);
                egui::Area::new(popup_id)
                    .order(egui::Order::Foreground)
                    .fixed_pos(popup_pos)
                    .show(ctx, |ui| {
                        ui.multiply_opacity(recent_t);
                        Frame::new()
                            .fill(palette.bg_dark)
                            .stroke(Stroke::new(1.0_f32, palette.border_highlight))
                            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                            .inner_margin(Margin::same(10))
                            .show(ui, |ui| {
                                ui.set_width((search_w - 20.0).max(120.0));
                                ui.horizontal(|ui| {
                                    ui.label(bold_text(
                                        strings.get("search_recent_title"),
                                        11.0,
                                        palette.text_muted,
                                    ));
                                    ui.with_layout(Layout::right_to_left(Align::Center), |ui| {
                                        if ui
                                            .add(
                                                egui::Button::new(
                                                    RichText::new(strings.get("search_recent_clear"))
                                                        .size(11.0)
                                                        .color(palette.primary),
                                                )
                                                .frame(false),
                                            )
                                            .clicked()
                                        {
                                            action = TitleBarAction::ClearRecentSearches;
                                            *show_recent_popup = false;
                                        }
                                    });
                                });
                                ui.add_space(4.0);
                                for item in recent_searches.iter().take(6) {
                                    if popup_menu_row(ui, None, item, None, false, palette).clicked()
                                    {
                                        *search_query = item.clone();
                                        action = TitleBarAction::SelectRecentSearch(item.clone());
                                        *show_recent_popup = false;
                                    }
                                }
                            });
                    });
                if *show_recent_popup
                    && frame_resp.response.clicked_elsewhere()
                    && ctx.input(|i| i.pointer.any_click())
                {
                    *show_recent_popup = false;
                }
            }
        }
    }

    action
}

pub fn handle_frameless_resize(ctx: &Context, is_maximized: bool) {
    if is_maximized {
        return;
    }
    let Some(rect) = ctx.input(|i| i.viewport().inner_rect) else {
        return;
    };
    let Some(pos) = ctx.input(|i| i.pointer.hover_pos()) else {
        return;
    };
    let edge = 6.0;
    let w = rect.width();
    let h = rect.height();

    let left = pos.x <= edge;
    let right = pos.x >= w - edge;
    let top = pos.y <= edge;
    let bottom = pos.y >= h - edge;

    let dir = match (left, right, top, bottom) {
        (true, _, true, _) => Some((ResizeDirection::NorthWest, CursorIcon::ResizeNwSe)),
        (_, true, true, _) => Some((ResizeDirection::NorthEast, CursorIcon::ResizeNeSw)),
        (true, _, _, true) => Some((ResizeDirection::SouthWest, CursorIcon::ResizeNeSw)),
        (_, true, _, true) => Some((ResizeDirection::SouthEast, CursorIcon::ResizeNwSe)),
        (true, _, _, _) => Some((ResizeDirection::West, CursorIcon::ResizeHorizontal)),
        (_, true, _, _) => Some((ResizeDirection::East, CursorIcon::ResizeHorizontal)),
        (_, _, true, _) => Some((ResizeDirection::North, CursorIcon::ResizeVertical)),
        (_, _, _, true) => Some((ResizeDirection::South, CursorIcon::ResizeVertical)),
        _ => None,
    };

    if let Some((resize_dir, cursor)) = dir {
        ctx.set_cursor_icon(cursor);
        if ctx.input(|i| i.pointer.primary_pressed()) {
            ctx.send_viewport_cmd(ViewportCommand::BeginResize(resize_dir));
        }
    }
}

fn configure_popup_visuals(ui: &mut Ui, palette: &ThemePalette) {
    let visuals = ui.visuals_mut();
    visuals.window_fill = palette.bg_dark;
    visuals.window_stroke = Stroke::new(1.0_f32, palette.border_highlight);
    visuals.window_corner_radius = CornerRadius::same(RADIUS_MD as u8);
    visuals.widgets.inactive.weak_bg_fill = Color32::TRANSPARENT;
    visuals.widgets.inactive.bg_fill = Color32::TRANSPARENT;
    visuals.widgets.inactive.bg_stroke = Stroke::NONE;
    visuals.widgets.hovered.weak_bg_fill = palette.bg_hover;
    visuals.widgets.hovered.bg_fill = palette.bg_hover;
    visuals.widgets.hovered.bg_stroke = Stroke::NONE;
    visuals.widgets.hovered.corner_radius = CornerRadius::same(7);
    visuals.widgets.active.weak_bg_fill = palette.primary_dim;
    visuals.widgets.active.bg_fill = palette.primary_dim;
    visuals.widgets.active.bg_stroke = Stroke::NONE;
    ui.spacing_mut().item_spacing.y = 2.0;
}

fn menu_icon_for_id(id: i32) -> Option<&'static str> {
    match id {
        100 => Some("folder"),
        200 => Some("play"),
        300 => Some("eye"),
        400 => Some("audio-lines"),
        500 => Some("info"),
        _ => None,
    }
}

fn popup_menu_row(
    ui: &mut Ui,
    leading_icon: Option<&str>,
    label: &str,
    toggle_checked: Option<bool>,
    has_submenu: bool,
    palette: &ThemePalette,
) -> egui::Response {
    let w = ui.available_width().max(160.0);
    let (rect, resp) = ui.allocate_exact_size(Vec2::new(w, 28.0), Sense::click());
    let (h_t, a_t) = animate_hover_and_active(
        ui,
        ui.id().with(("popup_row", label, has_submenu)),
        resp.hovered(),
        toggle_checked == Some(true),
    );
    let fill = lerp_color(
        Color32::TRANSPARENT,
        if toggle_checked == Some(true) && h_t < 0.1 {
            ThemePalette::with_alpha(palette.primary_dim, 0.55)
        } else {
            palette.bg_hover
        },
        h_t.max(a_t * 0.45),
    );
    ui.painter().rect_filled(rect, CornerRadius::same(7), fill);

    let fg = lerp_color(palette.text_light, Color32::WHITE, h_t);
    let mut text_x = rect.left() + 10.0;

    if let Some(checked) = toggle_checked {
        if checked {
            ui.painter().text(
                Pos2::new(rect.left() + 16.0, rect.center().y),
                egui::Align2::CENTER_CENTER,
                icon_glyph("check"),
                egui::FontId::new(13.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
                palette.primary,
            );
        }
        text_x = rect.left() + 30.0;
    } else if let Some(icon) = leading_icon {
        ui.painter().text(
            Pos2::new(rect.left() + 16.0, rect.center().y),
            egui::Align2::CENTER_CENTER,
            icon_glyph(icon),
            egui::FontId::new(13.5, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            lerp_color(palette.text_muted, palette.primary, h_t),
        );
        text_x = rect.left() + 30.0;
    }

    ui.painter().text(
        Pos2::new(text_x, rect.center().y),
        egui::Align2::LEFT_CENTER,
        label,
        egui::FontId::new(12.5, egui::FontFamily::Proportional),
        fg,
    );

    if has_submenu {
        ui.painter().text(
            Pos2::new(rect.right() - 12.0, rect.center().y),
            egui::Align2::CENTER_CENTER,
            icon_glyph("chevron-right"),
            egui::FontId::new(12.5, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            lerp_color(palette.text_muted, palette.text_light, h_t),
        );
    }

    resp
}

fn render_popup_menu_tree(
    ui: &mut Ui,
    entries: &[MenuEntry],
    palette: &ThemePalette,
    action: &mut TitleBarAction,
    is_root: bool,
    root_popup_id: egui::Id,
) {
    let sub_state_id = root_popup_id.with("open_sub");
    for entry in entries {
        if entry.is_separator {
            let (sep_rect, _) =
                ui.allocate_exact_size(Vec2::new(ui.available_width(), 7.0), Sense::hover());
            ui.painter().line_segment(
                [
                    Pos2::new(sep_rect.left() + 8.0, sep_rect.center().y),
                    Pos2::new(sep_rect.right() - 8.0, sep_rect.center().y),
                ],
                Stroke::new(1.0_f32, palette.border_dark),
            );
            continue;
        }
        if !entry.children.is_empty() {
            let sub_id = ui.id().with(("sub_pop", entry.id));
            let row_resp = popup_menu_row(
                ui,
                if is_root { menu_icon_for_id(entry.id) } else { None },
                &entry.label,
                None,
                true,
                palette,
            );
            if row_resp.hovered() || row_resp.clicked() {
                ui.ctx().data_mut(|d| d.insert_temp(sub_state_id, entry.id));
            }
            let open_sub = ui
                .ctx()
                .data(|d| d.get_temp::<i32>(sub_state_id))
                == Some(entry.id);
            if open_sub {
                let sub_t = ui
                    .ctx()
                    .animate_bool_with_time(sub_id.with("anim"), true, 0.12);
                egui::Area::new(sub_id)
                    .order(egui::Order::Tooltip)
                    .fixed_pos(Pos2::new(row_resp.rect.right() + 6.0, row_resp.rect.top() - 4.0))
                    .show(ui.ctx(), |ui| {
                        ui.multiply_opacity(0.35 + 0.65 * sub_t);
                        Frame::new()
                            .fill(palette.bg_dark)
                            .stroke(Stroke::new(1.0_f32, palette.border_highlight))
                            .corner_radius(CornerRadius::same(RADIUS_MD as u8))
                            .inner_margin(Margin::same(6))
                            .show(ui, |ui| {
                                ui.set_min_width(214.0);
                                configure_popup_visuals(ui, palette);
                                render_popup_menu_tree(
                                    ui,
                                    &entry.children,
                                    palette,
                                    action,
                                    false,
                                    root_popup_id,
                                );
                            });
                    });
            }
        } else if let Some(ref act) = entry.action {
            if popup_menu_row(ui, None, &entry.label, entry.toggle_checked, false, palette)
                .clicked()
            {
                *action = TitleBarAction::MenuAction(act.clone());
                ui.ctx().data_mut(|d| d.remove::<i32>(sub_state_id));
                ui.memory_mut(|m| m.close_popup());
                ui.close_menu();
            }
        }
    }
}

