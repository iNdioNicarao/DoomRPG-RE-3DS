#!/bin/sh
# Build a CIA from a built DoomRPG.elf.
# This script does NOT bump the version — bump VERSION yourself.
#
# Usage (run from inside vibe/cia/):
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
ELF="${1:-$HERE/../../public/build/src/DoomRPG.elf}"
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

if [ -f banner_assets/banner.cgfx ]; then
    if [ -f banner_assets/banner_audio.wav ]; then
        bannertool makecwav -i banner_assets/banner_audio.wav -o banner_assets/banner_audio.bcwav
    fi
    if [ -f banner_assets/banner_audio.bcwav ]; then
        bannertool makebanner -ci banner_assets/banner.cgfx -ca banner_assets/banner_audio.bcwav -o banner.bnr
    elif [ -f banner_assets/banner_audio.wav ]; then
        bannertool makebanner -ci banner_assets/banner.cgfx -a banner_assets/banner_audio.wav -o banner.bnr
    else
        bannertool makebanner -ci banner_assets/banner.cgfx -a silence.wav -o banner.bnr
    fi
    bannertool makesmdh -s "Doom RPG" -l "Doom RPG RE - 3DS" -p "Dennis Isaac Gutierrez Zeledon" -i banner_assets/icon.png -f allow3d,extendedbanner,visible -o icon.icn
elif [ -f banner.png ]; then
    bannertool makebanner -i banner.png -a silence.wav -o banner.bnr
    bannertool makesmdh -s "Doom RPG" -l "Doom RPG RE - 3DS port" -p "Dennis Isaac Gutierrez Zeledon" -i icon.png -o icon.icn
fi
makerom -f cia -o "DoomRPG-${APP_VERSION}.cia" \
    -DAPP_ENCRYPTED=false -rsf DoomRPG-3DS.rsf -target t -exefslogo \
    -elf DoomRPG.elf -icon icon.icn -banner banner.bnr

echo "=== Built DoomRPG-${APP_VERSION}.cia ==="
