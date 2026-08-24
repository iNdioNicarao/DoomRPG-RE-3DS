# Doom RPG RE — 3DS v1.0.1

New 3DS home-menu build (`.cia`) of Doom RPG RE, with the 3DS-specific fixes
below. Source-only: no game assets or audio are bundled in the repository.
They are supplied on the SD card (see *Install*).

- Build: 2026-08-23
- Title ID: `000400000FD0BB01`
- License: GPLv3

## What changed

Upstream built and ran under the loader it was developed against, but on real
hardware the boot path hit heap exhaustion and null-surface dereferences that
crashed or blanked the screen before gameplay. The fixes:

| Area | Fix |
|------|-----|
| Boot / assets | Resolve all SD asset and save paths against `sdmc:/3ds/doomrpg/` (and `…/saves/`). Relative/zip-backed paths fail on the 3DS FAT layer. |
| Heap (intro) | Stop per-frame `SDL_Surface` allocation in `MenuSystem_paint`. The menu surface is now a persistent `MenuSystem_t` member, allocated once and reused. The old per-frame alloc could not be satisfied after the intro spin, leaving `SDL_CreateRGBSurface` returning NULL and the menu cursor appearing frozen / the menu→game transition going blank. |
| Heap (HUD) | Stack-allocate the HUD blit rects instead of `malloc` per frame, so the status bar is never skipped on allocation failure. |
| Heap (textures) | Keep the texture decode buffer persistent and bound the decode loop; removes the heap fragmentation that crashed on the ARM11 after a few minutes. |
| Music | Stream music as MP3 via libmad inside SDL_mixer's music callback (3DS SDL_mixer has no MP3/MIDI decoder). SFX are loaded as WAV chunks and cached by resource ID so the intro loop does not re-read multi-MB files every iteration. |
| Pointer safety | Guard NULL image loads and retry SD reads during boot instead of dereferencing a failed allocation. |
| HUD | Guard NULL HUD surfaces and free the temp surface on early-return paths (leak fix). |
| Intro | Reset the player on intro dispose — the first-spin new-game path could reach `disposeIntro` without passing through `Menu_startGame`, leaving the player struct uninitialized and the camera spawning into a black screen. |
| Menu | Keep repainting the active menu every frame after the intro spin stops, so the Enable-Sound / Main menu cursor tracks input. |
| Input | Rebind strafe to L/R, weapons to ZL/ZR. Boot stack raised to 256 KB. |

No copyrighted material is included. Game data (`.wav` SFX, `.mp3` music,
`DoomRPG.bar`/`.zip` assets) must be supplied on the SD card.

## Build (from a checkout)

Requires Docker and the devkitARM toolchain image (built automatically):

```
cd tools/cia
./build_cia.sh
```

Output: `tools/cia/DoomRPG-<version>.cia` and `DoomRPG.3dsx`. The `VERSION`
file controls the CIA filename; the Title ID / version byte comes from
`DoomRPG-3DS.rsf` and is left at upstream's value.

## Install

1. Install `DoomRPG-1.0.1.cia` with FBI (or run `DoomRPG-1.0.1.3dsx` from the
   Homebrew Menu).
2. Place game assets on the SD card at `sdmc:/3ds/doomrpg/`:
   - `DoomRPG.bar` (or the equivalent asset archive) — game data
   - `authentic_wavs/*.wav` — SFX
   - `*.mp3` — music tracks
   - `saves/` is created at runtime for config/player/world state.

The app will not start without the asset directory present.

## Controls (New 3DS)

- D-pad / Circle Pad: move
- A: use / confirm
- B: back
- L / R: strafe left / right
- ZL / ZR: previous / next weapon
- Touch: menu

## Release assets

This release includes the built binaries as download assets:

- `DoomRPG-1.0.1.cia` — MD5 `b69a586807d453f829573dcbc9d8f3be`
- `DoomRPG-1.0.1.3dsx` — MD5 `5e44f7b1c1a4365f7f9541c032332697`

## Notes

- Targets New 3DS (the larger heap and extra RAM are required for the texture
  and menu surfaces). Old 3DS is not supported.
