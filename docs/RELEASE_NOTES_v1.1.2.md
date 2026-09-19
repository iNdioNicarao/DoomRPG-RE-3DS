# Doom RPG RE — 3DS v1.1.2 Release Notes

Maintenance, visual polish, and quality-of-life release for the Nintendo 3DS family (`.cia`), following v1.1.1. Source-only release: no copyrighted id Software game assets or audio are bundled in the repository.

- **Build Date**: 2026-09-19
- **Version**: 1.1.2
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Maintainer & Lead Developer**: Dennis Isaac Gutierrez Zeledon

---

## Overview of v1.1.2

Doom RPG RE 3DS v1.1.2 builds directly upon the performance foundation of v1.1.1, addressing visual and layout refinements across both screens, eliminating bottom-screen redraw blackouts, upgrading NPC and terminal dialog typography, enhancing Turbo Mode responsiveness, bringing full stereoscopic 3D support to the end-game monster cast, and correcting weapon vertical positioning.

---

## 🖥️ Screen & Rendering Fixes

### 1. Bottom-Screen Blackout Resolution
* **The Issue:** Under certain conditions when stationary or during specific turn sequences, the bottom screen (automap and Touch HUD) would intermittently go completely black until player movement or an interaction forced a redraw.
* **Root Cause:** In [`src/SDL_Video.c`](../src/SDL_Video.c), `SDL_RenderClear` was executing an unconditional memory clear (`SDL_memset`) across the full vertical extent of `screenSurface` (400×480 pixels, rows 0..479). Because the bottom screen (rows 240..479) uses dirty-flag blit caching to save CPU cycles and bus bandwidth when stationary, wiping rows 240..479 left the lower display completely black until a dirty event occurred.
* **The Fix:**
  - Restricted `SDL_RenderClear` on 3DS exclusively to top-screen scanlines (rows 0..239, $240 \times \text{pitch}$ bytes), preserving cached bottom-screen content.
  - Added proactive `g_botScreenDirty` triggers on state changes in [`src/DoomCanvas.c`](../src/DoomCanvas.c) (`DoomCanvas_setState`) and whenever player health, armor, or weapon selection updates in `DoomCanvas_playingState`.

### 2. Authentic Weapon Height & Viewport Grounding
* **The Issue:** In earlier revisions, idle weapons were pushed down by approximately 48 pixels, leaving only a tiny 3–6 pixel strip visible above the bottom HUD bar.
* **Root Cause:** [`Render_setup`](../src/Render.c) was being called with `displayRect` ($h = 240, y = 0$) rather than `screenRect` ($h = 192, y = 20$). In [`Combat_drawWeapon`](../src/Combat.c), weapon sprite vertical position is calculated relative to `render->screenHeight`. When height became 240, weapons sank 48 pixels downward, burying their idle silhouettes behind the status bar ($y = 212..239$).
* **The Fix:** Restored `Render_setup` in [`DoomCanvas.c`](../src/DoomCanvas.c) and [`MenuSystem.c`](../src/MenuSystem.c) back to `screenRect` ($h = 192$). Idle weapons now properly display ~30–60 pixels above the HUD line (matching the height of the HUD bar and authentic BREW reference footage), while attack animations remain perfectly grounded.

### 3. Autostereoscopic 3D End-Game Monster Cast
* **Enhancement:** The end-game monster showcase (`ST_CAST`) is now rendered in full autostereoscopic 3D on consoles with 3D enabled:
  - Both eye views render with horizontal parallax separation driven by the physical 3D slider (`g_stereoSep`).
  - Right-eye buffer is blitted cleanly to `g_stereoRight` with stereo validation flags.
  - Monster names and credits are rendered in both eyes to maintain comfortable depth perception without ghosting.

---

## 💬 Typography & Dialog Modernization

### Compact 1x Proportional Font with 6-Line Display
* **The Problem:** In previous builds, NPC and computer terminal dialogs used an oversized 2x font that fit only 3 lines of text per window. Mobile-era line breaks frequently left solitary 2-to-3 character words stranded on their own line, forcing excessive scrolling and button presses.
* **The Solution:**
  - Upgraded NPC and terminal message dialogs to the sharp 1x proportional font (`imgFont`).
  - Doubled window capacity from 3 lines to **6 full lines of text** per page.
  - Implemented dynamic word wrapping at 246 font pixels with syllable de-hyphenation (`DoomCanvas_cleanDialogText`) and single-pipe newline conversion.
  - Formatted terminal screens with authentic retro **phosphor green** modulation (`0x33ff33`), contrasting with crisp white text for NPC conversations.

---

## 📖 Widescreen Help & Controls Layout

* Rebuilt the Help & About menu in [`src/Menu.c`](../src/Menu.c) to take full advantage of the top screen's 400px widescreen display:
  - Expanded text buffer from 32 to 64 characters per line.
  - Hardware-aware context layouts:
    - **New 3DS / New 2DS**: Highlights dedicated `ZL` / `ZR` strafing, C-Stick camera/text navigation, and face-button mappings.
    - **Old 3DS / Old 2DS**: Highlights `L` / `R` cycling, Touch HUD shortcuts, and Circle Pad controls.

---

## ⏩ Turbo Turn-Pass Acceleration

* In Turbo Mode, pressing `Select` (Pass Turn) now calls `Game_snapMonsters` immediately, snapping all monster turn animations off-screen and eliminating delay between turns.

---

## Installation & Upgrading

1. Install `DoomRPG-1.1.2.cia` using FBI over network (FTP/URL) or from SD card.
2. Existing save files (`sdmc:/3ds/doomrpg/saves/`) and configuration settings are 100% preserved.

---

## Verification & File Hashes

- **File**: `DoomRPG-1.1.2.cia` (1,083,840 bytes)
  - **MD5**: `96f0cec89533d08c70edc3c96245f2d6`
  - **SHA256**: `1e3bc4358093b204aaffb880a4db306d6a22a23ad0774b3a16743072aac21c19`
- **File**: `DoomRPG.3dsx` (1,097,192 bytes)
  - **MD5**: `04d20ebfe00599e2905a8f3e07e3b90a`
  - **SHA256**: `c882db2db066d1eae7dca220f1fae47f249604a8680049a00db336f96e6018fd`

---

## Commit History (v1.1.1 $\to$ v1.1.2)

- `1547432`: fix: resolve bottom-screen blackouts, weapon cutoff, and add stereo 3D monster cast
- `61c46d8`: feat: 1x dialog font, widescreen help layout, and turbo turn-pass acceleration
- `07f50bc`: fix(render): restore 3D viewport height to screenRect to fix idle weapon vertical position
