# Performance Tuning Plan — DoomRPG-RE-3DS

> Goal: free up CPU headroom on the New 3DS so stereoscopy (SBS eye-offset
> raycaster) can run at playable framerate.
>
> Method: line-by-line audit of the actual hot paths in `src/Render.c`,
> `src/DoomCanvas.c`, `src/SDL_Video.c`, `src/DoomRPG.c`. Every claim is
> grounded in code read, with file:line references. No guessed optimizations.
>
> This file is a DEV WORKING NOTE — not committed to git (per project rule:
> generated/dev docs are noise in the repo).

---

## Architecture facts (verified)

- Video surface: `SDL_HWSURFACE | SDL_DUALSCR | SDL_DOUBLEBUF`, 400×480.
  Top screen = rows 0–239, bottom screen = rows 240–479 (`SDL_Video.c:54`).
- **`SDL_FillRect` does NOT paint this surface; `SDL_BlitSurface` DOES.**
  (Established during the white-strip fix. Any future draw must blit, not fill.)
- 3D view (`piDIB`) is wrapped over `render->framebuffer` via
  `SDL_CreateRGBSurfaceFrom` (`Render.c:213`, committed `d236c3a`). The
  raycaster writes directly into `framebuffer`; `DoomCanvas_drawRGB` blits it to
  the top screen every frame. **The 3D scene itself only re-renders when
  `isUpdateView` is set** (movement/rotation) — `DoomCanvas.c:363-377`. So
  static viewing is cheap; turning/moving is the cost spike.
- `DoomCanvas_drawAutomap` had THREE definitions (`DoomCanvas.c:584` `#ifdef
  __3DS__`, `:763` `#else`, `:951` bare orphan in the `#else`). The 951 copy
  double-defined the function for non-3DS builds. Deleted (`4dc0321`); the
  `#ifdef`/`#else` pair remains. 3DS behavior unchanged.

---

## DONE (committed on `perf` branch)

| Commit | What |
|---|---|
| `c1a50a5` | Step 1 — floor/ceiling default to solid (skip per-pixel textured projection). |
| `d236c3a` | Step 2 — piDIB `SDL_CreateRGBSurfaceFrom` aliasing (kill 192 KB framebuffer→piDIB memcpy/frame). |
| `16fe6db` | Step 3 — bottom-screen white-strip fix (per-frame opaque-black blit at y=240). |
| `4dc0321` | Step 4 — delete duplicate `DoomCanvas_drawAutomap` (951 orphan). |

---

## AUDIT FINDINGS — reclassified after a DEEP read (not all are safe wins)

### P1 — Textured floor/ceiling is the dominant cost, but resolution-bound
`Render_renderFloorAndCeilingBG` (`Render.c:1396`) loops one scanline at a time
(`while(top<bottom) top++`), calling `Render_drawplane` (`Render.c:1434`) per
scanline → `Render_spanPlane` (`Render.c:1487`) per-pixel.

**Corrected claim:** the integer division `v16 = (width*v14)/(v15+1)`
(`Render.c:1472`) is computed **once per scanline**, NOT per pixel — so a
fixed-point reciprocal is only marginal. The real cost is the scanline×pixel
*volume* of textured floor projection, which is inherently resolution-bound.
→ Only addressable via internal-res scale (P7). **No non-resolution fix exists
for this.** (Solid floors from Step 1 already skip it where the level uses solid
floors.)

### P2 — `DoomCanvas_drawRGB` blits full 400×240 every frame
`DoomCanvas_drawRGB` (`DoomCanvas.c:1670`) blits `piDIB` (clip 400×240) to the
top screen every frame. The blit is cheap; combined with the fact the 3D scene
only re-renders on move, this is fine. **No action.**

### P3 — `Render_walkNode` BSP traversal — **DEFERRED (unsafe as scoped)**
`Render_walkNode` (`Render.c:1578`) does NOT just collect nodes: it **directly
rasterizes walls** (`Render_drawLines`, `Render.c:1634`) and **depth-sorts
sprites** by `sortZ` (`Render.c:1638-1695`), and its recursion splits on
`viewX`/`viewY` (`Render.c:1700-1708`) while sprite `sortZ` depends on
`viewCos_`/`viewSin_`/`viewTransX` (view ANGLE). So the visible set is
**view-angle dependent**, not just sector-dependent. A sector-keyed cache would
break wall occlusion / sprite ordering the instant you turn — a visual
regression. A correct cache needs keying on (sector + angle bucket) AND
replaying the view-dependent split/sort: a large, risky rewrite. **Deferred.**

### P4 — Span / weapon-sprite routines — **DEFERRED (marginal / unsafe)**
First-pass audit called these easy LUT wins. Closer read shows otherwise:
- The span functions (`Render_SpanMode9`, `Render.c:2791`, etc.) are ALREADY
  table-driven (`mediaTexels[...]` lookups) — no redundant per-pixel math to
  hoist. A 256-entry LUT is marginal.
- `Render_draw2DSprite` (`Render.c:2828`, weapon/flash) re-rasterizes every
  frame, but the berserk/damage flash is **animated on purpose** via
  `DoomRPG_randNextByte` dither. Caching to a static blit would freeze the
  animation. **Deferred** — would regress the flash effect.

### P5 — Asset/texture decode at level load
`Render_addMapTextures` (`Render.c:564`) builds `mediaTexels`, `spanPalettes`,
`planeTextures` once per level, not per frame. Not a steady-state cost.
**No action** unless load times are bad.

### P6 — Stereoscopy headroom target (the reason for this plan)
When SBS stereoscopy lands, the raycaster runs **twice** (left/right eye with a
horizontal view-angle offset). That ~doubles the per-move render cost. The
headroom we need comes from:
- Step 1 (solid floors) — already banked.
- **P7 (internal-res scale-down + blit-up)** — the single biggest lever for SBS
  headroom. Rendering the 3DS view at 320×192 or 200×120 and scaling to
  400×240 is a classic, large, safe win and directly halves raycaster cost.
- P3/P4 were hoped to help but are deferred (unsafe/marginal) — so P7 is the
  realistic headroom source.

---

## Execution status

- ✅ Step 1 (solid floors) — `c1a50a5`
- ✅ Step 2 (piDIB aliasing) — `d236c3a`
- ✅ Step 3 (strip fix) — `16fe6db`
- ✅ Step 4 (dead `drawAutomap`) — `4dc0321`
- ⏸ P1 textured floor — resolution-bound; only via P7.
- ⏸ P3 BSP cache — **DEFERRED: unsafe** (view-angle dependent).
- ⏸ P4 span/weapon LUT — **DEFERRED: marginal/unsafe** (table-driven; weapon
  sprite animated on purpose).
- ⏸ P7 internal-res scale — **DEFERRED per user decision** (resolution change;
  only if SBS proves too slow once 3D works).

**Conclusion:** the only safe, non-resolution perf win was Step 4 (dead code).
The remaining real headroom for stereoscopy is the resolution lever (P7), which
we agreed to gate on actual SBS slowness. **No further speculative
optimizations should be committed** — they would be marginal or regressive.

---

## Guardrails (so this doesn't regress / get lost again)
- This plan lives in `docs/PERF_PLAN.md` (local dev note, not committed).
- The `SDL_FillRect`-is-a-noop / `SDL_BlitSurface`-works fact is documented in
  `docs/BOTTOM_SCREEN_STRIP_POSTMORTEM.md`.
- Any plan longer than "do X then Y" is written to a file BEFORE code is
  touched (this file exists because the original chat-only 7-step plan was
  lost mid-session).
- Each committed step: own commit, diff shown before approval, verified
  on-device. No conversational filler, no AI-slop.
