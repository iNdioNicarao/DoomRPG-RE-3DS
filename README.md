# DoomRPG-RE - Nintendo 3DS port

This is a Nintendo 3DS port of the [reverse engineered Doom RPG](https://github.com/Erick194/DoomRPG-RE) by [GEC]. All credits go to the GEC team, this project would not exist without their hard work.

## How to install

1. Search for "Doom RPG BREW" on [archive.org](https://archive.org/) to get the original mobile game assets file: `doomrpg.bar` (CRC32: d7cf11c5).
2. Next, you need `BarToZip.exe` converter tool (Windows only) included in the [PC release](https://github.com/Erick194/DoomRPG-RE/releases/latest).
3. Copy `doomrpg.bar` to a folder containing `BarToZip.exe`. Run `BarToZip.exe` to generate `DoomRPG.zip` file.
4. Download the [latest 3dsx release](https://github.com/efimandreev0/DoomRPG-RE-3DS/releases/latest) and extract this to `3ds/`.
5. Extract the newly generated `DoomRPG.zip` to `3ds/doomrpg/`.
6. Extract `datafiles.zip` from last Release to `3ds/doomrpg/`.<br />
Optional: You can install localizations from Release too.
8. You can now start the game

## Default controls

| Action           | Button          |
| ---------------  | --------------- |
| Move Forward     | D-pad Up        |
| Move Backward    | D-pad Down      |
| Move Left        | ZL (or C-Left)  |
| Move Right       | ZR (or C-Right) |
| Turn Left        | D-pad Left      |
| Turn Right       | D-pad Right     |
| Attack/Talk/Use  | A               |
| Next Weapon      | R               |
| Prev Weapon      | L               |
| Pass Turn        | B               |
| Automap          | Select          |
| Menu Open/Back   | Start           |

NOTE: Automap is always on your downscreen.

## Save and config data

All user data is stored in `/3ds/doomrpg/saves` - these files are compatible with the PC release.

## Building instructions

This port uses SDL1.2 and SDL_Mixer (1.2)

1. Install [Devkitpro] and 3DS-dev package, also make sure `DEVKITPRO` env variable is set
1. Install SDL1.2 and SDL_Mixer by Pacman
1. git clone https://github.com/efimandreev0/DoomRPG-RE-3DS.git && cd DoomRPG-RE-3DS
1. mkdir build && cd build
1. cmake .. -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake
1. make

## Bug reporting

If you find a bug, please open an issue here on github.

## License

GNU General Public License v3.0
