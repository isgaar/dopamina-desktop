// Location: src/ui/theme.rs
// Function: egui visual style bridge loading LumpySans and Lucide fonts,
// exposing design tokens, rounded panel helpers, and icon glyph mapping.
// All Rights Reserved isgaar

use std::sync::Arc;

use egui::{
    Color32, Context, CornerRadius, FontData, FontDefinitions, FontFamily, FontId, Id, Pos2, Rect,
    RichText, Stroke, Ui, Visuals,
};

use crate::shared::colors::ThemePalette;

pub const SPACE_XS: f32 = 4.0;
pub const SPACE_SM: f32 = 8.0;
pub const SPACE_MD: f32 = 16.0;
pub const SPACE_LG: f32 = 24.0;
pub const SPACE_XL: f32 = 32.0;

pub const RADIUS_SM: f32 = 6.0;
pub const RADIUS_MD: f32 = 12.0;
pub const RADIUS_LG: f32 = 14.0;
pub const RADIUS_FULL: f32 = 999.0;

pub const FONT_SM: f32 = 11.0;
pub const FONT_BASE: f32 = 13.0;
pub const FONT_MD: f32 = 15.0;
pub const FONT_LG: f32 = 18.0;

pub fn install_fonts(ctx: &Context) {
    let mut fonts = FontDefinitions::default();
    fonts.font_data.insert(
        String::from("LumpySans"),
        Arc::new(FontData::from_static(include_bytes!(
            "../shared/assets/fonts/LumpySansRegular.ttf"
        ))),
    );
    fonts.font_data.insert(
        String::from("LumpySansBold"),
        Arc::new(FontData::from_static(include_bytes!(
            "../shared/assets/fonts/LumpySansBold.ttf"
        ))),
    );
    fonts.font_data.insert(
        String::from("lucide"),
        Arc::new(FontData::from_static(include_bytes!(
            "../shared/assets/fonts/lucide.ttf"
        ))),
    );

    if let Some(prop) = fonts.families.get_mut(&FontFamily::Proportional) {
        prop.insert(0, String::from("LumpySans"));
    }
    fonts.families.insert(
        FontFamily::Name(Arc::from("lucide")),
        vec![String::from("lucide")],
    );
    fonts.families.insert(
        FontFamily::Name(Arc::from("LumpySansBold")),
        vec![String::from("LumpySansBold"), String::from("LumpySans")],
    );
    ctx.set_fonts(fonts);
}

pub fn apply_palette(ctx: &Context, palette: &ThemePalette) {
    let mut visuals = Visuals::dark();
    visuals.window_fill = palette.bg_darkest;
    visuals.panel_fill = palette.bg_darkest;
    visuals.faint_bg_color = palette.bg_dark;
    visuals.extreme_bg_color = palette.bg_darkest;
    visuals.override_text_color = Some(palette.text_light);
    visuals.selection.bg_fill = palette.primary_dim;
    visuals.selection.stroke = Stroke::new(1.0_f32, palette.primary);

    visuals.widgets.noninteractive.bg_fill = palette.bg_card;
    visuals.widgets.noninteractive.fg_stroke = Stroke::new(1.0_f32, palette.text_light);
    visuals.widgets.noninteractive.bg_stroke = Stroke::new(1.0_f32, palette.border_dark);
    visuals.widgets.noninteractive.corner_radius = CornerRadius::same(RADIUS_SM as u8);

    visuals.widgets.inactive.bg_fill = palette.bg_surface;
    visuals.widgets.inactive.fg_stroke = Stroke::new(1.0_f32, palette.text_light);
    visuals.widgets.inactive.bg_stroke = Stroke::new(1.0_f32, palette.border_dark);
    visuals.widgets.inactive.corner_radius = CornerRadius::same(RADIUS_SM as u8);

    visuals.widgets.hovered.bg_fill = palette.bg_hover;
    visuals.widgets.hovered.fg_stroke = Stroke::new(1.0_f32, palette.text_light);
    visuals.widgets.hovered.bg_stroke = Stroke::new(1.0_f32, palette.border_highlight);
    visuals.widgets.hovered.corner_radius = CornerRadius::same(RADIUS_SM as u8);

    visuals.widgets.active.bg_fill = palette.primary_dim;
    visuals.widgets.active.fg_stroke = Stroke::new(1.5_f32, palette.text_light);
    visuals.widgets.active.bg_stroke = Stroke::new(1.0_f32, palette.primary);
    visuals.widgets.active.corner_radius = CornerRadius::same(RADIUS_SM as u8);

    visuals.window_corner_radius = CornerRadius::same(RADIUS_MD as u8);
    visuals.window_stroke = Stroke::new(1.0_f32, palette.border_dark);
    ctx.set_visuals(visuals);
}

pub fn icon_glyph(name: &str) -> &'static str {
    match name {
        "home" | "house" => "\u{e0f8}",
        "compass" => "\u{e09f}",
        "layers" => "\u{e104}",
        "library" => "\u{e106}",
        "plus" => "\u{e141}",
        "heart" => "\u{e0f5}",
        "sparkles" => "\u{e417}",
        "audio-lines" => "\u{e55f}",
        "music" => "\u{e126}",
        "music-2" => "\u{e34e}",
        "disc" => "\u{e0b3}",
        "disc-album" => "\u{e561}",
        "list" => "\u{e10c}",
        "list-music" => "\u{e2e0}",
        "play" => "\u{e140}",
        "circle-play" => "\u{e085}",
        "pause" => "\u{e132}",
        "stop" | "square" => "\u{e16b}",
        "next" | "skip-forward" => "\u{e164}",
        "previous" | "skip-back" => "\u{e163}",
        "repeat" => "\u{e14a}",
        "mix" | "shuffle" => "\u{e162}",
        "refresh" | "refresh-cw" => "\u{e149}",
        "search" => "\u{e155}",
        "filter" => "\u{e0d5}",
        "check" => "\u{e070}",
        "database" => "\u{e0b1}",
        "sliders" | "sliders-horizontal" => "\u{e29a}",
        "folder" => "\u{e0dc}",
        "clock" | "clock-3" => "\u{e250}",
        "moon" => "\u{e122}",
        "image" => "\u{e0f9}",
        "chevron-down" => "\u{e071}",
        "right" | "chevron-right" => "\u{e073}",
        "left" | "chevron-left" => "\u{e072}",
        "panel-left" => "\u{e12e}",
        "panel-right" => "\u{e436}",
        "info" => "\u{e0fe}",
        "power" => "\u{e144}",
        "star" => "\u{e17a}",
        "server" => "\u{e157}",
        "wifi" => "\u{e1ae}",
        "wifi-off" => "\u{e1af}",
        "user" | "user-round" => "\u{e46d}",
        "log-in" => "\u{e111}",
        "log-out" => "\u{e112}",
        "qr-code" => "\u{e1df}",
        "shield-check" => "\u{e1ff}",
        "mail" => "\u{e113}",
        "lock" => "\u{e10f}",
        "bookmark" => "\u{e064}",
        "rotate-ccw" => "\u{e14c}",
        "settings" => "\u{e158}",
        "users" => "\u{e1a4}",
        "tag" => "\u{e183}",
        "smartphone" | "mobile" | "phone" => "\u{e167}",
        "monitor" | "desktop" => "\u{e121}",
        "speaker" => "\u{e16a}",
        "volume" => "\u{e1a9}",
        "volume-1" => "\u{e1aa}",
        "volume-2" => "\u{e1ab}",
        "volume-x" | "volume-mute" => "\u{e1ac}",
        "contrast" => "\u{e0a1}",
        "plug" => "\u{e383}",
        "eye" => "\u{e0be}",
        "close" | "x" => "\u{e1b2}",
        "palette" => "\u{e1dd}",
        "globe" => "\u{e0eb}",
        "circle-check" => "\u{e226}",
        "circle-alert" => "\u{e07b}",
        "triangle-alert" => "\u{e193}",
        _ => "\u{e126}",
    }
}

pub fn icon_text(name: &str, size: f32, color: Color32) -> RichText {
    RichText::new(icon_glyph(name))
        .font(FontId::new(size, FontFamily::Name(Arc::from("lucide"))))
        .color(color)
}

pub fn bold_text(text: impl Into<String>, size: f32, color: Color32) -> RichText {
    RichText::new(text)
        .font(FontId::new(
            size,
            FontFamily::Name(Arc::from("LumpySansBold")),
        ))
        .color(color)
}

pub fn format_duration(seconds: f64) -> String {
    let total = seconds.max(0.0).floor() as u64;
    let mins = total / 60;
    let secs = total % 60;
    format!("{mins}:{secs:02}")
}

pub fn fidelity_color(tier: &str, palette: &ThemePalette) -> Color32 {
    match tier {
        "MAX" => palette.accent,
        "HIGH" => Color32::WHITE,
        "OK" => Color32::from_rgb(0x4D, 0xB6, 0xAC),
        _ => Color32::from_rgb(0x8E, 0x8E, 0x93),
    }
}

#[inline]
pub fn ease_out_cubic(t: f32) -> f32 {
    let clamped = t.clamp(0.0, 1.0);
    let inv = 1.0 - clamped;
    1.0 - inv * inv * inv
}

#[inline]
pub fn lerp_f32(a: f32, b: f32, t: f32) -> f32 {
    let k = t.clamp(0.0, 1.0);
    a + (b - a) * k
}

#[inline]
pub fn lerp_color(a: Color32, b: Color32, t: f32) -> Color32 {
    let k = t.clamp(0.0, 1.0);
    let lerp_u8 = |x: u8, y: u8| -> u8 {
        ((x as f32) + ((y as f32) - (x as f32)) * k).round().clamp(0.0, 255.0) as u8
    };
    Color32::from_rgba_unmultiplied(
        lerp_u8(a.r(), b.r()),
        lerp_u8(a.g(), b.g()),
        lerp_u8(a.b(), b.b()),
        lerp_u8(a.a(), b.a()),
    )
}

pub fn animate_hover_and_active(ui: &Ui, id: Id, hovered: bool, active: bool) -> (f32, f32) {
    let h = ui.ctx().animate_bool_with_time(id.with("h"), hovered, 0.12);
    let a = ui.ctx().animate_bool_with_time(id.with("a"), active, 0.18);
    (h, a)
}

pub fn hash_transition_key(parts: &[&str]) -> u64 {
    let mut h: u64 = 0xcbf29ce484222325;
    for part in parts {
        for &b in part.as_bytes() {
            h ^= b as u64;
            h = h.wrapping_mul(0x100000001b3);
        }
        h ^= 0xff;
        h = h.wrapping_mul(0x100000001b3);
    }
    h
}

/// Tracks changes to `key_hash` in egui memory and returns an eased `0.0..=1.0`
/// transition progress while scheduling repaints until completion.
pub fn begin_keyed_transition(ui: &Ui, id: Id, key_hash: u64, duration_sec: f32) -> f32 {
    #[derive(Clone, Copy)]
    struct TransitionState {
        last_key: u64,
        started_at: f64,
    }

    let now = ui.ctx().input(|i| i.time);
    let state_id = id.with("keyed_tx");
    let mut state = ui
        .ctx()
        .data_mut(|d| d.get_temp::<TransitionState>(state_id))
        .unwrap_or(TransitionState {
            last_key: key_hash,
            started_at: now - (duration_sec as f64),
        });

    if state.last_key != key_hash {
        state.last_key = key_hash;
        state.started_at = now;
    }
    ui.ctx().data_mut(|d| d.insert_temp(state_id, state));

    let raw = if duration_sec <= 0.001 {
        1.0
    } else {
        ((now - state.started_at) as f32 / duration_sec).clamp(0.0, 1.0)
    };
    if raw < 1.0 {
        ui.ctx().request_repaint();
    }
    ease_out_cubic(raw)
}

/// Draws a miniature 3-bar audio level indicator inside `rect`.
pub fn paint_animated_eq_bars(ui: &Ui, rect: Rect, color: Color32, playing: bool) {
    let t = ui.ctx().input(|i| i.time) as f32;
    let bar_w = 2.6_f32;
    let gap = 2.2_f32;
    let total_w = bar_w * 3.0 + gap * 2.0;
    let start_x = rect.center().x - total_w * 0.5;
    let max_h = 13.0_f32.min(rect.height() - 4.0);
    let bottom_y = rect.center().y + max_h * 0.5;

    for i in 0..3 {
        let phase = i as f32 * 1.45;
        let factor = if playing {
            0.35 + 0.65 * (0.5 + 0.5 * (t * (5.2 + i as f32 * 1.3) + phase).sin())
        } else {
            0.25 + (i as f32 * 0.14)
        };
        let h = (max_h * factor).max(3.0);
        let x = start_x + (bar_w + gap) * i as f32;
        let bar_rect = Rect::from_min_max(Pos2::new(x, bottom_y - h), Pos2::new(x + bar_w, bottom_y));
        ui.painter()
            .rect_filled(bar_rect, CornerRadius::same(1), color);
    }
    if playing {
        ui.ctx().request_repaint();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn eases_and_interpolates_colors_within_bounds() {
        assert_eq!(ease_out_cubic(0.0), 0.0);
        assert_eq!(ease_out_cubic(1.0), 1.0);
        assert!(ease_out_cubic(0.5) > 0.5);

        let mid = lerp_color(Color32::BLACK, Color32::WHITE, 0.5);
        assert_eq!(mid.r(), 128);
        assert_eq!(mid.g(), 128);
        assert_eq!(mid.b(), 128);

        assert_ne!(
            hash_transition_key(&["library", "tracks"]),
            hash_transition_key(&["library", "albums"])
        );
    }
}


