# Cheats

The engine includes a Debug menu with cheats (inherited from the original
mobile/BREW port). On the stock upstream 3DS build these were unreachable
because they were gated behind a phone-style numeric keypad code
(`AVK_0`–`AVK_9`), and the 3DS input layer never produces those digit keys.

As of v1.0.2, the **in-game menu** has a **Cheats** entry so the debug tree is
reachable with the normal controls.

## How to open (3DS, v1.0.2+)

1. During play, open the game menu (Start).
2. Scroll down to **Cheats** (D-pad Up/Down) and select it (A).
3. You are now in the Debug menu: **Cheats**, **Change Map**, **Stats**,
   **Developer**.
4. Pick **Cheats** for the toggle list, or **Change Map** to jump to any level.

Navigation is the standard menu scheme: D-pad to move, A to select/activate,
B/Start to go back.

## Cheats (Debug → Cheats)

| Cheat | Effect |
|-------|--------|
| Noclip | Walk through walls |
| Disable AI | Enemies stop acting |
| Give all | All weapons and items |
| Give ammo | Refill ammo |
| God mode | Invulnerability (no damage taken) |
| Level up | Grant XP / level |
| Give map | Reveal the automap |

## Change Map

Jump straight to any level: Intro, Junction, Sectors 1–7, Des. Junction,
Reactor, Credits, Items.

## Developer

Render/debug toggles (`r_frames`, `r_speeds`, `r_skipCull`, `s_debug`, …),
Benchmark, and memory/state inspectors. Intended for development; harmless
to toggle.

## Note on the old keypad codes

The upstream mobile port opened this menu with digit sequences such as
`3666` (open debug) — those refer to the phone's 0–9 keys and do not apply
to the 3DS, which has no numeric keypad. Use the in-game **Cheats** menu
entry instead.
