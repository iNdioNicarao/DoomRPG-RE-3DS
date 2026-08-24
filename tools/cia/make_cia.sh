#!/bin/sh
# Build a CIA from a built DoomRPG.elf.
# Layout mirrors the dxx-3ds make_cia.sh. This script does NOT bump the
# version — bump tools/cia/VERSION yourself, exactly once per real build.
#
# Usage (run from inside tools/cia/):
#   ./make_cia.sh [path/to/DoomRPG.elf]
# ELF defaults to ../../public/build/src/DoomRPG.elf (CMake out-dir).
#
# Requires DEVKITPRO on PATH (bannertool, makerom in $DEVKITPRO/tools/bin).
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"

# --- version (filename only; makerom reads Title/Version from the .rsf) ---
if [ -f VERSION ]; then
    APP_VERSION="$(cat VERSION | tr -d '[:space:]')"
else
    APP_VERSION="0.0.0"
fi

# --- locate the built ELF (HERE-based so cwd doesn't matter) ---
ELF="${1:-$HERE/../../build/src/DoomRPG.elf}"
if [ ! -f "$ELF" ]; then
    echo "ERROR: ELF not found at: $ELF" >&2
    echo "  Build first (from public/):" >&2
    echo "    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/3DS.cmake" >&2
    echo "    cmake --build build" >&2
    exit 1
fi
cp "$ELF" ./DoomRPG.elf 2>/dev/null || true
if [ ! -s ./DoomRPG.elf ] && [ "$ELF" != "./DoomRPG.elf" ]; then
    cp "$ELF" ./DoomRPG.elf
fi

bannertool makebanner -i banner.png -a silence.wav -o banner.bnr
bannertool makesmdh -s "Doom RPG" -l "Doom RPG RE - 3DS port" -p "efimandreev0 / GEC" -i icon.png -o icon.icn
makerom -f cia -o "DoomRPG-${APP_VERSION}.cia" \
    -DAPP_ENCRYPTED=false -rsf DoomRPG-3DS.rsf -target t -exefslogo \
    -elf DoomRPG.elf -icon icon.icn -banner banner.bnr

echo "=== Built DoomRPG-${APP_VERSION}.cia ==="
