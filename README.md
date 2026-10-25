# dopamina-desktop

Native Dopamine desktop client aligned with the Android app. The presentation
layer is Qt 6/QML; the core is being migrated to Rust with
[cxx-qt](https://kdab.github.io/cxx-qt/).

## Features

- Local library playback with Hi-Fi DSP, crossfade, equalizer, and `.lrc`
  synchronized lyrics. FM stations and the legacy playback sync API are gone.
- Queue with song mix ("Oír mix"), playlists, folders, albums, artists, and
  genres.
- Dynamic resizable sidebar with drag-to-resize splitter, snap collapse
  under 120px, double-click collapse toggle, and width persistence.
- Full capsule navigation pills (`Theme.radiusFull`) with contrast-aware
  adaptive text colors for extracted album accents, and floating `ContentTabs`.
- Native window caption controls docked flush to the real corner with hover
  fill and window-radius matching.
- Five themes ported from Android: Rose Glow (default), ExoBlur, TheGrand,
  Redline, and Flat. The interface is dark by default.
- Seamless handoff from web console with automatic listener session and token
  adoption, instant catalog population, and single-instance IPC window focus.
- Theme and language persist in `appearance.xml`; remaining preferences stay in
  the native INI settings.
- Lumpy Sans is the only interface typeface. Main and secondary windows use
  four-corner rounded chrome.

## Appearance

`SettingsStorage` writes `appearance.xml` with `QXmlStreamWriter` under
`QStandardPaths::GenericConfigLocation/<org>/Dopamine/`. Qt 6 does not provide
`QSettings::XmlFormat`, so the file mirrors Android's XML resource model:

```xml
<appearance>
    <theme>roseglow</theme>
    <language>es</language>
</appearance>
```

Theme colors live only in `src/shared/colors/theme_*.xml`:
`roseglow`, `exoblur`, `thegrand`, `redline`, and `flat`. `ColorManager` loads
them at runtime and `Theme` exposes the active palette and language tokens.

## Layout

- `src/`: C++/QML application shell and feature controllers.
- `rust/`: Rust core built in parallel, mirroring the Android layers.
- `cmake/`: source, QML, test, and Rust manifests.

## Sessions

Desktop sessions are owned by Rust under `rust/src/session/`. The
`SessionBridge` QObject, exposed to QML as `sessionController`, validates the
cached token, drives login and logout, and handles `dopamine://connect`
handoffs from the web console.

- Profile: `~/.config/isgaar/Dopamine/session.xml`
- Token: `~/.config/isgaar/Dopamine/credentials/session.token`, mode `0600`
- Handoff: `/auth/qr/sessions` for real accounts, otherwise the server URL
  deep link

`install.sh` registers `x-scheme-handler/dopamine`, so browsers open the
running desktop instance instead of starting a duplicate process.

## Rust core

- `rust/src/data/model/track.rs`: track model with the Android catalog contract
  and Spanish/English server payload aliases.
- `rust/src/data/model/playback.rs`: synchronized playback snapshot.
- `rust/src/data/remote/dopamine_api_client.rs`: asynchronous `reqwest` client
  for the FastAPI backend.
- `rust/src/bridge.rs`: `CoreBridge` QObject exposed to QML as `rustBridge`.

`CoreBridge` exposes `endpoint`, `status`, `last`, `count`, `busy`, `connected`,
plus `setBaseUrl()`, `checkConnection()` and `loadTracks()`. The C++ bootstrap
keeps `endpoint` synchronized with `ProjectConfig::apiUrl()`.

## Build and test

Prerequisites: Rust stable, CMake 3.24+, a C++17 compiler and Qt 6.

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --test-dir build/linux-debug --output-on-failure
./build/linux-debug/dopamine
```

The first configure downloads `cxx-qt-cmake` 0.10.0 with `FetchContent`.
Run the Rust tests independently:

```sh
cargo test --manifest-path rust/Cargo.toml
```

Relevant environment variables:

- `DOPAMINE_API_URL`: backend URL read by `ProjectConfig`.
- `DOPAMINE_SERVER_URL`: fallback used by the Rust bridge when it is created
  directly, without the C++ configuration.
