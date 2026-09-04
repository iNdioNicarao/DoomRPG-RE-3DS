# DoomRPG-RE - Nintendo 3DS port

This is a Nintendo 3DS port of the [reverse engineered Doom RPG](https://github.com/Erick194/DoomRPG-RE) by [GEC]. All credits go to the GEC team, this project would not exist without their hard work.

This repository is a fork of [`efimandreev0/DoomRPG-RE-3DS`](https://github.com/efimandreev0/DoomRPG-RE-3DS) created by **Dennis Isaac Gutierrez Zeledon** to provide a **home-menu `.cia` build** (installable on a New 3DS from the HOME Menu via FBI), **hardware autostereoscopic 3D with dynamic motion damping**, an interactive **bottom-screen Touch HUD with draggable automap and combat turbo**, a custom **3D parallax diorama HOME banner**, and stability fixes that make the port shine on real hardware. Recent enhancements are detailed in [`docs/RELEASE_NOTES_v1.0.6.md`](docs/RELEASE_NOTES_v1.0.6.md).

## Highlights (v1.0.6)

- **Combat Turbo Button**: Dedicated bold green `[ TURBO ]` toggle button on the bottom-screen map control bar (adjacent to `[ CTR ]`). Accelerates enemy turns and combat pauses by ~3×, doubles missile flight speed, and accelerates attack animations for snappy turn-based encounters.
- **Stereoscopic 3D Combat Particles**: All combat particle effects (blood splatters, sparks, monster gibs, weapon explosions) render to both left and right stereo buffers with depth-matched parallax offset, completely eliminating single-eye retinal flicker and adding full stereoscopic depth.
- **In-Game 3D Depth Multiplier**: Seamlessly modulate hardware 3D stereoscopic depth between `Low 0.7x`, `Normal 1.0x` (default), `High 1.4x`, and `Max 1.8x` directly from the in-game Pause Menu (`MENU_INGAME`) or *Video Options* (`MENU_VIDEO`).
- **Real-Time Texture Filtering Toggle**: Toggle Citro3D texture filtering between `Crisp` (`GPU_NEAREST` — authentic pixel art) and `Smooth` (`GPU_LINEAR` — bilinear filtered scaling) on the fly in the Pause Menu and Video Options.
- **2x Scaled Cutscenes & Story Presentation**: High-definition 2x scaling for legal splash screens, teaser graphics (`g.bmp`), intro story cutscene (text pages, animated spaceship flyby, speed lines, planet layers, red tracking reticle), epilogue, scrolling credits, and error screens.
- **Stereoscopic 3D Rotating Title Menu**: Full autostereoscopic 3D depth applied to the rotating title menu background with the DOOM RPG logo, menu items, cursor, and prompts floating cleanly at screen depth (zero parallax).
- **Interactive Bottom-Screen Automap**: Smooth real-time player centering and tracking as you walk. Touch-and-drag pan across the map, with dedicated on-screen Zoom In `[+]`, Zoom Out `[-]`, and Recenter `[O]` touch buttons.
- **Full Metallic Quick-Access Bar**: Seamless metallic texture extended across the entire bottom screen down to the bezel (row 479) with clean beveled dividers.
- **Hotbar Feedback & Dimming**: Empty hotbar slots (count = 0) are visually dimmed by 50%. Tapping an empty slot triggers a red highlight flash, a distinct negative audio cue, and a top-screen notification banner.
- **Default-Enabled Textured Floors & Ceilings**: Authentic textured floors, ceilings, and ceiling lights enabled out-of-the-box with smooth performance.
- **Synchronized 3D Motion Recovery**: Depth recovery dynamically synchronizes with grid movement completion to eliminate settling lag.
- **Horizontally Centered HUD Messages**: Top-screen status messages centered for natural eye focus.
- **Lid-Close Audio Suspension & Power Management**: Clam-shell closure immediately mutes audio and enters low-power sleep; reopening resumes audio and visuals gracefully.
- **3D Parallax Diorama HOME Menu Banner & Audio Stinger**: Custom 3DS extended banner featuring a rotating 2-sided plaque (Mars Base diorama on front, brushed steel with embossed horned demon skull on back), matched pixel-art icon, and dual shotgun blast + grunt death scream audio stinger.
- **Pixel-Perfect 1:1 Dialogue Typography**: Proportional kerning and 5/2 virtual scaling for mathematically square ($2\times 2$ pixel) glyphs on the physical LCD.

## How to install

1. Get the original game data: the game reads its assets as **loose files** from
   `sdmc:/3ds/doomrpg/` — the 3DS build never opens a zip (the loader code paths
   are zip-named but actually read files directly from that folder). The data
   lives inside the `doomrpg.bar` container, which is bundled in the
   `doomrpg.zip` archive at the `doomrpg_brew` item on
   [archive.org](https://archive.org/details/doomrpg_brew). Extract `doomrpg.bar`
   (a BREW asset container; the upstream PC tools such as `BarToZip` turn it into
   loose files) and copy those loose files — no further zip step is needed.
2. Install `DoomRPG-1.0.6.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
3. On the SD card, copy the **extracted data files** into
   `sdmc:/3ds/doomrpg/` so they sit loose in that folder. Required files
   include the `*.bmp` sprites/UI, the `*.bsp` maps, `wtexels.bin`,
   `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, and
   `sintable.bin`.
4. Add audio next to the data, also under `sdmc:/3ds/doomrpg/`:
   - SFX as numbered `.wav` files (`5042.wav` through `5138.wav`).
   - Music as numbered `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`).
5. Launch from the HOME Menu. `sdmc:/3ds/doomrpg/saves/` is created at runtime.

The game will not start unless `sdmc:/3ds/doomrpg/` exists with the data files present.

## Controls (New 3DS)

| Action | Physical Button | Touch Screen |
|--------|-----------------|--------------|
| Move Forward / Backward | D-pad Up / Down or Circle Pad | — |
| Strafe Left / Right | L / R | — |
| Turn Left / Right | D-pad Left / Right or Circle Pad | — |
| Attack / Talk / Use / Confirm | A | On-screen dialog tap |
| Back / Dismiss Dialog / Pass Turn | B | Tap `[ PASS ]` on top bar |
| Next / Prev Weapon | ZR / ZL | — |
| Quick Use Items | Hotbar touch | Tap `S.MED`, `L.MED`, `SOUL`, `BRSK`, `DOG` |
| Combat Turbo Toggle | — | Tap `[ TURBO ]` next to map controls |
| Pan Automap | — | Touch & drag map with stylus / finger |
| Zoom Automap | — | Tap `[+]` or `[-]` |
| Recenter Automap | — | Tap `[O]` or tap player arrow |
| Passcode Entry | D-pad / A | Tap numpad digits `0`–`9`, `C`, `OK` |
| In-Game Menu / Back | Start | Tap `[ MENU ]` on top bar |
| Automap | Select (persistent on bottom screen) | — |
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

- **v1.0.3 – v1.0.4 (Stereo 3D, Motion Damping, 3D Banner & Touch UI):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of hardware autostereoscopic 3D, Citro2D/Citro3D stereo targets, dynamic motion-damped stereoscopy, 3D parallax diorama banner and stereo CWAV stinger, pixel-perfect 1:1 dialogue typography and word-wrapping, Touch HUD hotbar, passcode keypad, Play Coins exchange, and in-memory `libmad` MP3 audio streaming.

- **v1.0.0 – v1.0.2 (Initial Port & CIA Packaging):**
  - **Assistant:** **Hermes Agent** (Nous Research)
  - **Model:** `tencent/hy3:free`
  - **Role:** Tracing the initial 3DS boot/render path, resolving early heap-exhaustion and null-surface crashes, establishing the New 3DS home-menu `.cia` build pipeline, and initial repository setup.

## License

GNU General Public License v3.0. Doom RPG game data is the property of its
respective owners and is **not** included in this repository.
