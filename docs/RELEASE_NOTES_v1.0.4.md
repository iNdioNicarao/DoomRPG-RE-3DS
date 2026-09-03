# Doom RPG RE — 3DS v1.0.4

New 3DS home-menu build (`.cia`) and 3DSX package of Doom RPG RE, following v1.0.3. Source-only: no game assets or audio are bundled in the repository. They are supplied on the SD card (see *Install*).

- **Build**: 2026-09-03
- **Version**: 1.0.4
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Developer Attribution**: Dennis Isaac Gutierrez Zeledon

---

## Overview

Version 1.0.4 is a major audiovisual, stereoscopic, and UI polish release for Nintendo 3DS. It introduces a custom **3D parallax diorama HOME Menu banner** with a 2-channel stereo audio stinger, **Dynamic Motion-Damped Stereoscopy** to eliminate visual disorientation during movement and turns, a **pixel-perfect 1:1 square aspect ratio** and **proportional kerning** for bottom-screen dialogue, and **automatic word-wrapping** that prevents text from bleeding out of boxes or behind scrollbars.

---

## Highlights (v1.0.4)

- **3D Parallax Diorama HOME Menu Banner & Audio Stinger**:
  - Fully custom 3DS extended banner featuring a 2-sided rotating plaque: the front displays the high-detail Mars Base Research Facility diorama, while the back features an authentic dark brushed steel plate with corner rivets, metallic bevel, and an embossed horned demon skull in the center.
  - Floating Doom RPG foreground logo with depth blending.
  - Sized and framed to remain completely within the screen boundaries across all angles of rotation.
  - Pixel-matched HOME Menu icon.
  - Embedded 22,050 Hz 2-channel stereo CWAV audio stinger playing two shotgun blasts followed by the classic Doom grunt death scream upon application selection.
- **Dynamic Motion-Damped Stereoscopy**:
  - Eliminates visual fatigue and retinal shear in autostereoscopic 3D mode.
  - **During Camera Rotation (Turning)**: Dials down 3D eye separation by **70%** (0.30 strength) to eliminate lateral parallax shearing.
  - **During Grid Movement (Stepping)**: Dials down eye separation by **55%** (0.45 strength) to soften the step depth pop while preserving forward depth cues.
  - **Asymmetric Easing**: Rapidly softens depth in 2–3 frames when motion begins, and smoothly blooms back to 100% full slider depth over ~150ms once the player settles onto a tile or finishes a turn.
  - **Stationary**: 100% full, rich 3D autostereoscopy when standing still, exploring the view, in combat, or navigating menus.
- **Pixel-Perfect 1:1 Physical Aspect Ratio for Dialogue**:
  - Compensates for the 3DS bottom screen display driver's $4/5$ horizontal downscale ($400 \to 320$) using a $5/2$ virtual $X$ mapping: $\frac{5}{2} \times \frac{4}{5} = \mathbf{2.0\text{ physical pixels wide}}$, exactly matching the $2\times$ vertical scaling ($\mathbf{2.0\text{ physical pixels tall}}$).
  - Eliminates the tall, skinny font distortion; dialogue glyphs now render with mathematically square ($2\times 2$ pixel) aspect ratio on the physical LCD.
- **Proportional Kerning & Typography**:
  - Built a per-glyph bounding box metrics table (`s_dialogFontMetrics`) for all 96 characters in the font bitmap.
  - Eliminates excessive empty padding around narrow glyphs (`:`, `!`, `l`, `1`, `.`, `,`); strings like `Level: 3`, `Max Health: +3`, and `Max Armor: +4` are snug and naturally spaced.
- **Automatic Word-Wrapping & Text Overflow Protection**:
  - Dialogue text now measures line widths against the physical container and automatically breaks at word boundaries before reaching the scrollbar threshold (max 117 font pixels).
  - Guarantees text will never bleed out of the dialogue box or render behind the vertical scrollbar.
  - Sized the dialogue box to $330\text{px}$ virtual width ($264\text{px}$ physical width, perfectly centered with $28\text{px}$ automap margins on each side of the bottom screen) for comfortable breathing room.
  - Fixed Level Up notification header by removing menu divider padding so the title starts neatly at the left margin.
- **UI & Menu Alignment Fixes**:
  - Centered startup and pause menus with consistent left-justified text layout.
  - Refined Nintendo 3DS Play Coins store exchange (5 Play Coins for 20 Credits) and corrected shared extdata archive ID permissions.

---

## Detailed Commit Log (v1.0.4)

All 15 commits included in this release since v1.0.3:

1. `e7d6120` — `fix(dialog): fix bottom screen font kerning, 1:1 square pixel aspect ratio, word-wrapping, and level up header`
2. `1e9c33e` — `feat(3d): implement dynamic motion-damped stereoscopy during player movement and rotation`
3. `2e46414` — `fix(banner): shrink background by another 15% and scale down demon skull by 25%`
4. `ba6a294` — `feat(banner): add brushed steel back with centered horned demon skull on 40% shrunk background`
5. `4909b21` — `fix(banner): shrink background by 40% from 8eb1a45 baseline`
6. `8eb1a45` — `fix(banner): compile 2-channel stereo CWAV for 3DS HOME Menu audio`
7. `dd9e372` — `feat(banner): add two shotgun blasts and grunt death scream, match home icon`
8. `f760f12` — `fix(ui): center startup and pause menus, add level separator, and fix coins overlap`
9. `53e2cbf` — `build: bump version to DOOMRPG-3DS v1.0.4`
10. `bd1c30e` — `chore(clean): remove external project references and ignore plan/scratch files`
11. `7156498` — `fix(menu): widen container to 280px, left-justify text, and update play coins exchange to 5 for 20`
12. `d992004` — `feat(banner): implement 3D parallax diorama banner, custom audio stinger, and 3DS icon`
13. `1f93615` — `fix(store): execute Play Coins credit exchange and preserve bottom screen HUD during menus`
14. `a532027` — `fix(3ds): correct shared extdata archive ID and add RSF SystemSaveDataId1 for Play Coins`
15. `499a61a` — `docs: update AI assistant acknowledgments for v1.0.3+ (Gemini Antigravity)`

---

## Install

1. Install `DoomRPG-1.0.4.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
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

- `DoomRPG-1.0.4.cia` — MD5 `4280c884f4000a119f297e0057c836ce`
- `DoomRPG.3dsx` — MD5 `1990844a69888467904506a3c66d9765`

---

## Acknowledgments

This fork was developed by **Dennis Isaac Gutierrez Zeledon** with the assistance of AI coding assistants across its development milestones:

- **v1.0.3 – v1.0.4 (Stereo 3D, Motion Damping, 3D Banner & Touch UI):**
  - **Assistant:** **Gemini Antigravity** (Google DeepMind)
  - **Role:** Implementation of hardware autostereoscopic 3D, Citro2D/Citro3D stereo targets, dynamic motion-damped stereoscopy, 3D parallax diorama banner and stereo CWAV stinger, pixel-perfect 1:1 dialogue typography and word-wrapping, Touch HUD hotbar, passcode keypad, Play Coins exchange, and in-memory `libmad` MP3 audio streaming.

- **v1.0.0 – v1.0.2 (Initial Port & CIA Packaging):**
  - **Assistant:** **Hermes Agent** (Nous Research)
  - **Model:** `tencent/hy3:free`
  - **Role:** Tracing the initial 3DS boot/render path, resolving early heap-exhaustion and null-surface crashes, establishing the New 3DS home-menu `.cia` build pipeline, and initial repository setup.

---

## License

GNU General Public License v3.0. Doom RPG game data is the property of its respective owners and is **not** included in this repository.
