# Debug Menu, Cheats & Controls (3DS)

This document covers the in-game **Debug** menu (cheats), the **Controls**
(rebind) menu, and how to recover if bindings get messed up. All of these are
reachable from the in-game menu (Start) as of **v1.0.2**.

## Open the in-game menu

During play, press **Start** to open the game menu. Navigation is the standard
scheme: **D-pad** Up/Down to move, **A** to select/activate, **B / Start** to
go back.

The in-game menu contains: Resume Game, Inventory, Save Game, Load Game,
Status, Main Menu, **Debug**, **Controls**.

## Debug menu (cheats)

Upstream, this menu was gated behind a phone-style numeric keypad code
(`AVK_0`–`AVK_9`). The 3DS input layer never produces those digit keys, so on
the stock upstream 3DS build the cheats were unreachable. In v1.0.2 the
in-game menu has a **Debug** entry that opens the debug tree directly.

1. Open the in-game menu (Start).
2. Scroll to **Debug** and select it (A).
3. You are in the Debug menu: **Cheats**, **Change Map**, **Stats**,
   **Developer**.
4. Pick **Cheats** for the toggle list, or **Change Map** to jump to any level.

### Cheats (Debug → Cheats)

| Cheat | Effect |
|-------|--------|
| Noclip | Walk through walls |
| Disable AI | Enemies stop acting |
| Give all | All weapons and items |
| Give ammo | Refill ammo |
| God mode | Invulnerability (no damage taken) |
| Level up | Grant XP / level |
| Give map | Reveal the automap |

### Change Map

Jump straight to any level: Intro, Junction, Sectors 1–7, Des. Junction,
Reactor, Credits, Items.

### Developer

Render/debug toggles (`r_frames`, `r_speeds`, `r_skipCull`, `s_debug`, …),
Benchmark, and memory/state inspectors. Intended for development; harmless
to toggle.

### Touch Keypad & Passcode Entry (v1.0.3)

While the mobile version required physical phone digit keys to enter door passcodes, v1.0.3 introduces an on-screen **Touch Keypad** on the 3DS bottom screen. Whenever a locked door prompts for a security code, you can tap digits `0`–`9`, `C` (clear), and `OK` directly on the screen.

### Play Coins Exchange (v1.0.3)

You can exchange your Nintendo 3DS system **Play Coins** for in-game Credits at terminals and vending stations. Each Play Coin converts into **$50 Credits**, allowing you to put your 3DS steps toward ammo, armor, and medkits!

### Note on the old keypad codes

The upstream mobile port opened this menu with digit sequences such as
`3666` (open debug) — those refer to the phone's 0–9 keys and do not apply
to the 3DS. Use the in-game **Debug** menu entry instead.

## Controls (rebinding)

Open the in-game menu (Start) → **Controls**. This opens the input/bindings
screen where every action can be remapped to a 3DS button.

- Select an action and press **A**. The screen shows `Press New Key For
  <action>`.
- Press the button you want to bind. It is detected immediately and saved.
- **Back** (B / Start) applies your changes and writes them to the save file.

### Reset to defaults

If you want to undo all changes, open **Controls** → scroll to **Reset Binds**
and select it (A). This restores the default keymap and saves it immediately —
one tap, no need to also press Back.

### Fallback: delete the config file

Bindings are persisted to the SD card at:

```
sdmc:/3ds/doomrpg/saves/Config
```

If bindings ever get into a state where you cannot navigate the menu (for
example, if the movement/turn keys are unbound), delete that `Config` file
from the SD card (e.g. over FTP). On the next launch the file is absent, the
game falls back to the built-in default keymap, and you can rebind normally.

Do **not** delete the other files in `sdmc:/3ds/doomrpg/saves/` (Player,
World, etc.) — only `Config` holds control bindings.
