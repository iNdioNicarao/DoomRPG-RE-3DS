# Doom RPG RE — 3DS v1.0.3

New 3DS home-menu build (`.cia`) and 3DSX package of Doom RPG RE, following v1.0.2. Source-only: no game assets or audio are bundled in the repository. They are supplied on the SD card (see *Install*).

- Build: 2026-09-02
- Version: 1.0.3
- Title ID: `000400000FD0BB01`
- License: GPLv3

---

## Overview

Version 1.0.3 is a major feature and performance release that takes full advantage of Nintendo 3DS hardware. It brings **hardware autostereoscopic 3D** to the top screen, an interactive **bottom-screen Touch HUD** with an in-game quick-inventory hotbar, a **touch keypad** for entering terminal passcodes, **Play Coins exchange**, **in-memory MP3 music streaming via libmad**, and **New 3DS 804 MHz CPU boost**.

---

## What Changed

| Area | Change |
|------|--------|
| **Autostereoscopic 3D** | Implemented a true dual-camera stereoscopic 3D rendering pipeline using Citro2D/Citro3D GPU targets. The depth effect is hardware-gated and dynamically scaled by the physical 3DS 3D slider (smoothly drops to single-eye 2D when the slider is off). Depth separation has been carefully tuned across world geometry, particles, and weapon parallax to eliminate ghosting and eye strain. Menus and 2D splash states automatically disable 3D rendering. |
| **Touch HUD & Hotbar** | Added authentic metallic steel status bars to the bottom screen matching the top HUD aesthetic. Features an interactive 5-slot quick-use hotbar (`S.MED`, `L.MED`, `SOUL`, `BRSK`, `DOG`) with generous 80px touch targets, plus touch `[ PASS ]` and `[ MENU ]` buttons. Font kerning is compensated for the 3DS 320px downscale with transparent font blitting over metallic textures. |
| **Interactive Touch Keypad** | Terminal passcode doors now display an authentic on-screen touch numpad (`0`–`9`, `C`, `OK`) on the bottom screen, allowing players to enter door codes naturally using the stylus or thumb. |
| **MP3 Music Streaming** | Rewrote the audio streaming engine using `libmad` to decode MP3 tracks (`5039.mp3`, `5040.mp3`, `5043.mp3`) directly from memory. Compressed MP3 files (1.5–4.1 MB) are cached into RAM once at track start, eliminating all SD card file I/O inside the real-time SDL audio callback thread and preventing 3DS FS session deadlocks. Supports seamless looping and full volume scaling from the in-game Options menu. |
| **Bottom Screen Dialogs** | Dialogue sequences, terminal readouts, and character interaction popups now render cleanly on the bottom screen (scaled 2x with 280px width) over the persistent automap, keeping the top 3D screen uncluttered. |
| **Navigation & Controls** | The **B button** now functions as Back / Return in all menus and dismisses dialog boxes. Circle Pad analog inputs are fully supported alongside the D-pad. Unified `hidScanInput` polling to eliminate dropped inputs when holding multiple buttons. |
| **3DS Play Coins Exchange** | Players can exchange Nintendo 3DS system Play Coins for game Credits (5 Play Coins for 20 Credits) at shops and vending machines. |
| **New 3DS Performance** | Enabled 804 MHz clock speed and L2 cache configuration via RSF for New 3DS systems. Defaulted floor and ceiling rendering to solid mode to avoid costly per-pixel textured projection, maintaining smooth framerates. |

---

## Detailed Commit Log (v1.0.3)

All 25 commits included in this release:

1. `fa04bfd` — `fix(audio): rewrite MusicStream to decode MP3 from RAM, eliminate FS thread contention, and support volume scaling`
2. `74963f2` — `feat: decrease 3D effect 10%, remove keycard badges on bottom bar, restore full CREDITS text, support B button back in menus`
3. `b5c46d3` — `fix(hud): make font surface transparent over metallic bars, move PASS to top bar, expand items to 80px`
4. `7d7b600` — `fix(font): compensate bottom-screen font kerning for 320/400 downscale and match top status bar styling`
5. `4675855` — `style(hud): style bottom screen with authentic metallic status bars and single-line font`
6. `3e53c7b` — `feat(hud): expand bottom touch HUD to show full 5-item quick-inventory hotbar and keycards`
7. `0a3cdc1` — `fix(stereo): disable 3D stereoscopic rendering during menus and 2D states`
8. `08daef7` — `feat(3ds): implement touch numpad, quick-touch HUD, Play Coins exchange, and stereoscopic particle depth for v1.0.3`
9. `09db37c` — `build: bump version to DOOMRPG-3DS v1.0.3`
10. `86c1e30` — `fix(stereo): correct weapon depth parallax sign and clamp offset to 1px to eliminate double vision`
11. `3df4737` — `fix(input, stereo): resolve missed inputs by consolidating hidScanInput, support simultaneous keys and Circle Pad, soften weapon 3D depth`
12. `c8f8205` — `ui: widen 2x dialog box to 280px for scrollbar clearance and enable New 3DS 804MHz speedup`
13. `4d8cb1c` — `ui: scale dialog box by 2x, ensure persistent automap rendering, and soften 3D stereo by 10%`
14. `8bb01b8` — `ui: render dialog over automap with 2px border, clean bottom-screen lifecycle, and fix 3D blit spillover`
15. `47518c6` — `ui: render dialog and terminal interaction popups on 3DS bottom screen`
16. `1b44ea3` — `video: implement true dual-camera stereoscopic rendering with Citro2D eye targets`
17. `4f685a8` — `video: align Citro2D top subtexture V coordinates to eliminate 16px vertical offset`
18. `deb7965` — `video: correct Morton swizzle row order for upright display and clean up Citro2D/Citro3D/gfx on shutdown`
19. `dc18105` — `video: fix top screen aspect ratio, orientation, and HUD clipping via unrotated subtexture UVs`
20. `7463850` — `3DS: real hardware auto-stereoscopic 3D on top screen (slider-gated)`
21. `73458a0` — `3DS: raw-gfx dual-screen present — upright, full, no flicker, no crash`
22. `4dc0321` — `3DS: remove duplicate DoomCanvas_drawAutomap definition`
23. `16fe6db` — `3DS: fix white strip at top of bottom screen`
24. `d236c3a` — `3DS: alias piDIB to framebuffer via SDL_CreateRGBSurfaceFrom`
25. `c1a50a5` — `Perf: default floor/ceiling to solid (skip per-pixel textured projection)`

---

## Install

1. Install `DoomRPG-1.0.3.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
2. On the SD card, copy the **extracted data files** into `sdmc:/3ds/doomrpg/` so they sit loose in that folder. Required files include the `*.bmp` sprites/UI, the `*.bsp` maps, `wtexels.bin`, `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, and `sintable.bin`.
3. Add audio next to the data, also under `sdmc:/3ds/doomrpg/`:
   - SFX as numbered `.wav` files (`5042.wav` through `5138.wav`).
   - Music as numbered `.mp3` files (`5039.mp3`, `5040.mp3`, `5043.mp3`).
4. Launch from the HOME Menu. `sdmc:/3ds/doomrpg/saves/` is created at runtime.

The game will not start unless `sdmc:/3ds/doomrpg/` exists with the data files present.

---

## Controls

| Action | Physical Button | Touch Screen |
|--------|-----------------|--------------|
| Move Forward / Backward | D-pad Up / Down or Circle Pad | — |
| Strafe Left / Right | L / R | — |
| Turn Left / Right | D-pad Left / Right or Circle Pad | — |
| Attack / Talk / Use / Confirm | A | On-screen dialog tap |
| Back / Dismiss Dialog / Pass Turn | B | Tap `[ PASS ]` on top bar |
| Next / Prev Weapon | ZR / ZL | — |
| Quick Use Items | Hotbar touch | Tap `S.MED`, `L.MED`, `SOUL`, `BRSK`, `DOG` |
| Passcode Entry | D-pad / A | Tap numpad digits `0`–`9`, `C`, `OK` |
| In-Game Menu / Back | Start | Tap `[ MENU ]` on top bar |
| Automap | Select (persistent on bottom screen) | — |
| 3D Depth Adjustment | Physical 3D Slider | — |

---

## Release Assets

- `DoomRPG-1.0.3.cia` — MD5 `666391b114fabfcda3df4c65ffee9aed`
- `DoomRPG.3dsx` — MD5 `711d1717c01734db86ee3f5c3d6f0a77`

---

## Notes & Compatibility

- Recommended for **New 3DS** (New 3DS, New 3DS XL, New 2DS XL) to take advantage of 804 MHz CPU clock speed and hardware stereo 3D.
- Autostereoscopic 3D operates on the top screen and is continuously adjusted by the 3DS hardware depth slider.
