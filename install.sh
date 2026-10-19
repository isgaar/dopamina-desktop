#!/usr/bin/env bash
# Location: install.sh
# Function: Compilation and non-invasive temporary execution script launching
# the Dopamine desktop client directly from build tree without system changes.
# All Rights Reserved isgaar

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "==> Compilando Dopamine..."
cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --parallel 2

echo "==> Lanzando Dopamine Desktop temporalmente..."
exec "${BUILD_DIR}/dopamine" "$@"
