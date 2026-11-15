// Location: src/shared/colors/mod.rs
// Function: XML theme parser and reactive color palette manager loading the
// five official Android themes (roseglow, exoblur, thegrand, redline, flat).
// All Rights Reserved isgaar

use std::collections::HashMap;
use egui::Color32;

const THEME_ROSEGLOW: &str = include_str!("theme_roseglow.xml");
const THEME_EXOBLUR: &str = include_str!("theme_exoblur.xml");
const THEME_THEGRAND: &str = include_str!("theme_thegrand.xml");
const THEME_REDLINE: &str = include_str!("theme_redline.xml");
const THEME_FLAT: &str = include_str!("theme_flat.xml");

pub const THEME_IDS: [&str; 5] = [
    "roseglow",
    "exoblur",
    "thegrand",
    "redline",
    "flat",
];

#[derive(Debug, Clone)]
pub struct ThemePalette {
    pub id: String,
    pub primary: Color32,
    pub primary_hover: Color32,
    pub primary_dim: Color32,
    pub accent: Color32,
    pub accent_light: Color32,
    pub accent_orange: Color32,
    pub accent_cyan: Color32,
    pub bg_darkest: Color32,
    pub bg_dark: Color32,
    pub bg_surface: Color32,
    pub bg_card: Color32,
    pub bg_selected: Color32,
    pub bg_hover: Color32,
    pub border_dark: Color32,
    pub border_highlight: Color32,
    pub text_light: Color32,
    pub text_muted: Color32,
    pub text_dim: Color32,
    pub success: Color32,
    pub danger: Color32,
    pub warning: Color32,
    tokens: HashMap<String, Color32>,
}

impl Default for ThemePalette {
    fn default() -> Self {
        Self::load("roseglow")
    }
}

impl ThemePalette {
    pub fn normalize_id(raw: &str) -> &'static str {
        let lower = raw.trim().to_lowercase();
        for &id in &THEME_IDS {
            if id == lower {
                return id;
            }
        }
        "roseglow"
    }

    pub fn load(theme_id: &str) -> Self {
        let id = Self::normalize_id(theme_id);
        let xml = match id {
            "exoblur" => THEME_EXOBLUR,
            "thegrand" => THEME_THEGRAND,
            "redline" => THEME_REDLINE,
            "flat" => THEME_FLAT,
            _ => THEME_ROSEGLOW,
        };
        let tokens = parse_color_xml(xml);
        let get = |key: &str, fallback: Color32| -> Color32 {
            tokens.get(key).copied().unwrap_or(fallback)
        };

        Self {
            id: id.to_string(),
            primary: get("primary", Color32::from_rgb(0xFA, 0x2D, 0x48)),
            primary_hover: get("primary_hover", Color32::from_rgb(0xFF, 0x37, 0x5F)),
            primary_dim: get(
                "primary_dim",
                Color32::from_rgba_unmultiplied(0xFA, 0x2D, 0x48, 0x33),
            ),
            accent: get("accent", Color32::from_rgb(0xFA, 0x2D, 0x48)),
            accent_light: get(
                "accent_light",
                Color32::from_rgba_unmultiplied(0xFA, 0x2D, 0x48, 0x33),
            ),
            accent_orange: get("accent_orange", Color32::from_rgb(0xFF, 0x9F, 0x0A)),
            accent_cyan: get("accent_cyan", Color32::from_rgb(0x64, 0xD2, 0xFF)),
            bg_darkest: get("bg_darkest", Color32::from_rgb(0x00, 0x00, 0x00)),
            bg_dark: get("bg_dark", Color32::from_rgb(0x1C, 0x1C, 0x1E)),
            bg_surface: get("bg_surface", Color32::from_rgb(0x2C, 0x2C, 0x2E)),
            bg_card: get("bg_card_elevated", Color32::from_rgb(0x24, 0x24, 0x26)),
            bg_selected: get("bg_selected", Color32::from_rgb(0x2C, 0x2C, 0x2E)),
            bg_hover: get("bg_hover", Color32::from_rgb(0x2C, 0x2C, 0x2E)),
            border_dark: get(
                "border_dark",
                Color32::from_rgba_unmultiplied(0xFF, 0xFF, 0xFF, 0x26),
            ),
            border_highlight: get("border_highlight", Color32::from_rgb(0xFA, 0x2D, 0x48)),
            text_light: get("text_light", Color32::WHITE),
            text_muted: get("text_muted", Color32::from_rgb(0x98, 0x98, 0x9D)),
            text_dim: get("text_dim", Color32::from_rgb(0x63, 0x63, 0x66)),
            success: get("success", Color32::from_rgb(0x30, 0xD1, 0x58)),
            danger: get("danger", Color32::from_rgb(0xFF, 0x45, 0x3A)),
            warning: get("warning", Color32::from_rgb(0xFF, 0xD6, 0x0A)),
            tokens,
        }
    }

    pub fn color(&self, key: &str) -> Color32 {
        self.tokens.get(key).copied().unwrap_or(self.primary)
    }

    pub fn active_fg(&self) -> Color32 {
        let [r, g, b, _] = self.primary.to_array();
        let lum = 0.2126 * (r as f32 / 255.0)
            + 0.7152 * (g as f32 / 255.0)
            + 0.0722 * (b as f32 / 255.0);
        if lum > 0.62 {
            Color32::from_rgb(0x10, 0x10, 0x12)
        } else {
            Color32::WHITE
        }
    }

    pub fn with_alpha(color: Color32, alpha: f32) -> Color32 {
        let [r, g, b, _] = color.to_array();
        let a = (alpha.clamp(0.0, 1.0) * 255.0).round() as u8;
        Color32::from_rgba_unmultiplied(r, g, b, a)
    }
}

pub fn parse_hex_color(raw: &str) -> Option<Color32> {
    let hex = raw.trim().trim_start_matches('#');
    match hex.len() {
        6 => {
            let r = u8::from_str_radix(&hex[0..2], 16).ok()?;
            let g = u8::from_str_radix(&hex[2..4], 16).ok()?;
            let b = u8::from_str_radix(&hex[4..6], 16).ok()?;
            Some(Color32::from_rgb(r, g, b))
        }
        8 => {
            let a = u8::from_str_radix(&hex[0..2], 16).ok()?;
            let r = u8::from_str_radix(&hex[2..4], 16).ok()?;
            let g = u8::from_str_radix(&hex[4..6], 16).ok()?;
            let b = u8::from_str_radix(&hex[6..8], 16).ok()?;
            Some(Color32::from_rgba_unmultiplied(r, g, b, a))
        }
        _ => None,
    }
}

fn parse_color_xml(xml: &str) -> HashMap<String, Color32> {
    let mut map = HashMap::new();
    for line in xml.lines() {
        let line = line.trim();
        let Some(name_start) = line.find("name=\"") else {
            continue;
        };
        let after_name = &line[name_start + 6..];
        let Some(name_end) = after_name.find('"') else {
            continue;
        };
        let key = &after_name[..name_end];
        let Some(val_start) = after_name.find('>') else {
            continue;
        };
        let after_val = &after_name[val_start + 1..];
        let Some(val_end) = after_val.find("</color>") else {
            continue;
        };
        let raw_hex = &after_val[..val_end];
        if let Some(color) = parse_hex_color(raw_hex) {
            map.insert(key.to_string(), color);
        }
    }
    map
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loads_all_five_themes_with_valid_tokens() {
        for &id in &THEME_IDS {
            let palette = ThemePalette::load(id);
            assert_eq!(palette.id, id);
            assert!(!palette.tokens.is_empty());
        }
    }

    #[test]
    fn normalizes_unknown_theme_to_roseglow() {
        assert_eq!(ThemePalette::normalize_id("unknown"), "roseglow");
        assert_eq!(ThemePalette::normalize_id("ExoBlur"), "exoblur");
    }
}

