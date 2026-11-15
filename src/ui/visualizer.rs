// Location: src/ui/visualizer.rs
// Function: Real-time 60 FPS audio spectrum and waveform visualizer rendering
// classic presets (WonderWave, SuperStar, Bars, Firestorm, Scope, Ocean Mist).
// All Rights Reserved isgaar

use egui::{Color32, CornerRadius, Painter, Pos2, Rect, Stroke, Ui};
use serde::Deserialize;

use crate::dsp::dynamics::NUM_BANDS;
use crate::shared::colors::ThemePalette;

const PRESETS_JSON: &str = include_str!("../features/playback/visualizations/presets_catalog.json");

#[derive(Debug, Clone, Deserialize)]
struct PresetCatalog {
    collections: Vec<PresetCollection>,
}

#[derive(Debug, Clone, Deserialize)]
struct PresetCollection {
    presets: Vec<VisualizerPreset>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct VisualizerPreset {
    pub id: u32,
    pub name: String,
}

pub fn load_presets() -> Vec<VisualizerPreset> {
    let Ok(parsed) = serde_json::from_str::<PresetCatalog>(PRESETS_JSON) else {
        return vec![
            VisualizerPreset {
                id: 108,
                name: String::from("WonderWave"),
            },
            VisualizerPreset {
                id: 5501,
                name: String::from("Bars"),
            },
        ];
    };
    parsed
        .collections
        .into_iter()
        .flat_map(|c| c.presets)
        .collect()
}

pub fn render_visualizer(
    ui: &mut Ui,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    preset_id: u32,
    is_playing: bool,
    palette: &ThemePalette,
) {
    let painter = ui.painter_at(rect);
    painter.rect_filled(rect, CornerRadius::same(12), palette.bg_dark);
    painter.rect_stroke(
        rect,
        CornerRadius::same(12),
        Stroke::new(1.0_f32, palette.border_dark),
        egui::StrokeKind::Inside,
    );

    let time = ui.input(|i| i.time);
    if is_playing {
        ui.ctx().request_repaint();
    }

    match preset_id {
        5501 => draw_bars(&painter, rect, spectrum, palette),
        5502 => draw_ocean_mist(&painter, rect, spectrum, time, palette),
        5503 => draw_firestorm(&painter, rect, spectrum, time, palette),
        5504 => draw_scope(&painter, rect, spectrum, time, palette),
        107 => draw_superstar(&painter, rect, spectrum, time, palette),
        _ => draw_wonderwave(&painter, rect, spectrum, time, palette),
    }
}

fn draw_bars(painter: &Painter, rect: Rect, spectrum: &[f32; NUM_BANDS], palette: &ThemePalette) {
    let count = NUM_BANDS;
    let pad = 12.0;
    let avail_w = (rect.width() - pad * 2.0).max(1.0);
    let step = avail_w / count as f32;
    let bar_w = (step * 0.68).max(3.0);

    for (i, &mag) in spectrum.iter().enumerate() {
        let norm = mag.clamp(0.02, 1.0);
        let h = norm * (rect.height() - 24.0);
        let x = rect.left() + pad + i as f32 * step;
        let bar_rect = Rect::from_min_max(
            Pos2::new(x, rect.bottom() - 12.0 - h),
            Pos2::new(x + bar_w, rect.bottom() - 12.0),
        );
        let color = if i % 3 == 0 {
            palette.primary
        } else if i % 3 == 1 {
            palette.accent_cyan
        } else {
            ThemePalette::with_alpha(palette.primary, 0.75)
        };
        painter.rect_filled(bar_rect, CornerRadius::same(3), color);
    }
}

fn draw_wonderwave(
    painter: &Painter,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    time: f64,
    palette: &ThemePalette,
) {
    let points_n = 72usize;
    let cy = rect.center().y;
    let amp_scale = rect.height() * 0.38;

    for layer in 0..3 {
        let phase = time * (1.8 + layer as f64 * 0.6);
        let mut pts = Vec::with_capacity(points_n);
        for i in 0..points_n {
            let t = i as f32 / (points_n - 1) as f32;
            let band_idx = ((t * (NUM_BANDS - 1) as f32) as usize).min(NUM_BANDS - 1);
            let energy = spectrum[band_idx].clamp(0.04, 1.0);
            let wave = (t as f64 * 9.0 + phase).sin() as f32;
            let y = cy + wave * energy * amp_scale * (1.0 - layer as f32 * 0.22);
            let x = rect.left() + 8.0 + t * (rect.width() - 16.0);
            pts.push(Pos2::new(x, y));
        }
        let color = match layer {
            0 => palette.primary,
            1 => palette.accent_cyan,
            _ => ThemePalette::with_alpha(palette.accent_orange, 0.8),
        };
        painter.add(egui::Shape::line(pts, Stroke::new(2.2 - layer as f32 * 0.4, color)));
    }
}

fn draw_superstar(
    painter: &Painter,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    time: f64,
    palette: &ThemePalette,
) {
    let center = rect.center();
    let max_r = rect.height().min(rect.width()) * 0.42;
    for (i, &mag) in spectrum.iter().enumerate() {
        let angle = (i as f32 / NUM_BANDS as f32) * std::f32::consts::TAU + (time as f32 * 0.7);
        let len = 10.0 + mag.clamp(0.05, 1.0) * max_r;
        let end = Pos2::new(center.x + angle.cos() * len, center.y + angle.sin() * len);
        let color = if i % 2 == 0 {
            palette.primary
        } else {
            palette.accent_cyan
        };
        painter.line_segment([center, end], Stroke::new(2.0_f32, color));
        painter.circle_filled(end, 2.5, Color32::WHITE);
    }
}

fn draw_ocean_mist(
    painter: &Painter,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    time: f64,
    palette: &ThemePalette,
) {
    let n = 48usize;
    let mut pts = Vec::with_capacity(n);
    for i in 0..n {
        let t = i as f32 / (n - 1) as f32;
        let idx = ((t * (NUM_BANDS - 1) as f32) as usize).min(NUM_BANDS - 1);
        let energy = spectrum[idx].clamp(0.03, 1.0);
        let y = rect.bottom() - 12.0 - energy * (rect.height() - 28.0) * (0.6 + 0.4 * (t as f64 * 6.0 + time).sin() as f32);
        pts.push(Pos2::new(rect.left() + 8.0 + t * (rect.width() - 16.0), y));
    }
    painter.add(egui::Shape::line(pts, Stroke::new(2.5_f32, palette.accent_cyan)));
}

fn draw_firestorm(
    painter: &Painter,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    time: f64,
    palette: &ThemePalette,
) {
    for (i, &mag) in spectrum.iter().enumerate() {
        let t = i as f32 / NUM_BANDS as f32;
        let x = rect.left() + 14.0 + t * (rect.width() - 28.0);
        let jitter = ((time * 8.0 + i as f64).sin() as f32) * 4.0;
        let h = mag.clamp(0.04, 1.0) * (rect.height() - 24.0);
        let top = Pos2::new(x + jitter, rect.bottom() - 10.0 - h);
        let bot = Pos2::new(x, rect.bottom() - 10.0);
        painter.line_segment([bot, top], Stroke::new(3.0_f32, palette.accent_orange));
        painter.circle_filled(top, 2.5, palette.primary);
    }
}

fn draw_scope(
    painter: &Painter,
    rect: Rect,
    spectrum: &[f32; NUM_BANDS],
    time: f64,
    palette: &ThemePalette,
) {
    let mut pts = Vec::with_capacity(64);
    for i in 0..64 {
        let t = i as f32 / 63.0;
        let idx = ((t * (NUM_BANDS - 1) as f32) as usize).min(NUM_BANDS - 1);
        let y = rect.center().y
            + (t * 16.0 + time as f32 * 4.0).sin() * spectrum[idx].clamp(0.03, 1.0) * rect.height() * 0.35;
        pts.push(Pos2::new(rect.left() + 8.0 + t * (rect.width() - 16.0), y));
    }
    painter.add(egui::Shape::line(pts, Stroke::new(1.8_f32, palette.success)));
}

