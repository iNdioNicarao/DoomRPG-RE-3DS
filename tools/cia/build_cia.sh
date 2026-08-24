#!/bin/sh
# Host orchestrator: build the toolchain image, then build the game + CIA.
# Run from tools/cia/. Mounts the repo root so the image can
# reach the source tree (repo) and tools/cia/ (recipe + output).
#
# Assets (DoomRPG.zip + .wav SFX + .mp3 music) are supplied on the SD card at
# 3ds/doomrpg/ at runtime; they are not needed for the build itself.
#
# Usage:
#   ./build_cia.sh            # build image if missing, then game + CIA
#   ./build_cia.sh --no-build # just ensure the image exists
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
IMG="doomrpg-cia-build"

if ! docker image inspect "$IMG" >/dev/null 2>&1; then
    echo "=== building toolchain image $IMG (makerom + bannertool) ==="
    docker build -t "$IMG" "$HERE"
fi

if [ "$1" = "--no-build" ]; then
    echo "Image ready: $IMG"
    exit 0
fi

echo "=== building game + CIA ==="
docker run --rm \
    -v "$ROOT":/src -w /src \
    -v "$HERE":/cia \
    "$IMG"
echo "=== output: $HERE/DoomRPG-*.cia ==="
