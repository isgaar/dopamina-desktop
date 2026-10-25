#!/usr/bin/env bash
# Location: install.sh
# Function: Builds the desktop client and registers a user-level desktop entry
# with the dopamine:// protocol handler used by the web session handoff.
# All Rights Reserved isgaar

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${DOPAMINE_BUILD_DIR:-${SCRIPT_DIR}/build}"
DATA_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}"
APPS_DIR="${DATA_DIR}/applications"
ICON_DIR="${DATA_DIR}/icons/hicolor/512x512/apps"

echo "==> Compilando Dopamina..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel 2

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
Exec="${BUILD_DIR}/dopamine" %u
TryExec=${BUILD_DIR}/dopamine
Icon=dopamine
Terminal=false
Categories=AudioVideo;Audio;Player;
Keywords=music;player;hi-fi;dopamine;
MimeType=x-scheme-handler/dopamine;
StartupNotify=true
StartupWMClass=dopamine
EOF
update-desktop-database "${APPS_DIR}" >/dev/null 2>&1 || true
xdg-mime default dopamine.desktop x-scheme-handler/dopamine \
    >/dev/null 2>&1 || true

echo "==> Lanzando Dopamine Desktop..."
exec "${BUILD_DIR}/dopamine" "$@"
