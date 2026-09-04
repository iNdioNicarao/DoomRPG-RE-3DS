# Doom RPG RE — 3DS v1.0.6

New 3DS home-menu build (`.cia`) and 3DSX package of Doom RPG RE, following v1.0.5. Source-only: no game assets or audio are bundled in the repository. They are supplied on the SD card (see *Install*).

- **Build**: 2026-09-04
- **Version**: 1.0.6
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Developer Attribution**: Dennis Isaac Gutierrez Zeledon

---

## Overview

Version 1.0.6 introduces major gameplay acceleration and stereoscopic visual refinement. It adds a touchscreen Combat Turbo button on the automap control bar for lightning-fast battles and projectile animations, renders combat particles (blood, sparks, gibs, and explosions) in full stereoscopic 3D depth to eliminate eye flicker, introduces an in-game 3D Depth Multiplier setting (Low, Normal, High, Max), and adds an in-game Texture Filtering toggle (Crisp vs. Smooth) accessible directly from both the in-game Pause Menu and Video Options.

---

## Highlights (v1.0.6)

- **Combat Turbo Button**:
  - Dedicated bold green `[ TURBO ]` toggle button on the bottom-screen map control bar (directly adjacent to `[ CTR ]`).
  - Visual and audio feedback: double neon green border (`0xFF00FF44`) and active label when enabled, dark green border when inactive, click sound feedback, and top-screen HUD notification banner (`"Combat Turbo: ON"` / `"Combat Turbo: OFF"`).
  - Accelerates turn-based combat: enemy turn transitions and combat delays reduced by ~3× (down to 25ms floors), projectile speeds doubled, and attack animations accelerated for snappy, responsive engagements without altering the underlying turn-based balance.

- **Stereoscopic 3D Combat Particles & Retinal Flicker Elimination**:
  - Combat particle system (blood splatters, sparks, monster gibs, and weapon explosions) now renders to both the left and right stereoscopic eye buffers.
  - Right-eye particles are rendered with depth-matched parallax offset calculated from world geometry depth, fixing single-eye retinal rivalry and visual flicker while delivering stunning pop on the 3DS top screen.

- **In-Game 3D Depth Multiplier**:
  - New `3D Depth:` configuration option added to both the in-game Pause Menu (`MENU_INGAME`) and main *Video Options* (`MENU_VIDEO`).
  - Allows players to scale hardware 3D stereoscopic separation across four levels: `Low 0.7x`, `Normal 1.0x` (default), `High 1.4x`, and `Max 1.8x`. Seamlessly modulates the physical 3D slider value.

- **Real-Time Texture Filtering Toggle (Crisp vs. Smooth)**:
  - New `Filter:` option added to both the in-game Pause Menu and *Video Options*.
  - Instantly toggles Citro3D top-screen texture filtering between `Crisp` (`GPU_NEAREST` — authentic raw pixel art) and `Smooth` (`GPU_LINEAR` — bilinear filtered scaling) on the fly without restarting.

---

## Detailed Commit Log (v1.0.6)

1. `247ce59` — `feat: add combat turbo, 3D particle depth, 3D depth multiplier, and texture filtering`

---

## Installation & Verification

1. Install `DoomRPG-1.0.6.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
2. Assets remain located at `sdmc:/3ds/doomrpg/`.
3. Launch from the HOME Menu.

---

## Binaries & Hashes

- `DoomRPG-1.0.6.cia`
  - **MD5**: `ab6d4f99d24937c1427023f77c776661`
  - **SHA256**: `924af6c2672a90a50aee1c14caff6f7d2565a84875cb6f86ae4b761285030151`
- `DoomRPG.3dsx`
  - **MD5**: `fdeae4ed89eab5ba5d81072873ffe076`
  - **SHA256**: `7c1e0d709580a77f90a894e4123092222fa6a47f5d6ff4e8d4240cd7c8902ba0`

---

## Credits & Attribution

- **Port & 3DS Enhancements**: Dennis Isaac Gutierrez Zeledon
- **Original Reverse Engineering**: GEC Team (Erick194 and contributors)
- **Doom RPG**: id Software / Fountainhead Entertainment
