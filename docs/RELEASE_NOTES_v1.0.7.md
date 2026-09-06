# Doom RPG RE — 3DS v1.0.7

Patch release for the 3DS home-menu build (`.cia`) and 3DSX package of Doom RPG RE, following v1.0.6. Source-only: no game assets or audio are bundled in the repository. They are supplied on the SD card (see *Install*).

- **Build**: 2026-09-06
- **Version**: 1.0.7
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Developer Attribution**: Dennis Isaac Gutierrez Zeledon

---

## Overview

Version 1.0.7 addresses hardware input and filesystem usability feedback reported by the homebrew community:

1. **Circle Pad Backward (Down) Fix**: Fixed an issue where pulling down on the Circle Pad failed to move the character backward or navigate menus down. In libctru, `KEY_CPAD_DOWN` is defined as `BIT(31)` (`0x80000000`), which represents a negative value when stored in a signed 32-bit `int` (`-2147483648`). The input handling loop previously checked `if (kb <= 0) continue;`, inadvertently discarding `KEY_CPAD_DOWN` as an empty slot sentinel. The loop now explicitly checks for empty slot sentinels (`-1` and `0`), restoring full Circle Pad backward support.
2. **Automatic Save Directory Creation**: Implemented `Game_ensureSaveDir()` to recursively create `sdmc:/3ds/doomrpg/saves` on boot and prior to all file write routines. Fresh installs running exclusively via `.3dsx` no longer encounter fatal write errors when attempting to save game state or configuration.
3. **Repurposed SELECT Button (Automap Recenter & Zoom)**: The automap is permanently active on the bottom touch screen, rendering the legacy phone `#` map toggle obsolete. The `SELECT` hardware button now acts as a dedicated Automap Recenter (`[CTR]`). If the map is already centered, tapping `SELECT` cycles the zoom scale between 1x, 2x, and 3x, complete with audio chime feedback and an on-screen notification banner.
4. **Native 3DS Dialogue & UI Prompts**: Automatically adapts legacy phone keypad references found in level dialogue and computer logs into native 3DS controls in real-time (e.g., `* and 7` -> `ZL/ZR`, `0 key` -> `START`, `# key` -> touch screen automap, `9 button` -> `B button`, `OK button` -> `A button`). End-of-level summary screens, credits, and the in-game Help menu have also been refreshed for native 3DS gameplay.
5. **Documentation Clarifications**: Explicitly documented `entities.db` and `help.txt` as required root asset files, and clarified that the original extraction's `.mid` music tracks must be converted to numbered `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`).

---

## Highlights (v1.0.7)

- **Hardware Circle Pad Fix**: Restored full 4-directional Circle Pad support (Up, Down, Left, Right) in both 3D exploration and 2D menu navigation.
- **Standalone 3DSX Saves**: Full out-of-the-box compatibility for Homebrew Launcher users without needing manual directory creation or CIA pre-installation.
- **SELECT Recenter & Zoom**: Quick-recenter and cycle automap zoom levels at any time during gameplay with `SELECT`.
- **Modernized Text & Prompts**: No more legacy mobile keypad prompts—game logs, tutorials, and menus reflect actual 3DS hardware buttons.
- **Enhanced Installation Guide**: Clearer accounting of all required loose asset files and music format conversions.

---

## Detailed Commit Log (v1.0.7)

- `fix(input): resolve Circle Pad Down by replacing signed keycode comparison with sentinel check`
- `fix(fs): automatically create sdmc:/3ds/doomrpg/saves directory at boot and on save`
- `feat(controls): repurpose SELECT button to recenter automap and cycle zoom levels`
- `feat(dialog): dynamically adapt legacy phone keypad prompts to native 3DS controls`
- `docs: update required asset file list and MIDI-to-MP3 conversion guidance`

---

## Installation & Verification

1. Install `DoomRPG-1.0.7.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
2. Copy extracted assets directly into `sdmc:/3ds/doomrpg/`:
   - `entities.db` and `help.txt`
   - Maps: `*.bsp` (e.g. `junction.bsp`, `sector1.bsp`, etc.)
   - Binary tables: `wtexels.bin`, `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, `sintable.bin`
   - Graphics: `*.bmp` UI and sprite sheets
3. Copy audio into `sdmc:/3ds/doomrpg/`:
   - Sound effects: numbered `.wav` files (`5042.wav` through `5138.wav`)
   - Music tracks: converted `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`)
4. Launch from the HOME Menu or Homebrew Launcher. The `saves/` folder will be created automatically.

---

## Binaries & Hashes

- `DoomRPG-1.0.7.cia`
  - **MD5**: `527d55f7e7c728f42955747f03eb4c2a`
  - **SHA256**: `64c451edec02f9adfc91392dab28ded0d8b7baa38e37a4d6f730e1c4146360ea`
- `DoomRPG.3dsx`
  - **MD5**: `2350c877631573b643c1d04b4dff206d`
  - **SHA256**: `0e20b9365797aeb2da161c5c090982797a7b4d69f8577f70f68cf5fb6a4a763c`

---

## Credits & Attribution

- **Port & 3DS Enhancements**: Dennis Isaac Gutierrez Zeledon
- **AI Coding Partner**: Gemini Antigravity (Google DeepMind)
- **Original Reverse Engineering**: GEC Team (Erick194 and contributors)
- **Initial 3DS Port Base**: Efim Andreev (`efimandreev0`)
- **Doom RPG**: id Software / Fountainhead Entertainment
