# Doom RPG RE 3DS — v1.1.1 Playtesting & Hardware Verification Checklist

Comprehensive testing checklist for verifying all Phase 1–5 performance, input, and visual fidelity enhancements on Nintendo 3DS / 2DS hardware.

---

## 🎮 Category 1: Visual Fidelity & Immersion (New in this Build)

- [ ] **Sinusoidal Weapon Bobbing & Sway**
  - **Location:** Exploration / Hallway walking
  - **Action:** Walk forward and backward through hallways and rooms using the Circle Pad or D-Pad.
  - **Expected:** Weapon smoothly dips vertically by ~3.5px at the midpoint of each 64-unit tile step and returns up on arrival, with subtle 1.5px horizontal sway matching classic Doom walking cadence.
  - **Stereo 3D Check:** Push 3D slider up; verify the weapon bobbing remains properly depth-seated with no ghosting or stereo desynchronization.

- [ ] **Restored Blood Droplets & Combat Spark Particles**
  - **Location:** Combat encounters / Destructible environment
  - **Action:** Attack monsters with melee (Axe, Fist) and firearms; shoot explosive barrels and destructible objects.
  - **Expected:** Colored blood droplets (red for human/zombie, green/amber for demons) and sparks now visibly spray on impact (previously commented out). Destructible objects emit grey debris particles.
  - **Stereo 3D Check:** In 3D mode, particles project with authentic parallax into the right-eye buffer (`g_stereoRight`).

- [ ] **Desynchronized Flying Gib Debris Rotation**
  - **Location:** Explosive monster kills (Rocket Launcher, Berserk)
  - **Action:** Gib a monster using high-damage or explosive weapons.
  - **Expected:** Flying skull and meat chunks rotate with individual rotational phases rather than spinning in uniform lockstep.

- [ ] **3-Tier Stepped Red Damage Vignette**
  - **Location:** Combat damage / Slime or hazard tiles
  - **Action:** Take damage from an enemy strike or step on a damaging hazard tile.
  - **Expected:** Viewport borders flash with a bold 3-layer stepped red vignette (6px deep crimson outer border, stepping to 4px and 2px bright red) instead of the old thin 2px wireframe line.

- [ ] **Color Alpha & Primitive Opacity**
  - **Location:** Automap doors, colored bars, HUD overlays
  - **Action:** Check keycard door colors on the automap and highlight bars in menus.
  - **Expected:** Colors render fully opaque (alpha 0xFF) with crisp, vibrant saturation rather than transparent/washed out.

---

## ⚡ Category 2: Performance, Thermal & Battery Optimizations

- [ ] **Main Loop Idle CPU Yield (`svcSleepThread`)**
  - **Location:** Stationary idle / Extended gameplay
  - **Action:** Leave the game running stationary in a room for 5–10 minutes.
  - **Expected:** The back of the console / battery cover remains noticeably cooler. Battery lifespan is extended because the CPU sleeps for 1ms between frame ticks rather than spinning at 100% busy-wait.

- [ ] **Fast Vectorized Screen Clearing**
  - **Location:** Bottom-screen status / Automap / Transition scenes
  - **Action:** Navigate between rooms, open the automap, open dialogs.
  - **Expected:** Zero frame hitches; row 240 boundary between top and bottom screens remains clean with zero white strips or stale pixels.

- [ ] **Floor & Ceiling Scanline Perspective Math**
  - **Location:** Textured rooms (New 3DS / High Quality profile)
  - **Action:** Look at ceiling and floor textures while moving and turning rapidly.
  - **Expected:** Perspective-correct floor/ceiling textures render smoothly with no tearing, integer overflow glitches, or calculation hitches.

- [ ] **Direct $O(1)$ Sound Chunk Cache**
  - **Location:** Rapid combat / Menu navigation
  - **Action:** Fire rapid-rate weapons (Chaingun, Plasma Rifle); rapidly cycle items or menu options.
  - **Expected:** Instant audio trigger response with zero micro-stutters, crackles, or audio-thread hitches.

- [ ] **Music Volume Bitshift Scaling**
  - **Location:** Options Menu $\to$ Audio
  - **Action:** Slide the music volume from 100% down to 0% and back while background music is playing.
  - **Expected:** Clean, smooth volume attenuation without distortion, audio pops, or clicks.

---

## 🛡️ Category 3: System Stability, Controls & Regression Checks

- [ ] **Clean Cold Boot & SD Error Logging**
  - **Location:** Boot sequence / `sdmc:/doomrpg_error.log`
  - **Action:** Cold boot the 3DS, launch `DoomRPG-1.1.1.cia` from Home Menu, play for 1 minute, exit and inspect `sdmc:/doomrpg_error.log`.
  - **Expected:** No false-positive `Failed to read file...` FATAL ERROR entries are recorded during normal boot.

- [ ] **10-Slot Touchscreen Weapon Rack & Dog Companion**
  - **Location:** Bottom screen ($X=1..37, Y=241..450$)
  - **Action:** Tap icons on the touchscreen weapon rack to cycle weapons. Capture a dog using `COLR` on the bottom hotbar, and equip the dog from slot 9 in the weapon column.
  - **Expected:** Instant weapon switching; slot 9 exclusively represents the Dog weapon (`DG`) with hound bark sound effect upon selection (displays `--` when unowned). Dog collar item is used via the horizontal `COLR` hotbar button.

- [ ] **Stereoscopic 3D Depth Slider & Transition Safety**
  - **Location:** Top screen 3D slider
  - **Action:** Adjust the physical 3D slider from 0% to 100% during gameplay. Enter menus, sector transition doors, and notebook.
  - **Expected:** Smooth depth scaling; menus and transition screens cleanly fall back to 2D without eye strain or stereoscopic desynchronization.

- [ ] **Sector Transition Autosave & HUD Integrity**
  - **Location:** Sector airlocks / Doors to new areas
  - **Action:** Walk through an area transition door into a new sector; save the game and reload.
  - **Expected:** Checkpoint autosave completes silently; top status bar and bottom HUD remain fully rendered with no blackouts after saving.

- [ ] **Analog Circle Pad & Button Controls**
  - **Location:** Movement & Combat
  - **Action:** Test movement with Left Circle Pad; test weapon cycling with `X` / `Y` buttons; hold `A` to continuous-fire.
  - **Expected:** Full responsiveness; hold-to-fire buffers smoothly; dialogs scroll with `X` / `Y` or C-Stick nub.

---

## 🎯 Category 4: v1.1.1 Specific Regression & Enhancement Checks

- [ ] **Simultaneous Direction & Attack Input (`Right + A` / `Down + A`)**
  - **Location:** Combat / Grid traversal
  - **Action:** Press `Right + A` or `Down + A` simultaneously, and sequentially in rapid succession while engaging an enemy or facing a door.
  - **Expected:** Player attacks or interacts cleanly. The in-game menu NEVER unexpectedly pops open.

- [ ] **Bottom-Screen Automap Enemy / Kill Counter**
  - **Location:** Bottom-screen automap HUD badge ($X=44, Y=430$)
  - **Action:** Defeat enemies in a sector and observe the statistics badge.
  - **Expected:** Badge reads `MAP: xx% | SEC: x/x | KILLS: x/x` with proper padding. When 100% of both secrets and enemies are cleared, the badge illuminates with a double golden border.

- [ ] **Aggressive Old 3DS / 2DS Performance Profile & Config Migration**
  - **Location:** Options Menu $\to$ Video / Gameplay on 2DS or Old 3DS
  - **Action:** Launch on an Old 3DS or 2DS with an existing legacy configuration.
  - **Expected:** Game boots automatically into `Retro 200` scaling and flat floors/ceilings. Combat and exploration feel noticeably snappier, and Turbo Mode executes turns in rapid succession.

- [ ] **3-Second Notification LED Telemetry Suite**
  - **Location:** Physical 3DS notification LED
  - **Action:** Discover a secret, level up, pick up keycards, defeat a boss, or exit a level.
  - **Expected:** Physical LED illuminates with the corresponding event color for a full 3.0 seconds (3000ms) with high visibility.
