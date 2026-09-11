# Doom RPG RE — 3DS v1.1.0 Release Notes

Feature, performance, and stability release for the Nintendo 3DS family (`.cia` and `.3dsx`), following v1.0.7. Source-only release: no copyrighted id Software game assets or audio are bundled in the repository.

- **Build Date**: 2026-09-11
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

## Overview

Doom RPG RE 3DS v1.1.0 provides performance optimizations across Old 3DS, 2DS, and New 3DS consoles, updates input mappings and touch controls, expands UI layout widths, fixes HUD memory leaks, and adds gameplay options.

### 1. Dynamic Hardware Profiles & Universal Binary
- **Universal Single Binary**: A single unified `.cia` and `.3dsx` executable boots on all 3DS, 3DS XL, 2DS, New 3DS, New 3DS XL, and New 2DS XL consoles.
- **Runtime Console Detection**: Probes hardware capabilities via `APT_CheckNew3DS` and `CFGU_GetSystemModel` at startup, applying Old 3DS / 2DS vs New 3DS defaults without user intervention.
- **2DS Stereo Bypass**: Stereoscopic 3D rendering and left/right framebuffer setup are bypassed on Nintendo 2DS hardware (`consoleModel == 3`).

### 2. Rendering Pipeline & Engine Optimizations
- **Internal 200×240 Viewport Scaling**: On Old 3DS / 2DS, internal software raycasting column width can be halved to 200 columns (`Retro 200` mode), cutting column casting and vertical span rasterization while fitting within the ARM11 32 KB L1 data cache. The frame is scaled to 400px using Citro2D / PICA200 GPU hardware scaling with zero CPU cost. Text, dialogs, and HUD remain at native 400px.
- **Floor & Ceiling Span Step Optimization**: Simplified ceiling and floor texture step arithmetic (`stepX`, `stepY`) in `Render.c`, reducing per-span multiplications.
- **Solid Floor/Ceiling Preset**: Old 3DS profile defaults to flat solid floor and ceiling colors (`renderFloorCeilingTextures = false`), reducing frame rendering time on 268 MHz ARM11 CPUs.
- **Main Loop CPU Yielding**: Calls `svcSleepThread(1000000)` (1ms) in the main loop when the next game tick has not elapsed, eliminating busy-spin thread starvation and reducing system power draw.
- **Lookup Table Arithmetic**: Replaced per-pixel bottom screen integer division (`(sx * g_botH) / 400`) with a precomputed 400-entry lookup table (`s_lutBotH`), eliminating 96,000 divisions per frame.
- **Fast Bottom Screen Clearing**: Replaced 96,000-pixel manual loop in `SDL_Video.c` with `SDL_memset`.
- **Static Persistent HUD Surfaces**: Pre-allocated persistent top and bottom HUD surfaces (`s_topBarSurface`, `s_bottomBarSurface`), eliminating 120 dynamic heap allocations per second and preventing heap fragmentation.
- **Automap Dirty Blit Caching**: Stationary exploration skips the 96,000-pixel bottom screen blit entirely, updating only when movement, rotation, or touch interaction occurs.
- **Direct Sound Chunk Lookup Table**: Implemented an $O(1)$ direct array lookup table (`Sound_t.chunkLUT`) for cached WAV sound effects, replacing linear searches.
- **Hoisted Particle Math**: Hoisted invariant animation frame calculations out of the particle render loop.
- **Toolchain Compiler Flags**: Built with `-O3 -ffast-math -fomit-frame-pointer -flto` optimization flags.

### 3. Visual & Particle Systems
- **Re-enabled Blood and Spark Particles**: Optimized circle fill rasterizer in `SDL_Video.c` (`Video_fillCircle`) using symmetric horizontal scanline spans, allowing blood and spark particle systems to run at full frame rate.
- **Sinusoidal Weapon Bobbing**: Added vertical and horizontal weapon bobbing and sway during player movement in `DoomCanvas_renderWeapon`, driven by movement cycle phase.
- **Stepped Damage Vignette**: Replaced flat 2px border lines with a 3-tier stepped red vignette overlay on player damage in `Hud_drawDamageFlash`.
- **Gib Particle Phase Variation**: Desynchronized rotation animation phase across gib particles using particle index offsets.
- **Opaque Color Default**: Initialized alpha channel to `0xFF` in `DoomRPG_setColor`.

### 4. UI & Widescreen Layout
- **360px Widescreen Notebook**: Replaced legacy 128px flip-phone layout (`menuHalfW = 64`) with a 360px wide layout (`menuHalfW = 180`, `scrollX = 378`), utilizing the full 400px top screen.
- **Sentence Joining via Pipe Parsing**: Mission notes and terminal messages are parsed across double-pipe entries (`||`), joining mobile wrap segments into continuous sentences wrapping up to 48–50 characters per line.
- **Bottom-Screen 10-Slot Quick-Select Weapon Rack**: A vertical 10-slot rack on the left edge of the bottom screen ($X = 1..37, Y = 241..450$) displaying authentic HUD ammo icons and distinct weapon badges (`AX`, `EX`, `PI`, `SG`, `CG`, `SS`, `PL`, `RL`, `BF`, `RG`/`DG`). Accommodated by shifting the top status bar to $X = 40..399$. Slot 9 displays Dog Collar (`RG`) when collars are in inventory, switching to Dog companion (`DG`) once a hound is captured.
- **Color-Coded Locked Doors on Automap**: Scans bytecode and tile event triggers for `EV_CHECK_KEY` to render locked doors in Red (`0xFF2222`), Blue (`0x00D0FF`), Yellow (`0xFFEE00`), and Green (`0x20FF50`), with open doors in muted slate (`0x6688AA`) and normal closed doors in amber (`0xEEAA33`).
- **Real-Time Automap Stats Badge**: Displays walkable exploration percentage (`MAP: xx%`) and secret room discovery counter (`SEC: x/x`) at the bottom-left of the automap frame.

### 5. Memory Stability & I/O
- **Eliminated 60 FPS RAM Leak**: Removed un-freed temporary surface in `Hud_drawTopBar` that previously leaked heap memory whenever a notification message expired.
- **Stuck Message Watchdog Ceiling**: Enforced a 3.0-second timeout ceiling for slot 0 messages to prevent queue deadlocks from freezing top status notifications.
- **Black Screen After Saving Fix**: Pre-allocated static HUD surfaces prevent heap exhaustion from leaving the HUD cleared after save routines.
- **BitShapes Allocation Safety**: Added NULL check in `Render_loadBitShapes` after `SDL_malloc` to prevent ARM11 data abort crashes.
- **SD Card Read Retry Logic**: Retries SD card reads before logging fatal I/O read errors in `DoomRPG.c`.
- **Sector Transition Checkpoint Autosaves**: Transitioning through doors to a new sector automatically creates a checkpoint state save.

### 6. Controls & Input Handling
- **Direct Analog Circle Pad Polling**: Polled directly via `hidCircleRead` with deadzone handling, ensuring movement functions independently of save file binding state.
- **C-Stick (Right Nub) Scrolling**: Smooth scrolling using the C-Stick across dialogs, terminals, notebooks, menus, and help documentation.
- **Dedicated Weapon Cycling Face Buttons (`X` / `Y`)**: Mapped `X` to Next Weapon and `Y` to Previous Weapon during gameplay.
- **Contextual Dialog Scrolling via `X` / `Y`**: During dialogs and computer terminals, `X` dynamically acts as **Scroll Down** and `Y` acts as **Scroll Up**.
- **New 3DS `ZL` / `ZR` Triggers**: Bound to Previous / Next weapon.
- **Hold-to-Fire Attack Buffering**: Holding `A` during combat automatically executes attacks with an initial 350ms hold delay before repeating at 180ms intervals. Tile interactions (opening doors/switches) require releasing `A` before an attack can trigger.
- **Touchscreen Menu Debounce**: Implemented touch-up release verification, 100ms hold debounce, and corner inset ($X = 345..394, Y = 244..260$) for the `[ MENU ]` button to prevent accidental pause activations.
- **Config Auto-Migration**: Automatically backfills missing Circle Pad and weapon bindings into legacy configuration files.

### 7. Turbo Mode 2.0 & Gameplay Tuning
- **Truncated Post-Attack Recovery**: Reduced post-action tail wait from 333ms to 75ms in Turbo mode, with instant skip on pressing `A`.
- **4x Projectile Velocity**: High-speed missiles (`speed <<= 2`) cross rooms in 3–4 frames while retaining full 3D rendering, smoke trails, and audio.
- **Off-Screen Monster AI Instant Snap**: Distant or non-visible patrolling monsters resolve their movement turns instantly without multi-frame stepping.
- **2-Frame Grid Traversal & Fast Doors**: Movement and 90° turning dynamically step in 2 frames (33ms at 60 FPS) in Turbo exploration mode, and door animations open at 2x speed.
- **Accelerated Typewriters & Instant Reveal**: Terminal text speeds up from 25ms to 5ms per character in Turbo mode, with instant page reveal on pressing `A` or tapping the touchscreen.

### 8. Audio Subsystem
- **Independent Volume Sliders**: Independent Music and SFX volume sliders (0% to 100%) in Options menu (`Sound_t.musicVolume`, `Sound_t.sfxVolume`), dynamically adjusting `MusicStream_setVolume` and `Mix_Volume(-1)` and persisting in config.
- **Bitshift Volume Scaling**: Replaced division with bitshift (`val >> 7`) for music volume scaling.
- **Multi-Sample Sound Playback**: Enabled simultaneous multi-sample playback permanently.
- **Roland SC-55 Soundtrack Placement**: Fully compatible with Roland Sound Canvas SC-55 recordings (`d_dead.mp3` $\to$ `5039.mp3`, `d_e1m1.mp3` $\to$ `5040.mp3`, `d_inter.mp3` $\to$ `5043.mp3`).

### 9. Hardware Telemetry & Notification LED
- **3DS Notification LED**: Interfaces with `mcuHwc` to provide physical LED telemetry:
  - Low health: crimson heartbeat pulse (< 25% max HP).
  - Berserk active: amber/orange pulse.
  - Secret sector discovered: emerald green double-flash.

---

## Controls Reference

| Action | Physical Button | Touch Screen |
|--------|-----------------|--------------|
| Move Forward / Backward | D-pad Up / Down or Circle Pad (Left Stick) | — |
| Strafe Left / Right | L / R | — |
| Turn Left / Right | D-pad Left / Right or Circle Pad (Left Stick) | — |
| Attack / Talk / Use / Confirm | A (Hold for Attack Buffering) | Tap dialog text |
| Back / Dismiss Dialog / Pass Turn | B | Tap `[ PASS ]` on top bar |
| Next / Prev Weapon | ZR / ZL or X / Y (during gameplay) | Tap weapon slot in left rack (`AX`–`BF`, `RG`/`DG`) |
| Quick Select Weapon | — | Tap weapon slot in left quick-select rack |
| Scroll Dialogs, Terminals & Menus | C-Stick (Right Nub) or X / Y (during dialogs: X = Down, Y = Up) | Touch & drag text / scrollbar |
| Quick Use Items | — | Tap hotbar: `S.MED`, `L.MED`, `SOUL`, `BRSK`, `DOG` |
| Combat Turbo Toggle | — | Tap `[ TURBO ]` button above automap |
| Pan Automap | — | Touch & drag map viewport |
| Zoom Automap | — | Tap `[+]` or `[-]` |
| Recenter Automap | Select (recenter; tap again to cycle zoom) | Tap `[CTR]` |
| Passcode Entry | D-pad / A | Tap numpad `0`–`9`, `<-` (backspace), `OK` |
| In-Game Menu / Back | Start | Tap `[ MENU ]` on top bar (100ms hold) |
| 3D Depth Adjustment | Physical 3D Slider | — |

> **Note:** The D-pad and Circle Pad control forward/backward movement and turning; **L and R are dedicated strafe buttons** (step left/right without turning). The Automap is permanently displayed on the bottom screen.

---

## In-Game Settings Reference (Options Menu)

- **System Profile**: `[ Auto | N3DS (High) | O3DS (Perf) ]` — Controls default scaling and floor/ceiling texture settings.
- **Render Scaling**: `[ Crisp (400) | Retro (200) ]` — Selects native 400px top screen raycasting or 200px internal raycasting with GPU upscale.
- **Floor/Ceil Textures**: `[ on | off ]` — Toggles texture mapping on floors and ceilings (solid flat colors when off).
- **3D Depth**: `[ Low | Normal | High | Max ]` — Adjusts stereoscopic separation scale (3DS models only).
- **Music Volume**: `[ 0% - 100% ]` — Independent background music stream volume.
- **SFX Volume**: `[ 0% - 100% ]` — Independent sound effects mixer volume.
- **Touch Menu**: `[ on | off ]` — Enables or disables the bottom-screen `[ MENU ]` touch button.
- **Hold Fire**: `[ on | off ]` — Enables continuous attack buffering while holding `A` during combat.
- **Turbo Scope**: `[ Combat | Explore | All ]` — Determines which systems receive gameplay speed acceleration.
- **Typewriter**: `[ Classic | Fast | Instant ]` — Controls text display rate in dialogs and computer terminals.
- **Reset Defaults**: Restores default key bindings and configuration settings.

---

## Installation & Setup

1. Extract `doomrpg.bar` from `doomrpg.zip` at the `doomrpg_brew` item on [archive.org](https://archive.org/details/doomrpg_brew) using `BarToZip` into loose files.
2. Install `DoomRPG-1.1.0.cia` via FBI or copy `DoomRPG.3dsx` to `sdmc:/3ds/`.
3. Copy extracted game data files into `sdmc:/3ds/doomrpg/`:
   - `entities.db` and `help.txt`
   - Map files: `*.bsp`
   - UI and sprite textures: `*.bmp`
   - Binary tables: `wtexels.bin`, `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, `sintable.bin`
4. Copy audio files into `sdmc:/3ds/doomrpg/`:
   - Sound effects: numbered `.wav` files (`5042.wav` through `5138.wav`)
   - Music tracks: numbered `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`)
     - Recommended: Roland SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php) (`d_dead.mp3` $\to$ `5039.mp3`, `d_e1m1.mp3` $\to$ `5040.mp3`, `d_inter.mp3` $\to$ `5043.mp3`).
5. Launch from the HOME Menu or Homebrew Launcher.

---

## Detailed Commit Log

- `docs: add v1.1.0 playtesting and hardware verification checklist`
- `fix(io): only log fatal read error after exhausting all SD retries`
- `feat(particles): desynchronize rotation frame phase across gib particles`
- `feat(hud): replace flat 2px damage lines with 3-tier stepped red vignette`
- `feat(render): add sinusoidal weapon bobbing and sway during movement`
- `feat(particles): optimize circle rasterizer and re-enable blood and spark particles`
- `fix(gfx): default alpha to opaque (0xFF) in DoomRPG_setColor`
- `perf(sound): add direct O(1) lookup table for cached sound chunks`
- `perf(video): replace 96k-pixel manual loop with SDL_memset for bottom screen clear`
- `perf(render): simplify floor/ceiling span step math`
- `fix(types): eliminate pointer truncation and const-discard compiler warnings`
- `perf: cache and reuse bottom status bar surface across frames`
- `perf: optimize string formatting and eliminate buffer overflow warnings in Player`
- `build: enable -O3, -ffast-math, -fomit-frame-pointer, and -flto`
- `perf: hoist invariant animation calculation out of particle render loop`
- `perf: remove duplicate key resolution inside SDL event loop`
- `perf: yield CPU in main loop when game tick is not ready`
- `perf: replace division with bitshift for music volume scaling`
- `feat(ui): expand touchscreen weapon rack to 10 slots with dynamic collar and dog companion support`
- `fix(input): prevent weapon fire when opening doors and add hold-to-fire initial delay`
- `fix(video): fix 200px retro scaling by sampling full screen width without cropping`
- `fix(menu): restore stable centering under logo and balance submenu layouts`
- `fix(sound): prevent Sound_updateVolume from overwriting music volume display`
- `fix(config): synchronize audio settings and default volumes to 100%`
- `fix(script): ensure tile events resume post-dialog and advance NPC and trigger states`
- `feat(dialog): make tutorial NPC text and help controls device-specific for New 3DS vs Old 3DS`
- `feat(menu): center initial startup menu under logo and expand submenus to 320px width`
- `fix(dialog): dismiss message box on final message and prevent conversation auto-cycling`
- `refactor(audio): remove sound priority menu option and enable simultaneous multi-sample playback permanently`
- `fix(audio): connect sound priority menu option to 3DS audio playback`
- `fix(cia): remove unused camera and mic services to stay within 34-service ExHeader limit`
- `feat(ui): display real-time map exploration percentage and secret counter on automap`
- `feat(hardware): add interactive 3DS notification LED feedback for health, berserk, and secrets`
- `feat(ui): add touchscreen quick-select weapon bar on left side of bottom screen`
- `feat(save): add automatic checkpoint save on sector transitions`
- `feat(map): color-code locked keycard doors (red, blue, yellow) on automap`
- `feat(audio): add independent music and sound fx volume controls in options menu`
- `feat(input): add X and Y button scrolling for dialogs and computer terminals`
- `fix(ui): reduce spacing between item label and count on bottom HUD buttons`
- `fix(input): restrict interaction trigger to A and bind X/Y to weapon cycling`
- `feat(release): v1.1.0 universal binary, Old 3DS GPU scaling, widescreen notebook, HUD fixes, and Turbo 2.0`

---

## Verification & File Hashes

- `DoomRPG-1.1.0.cia` (1,079,232 bytes)
  - **MD5**: `6740d8c2567cc03455fe975c63f498cb`
  - **SHA256**: `7d3da3da1c0ee865ea240bb0d7066b2f04b14a8e59b936c669c8671969745419`
- `DoomRPG.3dsx` (1,074,956 bytes)
  - **MD5**: `57a14a9ab2b84865584cbd2298759b6f`
  - **SHA256**: `7ecf10396aaed8ad03670fad2de4dc7f8bcb2b03e4e54edcfa30297607735040`
- `DoomRPG.elf` (4,559,416 bytes)
  - **MD5**: `a76689a08050bb291b67b0de27522176`
  - **SHA256**: `84c999a9eff331973f73a73a7062e90daa4395b018a302be8685d1d6e4fe2df4`

---

## Credits & Attribution

- **Port & 3DS Enhancements**: Dennis Isaac Gutierrez Zeledon
- **AI Coding Partner**: Gemini Antigravity (Google DeepMind)
- **Soundtrack Research & Recommendation**: bakuDD (GBAtemp)
- **Original Reverse Engineering**: GEC Team (Erick194 and contributors)
- **Initial 3DS Port Base**: Efim Andreev (`efimandreev0`)
- **Doom RPG**: id Software / Fountainhead Entertainment
