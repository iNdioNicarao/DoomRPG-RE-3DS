# Doom RPG RE — 3DS v1.0.5

New 3DS home-menu build (`.cia`) and 3DSX package of Doom RPG RE, following v1.0.4. Source-only: no game assets or audio are bundled in the repository. They are supplied on the SD card (see *Install*).

- **Build**: 2026-09-04
- **Version**: 1.0.5
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Developer Attribution**: Dennis Isaac Gutierrez Zeledon

---

## Overview

Version 1.0.5 is a comprehensive presentation, stereoscopic, and quality-of-life update. It scales all legal teasers and narrative cutscenes by 2x for full clarity, adds autostereoscopic 3D depth to the rotating title menu background, brings interactive drag-panning and zoom controls to the bottom-screen automap, default-enables authentic textured floors and ceilings, provides visual and acoustic feedback for empty inventory hotbar actions, and implements instant audio suspension when the 3DS clam-shell is closed.

---

## Highlights (v1.0.5)

- **2x Scaled Cutscenes, Splash Screens & Teasers**:
  - Legal screens and splash teasers (`g.bmp`) are upscaled 2x to 256x256, centered cleanly on the 400x240 top screen.
  - Intro story cutscene upgraded with 2x presentation: crisp text pages with adjusted prompt placement, and full 2x scaling for the animated spaceship flyby, scrolling starfields, speed lines, planet layers, and pulsing red tracking crosshairs.
  - Epilogue, scrolling credits, and error screens rendered through the 2x cutscene pipeline.

- **Stereoscopic 3D on Rotating Title Menu**:
  - Pushing the 3D depth slider on the startup title screen renders the slowly rotating 3D background room in true autostereoscopic 3D.
  - The DOOM RPG logo, menu items, scrollbar, hand selector cursor, and button prompts float cleanly at screen depth (zero parallax) with zero ghosting.
  - Enabled full-frame 400x240 right-eye rendering (`g_stereoFullFrame`) bypassing HUD row clamps.

- **Interactive Bottom-Screen Automap**:
  - **Player Centering & Tracking**: The automap smoothly tracks and centers on the player's position as you step across tiles.
  - **Touch Drag / Free Pan**: Touch and drag with a finger or stylus to pan across explored areas of the level without moving your character.
  - **Zoom & Recenter Controls**: Added on-screen `[+]` (zoom in), `[-]` (zoom out), and `[O]` (recenter to player) buttons. Tapping the player arrow icon also instantly snaps the camera back.

- **Full Metallic Quick-Access Bar**:
  - Extended the authentic status bar metallic texture across the entire 400x240 bottom screen down to the lower bezel (row 479).
  - Cleaned up vertical dividers and eliminated horizontal baseline seams under hotbar text for an integrated console aesthetic.

- **Hotbar Feedback & Dimming**:
  - Hotbar buttons for empty items (count = 0) are visually dimmed by 50%.
  - Tapping an empty slot triggers an immediate red highlight flash, a distinct negative audio cue, and a top-screen HUD status notification (e.g. *"No Small Medkits!"*).

- **Default-Enabled Textured Floors & Ceilings**:
  - Authentic textured floors, ceilings, and overhead light fixtures are enabled out-of-the-box, running smoothly on 3DS hardware.

- **Synchronized 3D Motion Recovery**:
  - Motion-damped stereoscopic depth recovery is now tightly synchronized with grid movement completion, removing settling delay.

- **Horizontally Centered HUD Messages**:
  - In-game status banners ("Found Red Keycard", "Door is Locked", ammo pickups) are centered horizontally on the top screen for natural eye focus.

- **Lid-Close Audio Suspension & Power Management**:
  - Integrated `APT_HookType` audio suspension into `SDL_Video.c`. Closing the 3DS clam-shell immediately mutes game audio and enters low-power sleep; reopening resumes audio and visuals gracefully.

---

## Detailed Commit Log (v1.0.5)

1. `1e5c636` — `feat: scale cutscenes/teasers by 2x and add stereoscopic 3D to main menu`
2. `34b1cca` — `style(hud): center top screen status bar messages horizontally`
3. `3bf6231` — `feat(automap): add player-centered scrolling, touch drag, zoom controls, and full metallic quick-access bar`
4. `a6af3ba` — `fix: synchronize 3D depth recovery with movement completion to remove settling lag`
5. `449ba50` — `feat: default enable textured floors/ceilings, pause audio on lid close, and add zero-inventory hotbar feedback`

---

## Installation & Verification

1. Install `DoomRPG-1.0.5.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
2. Assets remain located at `sdmc:/3ds/doomrpg/`.
3. Launch from the HOME Menu.

---

## Credits & Attribution

- **Port & 3DS Enhancements**: Dennis Isaac Gutierrez Zeledon
- **Original Reverse Engineering**: GEC Team (Erick194 and contributors)
- **Doom RPG**: id Software / Fountainhead Entertainment
