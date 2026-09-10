# DoomRPG-RE - Nintendo 3DS port

This is a Nintendo 3DS port of the [reverse engineered Doom RPG](https://github.com/Erick194/DoomRPG-RE) by [GEC]. All credits go to the GEC team, this project would not exist without their hard work.

This repository is a fork of [`efimandreev0/DoomRPG-RE-3DS`](https://github.com/efimandreev0/DoomRPG-RE-3DS) created by **Dennis Isaac Gutierrez Zeledon** to provide a **home-menu `.cia` build** (installable on any 3DS/2DS model from the HOME Menu via FBI), **hardware autostereoscopic 3D with dynamic motion damping**, an interactive **bottom-screen Touch HUD with draggable automap and Turbo 2.0**, a custom **3D parallax diorama HOME banner**, dynamic hardware adaptation for Old 3DS / 2DS, and stability fixes that make the port shine on real hardware. Recent enhancements are detailed in [`docs/RELEASE_NOTES_v1.1.0.md`](docs/RELEASE_NOTES_v1.1.0.md).

> ### ⚠️ Notice for Upgrading Players (From v1.0.0 – v1.0.7)
> If you are upgrading from an earlier version and your Circle Pad (left thumbstick) or other newly mapped controls feel unresponsive, your existing saved configuration file (`sdmc:/3ds/doomrpg/saves/Config`) may still contain the older binding table where secondary slots were unset (`-1`).
> 
> **To restore all default controls:**
> 1. In the in-game menu, navigate to **Options $\to$ Controls / Bindings $\to$ Reset Defaults**.
> 2. Alternatively, you can delete or rename `sdmc:/3ds/doomrpg/saves/Config` on your SD card. *(Your save games in `saves/` are completely separate and will NOT be lost).*
> 
> *Note: v1.1.0 includes an in-engine auto-migration that backfills missing Circle Pad bindings into legacy config files automatically, but manual reset is documented as the recommended first troubleshooting step.*

## Highlights (v1.1.0)

- **Universal Single Binary & Dynamic Hardware Profiles**: A single unified `.cia` and `.3dsx` release boots on all 3DS and 2DS consoles. Automatically probes console capabilities (`APT_CheckNew3DS`, `CFGU_GetSystemModel`) at boot, dynamically applying Old 3DS / 2DS vs New 3DS profiles without user intervention. Stereo 3D is automatically bypassed on Nintendo 2DS hardware.
- **Old 3DS & 2DS Performance Suite**: Resolves CPU raycasting bottlenecks on legacy 268 MHz hardware. Scales internal software raycasting to 200 columns (saving 50% CPU raycasting and fitting inside the 32 KB L1 data cache), with zero-overhead PICA200 GPU upscaling to the 400px top screen via Citro2D. Combines with flat floor/ceiling presets, a 400-entry bottom-screen LUT (saving 96,000 divisions/frame), and automap dirty blit caching for locked 60 FPS gameplay.
- **360px Widescreen Notebook**: Replaced the legacy 128px flip-phone container (`menuHalfW = 64`) with a widescreen 360px container (`menuHalfW = 180`), utilizing the entire 400px top screen. Mission notes and terminal messages are parsed across double-pipe entries (`||`), joining mobile wrap segments into natural sentences and wrapping cleanly up to 48–50 characters per line with metallic headers and dividers.
- **HUD Memory Leak & Failsafe Watchdog Resolution**: Eliminated the 60 FPS RAM leak in `Hud_drawTopBar` which previously leaked ~1.92 MB/s on message expiration, causing black screens and heap exhaustion after saving. Allocated persistent static HUD surfaces (`s_topBarSurface`, `s_bottomBarSurface`), eliminating 120 dynamic heap allocations per second. Added a 3.0-second watchdog ceiling to prevent queue deadlocks from freezing notifications, and added OOM safety checks in BitShapes loader.
- **Turbo Mode 2.0 & Gameplay Acceleration Suite**:
  - Truncated post-attack tail wait from 333ms to 75ms in Turbo mode, with instant bypass on pressing `A` or `R`.
  - Continuous "Hold-to-Fire" attack buffering: holding `A` or `R` automatically executes attacks the exact frame the player's turn opens.
  - Quadrupled projectile travel velocity (4x missile speed, `speed <<= 2`) crossing rooms in 3–4 frames while preserving full 3D rendering, smoke trails, and explosions.
  - Off-screen monster AI instant snap for distant or non-visible monster patrol turns.
  - 2-frame exploration grid traversal (33ms at 60 FPS) and 2x fast door opening in Turbo exploration mode.
  - Accelerated typewriter terminal text (5ms/char) with instant page reveal on tapping `A` or the touchscreen.
- **Direct Analog Circle Pad Polling**: Polled directly via `hidCircleRead` with deadzones, guaranteeing Circle Pad thumbstick movement regardless of save file state.
- **Right Nub (C-Stick) & X/Y Face Button Scrolling**: Smooth scrolling using the C-Stick (New 3DS) or dedicated **X (Scroll Down)** and **Y (Scroll Up)** face buttons during dialogs and computer terminals, giving Old 3DS and 2DS players without a right nub identical hardware scrolling capabilities.
- **Terminal Password Auto-Scroll & Passcode Pinning**: In `ST_DIALOGPASSWORD`, the text automatically scrolls to reveal the prompt line, and the code input box remains pinned and visible at all times.
- **Touchscreen Menu Debounce & Phantom Rejection**: Implemented touch-up release verification, 100ms hold debounce, bezel corner inset ($X = 345..394, Y = 244..260$), and an optional menu toggle to eliminate accidental pause menu popups from hand grip or bezel flex.
- **New In-Game Video & Input Options**: Dedicated menu toggles for System Profiles (`Auto`, `High`, `Perf`), Render Scaling (`Crisp 400`, `Retro 200`), Floor/Ceiling textures, 3D Depth, Touch Menu, Hold Fire, Turbo Scope (`Combat`, `Explore`, `All`), Typewriter speed, and Reset Defaults.

## How to install

1. Get the original game data: the game reads its assets as **loose files** from
   `sdmc:/3ds/doomrpg/` — the 3DS build never opens a zip (the loader code paths
   are zip-named but actually read files directly from that folder). The data
   lives inside the `doomrpg.bar` container, which is bundled in the
   `doomrpg.zip` archive at the `doomrpg_brew` item on
   [archive.org](https://archive.org/details/doomrpg_brew). Extract `doomrpg.bar`
   (a BREW asset container; the upstream PC tools such as `BarToZip` turn it into
   loose files) and copy those loose files — no further zip step is needed.
2. Install `DoomRPG-1.1.0.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
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
     > The original BREW extractions only provide low-bitrate downmixed MIDI files. As identified by GBAtemp user **bakuDD**, these tracks are the exact same compositions as the original Doom and Doom II music. For authentic, high-quality audio, download the Roland Sound Canvas SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php) and place them as:
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
| Attack / Talk / Use / Confirm | A (Hold for Attack Buffering) | On-screen dialog tap |
| Back / Dismiss Dialog / Pass Turn | B | Tap `[ PASS ]` on top bar |
| Next / Prev Weapon | ZR / ZL or X / Y (during gameplay) | — |
| Scroll Dialogs, Terminals & Menus | C-Stick (Right Nub) or X / Y (during dialogs: X = Down, Y = Up) | Touch & drag dialog text or scrollbar |
| Quick Use Items | Hotbar touch | Tap `S.MED`, `L.MED`, `SOUL`, `BRSK`, `DOG` |
| Combat Turbo Toggle | — | Tap `[ TURBO ]` next to map controls |
| Pan Automap | — | Touch & drag map with stylus / finger |
| Zoom Automap | — | Tap `[+]` or `[-]` |
| Recenter / Zoom Automap | Select (recenter; tap again to cycle zoom) | Tap `[CTR]` or `[+]` / `[-]` |
| Passcode Entry | D-pad / A | Tap numpad digits `0`–`9`, `C`, `OK` |
| In-Game Menu / Back | Start | Tap `[ MENU ]` on top bar (100ms hold) |
| 3D Depth Adjustment | Physical 3D Slider | — |

NOTE: the D-pad/Circle Pad turns; **L/R are lateral movement (strafe)** — hold L or R to
step left/right without changing facing. Automap is always on your bottom screen.

## Save and config data

All user data is stored in `sdmc:/3ds/doomrpg/saves` — these files are compatible with the PC release.

## Building from source

This port uses SDL1.2 and SDL_Mixer (1.2). The reproducible build runs inside a Docker image with the devkitARM toolchain (makerom + bannertool); the CIA recipe lives under `tools/cia/`.

```
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

- **v1.1.0 (Universal Binary, Old 3DS GPU Scaling, Widescreen Notebook, Turbo 2.0 & Stability Suite):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Architectural design and implementation of universal single binary with runtime console detection (`APT_CheckNew3DS`), internal 200×240 raycaster scaling with zero-overhead PICA200 GPU scaling, 360px widescreen notebook with pipe-delimited note merging, elimination of HUD 60 FPS RAM leak and implementation of persistent static HUD surfaces and watchdog timer, Turbo Mode 2.0 gameplay acceleration suite (truncated post-attack wait, hold-to-fire attack buffering, 4x missile speed, off-screen monster AI snap, 2-frame exploration grid traversal, fast door animations, typewriter acceleration), direct Circle Pad analog polling via `hidCircleRead`, C-Stick scrolling, and touchscreen debounce.

- **v1.0.3 – v1.0.7 (Stereo 3D, Motion Damping, 3D Banner, Touch HUD & Engine Polish):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of hardware autostereoscopic 3D, Citro2D/Citro3D stereo targets, dynamic motion-damped stereoscopy, 3D parallax diorama banner and stereo CWAV stinger, pixel-perfect 1:1 dialogue typography and word-wrapping, Touch HUD hotbar and draggable automap, passcode keypad, Play Coins exchange, in-memory `libmad` MP3 audio streaming, combat turbo button, and Circle Pad / save directory fixes.

- **v1.0.0 – v1.0.2 (Initial Port & CIA Packaging):**
  - **Assistant:** **Hermes Agent** (Nous Research)
  - **Model:** `tencent/hy3:free`
  - **Role:** Tracing the initial 3DS boot/render path, resolving early heap-exhaustion and null-surface crashes, establishing the New 3DS home-menu `.cia` build pipeline, and initial repository setup.

- **Community Contributors & Research:**
  - **bakuDD (GBAtemp)**: Research identifying that the downmixed mobile BREW MIDI tracks match original Doom and Doom II songs, and recommending the authentic Roland Sound Canvas SC-55 recordings from [sc55.duke4.net](https://sc55.duke4.net/games.php).

## License

GNU General Public License v3.0. Doom RPG game data is the property of its
respective owners and is **not** included in this repository.
