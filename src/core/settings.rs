// Location: src/core/settings.rs
// Function: Persistent settings store writing appearance.xml and JSON desktop
// preferences under XDG_CONFIG_HOME with full Android-parity audio/UI options.
// All Rights Reserved isgaar

use std::collections::HashMap;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};

use serde::{Deserialize, Serialize};

use crate::shared::colors::ThemePalette;

static TEST_SCOPE: AtomicBool = AtomicBool::new(false);

pub fn set_test_scope(enabled: bool) {
    TEST_SCOPE.store(enabled, Ordering::Relaxed);
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct EqualizerSettings {
    pub bands_db: [f32; 12],
    pub preamp_db: f32,
    pub limiter_enabled: bool,
    pub agc_enabled: bool,
    pub balance: f32,
    pub z_bass: f32,
    pub compressor: f32,
    pub z_mids: f32,
    pub z_treble: f32,
    pub pitch_semitones: f32,
    pub speed: f32,
    pub echo_enabled: bool,
    pub echo_gain: f32,
    pub autowah_enabled: bool,
    pub autowah_gain: f32,
    pub reverb_enabled: bool,
    pub reverb_gain: f32,
    pub flanger_enabled: bool,
}

impl Default for EqualizerSettings {
    fn default() -> Self {
        Self {
            bands_db: [0.0; 12],
            preamp_db: 0.0,
            limiter_enabled: true,
            agc_enabled: false,
            balance: 0.0,
            z_bass: 0.0,
            compressor: 0.0,
            z_mids: 0.0,
            z_treble: 0.0,
            pitch_semitones: 0.0,
            speed: 1.0,
            echo_enabled: false,
            echo_gain: 0.3,
            autowah_enabled: false,
            autowah_gain: 0.4,
            reverb_enabled: false,
            reverb_gain: 0.25,
            flanger_enabled: false,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct SettingsData {
    pub is_first_run: bool,
    pub mode: String,
    pub server_url: String,
    pub folder_path: String,
    pub smb_host: String,
    pub smb_share: String,
    pub smb_folder: String,
    pub smb_user: String,
    pub save_credentials: bool,

    // Appearance (stored in appearance.xml)
    pub theme: String,
    pub language: String,
    pub dynamic_accent: bool,

    // Layout
    pub sidebar_width: f32,
    pub sidebar_collapsed: bool,
    pub recent_searches: Vec<String>,

    // Audio parity
    pub gapless: bool,
    pub crossfade_enabled: bool,
    pub crossfade_sec: f32,
    pub fade_on_pause: bool,
    pub skip_silence: bool,
    pub volume_stabilizer: bool,
    pub visualizer_enabled: bool,
    pub visualizer_preset: u32,
    pub buffer_ms: u32,
    pub volume: f32,

    // Covers & Lyrics parity
    pub cover_carousel: bool,
    pub download_art: bool,
    pub covers_wifi_only: bool,
    pub prefer_folder_art: bool,
    pub cover_no_ext: bool,
    pub auto_lrc: bool,
    pub cover_quality: u32,
    pub cover_max_size: u32,

    // Library / Scanner parity
    pub scan_startup: bool,
    pub scan_notify: bool,
    pub skip_nomedia: bool,
    pub skip_mp4: bool,
    pub skip_ogg: bool,
    pub tag_encoding: String,
    pub skip_short_sec: u32,

    // System parity
    pub resume_after_call: bool,
    pub pause_on_focus_loss: bool,
    pub pause_on_unplug: bool,
    pub resume_on_plug: bool,
    pub resume_on_bt: bool,
    pub media_keys: bool,
    pub pause_on_zero_vol: bool,
    pub power_saving: bool,

    // Equalizer
    pub equalizer: EqualizerSettings,

    // Mix affinity and downloaded track paths
    pub mix_affinity: HashMap<String, f64>,
    pub downloaded_tracks: HashMap<i64, String>,
}

impl Default for SettingsData {
    fn default() -> Self {
        Self {
            is_first_run: false,
            mode: String::from("server"),
            server_url: std::env::var("DOPAMINE_SERVER_URL")
                .or_else(|_| std::env::var("DOPAMINE_API_URL"))
                .unwrap_or_else(|_| {
                    if cfg!(debug_assertions) {
                        String::from("http://127.0.0.1:8000")
                    } else {
                        String::new()
                    }
                }),
            folder_path: String::new(),
            smb_host: String::new(),
            smb_share: String::new(),
            smb_folder: String::new(),
            smb_user: String::new(),
            save_credentials: true,
            theme: String::from("roseglow"),
            language: String::from("system"),
            dynamic_accent: true,
            sidebar_width: 240.0,
            sidebar_collapsed: false,
            recent_searches: Vec::new(),
            gapless: true,
            crossfade_enabled: false,
            crossfade_sec: 4.0,
            fade_on_pause: true,
            skip_silence: false,
            volume_stabilizer: false,
            visualizer_enabled: true,
            visualizer_preset: 108,
            buffer_ms: 1000,
            volume: 0.85,
            cover_carousel: true,
            download_art: true,
            covers_wifi_only: false,
            prefer_folder_art: true,
            cover_no_ext: false,
            auto_lrc: true,
            cover_quality: 80,
            cover_max_size: 500,
            scan_startup: false,
            scan_notify: true,
            skip_nomedia: true,
            skip_mp4: false,
            skip_ogg: false,
            tag_encoding: String::from("UTF-8"),
            skip_short_sec: 5,
            resume_after_call: true,
            pause_on_focus_loss: false,
            pause_on_unplug: true,
            resume_on_plug: false,
            resume_on_bt: false,
            media_keys: true,
            pause_on_zero_vol: false,
            power_saving: false,
            equalizer: EqualizerSettings::default(),
            mix_affinity: HashMap::new(),
            downloaded_tracks: HashMap::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct SettingsStore {
    directory: PathBuf,
}

impl Default for SettingsStore {
    fn default() -> Self {
        Self::at(default_directory())
    }
}

impl SettingsStore {
    pub fn at(directory: PathBuf) -> Self {
        Self { directory }
    }

    pub fn appearance_path(&self) -> PathBuf {
        self.directory.join("appearance.xml")
    }

    pub fn settings_path(&self) -> PathBuf {
        self.directory.join("settings.json")
    }

    pub fn load(&self) -> SettingsData {
        let mut data = match fs::read_to_string(self.settings_path()) {
            Ok(raw) => serde_json::from_str::<SettingsData>(&raw).unwrap_or_default(),
            Err(_) => {
                let mut d = SettingsData::default();
                d.is_first_run = !self.appearance_path().exists() && d.server_url.is_empty();
                d
            }
        };

        if let Ok(xml) = fs::read_to_string(self.appearance_path()) {
            if let Some(theme) = xml_tag(&xml, "theme") {
                data.theme = ThemePalette::normalize_id(&theme).to_string();
            }
            if let Some(lang) = xml_tag(&xml, "language") {
                data.language = normalize_language(&lang);
            }
        }
        data
    }

    pub fn save(&self, data: &SettingsData) {
        let _ = fs::create_dir_all(&self.directory);
        let _ = save_appearance_xml(&self.appearance_path(), &data.theme, &data.language);
        if let Ok(json) = serde_json::to_string_pretty(data) {
            let _ = write_atomic(&self.settings_path(), json.as_bytes());
        }
    }

    pub fn record_recent_search(&self, data: &mut SettingsData, query: &str) {
        let trimmed = query.trim().to_string();
        if trimmed.is_empty() {
            return;
        }
        data.recent_searches
            .retain(|item| !item.eq_ignore_ascii_case(&trimmed));
        data.recent_searches.insert(0, trimmed);
        data.recent_searches.truncate(10);
        self.save(data);
    }
}

fn normalize_language(raw: &str) -> String {
    match raw.trim().to_lowercase().as_str() {
        "es" => String::from("es"),
        "en" => String::from("en"),
        _ => String::from("system"),
    }
}

fn save_appearance_xml(path: &Path, theme: &str, language: &str) -> std::io::Result<()> {
    let theme_id = ThemePalette::normalize_id(theme);
    let lang = normalize_language(language);
    let xml = format!(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<appearance>\n    <theme>{theme_id}</theme>\n    <language>{lang}</language>\n</appearance>\n"
    );
    write_atomic(path, xml.as_bytes())
}

fn write_atomic(path: &Path, bytes: &[u8]) -> std::io::Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let tmp = path.with_extension("tmp");
    {
        let mut f = OpenOptions::new()
            .write(true)
            .create(true)
            .truncate(true)
            .open(&tmp)?;
        f.write_all(bytes)?;
        f.sync_all()?;
    }
    fs::rename(tmp, path)
}

fn xml_tag(xml: &str, tag: &str) -> Option<String> {
    let open = format!("<{tag}>");
    let close = format!("</{tag}>");
    let start = xml.find(&open)? + open.len();
    let end = xml[start..].find(&close)? + start;
    Some(xml[start..end].trim().to_string())
}

fn default_directory() -> PathBuf {
    let org = if TEST_SCOPE.load(Ordering::Relaxed) {
        "isgaar_test"
    } else {
        "isgaar"
    };
    config_root().join(org).join("Dopamine")
}

fn config_root() -> PathBuf {
    if let Ok(base) = std::env::var("XDG_CONFIG_HOME") {
        if !base.trim().is_empty() {
            return PathBuf::from(base);
        }
    }
    if let Ok(home) = std::env::var("HOME") {
        if !home.trim().is_empty() {
            return PathBuf::from(home).join(".config");
        }
    }
    std::env::temp_dir()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn saves_and_loads_appearance_xml_and_settings() {
        let dir = std::env::temp_dir().join(format!("dopamine-settings-{}", std::process::id()));
        let _ = fs::remove_dir_all(&dir);
        let store = SettingsStore::at(dir.clone());
        let mut data = SettingsData::default();
        data.theme = String::from("exoblur");
        data.language = String::from("en");
        data.sidebar_width = 310.0;
        data.equalizer.bands_db[0] = 3.5;
        store.save(&data);

        let xml = fs::read_to_string(store.appearance_path()).unwrap();
        assert!(xml.contains("<theme>exoblur</theme>"));
        assert!(xml.contains("<language>en</language>"));

        let loaded = store.load();
        assert_eq!(loaded.theme, "exoblur");
        assert_eq!(loaded.language, "en");
        assert_eq!(loaded.sidebar_width, 310.0);
        assert_eq!(loaded.equalizer.bands_db[0], 3.5);
        let _ = fs::remove_dir_all(&dir);
    }
}

