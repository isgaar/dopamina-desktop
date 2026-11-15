// Location: src/ui/player_bar.rs
// Function: Persistent 74px bottom PlayerBar displaying active track identity,
// fidelity badge, transport controls, seek scrubber, volume, and Android handoff.
// All Rights Reserved isgaar

use egui::{
    Align, Color32, CornerRadius, Frame, Layout, Margin, Pos2, Rect, RichText, Sense, Stroke,
    TextureHandle, Ui, Vec2,
};

use crate::data::model::Track;
use crate::shared::colors::ThemePalette;
use crate::shared::i18n::StringCatalog;
use crate::ui::theme::{
    animate_hover_and_active, bold_text, fidelity_color, format_duration, icon_glyph, lerp_color,
    RADIUS_MD,
};

pub enum PlayerBarAction {
    None,
    TogglePlay,
    Next,
    Previous,
    ToggleShuffle,
    ToggleRepeat,
    Seek(f64),
    SetVolume(f32),
    ToggleLike(i64),
    OpenQueue,
    OpenEqualizer,
    ToggleLyrics,
    ToggleVisualizer,
    OpenQualityDialog,
    SwitchDevice,
}

#[allow(clippy::too_many_arguments)]
pub fn render_player_bar(
    ui: &mut Ui,
    bar_rect: Rect,
    is_maximized: bool,
    palette: &ThemePalette,
    strings: &StringCatalog,
    track: Option<&Track>,
    cover_tex: Option<&TextureHandle>,
    is_playing: bool,
    position_sec: f64,
    duration_sec: f64,
    volume: f32,
    is_shuffle: bool,
    is_repeat: bool,
    is_liked: bool,
    is_controlling_android: bool,
    has_remote_session: bool,
    show_lyrics: bool,
    show_visualizer: bool,
) -> PlayerBarAction {
    let mut action = PlayerBarAction::None;
    let bar_h = bar_rect.height();
    let bottom_rounding = if is_maximized { 0 } else { RADIUS_MD as u8 };

    ui.painter().rect_filled(
        bar_rect,
        CornerRadius {
            nw: 0,
            ne: 0,
            sw: bottom_rounding,
            se: bottom_rounding,
        },
        palette.bg_dark,
    );
    ui.painter().line_segment(
        [
            Pos2::new(bar_rect.left(), bar_rect.top()),
            Pos2::new(bar_rect.right(), bar_rect.top()),
        ],
        Stroke::new(1.0_f32, palette.border_dark),
    );

    let left_w = 310.0f32.min(bar_rect.width() * 0.28);
    let right_w = 320.0f32.min(bar_rect.width() * 0.33);
    let center_w = (bar_rect.width() - left_w - right_w - 36.0).max(200.0);

    // 1. Left: Identity
    let left_rect = Rect::from_min_size(
        Pos2::new(bar_rect.left() + 16.0, bar_rect.top() + 8.0),
        Vec2::new(left_w, bar_h - 16.0),
    );
    let mut left_ui = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(left_rect)
            .layout(Layout::left_to_right(Align::Center)),
    );
    let (cov_rect, _) = left_ui.allocate_exact_size(Vec2::splat(48.0), Sense::hover());
    if is_playing {
        let pulse = 0.5 + 0.5 * (ui.ctx().input(|i| i.time) as f32 * 3.0).sin();
        ui.painter().rect_stroke(
            cov_rect.expand(2.0),
            CornerRadius::same(10),
            Stroke::new(
                1.5_f32,
                ThemePalette::with_alpha(palette.primary, 0.25 + 0.35 * pulse),
            ),
            egui::StrokeKind::Outside,
        );
    }
    if let Some(tex) = cover_tex {
        ui.painter().image(
            tex.id(),
            cov_rect,
            Rect::from_min_max(Pos2::ZERO, Pos2::new(1.0, 1.0)),
            Color32::WHITE,
        );
        ui.painter().rect_stroke(
            cov_rect,
            CornerRadius::same(8),
            Stroke::new(1.0_f32, palette.border_dark),
            egui::StrokeKind::Inside,
        );
    } else {
        ui.painter()
            .rect_filled(cov_rect, CornerRadius::same(8), palette.bg_surface);
        ui.painter().rect_stroke(
            cov_rect,
            CornerRadius::same(8),
            Stroke::new(1.0_f32, palette.border_dark),
            egui::StrokeKind::Inside,
        );
        ui.painter().text(
            cov_rect.center(),
            egui::Align2::CENTER_CENTER,
            icon_glyph("music"),
            egui::FontId::new(18.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            palette.primary,
        );
    }
    left_ui.add_space(10.0);

    if let Some(t) = track {
        left_ui.vertical(|ui| {
            ui.set_max_width((left_w - 110.0).max(80.0));
            ui.spacing_mut().item_spacing.y = 2.0;
            ui.horizontal(|ui| {
                ui.label(bold_text(&t.title, 13.0, palette.text_light));
                let badge_col = fidelity_color(&t.fidelity_tier, palette);
                let badge = Frame::new()
                    .fill(ThemePalette::with_alpha(badge_col, 0.16))
                    .stroke(Stroke::new(1.0_f32, ThemePalette::with_alpha(badge_col, 0.55)))
                    .corner_radius(CornerRadius::same(4))
                    .inner_margin(Margin::symmetric(5, 1))
                    .show(ui, |ui| {
                        ui.label(bold_text(&t.fidelity_tier, 9.5, badge_col));
                    })
                    .response
                    .interact(Sense::click());
                if badge.clicked() {
                    action = PlayerBarAction::OpenQualityDialog;
                }
            });
            ui.label(
                RichText::new(format!("{} • {}", t.artist, t.album))
                    .size(11.0)
                    .color(palette.text_muted),
            );
        });
        left_ui.add_space(6.0);
        if player_icon_btn(
            &mut left_ui,
            palette,
            "pb_like",
            "heart",
            15.0,
            is_liked,
            false,
            "",
        ) {
            action = PlayerBarAction::ToggleLike(t.id);
        }
    } else {
        left_ui.vertical(|ui| {
            ui.label(bold_text(
                strings.get("player_idle_title"),
                12.5,
                palette.text_light,
            ));
            ui.label(
                RichText::new(strings.get("player_hi_fi"))
                    .size(11.0)
                    .color(palette.text_muted),
            );
        });
    }

    // 2. Center: Transport & Seek
    let center_rect = Rect::from_min_size(
        Pos2::new(left_rect.right() + 8.0, bar_rect.top() + 6.0),
        Vec2::new(center_w, bar_h - 12.0),
    );
    let mut center_ui = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(center_rect)
            .layout(Layout::top_down(Align::Center)),
    );
    center_ui.horizontal(|ui| {
        let btns_w = 204.0;
        let pad = ((center_w - btns_w) * 0.5).max(0.0);
        ui.add_space(pad);

        if player_icon_btn(
            ui,
            palette,
            "pb_shuffle",
            "shuffle",
            14.0,
            is_shuffle,
            true,
            "",
        ) {
            action = PlayerBarAction::ToggleShuffle;
        }
        ui.add_space(4.0);
        if player_icon_btn(ui, palette, "pb_prev", "skip-back", 16.0, false, false, "") {
            action = PlayerBarAction::Previous;
        }
        ui.add_space(8.0);

        let (play_rect, play_resp) = ui.allocate_exact_size(Vec2::splat(36.0), Sense::click());
        let (play_h, play_a) =
            animate_hover_and_active(ui, ui.id().with("pb_play"), play_resp.hovered(), is_playing);
        let scale_extra = play_h * 2.0 + if play_resp.is_pointer_button_down_on() { -1.5 } else { 0.0 };
        let radius = 17.0 + scale_extra;
        if play_a > 0.01 {
            let pulse = 0.5 + 0.5 * (ui.ctx().input(|i| i.time) as f32 * 4.2).sin();
            ui.painter().circle_filled(
                play_rect.center(),
                radius + 4.0 + pulse * 2.5,
                ThemePalette::with_alpha(palette.primary, (0.20 + 0.14 * pulse) * play_a),
            );
        }
        ui.painter().circle_filled(
            play_rect.center(),
            radius,
            lerp_color(Color32::WHITE, palette.primary, play_a * 0.15),
        );
        ui.painter().text(
            play_rect.center(),
            egui::Align2::CENTER_CENTER,
            icon_glyph(if is_playing { "pause" } else { "play" }),
            egui::FontId::new(16.0, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
            Color32::BLACK,
        );
        if play_resp.clicked() {
            action = PlayerBarAction::TogglePlay;
        }

        ui.add_space(8.0);
        if player_icon_btn(ui, palette, "pb_next", "skip-forward", 16.0, false, false, "") {
            action = PlayerBarAction::Next;
        }
        ui.add_space(4.0);
        if player_icon_btn(
            ui,
            palette,
            "pb_repeat",
            "repeat",
            14.0,
            is_repeat,
            true,
            "",
        ) {
            action = PlayerBarAction::ToggleRepeat;
        }
    });

    center_ui.horizontal(|ui| {
        let display_total = if track.is_some() {
            duration_sec
                .max(track.map(|t| t.duration).unwrap_or(0.0))
                .max(0.0)
        } else {
            0.0
        };
        let slider_max = display_total.max(1.0);
        let mut pos = if track.is_some() {
            position_sec.clamp(0.0, slider_max)
        } else {
            0.0
        };
        ui.label(
            RichText::new(format_duration(pos))
                .size(10.5)
                .color(palette.text_muted),
        );
        let slider_w = (center_w - 86.0).max(80.0);
        ui.spacing_mut().slider_width = slider_w;
        let slider = egui::Slider::new(&mut pos, 0.0..=slider_max)
            .show_value(false)
            .trailing_fill(true);
        if ui.add_enabled(track.is_some(), slider).changed() {
            action = PlayerBarAction::Seek(pos);
        }
        ui.label(
            RichText::new(format_duration(display_total))
                .size(10.5)
                .color(palette.text_muted),
        );
    });

    // 3. Right: Utilities & Volume (spacious 28x28 rounded buttons + active indicators)
    let right_rect = Rect::from_min_size(
        Pos2::new(bar_rect.right() - right_w - 14.0, bar_rect.top() + 8.0),
        Vec2::new(right_w, bar_h - 16.0),
    );
    let mut right_ui = ui.new_child(
        egui::UiBuilder::new()
            .max_rect(right_rect)
            .layout(Layout::right_to_left(Align::Center)),
    );
    right_ui.spacing_mut().item_spacing.x = 4.0;

    let mut vol_pct = (volume * 100.0).round() as i32;
    right_ui.spacing_mut().slider_width = 80.0;
    if right_ui
        .add(
            egui::Slider::new(&mut vol_pct, 0..=100)
                .show_value(false)
                .trailing_fill(true),
        )
        .changed()
    {
        action = PlayerBarAction::SetVolume(vol_pct as f32 / 100.0);
    }

    let device_icon = if is_controlling_android {
        "smartphone"
    } else if vol_pct == 0 {
        "volume-x"
    } else if vol_pct < 50 {
        "volume-1"
    } else {
        "volume-2"
    };
    let device_tip = if is_controlling_android {
        strings.get("session_switch_to_desktop")
    } else if has_remote_session {
        strings.get("session_switch_to_android")
    } else {
        strings.get("session_source_desktop")
    };
    if player_icon_btn(
        &mut right_ui,
        palette,
        "pb_device",
        device_icon,
        15.0,
        is_controlling_android,
        is_controlling_android,
        device_tip,
    ) {
        action = PlayerBarAction::SwitchDevice;
    }

    if player_icon_btn(
        &mut right_ui,
        palette,
        "pb_queue",
        "list",
        15.0,
        false,
        false,
        strings.get("nav_queue"),
    ) {
        action = PlayerBarAction::OpenQueue;
    }

    if player_icon_btn(
        &mut right_ui,
        palette,
        "pb_eq",
        "sliders-horizontal",
        15.0,
        false,
        false,
        strings.get("nav_equalizer"),
    ) {
        action = PlayerBarAction::OpenEqualizer;
    }

    if player_icon_btn(
        &mut right_ui,
        palette,
        "pb_viz",
        "audio-lines",
        15.0,
        show_visualizer,
        true,
        strings.get("settings_visualizer_title"),
    ) {
        action = PlayerBarAction::ToggleVisualizer;
    }

    if player_icon_btn(
        &mut right_ui,
        palette,
        "pb_lyrics",
        "panel-right",
        15.0,
        show_lyrics,
        true,
        ".LRC",
    ) {
        action = PlayerBarAction::ToggleLyrics;
    }

    action
}

#[allow(clippy::too_many_arguments)]
fn player_icon_btn(
    ui: &mut Ui,
    palette: &ThemePalette,
    id_salt: &str,
    icon: &str,
    size: f32,
    active: bool,
    show_dot: bool,
    tooltip: &str,
) -> bool {
    let (rect, mut resp) = ui.allocate_exact_size(Vec2::splat(28.0), Sense::click());
    let (h_t, a_t) = animate_hover_and_active(ui, ui.id().with(id_salt), resp.hovered(), active);
    let bg = lerp_color(
        Color32::TRANSPARENT,
        if active {
            ThemePalette::with_alpha(palette.primary_dim, 0.85)
        } else {
            palette.bg_hover
        },
        h_t.max(a_t * 0.55),
    );
    ui.painter().circle_filled(rect.center(), 13.5, bg);
    let fg = lerp_color(
        lerp_color(palette.text_muted, palette.text_light, h_t),
        palette.primary,
        a_t,
    );
    ui.painter().text(
        rect.center(),
        egui::Align2::CENTER_CENTER,
        icon_glyph(icon),
        egui::FontId::new(size, egui::FontFamily::Name(std::sync::Arc::from("lucide"))),
        fg,
    );
    if show_dot && a_t > 0.05 {
        ui.painter().circle_filled(
            Pos2::new(rect.center().x, rect.bottom() - 2.0),
            1.8 * a_t,
            ThemePalette::with_alpha(palette.primary, a_t),
        );
    }
    if !tooltip.is_empty() {
        resp = resp.on_hover_text(tooltip);
    }
    resp.clicked()
}

