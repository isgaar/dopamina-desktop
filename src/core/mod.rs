// Location: src/core/mod.rs
// Function: Core infrastructure exports for configuration, settings storage,
// synchronized LRC parsing, and single-instance IPC.
// All Rights Reserved isgaar

pub mod config;
pub mod global_menu;
pub mod lrc;
pub mod settings;
pub mod single_instance;

pub use config::{validate_api_url, ProjectConfig, SourceKind};
pub use global_menu::{GlobalMenuAction, GlobalMenuService, GlobalMenuSnapshot};
pub use lrc::{active_line_index, parse_lrc, LyricLine};
pub use settings::{EqualizerSettings, SettingsData, SettingsStore};
pub use single_instance::SingleInstanceGuard;

