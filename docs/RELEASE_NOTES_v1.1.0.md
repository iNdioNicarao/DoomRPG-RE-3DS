# Doom RPG RE — 3DS v1.1.0 Release Notes

Major feature and performance release for the Nintendo 3DS family (`.cia` and `.3dsx`), following v1.0.7. Source-only release: no copyrighted id Software game assets or audio are bundled in the repository.

- **Build Date**: 2026-09-10
- **Version**: 1.1.0
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Maintainer & Lead Developer**: Dennis Isaac Gutierrez Zeledon

---

> ### ⚠️ Notice for Upgrading Players (From v1.0.0 – v1.0.7)
> If you are upgrading from an earlier version and your Circle Pad (left thumbstick) or other newly mapped controls feel unresponsive, your existing saved configuration file (`sdmc:/3ds/doomrpg/saves/Config`) may still contain the older binding table where secondary slots were unset (`-1`).
> 
> **To restore all default controls:**
> 1. In the in-game menu, navigate to **Options $\to$ Controls / Bindings $\to$ Reset Defaults**.
> 2. Alternatively, you can delete or rename `sdmc:/3ds/doomrpg/saves/Config` on your SD card. *(Your save games in `saves/` are completely separate and will NOT be lost).*
> 
> *Note: v1.1.0 includes an in-engine auto-migration that backfills missing Circle Pad bindings into legacy config files automatically, but manual reset is documented as the recommended first troubleshooting step.*

---

## Executive Overview

**Doom RPG RE 3DS v1.1.0** represents a comprehensive performance and feature overhaul designed to provide a locked 60 FPS experience across all Nintendo 3DS and 2DS family models, while modernizing the controls, UI real estate, memory stability, and exploration flow:

1. **Universal Single Binary with Dynamic Hardware Profiles**:
   - Maintains a single unified `.cia` and `.3dsx` for all consoles.
   - Automatically probes hardware capabilities via `APT_CheckNew3DS` and `CFGU_GetSystemModel` at boot, intelligently applying Old 3DS / 2DS vs New 3DS profiles without user intervention.
   - Stereo 3D rendering and texture uploads are completely bypassed on 2DS hardware (`consoleModel == 3`).

2. **Old 3DS & 2DS Performance Suite**:
   - **Internal 200×240 Viewport with Zero-Overhead PICA200 GPU Scaling**: On Old 3DS/2DS, the internal software raycasting column count is halved to 200, cutting column casting and vertical span rasterization by 50% while fitting completely inside the 32 KB L1 data cache. The PICA200 GPU automatically upscales the frame to 400px using Citro2D hardware scaling with zero CPU cost. Top overlays, text, and HUD remain crisp at native 400px.
   - **Solid Floor & Ceiling Preset**: Old 3DS hardware defaults to fast solid floor/ceiling rendering (`renderFloorCeilingTextures = false`), saving 30–40% frame time on 268 MHz ARM11 CPUs while preserving the authentic mobile release look.
   - **LUT Math & Blit Optimization**: Replaced per-pixel bottom screen integer division (`(sx * g_botH) / 400`) with a precomputed 400-entry lookup table (`s_lutBotH`), saving 96,000 divisions per frame.
   - **Automap Dirty Blitting**: Stationary exploration skips the 96,000-pixel bottom screen blit entirely, updating only when movement, rotation, or touch interaction occurs.

3. **Widescreen UI & Text Real Estate Overhaul**:
   - **360px Widescreen Notebook**: Replaced the legacy 128px-wide flip-phone container (`menuHalfW = 64`) with a full 360px wide layout (`menuHalfW = 180`, `scrollX = 378`), utilizing the entire 400px top screen.
   - **Multi-Sentence Pipe Merging**: Mission notes and terminal messages are parsed across double-pipe entries (`||`), joining mobile wrap segments into natural sentences and wrapping cleanly up to 48–50 characters per line.
   - **Header Banners & Dividers**: Styled section banners and clean separators display mission logs clearly.

4. **HUD Memory Leak & Failsafe Watchdog Resolution**:
   - **Eliminated 60 FPS RAM Leak**: Removed the un-freed temporary surface early-exit in `Hud_drawTopBar` which previously leaked 1.92 MB/s of RAM whenever a message expired.
   - **Static Persistent HUD Surfaces**: Pre-allocated persistent top and bottom HUD surfaces (`s_topBarSurface`, `s_bottomBarSurface`), eliminating 120 dynamic heap allocations per second and ending userland heap fragmentation.
   - **Stuck Message Watchdog**: Enforced a hard 3.0-second timeout ceiling for slot 0 messages to prevent queue deadlocks from freezing top status notifications.
   - **Black Screen After Saving Fix**: Resolved the condition where heap exhaustion caused `ST_SAVING` black screen wipes to persist permanently across top and bottom HUD bars.
   - **OOM Safety Check in BitShapes**: Added explicit NULL validation in `Render_loadBitShapes` to prevent ARM11 Data Aborts (Crash Dump #146).

5. **Turbo Mode 2.0 & Gameplay Acceleration Suite**:
   - **Truncated Post-Attack Recovery**: Reduced post-action tail wait from 333ms to 75ms in Turbo mode, with immediate skip on pressing `A`.
   - **"Hold-to-Fire" Attack Buffering**: Holding `A` continuously queues and executes attacks the exact frame the player's turn opens.
   - **4x Projectile Velocity**: High-speed missiles (`speed <<= 2`) cross rooms in 3–4 frames while retaining full 3D rendering, smoke trails, and sound effects.
   - **Off-Screen Monster AI Instant Snap**: Distant or non-visible patrolling monsters resolve their movement turns instantly without multi-frame stepping.
   - **2-Frame Grid Traversal & Snappy Turning**: Walking and 90° turning dynamically step in 2 frames (33ms at 60 FPS) when Turbo exploration is active.
   - **2x Fast Door Opening**: Sliding doors and secret walls open at double speed without stalling movement.
   - **Accelerated Typewriters & Instant Reveal**: Terminal text speeds up from 25ms to 5ms per character in Turbo mode, with instant page reveals on tapping `A` or touching the screen.

6. **Control Modernization & Usability Enhancements**:
   - **Dedicated Weapon Cycling Face Buttons (`X` / `Y`)**: Mapped `X` to Next Weapon and `Y` to Previous Weapon in the default controls and config migration table. This enables physical weapon switching on Old 3DS, 3DS XL, and 2DS consoles (which lack `ZL`/`ZR` triggers) while giving New 3DS players both face button and trigger options.
   - **Contextual Dialog & Terminal Message Scrolling via `X` / `Y`**: Because weapons cannot be fired or switched while reading terminals or NPC speech, `X` and `Y` dynamically double as **Scroll Down (`X`)** and **Scroll Up (`Y`)** buttons whenever dialogs or computer passcode prompts are displayed. This gives Old 3DS and 2DS players without a C-Stick nub full hardware message scrolling parity.
   - **Direct Analog Circle Pad Polling**: Deflections are polled directly via `hidCircleRead` with deadzones, guaranteeing Circle Pad thumbstick movement regardless of save file state.
   - **Right Nub (C-Stick) Universal Scrolling**: Smooth scrolling using the C-Stick across dialogs, terminals, notebooks, menus, and help documentation.
   - **Terminal Password Auto-Scroll & Passcode Pinning**: In `ST_DIALOGPASSWORD`, the text automatically scrolls to reveal the prompt line, and the code input box remains pinned and visible at all times.
   - **Touchscreen Menu Debounce & Phantom Rejection**: Implemented touch-up release verification, 100ms hold debounce, bezel corner inset ($X = 345..394, Y = 244..260$), and an optional menu toggle to prevent accidental pause menu popups from hand grip or bezel flex.

7. **New In-Game Video & Input Options**:
   - **System Profile**: `[ Auto | N3DS (High) | O3DS (Perf) ]`
   - **Render Scaling**: `[ Crisp (400) | Retro (200) ]`
   - **Floor/Ceil Textures**: `[ on | off ]`
   - **3D Depth**: `[ Low | Normal | High | Max ]`
   - **Touch Menu**: `[ on | off ]`
   - **Hold Fire**: `[ on | off ]`
   - **Turbo Scope**: `[ Combat | Explore | All ]`
   - **Typewriter**: `[ Classic | Fast | Instant ]`
   - **Reset Defaults**: Restores factory controls and binding tables instantly.

8. **High-Quality Roland SC-55 Soundtrack**:
   - Upgraded in-game music tracks (`5039.mp3`, `5040.mp3`, `5043.mp3`) with authentic Roland Sound Canvas SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php) (`d_dead.mp3`, `d_e1m1.mp3`, `d_inter.mp3`).
   - Sincere thanks to GBAtemp user **bakuDD** for identifying that the downmixed mobile BREW MIDI tracks correspond directly to original Doom and Doom II compositions and recommending these definitive Roland SC-55 recordings.

---

## Detailed Changelog

- `feat(arch): implement universal binary with runtime console detection (New 3DS vs Old 3DS/2DS)`
- `feat(perf): add internal 200x240 raycaster scaling with Citro2D zero-overhead GPU scaling`
- `feat(perf): add 400-entry bottom screen LUT and automap dirty blit checks`
- `feat(perf): bypass stereoscopic 3D routines on Nintendo 2DS hardware`
- `feat(ui): redesign in-game notebook with 360px widescreen layout and pipe note parsing`
- `fix(hud): eliminate 60 FPS memory leak in Hud_drawTopBar and add 3.0s watchdog timer`
- `fix(hud): allocate static persistent HUD surfaces to prevent post-save blackouts`
- `fix(render): add NULL check after SDL_malloc in Render_loadBitShapes to prevent ARM11 Data Aborts`
- `feat(turbo): reduce post-attack delay to 75ms and add hold-to-fire attack buffering`
- `feat(turbo): quadruple missile projectile velocity (speed <<= 2)`
- `feat(turbo): add off-screen monster AI instant snap for non-visible tiles`
- `feat(turbo): support 2-frame exploration grid traversal and 2x door opening speed`
- `feat(input): add direct analog Circle Pad polling via hidCircleRead and config migration`
- `feat(input): add Right Nub (C-Stick) scrolling for dialogs, terminals, and menus`
- `feat(input): map X and Y face buttons to weapon cycle next/prev and add config migration`
- `feat(input): support contextual message scroll down (X) and up (Y) during dialogs and terminals`
- `fix(input): restrict interaction trigger to A and remove strafe dual-purpose on R`
- `feat(dialog): auto-scroll password dialogs and pin code input box to visible viewport`
- `feat(touch): add touch-up release verification, 100ms debounce, and inset for [ MENU ] button`
- `feat(menu): add dedicated Video and Input options for profiles, scaling, turbo, and defaults`
- `fix(video): prevent stereoscopic eye desynchronization during sector loading and transition screens`
- `feat(audio): replace downmixed MIDI tracks with authentic Roland SC-55 recordings (credit bakuDD)`

---

## Verification & File Hashes

- `DoomRPG-1.1.0.cia`
  - **MD5**: `c611048de3a755ffba01faa3d57dc286`
  - **SHA256**: `cf1c2c35b05113709b8244cf66a23f3b2faf5475788db2fa753ba65b7ec2da08`
- `DoomRPG.3dsx`
  - **MD5**: `2ce686f7e2b1c7e46776164f1bfb06d7`
  - **SHA256**: `41f45b9131cb3a31d43cb574884b2bf7c6468e112a020c1fabf7364eaa70e259`
- `DoomRPG.elf`
  - **MD5**: `05cea894c2242347491d6bb2140a736b`
  - **SHA256**: `cc77b570978fe5bd18dc8f472e0cbc8e516849f0b8073a9905372b3b844cae7a`

---

## Credits & Attribution

- **Port & 3DS Enhancements**: Dennis Isaac Gutierrez Zeledon
- **AI Coding Partner**: Gemini Antigravity (Google DeepMind)
- **Soundtrack Research & Recommendation**: bakuDD (GBAtemp)
- **Original Reverse Engineering**: GEC Team (Erick194 and contributors)
- **Initial 3DS Port Base**: Efim Andreev (`efimandreev0`)
- **Doom RPG**: id Software / Fountainhead Entertainment
