# Doom RPG RE — 3DS v1.1.1 Release Notes

Performance, input stability, and feature release for the Nintendo 3DS family (`.cia` and `.3dsx`), following v1.1.0. Source-only release: no copyrighted id Software game assets or audio are bundled in the repository.

- **Build Date**: 2026-09-18
- **Version**: 1.1.1
- **Title ID**: `000400000FD0BB01`
- **License**: GPLv3
- **Maintainer & Lead Developer**: Dennis Isaac Gutierrez Zeledon

---

## Overview of v1.1.1

Doom RPG RE 3DS v1.1.1 delivers a major performance overhaul specifically engineered to make the game fully responsive and snappy on **Original 3DS, 3DS XL, and 2DS** hardware. This release resolves the underlying issues that prevented earlier v1.1.0 performance settings from taking effect, accelerates Turbo Mode combat flow, completely eliminates the accidental menu popup bug when pressing directional buttons and Attack (`A`), introduces a real-time Enemy / Kill counter on the bottom-screen automap, and expands the physical 3DS notification LED system with 3-second colored alert flashes across 17 in-game events.

---

## ⚡ Performance Deep Dive: Why Earlier Improvements Did Not Take Effect & Why They Work Now

Following the release of v1.1.0, playtesting feedback on Nintendo 2DS and Original 3DS systems revealed that the game still felt sluggish in combat and exploration despite several optimizations having been introduced into the codebase. 

A thorough profiling and code audit identified four critical bottlenecks and silent override conditions that prevented earlier optimizations from improving performance:

### 1. Saved Configuration Overwrite Bug
* **The Problem in v1.1.0:** Although internal `Retro 200` scaling (200×240 raycasting with Citro2D PICA200 GPU upscale) and solid floor/ceiling presets were added to the engine, `SDL_InitVideo()` still hardcoded the startup fallback to `g_renderScaling = 0` (Native Crisp 400). More critically, `Game_loadSettings()` loaded settings from the player's existing SD card configuration file (`sdmc:/3ds/doomrpg/saves/Config`). Any player upgrading from earlier versions—or anyone who launched the game before changing settings—had a saved config that explicitly mandated full 400×240 native raycasting with heavy textured ceilings and floors. The intended Old 3DS defaults were silently discarded on every boot.
* **How It Works Now in v1.1.1:** `SDL_InitVideo()` now dynamically initializes `g_renderScaling = g_isOldHardware ? 1 : 0`. Furthermore, `Game.c` now includes an automatic configuration migration pass on boot: if an Old 3DS or 2DS console is detected and the existing config file has not been explicitly modified by the user, the engine migrates the active hardware profile to `Retro 200` internal resolution with flat solid floors and ceilings automatically.

### 2. Raycaster Span Rasterization Was Still Running for All 400 Columns
* **The Problem in v1.1.0:** While the initial raycasting DDA loop stepped across 200 columns when scaling was enabled, the inner rasterizer functions—`Render_drawWallSpans()` and `Render_drawSpriteSpan()`—were still looping across all 400 horizontal screen columns (`colStep = 1`). As a result, the 268 MHz ARM11 CPU was still performing perspective division, texture coordinate calculation, and pixel span rasterization across 400 columns per frame, negating nearly all the expected CPU savings.
* **How It Works Now in v1.1.1:** `Render_drawWallSpans()` and `Render_drawSpriteSpan()` now genuinely step by 2 columns (`colStep = 2`) when `g_renderScaling == 1`. This cuts wall and sprite span rasterization calculations by exactly **50%**, ensuring the working set comfortably fits within the ARM11's 32 KB L1 data cache.

### 3. Particle Flooding & Lingering Lifetimes During Combat
* **The Problem in v1.1.0:** Blood splatters and spark particles were re-enabled with an optimized scanline circle fill. However, on projectile hits and explosive deaths, the particle emitter spawned up to 64 particles with lifetimes exceeding 500ms. Calculating 3D projections, velocity physics, and circle fills for dozens of active particles on an Old 3DS caused frame rates to plummet into the single digits during intense firefights.
* **How It Works Now in v1.1.1:** On Old 3DS and 2DS consoles, particle counts are hard-capped to a maximum of **8 particles**, and their active lifetime is reduced to **150ms**. Furthermore, `ParticleSystem_freeAllParticles()` is immediately invoked the instant combat ends, preventing background particle processing from stalling turn transitions.

### 4. Redundant 384 KB Bottom-Screen Framebuffer Clearing Every Frame
* **The Problem in v1.1.0:** Even though the bottom-screen automap included dirty-blit caching to skip rendering when the player was stationary, `SDL_PresentGfx()` was executing an unconditional `SDL_memset` across the entire 400×240 bottom region (384,000 bytes) on every single frame. This generated constant, unnecessary memory bus traffic on hardware with limited RAM bandwidth.
* **How It Works Now in v1.1.1:** The redundant 384 KB per-frame `memset` was eliminated. The bottom screen framebuffer is cleared selectively only when dirty regions require updating.

### 5. High-Frequency Audio Mixing & Resampling Overhead
* **The Problem in v1.1.0:** The audio subsystem initialized SDL Mixer at 44,100 Hz stereo. Software MP3 decoding via `libmad` and sound effect mixing at 44.1 kHz consumed a noticeable percentage of the single-core CPU budget.
* **How It Works Now in v1.1.1:** On Old 3DS and 2DS consoles, `Mix_OpenAudio()` now runs at **22,050 Hz**, and `MusicStream` downsamples 44.1 kHz MP3 streams 2:1 during synthesis. This cuts audio mixing CPU overhead in half while preserving sound fidelity through the 3DS speakers.

### 6. Frame Pacing & Why Turbo Mode Finally Feels Fast
* **Why It Feels Snappy in v1.1.1:** In v1.1.0, Turbo Mode combat was still subjected to artificial post-action wait intervals (333ms), and frame pacing in the main loop artificially delayed turn progression. In v1.1.1, post-attack recovery wait in Turbo Mode has been slashed to **75ms** (with instant turn advance on pressing `A`), monster movement turns resolve instantaneously off-screen, projectile velocity is quadrupled, and the Old 3DS frame loop runs at a steady 30 FPS tick cap (`frameDelay = 33ms`) without idle spin stalls. The result is instant, snappy combat and exploration.

---

## 🛠️ Key Bug Fix: Accidental Menu Popups on `Right + A` / `Down + A`

### Root Cause
In [`DoomRPG.h`](src/DoomRPG.h), action codes in the `_AVKType` enum are sequential integer indices, **not bitflags**:
* `AVK_DOWN` = **19** (`0b00010011`)
* `AVK_RIGHT` = **21** (`0b00010101`)
* `AVK_SELECT` (Button `A`) = **22** (`0b00010110`)
* `AVK_SOFT1` = **23** (`0b00010111`)

In [`DoomCanvas.c`](src/DoomCanvas.c), `AVK_SOFT1` was mapped to `MENUOPEN` (legacy mobile phone softkey 1).

In [`DoomRPG.c`](src/DoomRPG.c), the 3DS input loop was aggregating all held and pressed buttons into a single integer using bitwise OR: `key |= keyMapping[i].avk_action`. This created mathematical collisions:
* **`AVK_DOWN | AVK_SELECT`** = `19 | 22` = **`23`** (`AVK_SOFT1` $\rightarrow$ **`MENUOPEN`**)!
* **`AVK_RIGHT | AVK_SELECT`** = `21 | 22` = **`23`** (`AVK_SOFT1` $\rightarrow$ **`MENUOPEN`**)!
* **`AVK_DOWN | AVK_RIGHT`** = `19 | 21` = **`23`** (`AVK_SOFT1` $\rightarrow$ **`MENUOPEN`**)!

Whenever a player pressed `Right + A` or `Down + A` simultaneously—or in rapid succession where the physical button release overlapped with the `A` press by even a single frame—the bitwise OR evaluated to 23, immediately popping open the in-game menu.

### The Fix
1. **Priority Input Resolution**:
   - **Phase 1 (Fresh Presses)**: Fresh button presses (`kDown` and Circle Pad edge changes) are evaluated before held buttons. Tapping `A` immediately takes precedence over any held direction.
   - **Action Hierarchy (`s_actionPriority`)**: Evaluates inputs in strict order: Menu/System (`Start`, `Select`) $\rightarrow$ Actions (`A`, `B`, Weapons, Strafe) $\rightarrow$ Directional (`D-Pad`, `Circle Pad`). If `A` and a directional input occur on the exact same frame, the Attack/Interact action always wins.
   - **Single Base Action**: Exactly one base action (`baseKey`) is selected. Multiple base actions are **never bitwise ORed together**.
   - **Clean Menu Separation**: Menu navigation bitflags (`AVK_MENU_*`, bits `0x40..0x800`) are masked via `AVK_MENU_ALL_FLAGS` into a disjoint bit field that can never alter the base action.
2. **Sanitized Legacy Phone Code**: `AVK_SOFT1` mapping in `keys_codeActions` has been set to `0` for defense-in-depth.

---

## 🎯 New Feature: Bottom-Screen Enemy / Kill Counter

* **Real-Time Kill Tracking**: Integrated `Player_fillMonsterStats()` into the automap renderer alongside map exploration and secrets.
* **Expanded HUD Badge**: The statistics badge at the bottom-left of the automap ($X = 44, Y = 430$) has been widened from 192px to 318px, formatted as:
  ```
  MAP:%d%% | SEC:%s | KILLS:%s
  ```
  *(e.g., `MAP:100% | SEC:2/2 | KILLS:14/14` or `KILLS:--` if no monsters exist on the map).*
* **100% Full Map Clear Highlight**: When 100% of both secrets and enemies on the map have been discovered and eliminated, the badge illuminates with an accented double golden border (`0xFFFFCC33`).

---

## 🗃️ UI Architecture: Strict Weapon vs. Item Separation on Touch HUD

To eliminate dual-function ambiguity on the bottom screen Touch HUD:
* **Horizontal Bottom Bar (Items Only)**: Button 4 has been updated from `DOG` to `COLR`, displaying the live count of Dog Collars in inventory (`COLR:x`). Tapping `COLR` uses the collar item (`Player_useItem(29)`) to tame a facing hound.
* **Vertical Left Column (Weapons Only)**: Slot 9 is now strictly dedicated to equipping the Dog companion weapon (`DG`). When the player owns a hound, slot 9 is selectable with the hound head HUD icon; when unowned, it displays `--` like any other unacquired weapon in slots 0..8.
* **Result**: The vertical column remains exclusively weapons, and the horizontal bottom bar remains exclusively usable inventory items.

---

## 💡 Expanded 3-Second Notification LED Suite

All physical 3DS notification LED alerts have been upgraded to a sustained **3.0-second (3000ms)** flash duration for unmistakable visibility, covering **17 distinct gameplay events**:

| Event | LED Color & Pattern | Description |
|---|---|---|
| **Secret Found** | Emerald Green (3s flash) | Triggered upon discovering a hidden secret sector |
| **Level Up** | Vibrant Cyan (3s flash) | Triggered when player levels up |
| **Green Keycard** | Bright Green (3s flash) | Acquired Green Keycard |
| **Yellow Keycard** | Golden Yellow (3s flash) | Acquired Yellow Keycard |
| **Blue Keycard** | Deep Sky Blue (3s flash) | Acquired Blue Keycard |
| **Red Keycard** | Vivid Crimson (3s flash) | Acquired Red Keycard |
| **Soul Sphere** | Radiant Violet (3s flash) | Consumed a Soul Sphere |
| **New Weapon** | Bright Orange (3s flash) | Picked up a new weapon |
| **Game Saved** | Soft Cyan (3s flash) | Manual or checkpoint save written to SD |
| **Dog Tamed** | Warm Pink (3s flash) | Successfully captured a Cerberus hound companion |
| **Play Coins Exchanged**| Amber Gold (3s flash) | Exchanged Nintendo 3DS Play Coins for credits |
| **Player Death** | Flashing Red (3s) | Health drops to 0 |
| **Boss Defeated** | Magenta / Purple (3s flash) | Defeated a sector boss entity |
| **Level Complete** | Pure White (3s flash) | Stepped on level exit / transition |
| **Armor Broken** | Dark Slate Gray (3s flash) | Armor depleted to 0 during combat |
| **Capacity Full** | Bright Yellow-Orange (3s) | Attempted to collect item when inventory is full |
| **Dog Strike** | Coral Orange (3s flash) | Cerberus companion executes a combat strike |
| **Critical Health** | Crimson Strobe (<15% HP) | Continuous urgent pulse while in critical danger |
| **Low Health** | Red Pulse (<25% HP) | Continuous heartbeat pulse while low on health |
| **Berserk Active** | Fiery Orange Pulse | Continuous pulse while Berserk powerup is active |

---

## Installation & Upgrading

1. Install `DoomRPG-1.1.1.cia` using FBI (or copy `DoomRPG.3dsx` to `sdmc:/3ds/`).
2. Existing save games in `sdmc:/3ds/doomrpg/saves/` are 100% compatible and will not be overwritten.
3. On first launch on Old 3DS / 2DS consoles, the engine automatically migrates settings to the optimized performance profile.

---

## Commit History (v1.1.0 $\to$ v1.1.1)

- `9cd1105`: `feat: v1.1.1 aggressive Old 3DS/2DS performance suite and config migration`
- `58da929`: `feat(led): add 3-second colored notification LED flashes for secrets, level ups, keycards, and gameplay events`
- `ec3abc6`: `feat(led): add 3-second colored LED flashes for level complete, armor broken, capacity full, and dog attack`
- `5dbbf7d`: `Fix inadvertent menu opening on simultaneous input and add enemy counter to bottom screen`
