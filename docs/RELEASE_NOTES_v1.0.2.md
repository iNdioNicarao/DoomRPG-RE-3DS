# Doom RPG RE — 3DS v1.0.2

New 3DS home-menu build (`.cia`) of Doom RPG RE, following v1.0.1. Source-only:
no game assets or audio are bundled in the repository. They are supplied on the
SD card (see *Install*).

- Build: 2026-08-25
- Version: 1.0.2
- Title ID: `000400000FD0BB01`
- License: GPLv3

## What changed

| Area | Change |
|------|--------|
| HUD | Split the two on-screen bars: the **status/info text bar** now renders at the **top** of the screen, and the **health/ammo HUD** stays at the **bottom**. The top strip is always painted (metallic background) instead of only when a message is active. |
| Input / binds | Custom button bindings now store a `CONTROLLER_BUTTON_*` code so gameplay and controller binds decode independently. Menu navigation is always resolved from the default keymap, so a gameplay rebind can no longer break menu movement/selection. |
| Menu | Added an in-game **Controls** entry (rebind screen) and a **Debug** entry (cheats). Removed the dead Mouse/Controller sub-items and the Automap bind row. **Reset Binds** now restores the default keymap and saves immediately. |
| 3DS layer | `SDL_GameControllerGetButtonID` / `SDL_JoystickGetButtonID` map HID keys to `CONTROLLER_BUTTON_*` so bind capture detects the pressed button. |
| Logging | Asset-load failure messages now report the real path (`sdmc:/3ds/doomrpg/`) instead of the misleading "from zip" text. The 3DS loader reads loose files directly — there is no `DoomRPG.zip` and none is needed. |

## Install

1. Install `DoomRPG-1.0.2.cia` with FBI (or run `DoomRPG.3dsx` from the
   Homebrew Menu).
2. On the SD card, copy the **extracted data files** into
   `sdmc:/3ds/doomrpg/` so they sit loose in that folder. Required files
   include the `*.bmp` sprites/UI, the `*.bsp` maps, `wtexels.bin`,
   `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, and
   `sintable.bin`.
3. Add audio next to the data, also under `sdmc:/3ds/doomrpg/`:
   - SFX as numbered `.wav` files (`001.wav`, `002.wav`, …).
   - Music as numbered `.mp3` files (`1.mp3`, `2.mp3`, …) by track ID.
4. Launch from the HOME Menu. `sdmc:/3ds/doomrpg/saves/` is created at runtime.

The game will not start unless `sdmc:/3ds/doomrpg/` exists with the data files present.

Game data (loose files extracted from the `doomrpg.bar` archive, numbered
`.wav` SFX, numbered `.mp3` music) must be supplied on the SD card under
`sdmc:/3ds/doomrpg/`. The 3DS build reads these files directly — there is no
`DoomRPG.zip` and none is needed.

## Controls (New 3DS)

- D-pad: turn left / right
- L / R: strafe left / right (lateral movement)
- ZL / ZR: previous / next weapon
- A: use / confirm
- B: back / pass turn
- Start: menu open / back
- Select: automap (always on bottom screen)

See `docs/CHEATS.md` for the Debug menu, rebinding, and Reset Binds.

## Release assets

This release includes the built binaries as download assets:

- `DoomRPG-1.0.2.cia` — MD5 `fb9f03645dc3097cc77481d665ad9e5d`
- `DoomRPG.3dsx` — MD5 `95337b4dbcd2e5f6cb383d7e8bc8850b`

## Notes

- Targets New 3DS (the larger heap and extra RAM are required for the texture
  and menu surfaces). Old 3DS is not supported.
