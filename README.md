# DoomRPG-RE - Nintendo 3DS port

This is a Nintendo 3DS port of the [reverse engineered Doom RPG](https://github.com/Erick194/DoomRPG-RE) by [GEC]. All credits go to the GEC team, this project would not exist without their hard work.

This repository is a fork of [`efimandreev0/DoomRPG-RE-3DS`](https://github.com/efimandreev0/DoomRPG-RE-3DS) created to provide a **home-menu `.cia` build** (installable on a New 3DS from the HOME Menu via FBI) and to fold in a set of quality-of-life and stability fixes that make the port actually playable on real hardware. The original work targets the loader it was developed against; on a stock New 3DS the boot path hit heap exhaustion and null-surface dereferences that crashed or blanked the screen before gameplay. The fixes in this fork are summarized in [`docs/RELEASE_NOTES_v1.0.1.md`](docs/RELEASE_NOTES_v1.0.1.md).

## How to install

1. Get the original game data: the game reads its assets as **loose files** from
   `sdmc:/3ds/doomrpg/` — the 3DS build never opens a zip (the loader code paths
   are zip-named but actually read files directly from that folder). The data
   lives inside the `doomrpg.bar` container, which is bundled in the
   `doomrpg.zip` archive at the `doomrpg_brew` item on
   [archive.org](https://archive.org/details/doomrpg_brew). Extract `doomrpg.bar`
   (a BREW asset container; the upstream PC tools such as `BarToZip` turn it into
   loose files) and copy those loose files — no further zip step is needed.
2. Install `DoomRPG-1.0.1.cia` with FBI (or run `DoomRPG.3dsx` from the Homebrew Menu).
3. On the SD card, copy the **extracted data files** into
   `sdmc:/3ds/doomrpg/` so they sit loose in that folder. Required files
   include the `*.bmp` sprites/UI, the `*.bsp` maps, `wtexels.bin`,
   `stexels.bin`, `bitshapes.bin`, `palettes.bin`, `mappings.bin`, and
   `sintable.bin`.
4. Add audio next to the data, also under `sdmc:/3ds/doomrpg/`:
   - SFX as numbered `.wav` files (`001.wav`, `002.wav`, …) — these are the
     game's sound resources.
   - Music as numbered `.mp3` files (`1.mp3`, `2.mp3`, …) by track ID.
5. Launch from the HOME Menu. `sdmc:/3ds/doomrpg/saves/` is created at runtime.

The game will not start unless `sdmc:/3ds/doomrpg/` exists with the data files present.

## Default controls (New 3DS)

| Action           | Button          |
| ---------------  | --------------- |
| Move Forward     | D-pad Up        |
| Move Backward    | D-pad Down      |
| Strafe Left      | L               |
| Strafe Right     | R               |
| Turn Left        | D-pad Left      |
| Turn Right       | D-pad Right     |
| Attack/Talk/Use  | A               |
| Next Weapon      | ZR              |
| Prev Weapon      | ZL              |
| Pass Turn        | B               |
| Automap          | Select          |
| Menu Open/Back   | Start           |

NOTE: the D-pad turns; **L/R are lateral movement (strafe)** — hold L or R to
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

This fork — including the home-menu `.cia` packaging, the quality-of-life and
stability fixes, the documentation, and the build/release tooling — was
developed with the assistance of **Hermes Agent**, an AI coding assistant.

- **Assistant:** Hermes Agent ([Nous Research](https://nousresearch.com))
- **Model used:** `tencent/hy3:free`
- **How it was used:** end-to-end — tracing the 3DS boot/render path,
  root-causing and fixing the heap-exhaustion and null-surface crashes,
  preparing the repository for public release (source-only scrub, clean
  commit history, README + `docs/`), and building/packaging the CIA.

## License

GNU General Public License v3.0. Doom RPG game data is the property of its
respective owners and is **not** included in this repository.
