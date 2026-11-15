#!/usr/bin/env bash
# Location: install.sh
# Function: Builds the pure Rust desktop client and registers a user-level
# desktop entry with the dopamine:// protocol handler used by session handoff.
# All Rights Reserved isgaar

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="${CARGO_TARGET_DIR:-${SCRIPT_DIR}/target}"
BIN_PATH="${TARGET_DIR}/release/dopamine"
DATA_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}"
APPS_DIR="${DATA_DIR}/applications"
ICON_DIR="${DATA_DIR}/icons/hicolor/512x512/apps"

echo "==> Compilando Dopamina (100% Rust)..."
cargo build --release --manifest-path "${SCRIPT_DIR}/Cargo.toml"

echo "==> Registrando Dopamina Desktop a nivel de usuario..."
mkdir -p "${APPS_DIR}" "${ICON_DIR}"
install -m 0644 "${SCRIPT_DIR}/src/shared/assets/app-icon.png" \
    "${ICON_DIR}/dopamine.png"
cat > "${APPS_DIR}/dopamine.desktop" <<EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=Dopamine
GenericName=Music Player
Comment=Native Hi-Fi music client for Dopamine Core
Exec="${BIN_PATH}" %u
TryExec=${BIN_PATH}
Icon=dopamine
Terminal=false
Categories=AudioVideo;Audio;Player;
Keywords=music;player;hi-fi;dopamine;
MimeType=x-scheme-handler/dopamine;
StartupNotify=true
StartupWMClass=com.isgaar.dopamine
EOF
update-desktop-database "${APPS_DIR}" >/dev/null 2>&1 || true
xdg-mime default dopamine.desktop x-scheme-handler/dopamine \
    >/dev/null 2>&1 || true

echo "==> Lanzando Dopamine Desktop..."
exec "${BIN_PATH}" "$@"
