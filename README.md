# DoomRPG-RE - Nintendo 3DS port

This is a Nintendo 3DS port of the [reverse engineered Doom RPG](https://github.com/Erick194/DoomRPG-RE) by [GEC]. All credits go to the GEC team, this project would not exist without their hard work.

This repository is a fork of [`efimandreev0/DoomRPG-RE-3DS`](https://github.com/efimandreev0/DoomRPG-RE-3DS) maintained by **Dennis Isaac Gutierrez Zeledon**. It provides installable HOME Menu `.cia` and Homebrew Launcher `.3dsx` builds, hardware autostereoscopic 3D with motion damping, an interactive bottom-screen Touch HUD with draggable automap, Turbo 2.0 combat and exploration speed toggles, a 3D parallax HOME banner, dynamic hardware profiling for Old 3DS / 2DS / New 3DS, and performance and stability optimizations. Complete details of changes are documented in [`docs/RELEASE_NOTES_v1.1.1.md`](docs/RELEASE_NOTES_v1.1.1.md).

> ### ⚠️ Notice for Upgrading Players
> If you are upgrading from an earlier version and your Circle Pad (left thumbstick) or other newly mapped controls feel unresponsive, your existing saved configuration file (`sdmc:/3ds/doomrpg/saves/Config`) may still contain the older binding table where secondary slots were unset (`-1`).
> 
> **To restore all default controls:**
> 1. In the in-game menu, navigate to **Options $\to$ Controls / Bindings $\to$ Reset Defaults**.
> 2. Alternatively, you can delete or rename `sdmc:/3ds/doomrpg/saves/Config` on your SD card. *(Your save games in `saves/` are completely separate and will NOT be lost).*
> 
> *Note: v1.1.1 includes an in-engine auto-migration that backfills missing Circle Pad bindings and applies Old 3DS / 2DS performance settings to legacy config files automatically.*

## Highlights (v1.1.1)

- **Aggressive Old 3DS / 2DS Performance Suite**:
  - Automatic config migration pass: enforces `Retro 200` internal scaling (200×240 raycasting with PICA200 GPU upscale) and flat solid floors and ceilings on Old 3DS / 2DS systems, bypassing stale config files that silently forced heavy 400×240 textured rendering.
  - Span rasterization halving: inner wall and sprite span loops step by 2 columns (`colStep = 2`), reducing CPU perspective division and texture sampling instructions by 50%.
  - Particle system flood protection: capped particles to max 8 on Old 3DS / 2DS with 150ms lifetime, and instant memory freeing upon combat completion.
  - Eliminated redundant 384 KB bottom-screen framebuffer clearing (`SDL_memset`) every frame.
  - Old 3DS audio mixing reduced to 22,050 Hz with 2:1 MP3 downsampling to alleviate single-core ARM11 load.
  - Disabled decorative sprite light glow passes on Old 3DS hardware.
- **Snappy Combat & True Turbo Mode 2.0**:
  - Slashing post-attack recovery wait in Turbo mode to 75ms (with instant skip on pressing `A`).
  - Quadrupled projectile travel velocity and instant off-screen monster tile snapping.
  - Decoupled turn animations from frame delays, making Turbo mode combat and exploration genuinely fast.
- **Priority Input Resolution & Accidental Menu Popup Fix**:
  - Resolved bug where holding or pressing `Right + A` or `Down + A` simultaneously (or in rapid overlapping succession) bitwise ORed sequential enum values into 23 (`AVK_SOFT1` $\rightarrow$ `MENUOPEN`).
  - Input system now resolves actions with strict priority: fresh button taps (`kDown`) always take precedence over held directions, combat/interact actions take precedence over movement, and base actions are strictly single-value.
  - Sanitized legacy phone softkey code in `keys_codeActions` to `0` for defense-in-depth.
- **Bottom-Screen Enemy / Kill Counter**:
  - Integrated `Player_fillMonsterStats()` into automap HUD badge: `MAP:%d%% | SEC:%s | KILLS:%s`.
  - Widened badge box to 318px ($X = 44, Y = 430, W = 318, H = 19$).
- **Weapons & Items UI Separation on Touch HUD**:
  - Horizontal bottom bar button 4 updated to `COLR` for using the Dog Collar item (`Player_useItem(29)`).
  - Left-side weapon rack slot 9 dedicated strictly to equipping the Dog companion weapon (`DG`).
  - Eliminates dual-function ambiguity so the vertical column remains strictly weapons and the horizontal bar remains strictly items.
- **Expanded 3-Second Notification LED Suite**:
  - Upgraded physical 3DS notification LED flash duration to 3.0 seconds across 17 distinct gameplay events (Secret found, Level Up, 4 Keycards, Soul Sphere, New Weapon, Game Saved, Dog Tamed, Play Coins exchanged, Player Death, Boss Defeated, Level Complete, Armor Broken, Capacity Full, Dog Strike, Critical Health strobe, Low Health pulse, Berserk pulse).

## Highlights (v1.1.0)

- **Universal Binary & Dynamic Hardware Profiling**: Single `.cia` and `.3dsx` binary for all 3DS and 2DS consoles. Boot detection queries `APT_CheckNew3DS` and `CFGU_GetSystemModel` to configure Old 3DS / 2DS vs New 3DS defaults automatically. Stereoscopic 3D rendering and left/right framebuffer setup are automatically bypassed on 2DS hardware (`consoleModel == 3`).
- **Rendering & Frame Pacing Optimizations**:
  - Optional 200×240 internal raycaster column rendering on Old 3DS / 2DS, halving raycast columns to fit within the ARM11 32 KB L1 data cache and upscaling to 400px via Citro2D / PICA200 hardware scaling with zero CPU overhead.
  - Span rasterizer simplification in `Render.c`: reduced DDA step arithmetic for floor and ceiling texturing.
  - Main loop thread yielding: executes `svcSleepThread(1000000)` (1ms) when the engine tick interval has not elapsed, reducing CPU idle spinning and power draw.
  - Fast bottom screen clearing via `SDL_memset` and a 400-entry lookup table (`s_lutBotH`) replacing per-pixel integer division.
  - Static persistent SDL surfaces (`s_topBarSurface`, `s_bottomBarSurface`) for HUD drawing, eliminating per-frame heap allocations.
  - Automap dirty blit caching: skips bottom-screen redraws when player position, orientation, and zoom remain unchanged.
  - Compiler optimization flags: `-O3 -ffast-math -fomit-frame-pointer -flto`.
- **Visuals & Particle Systems**:
  - Re-enabled blood and spark particle systems with an optimized circle rasterizer (`Video_fillCircle`).
  - Added sinusoidal weapon bobbing and sway during player movement in `DoomCanvas_renderWeapon`.
  - Replaced 2px damage lines with a 3-tier stepped red vignette during player damage (`Hud_drawDamageFlash`).
  - Desynchronized rotation phases across gib particles.
  - Initialized alpha channel to opaque (`0xFF`) in `DoomRPG_setColor`.
- **UI & Widescreen Layout**:
  - 360px widescreen Notebook layout (`menuHalfW = 180`, `scrollX = 378`), utilizing the full 400px top screen width instead of the legacy 128px container.
  - Double-pipe (`||`) delimiter parser joins mobile wrap segments into continuous sentences (up to 50 characters per line).
  - Bottom-screen 10-slot quick-select weapon rack ($X = 1..37$) supporting weapons `AX` through `BF`, plus dynamic Dog Collar / Hound progression (`RG` / `DG`).
  - Real-time automap statistics badge: walkable exploration percentage (`MAP: xx%`) and secret sector counter (`SEC: x/x`).
  - Color-coded locked keycard doors on automap (Red, Blue, Yellow, Green) parsed from script bytecode.
- **Audio Subsystem**:
  - Independent Music and SFX volume sliders (0–100%) in the Options menu with configuration persistence.
  - Direct $O(1)$ lookup table (`chunkLUT`) for cached WAV sound effects, eliminating linear search overhead.
  - Optimized music volume scaling via bitwise shifts (`>> 7`) instead of integer division.
  - Support for Roland SC-55 MP3 soundtrack replacements.
- **Controls & Input Modernization**:
  - Direct Circle Pad analog polling via `hidCircleRead` with deadzone handling.
  - C-Stick (Right Nub) smooth scrolling for dialogs, computer terminals, notebook logs, and menus.
  - Physical face button mapping for weapon switching: `X` (Next Weapon) and `Y` (Previous Weapon).
  - Contextual text scrolling: `X` (Scroll Down) and `Y` (Scroll Up) during dialogs and terminal screens.
  - New 3DS `ZL` / `ZR` buttons mapped to Previous / Next weapon.
  - Hold-to-fire attack buffering with 350ms initial press delay and interaction guard (prevents firing when opening doors).
  - Touchscreen menu button with 100ms hold debounce to prevent accidental activations.
  - Automatic configuration migration backfilling missing bindings into legacy config files.
- **Turbo Mode 2.0**:
  - Truncated post-attack wait from 333ms to 75ms in Turbo mode, with instant bypass on pressing `A`.
  - Quadrupled projectile travel velocity (`speed <<= 2`).
  - Off-screen monster AI instant tile snap for distant or non-visible tiles.
  - 2-frame exploration grid traversal (33ms at 60 FPS) and 2x door opening animation speed.
  - Typewriter terminal text accelerated to 5ms/char with instant page reveal on pressing `A` or touching the screen.
- **Memory & Stability**:
  - Resolved 60 FPS RAM leak in `Hud_drawTopBar` by eliminating un-freed temporary surfaces.
  - Implemented 3.0-second watchdog ceiling to prevent queued HUD messages from freezing.
  - Added NULL check after `SDL_malloc` in `Render_loadBitShapes` to prevent ARM11 data aborts on allocation failure.
  - Added SD card read retry logic before logging fatal I/O read errors in `DoomRPG.c`.
  - Automatic checkpoint state save on sector transitions.

## How to install

1. Get the original game data: the game reads its assets as **loose files** from
   `sdmc:/3ds/doomrpg/` — the 3DS build reads files directly from that folder. The data
   lives inside the `doomrpg.bar` container, which is bundled in the
   `doomrpg.zip` archive at the `doomrpg_brew` item on
   [archive.org](https://archive.org/details/doomrpg_brew). Extract `doomrpg.bar`
   (a BREW asset container; upstream PC tools such as `BarToZip` turn it into
   loose files) and copy those loose files — no further zip step is needed.
2. Install `DoomRPG-1.1.1.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
3. On the SD card, copy the **extracted data files** into
   `sdmc:/3ds/doomrpg/` so they sit loose in that folder. Required files include:
   - `entities.db` and `help.txt`
   - Game maps: `*.bsp`
   - UI and sprites: `*.bmp`
   - Engine data tables: `wtexels.bin`, `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, and `sintable.bin`.
4. Add audio next to the data, also under `sdmc:/3ds/doomrpg/`:
   - SFX as numbered `.wav` files (`5042.wav` through `5138.wav`).
   - Music as numbered `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`):
     > **High-Quality Roland SC-55 Music (Recommended):**
     > The original BREW extractions provide low-bitrate downmixed MIDI files. As identified by GBAtemp user **bakuDD**, these tracks correspond directly to original Doom and Doom II music. For higher fidelity, download the Roland Sound Canvas SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php) and place them as:
     > - `5039.mp3` $\leftarrow$ `d_dead.mp3` from Doom II ("The Demon's Dead" / "Waiting for Romero to Play")
     > - `5040.mp3` $\leftarrow$ `d_e1m1.mp3` from Doom ("At Doom's Gate")
     > - `5043.mp3` $\leftarrow$ `d_inter.mp3` from Doom ("Sweet Little Dead Bunny" / Intermission)
5. Launch from the HOME Menu or Homebrew Menu. Save files will automatically be created under `sdmc:/3ds/doomrpg/saves/`.

The game will not start unless `sdmc:/3ds/doomrpg/` exists with the data files present.

## Controls (3DS Family)

| Action | Physical Button | Touch Screen |
|--------|-----------------|--------------|
| Move Forward / Backward | D-pad Up / Down or Circle Pad (Left Stick) | — |
| Strafe Left / Right | L / R | — |
| Turn Left / Right | D-pad Left / Right or Circle Pad (Left Stick) | — |
| Attack / Talk / Use / Confirm | A (Hold for Attack Buffering) | Tap dialog text |
| Back / Dismiss Dialog / Pass Turn | B | Tap `[ PASS ]` on top bar |
| Next / Prev Weapon | ZR / ZL or X / Y (during gameplay) | Tap weapon slot in left rack (`AX`–`BF`, `DG`) |
| Quick Select Weapon | — | Tap weapon slot in left quick-select rack |
| Scroll Dialogs, Terminals & Menus | C-Stick (Right Nub) or X / Y (during dialogs: X = Down, Y = Up) | Touch & drag text / scrollbar |
| Quick Use Items | — | Tap hotbar: `S.MED`, `L.MED`, `SOUL`, `BRSK`, `COLR` |
| Combat Turbo Toggle | — | Tap `[ TURBO ]` button above automap |
| Pan Automap | — | Touch & drag map viewport |
| Zoom Automap | — | Tap `[+]` or `[-]` |
| Recenter Automap | Select (recenter; tap again to cycle zoom) | Tap `[CTR]` |
| Passcode Entry | D-pad / A | Tap numpad `0`–`9`, `<-` (backspace), `OK` |
| In-Game Menu / Back | Start | Tap `[ MENU ]` on top bar (100ms hold) |
| 3D Depth Adjustment | Physical 3D Slider | — |

> **Note:** The D-pad and Circle Pad control forward/backward movement and turning; **L and R are dedicated strafe buttons** (step left/right without turning). The Automap is permanently displayed on the bottom screen.

## In-Game Settings (Options Menu)

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

## Save and config data

All user data is stored in `sdmc:/3ds/doomrpg/saves` — these files are compatible with the PC release.

## Building from source

This port uses SDL1.2 and SDL_Mixer (1.2). The reproducible build runs inside a Docker image with the devkitARM toolchain (makerom + bannertool); the CIA recipe lives under `tools/cia/`.

```bash
cd tools/cia
./build_cia.sh
```

This produces `tools/cia/DoomRPG-<version>.cia` and a `DoomRPG.3dsx`. The `VERSION` file controls the CIA filename; the Title ID / version byte comes from `DoomRPG-3DS.rsf` and is left at upstream's value.

To build manually instead:

1. Install [DevkitPro](https://devkitpro.org/) and the 3DS-dev package; ensure `DEVKITPRO` is set.
2. Install SDL1.2 and SDL_Mixer via pacman (`dkp-pacman -S 3ds-sdl 3ds-sdl_mixer`).
3. `git clone <this-repo> && cd <this-repo>`
4. `mkdir build && cd build`
5. `cmake .. -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake`
6. `make`

## Bug reporting

If you find a bug, please open an issue here on GitHub.

## Acknowledgments

This fork was developed by **Dennis Isaac Gutierrez Zeledon** with the assistance of AI coding assistants across its development milestones:

- **v1.1.1 (Aggressive Old 3DS/2DS Performance Suite, Input Priority Fix, Enemy Counter & LED Suite):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of aggressive Old 3DS / 2DS performance overhaul (config auto-migration, 50% wall and sprite span rasterizer step halving, particle cap to 8 with 150ms lifetime, 22,050 Hz audio mixing with 2:1 MP3 downsampling, elimination of redundant 384 KB per-frame memset), priority input resolution eliminating accidental menu popups on simultaneous `Right + A` / `Down + A`, bottom-screen automap enemy kill counter integration with full-clear gold illumination, and 3-second colored notification LED alert expansion across 17 gameplay events.

- **v1.1.0 (Universal Binary, Old 3DS GPU Scaling, Widescreen Notebook, Turbo 2.0 & Stability Suite):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of universal single binary with runtime console detection (`APT_CheckNew3DS`), internal 200×240 raycaster scaling with Citro2D PICA200 GPU scaling, 360px widescreen notebook with pipe-delimited note merging, elimination of HUD 60 FPS RAM leak and implementation of persistent static HUD surfaces and watchdog timer, Turbo Mode 2.0 gameplay acceleration suite (truncated post-attack wait, hold-to-fire attack buffering, 4x missile speed, off-screen monster AI snap, 2-frame exploration grid traversal, fast door animations, typewriter acceleration), direct Circle Pad analog polling via `hidCircleRead`, C-Stick scrolling, touchscreen debounce, floor/ceiling span step optimizations, $O(1)$ sound chunk lookup table, and particle circle rasterizer optimizations.

- **v1.0.3 – v1.0.7 (Stereo 3D, Motion Damping, 3D Banner, Touch HUD & Engine Polish):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of hardware autostereoscopic 3D, Citro2D/Citro3D stereo targets, dynamic motion-damped stereoscopy, 3D parallax diorama banner and stereo CWAV stinger, pixel-perfect 1:1 dialogue typography and word-wrapping, Touch HUD hotbar and draggable automap, passcode keypad, Play Coins exchange, in-memory `libmad` MP3 audio streaming, combat turbo button, and Circle Pad / save directory fixes.

- **v1.0.0 – v1.0.2 (Initial Port & CIA Packaging):**
  - **Assistant:** **Hermes Agent** (Nous Research)
  - **Model:** `tencent/hy3:free`
  - **Role:** Tracing the initial 3DS boot/render path, resolving early heap-exhaustion and null-surface crashes, establishing the New 3DS home-menu `.cia` build pipeline, and initial repository setup.

- **Community Contributors & Research:**
  - **CrashMidnick (GBAtemp)**: Invaluable community testing, file checklist verification, and input bug reporting.
  - **bakuDD (GBAtemp)**: Research identifying that downmixed mobile BREW MIDI tracks correspond directly to original Doom and Doom II songs, and recommending the Roland Sound Canvas SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php).

## License

GNU General Public License v3.0. Doom RPG game data is the property of its
respective owners and is **not** included in this repository.
