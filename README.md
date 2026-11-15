# dopamina-desktop

Native Dopamine desktop client implemented 100% in pure Rust (`eframe`/`egui`
for frameless presentation, `symphonia` for Hi-Fi decoding, native Rust DSP,
runtime `libpulse-simple` audio output, and `zbus` + Wayland protocols for
desktop integration). Zero Qt, QML, C++, or CMake dependencies.

## Features

- **Hi-Fi Audio & Pure-Rust DSP**: Playback for `FLAC`, `ALAC`, `WAV`, `MP3`,
  `AAC`, and `OGG` decoded via `symphonia` and processed through a 32-bit float
  chain (`12-band EQ`, `limiter`, `AGC`, `Z-Bass/Mids/Treble`, `compressor`,
  `echo`, `reverb`, `autowah`, `flanger`, equal-power `crossfade`, real-time FFT
  visualizer, and `.lrc` synchronized lyrics).
- **Catalog & Library Organization**: Local folder, mounted SMB share, or
  Dopamine Core server catalog with acoustic deduplication, A-Z jump rail,
  search with persistent recents, and offline track caching.
- **Navigation, Views & Queue**: *Listen* home dashboard, *Library* (`Tracks`,
  `Albums`, `Artists`, `Genres`, `Folders`), *Favorites*, *Playlists*, *Queue*
  with mix triangulation, bidirectional desktop/mobile handoff
  (`SessionPlaybackController`), and `dopamine://` single-instance IPC
  (`SingleInstanceGuard`).
- **Deterministic Frameless Layout & Smooth Animations**:
  - Pixel-accurate `Rect` workspace partitioning (`56px` custom TitleBar,
    collapsible resizable SideNav `72..500px`, main content area, optional
    `.lrc` drawer, and `74px` PlayerBar).
  - Shared `egui` animation helpers (`src/ui/theme.rs`): sliding active
    capsules in SideNav and `ContentTabs`, keyed view/sub-tab slide + fade
    transitions (`begin_keyed_transition`), smooth open/close transitions for the
    sidebar, visualizer, and `.lrc` drawer, hover/active color interpolation,
    animated 3-bar playing EQ indicator, pulsing play glow, and modal backdrop
    scrim.
- **KDE Plasma Global Menu (`GlobalMenuService`)**:
  - Exposes `com.canonical.dbusmenu` at `/MenuBar` over Session D-Bus (`zbus`)
    with localized *Archivo/File*, *Reproducción/Playback*, *Ver/View*,
    *Audio*, and *Ayuda/Help* menus synchronized with live playback state.
  - Binds KWin Wayland `org_kde_kwin_appmenu_manager` (`wayland-client` +
    `wayland-protocols-plasma`) on Plasma Wayland, falls back to
    `com.canonical.AppMenu.Registrar.RegisterWindow` on X11, and shares the
    same `MenuEntry` tree with an elevated in-app menu popup.
- **Themes & Localization**: Five XML themes (`roseglow`, `exoblur`,
  `thegrand`, `redline`, `flat`) plus cover-derived dynamic accents, Lumpy Sans
  + Lucide fonts, and complete bilingual Spanish/English catalogs (413 keys in
  `src/shared/i18n/strings_*.xml`).

## Appearance & Storage

Theme and language persist under `~/.config/isgaar/Dopamine/` in
`appearance.xml` alongside full application settings in `settings.json`:

```xml
<appearance>
    <theme>roseglow</theme>
    <language>es</language>
</appearance>
```

Theme tokens live in `src/shared/colors/theme_*.xml` and translations live in
`src/shared/i18n/strings_*.xml`.

## Project Layout

- `src/main.rs` & `src/app.rs`: `eframe`/`egui` bootstrap, deterministic layout
  slicing, animated drawer orchestration, and `GlobalMenuService` sync.
- `src/audio/`: Symphonia decoder, crossfade/transition planner,
  `libpulse-simple` sink, and playback worker.
- `src/dsp/`: 32-bit float equalizer, dynamics, multi-FX, and ADPCM DSP chain.
- `src/core/`: URL/source config, XML+JSON settings, LRC parser, Unix IPC guard,
  and native KDE `GlobalMenuService` (`global_menu.rs`).
- `src/data/`: Track/Playback/Playlist models and `reqwest` API client.
- `src/features/`: Library, Queue/Mix, Playlists, and Interconnect controllers.
- `src/session/`: `dopamine://` URI handoff, XML/token storage, and auth API.
- `src/shared/`: XML themes (`colors/`) and ES/EN catalogs (`i18n/`).
- `src/ui/`: Custom frameless chrome, popup menus, sidebar, content views,
  visualizer, player bar, modals, and shared animation primitives (`theme.rs`).

## Build and Test

Prerequisites: Rust stable (`cargo`) and PulseAudio client libraries (`libpulse`
loaded dynamically at runtime).

```sh
cargo test
cargo build --release
./target/release/dopamine
```

Or run `./install.sh` to compile in Release mode, register the desktop entry
and `x-scheme-handler/dopamine`, and launch the app.

Relevant environment variables:

- `DOPAMINE_API_URL`: backend URL read by `ProjectConfig`.
- `DOPAMINE_SERVER_URL`: fallback server URL used by session and settings
  defaults.
