# Stereoscopy Plan — DoomRPG-RE-3DS

> Goal: render the 3D view on the New 3DS TOP SCREEN in real hardware 3D
> (auto-stereoscopic parallax barrier — no glasses). The 3DS top screen IS a 3D
> display; the slider controls eye separation. This is a GPU framebuffer feature,
> NOT an SDL feature.
>
> Method: reference implementations studied from working 3DS homebrew ports:
> - Heretic3DS (elhobbs) — `ctr/gpu.c` `gpuFrameEnd()` (GL/citro3d port)
> - **DXX-3DS (Descent I, iNdioNicarao)** — picaGL top + raw-gfx bottom, real
>   stereo 3D. Closest architectural match to what we need.
> - libctru `gfx.h` docs (authoritative API).
> Web search re-enabled (web.backend firecrawl -> nous) to confirm the API.
>
> This file is a DEV WORKING NOTE — not committed to git.

---

## How the 3DS actually does 3D (confirmed, multiple sources)

**libctru `gfx.h` (authoritative):**
- `void gfxSet3D(bool enable)` — enables/disables top-screen stereoscopic 3D.
- `u8* gfxGetFramebuffer(gfxScreen_t screen, gfx3dSide_t side, u16* w, u16* h)`
  — `side` is `GFX_LEFT` (left eye) / `GFX_RIGHT` (right eye). `GFX_RIGHT` only
  meaningful when 3D is enabled.
- libctru note: "Initially, stereoscopic 3D is disabled and double buffering is
  enabled." → `gfxSet3D(true)` REQUIRED.
- **CRITICAL orientation:** top screen is **240 wide × 400 tall** (portrait,
  rotated 90° CCW). Each eye framebuffer is 240×400 RGBA8.

**DXX-3DS (Descent) — the proven pattern:**
- `d1/arch/3ds/bottom_screen.c` header: *"The top screen is picaGL's and is
  NEVER touched"* by the bottom code. Bottom = raw `gfxGetFramebuffer(GFX_BOTTOM,
  GFX_LEFT)` RGB565 blit. **SDL presents NEITHER screen** — picaGL owns top, raw
  gfx owns bottom; SDL is only input/audio.
- `d1/arch/ogl/gr.c` `ogl_swap_buffers_internal()`: *"When stereo 3D is active,
  the eye pair is already presented inside game_render_frame_mono (two
  pglSwapBuffers: LEFT, then RIGHT+present)."* => stereo = **two presents/frame**,
  one per eye, to GFX_LEFT/GFX_RIGHT.
- `gr.c` (377-381): `gfxSet3D` + `pglSetStereo` set **in LOCKSTEP, only when the
  slider is up**. Setting `pglSetStereo(true)` with `gfxSet3D(false)` *"makes
  picaGL enter its stereo branch against a 2D top framebuffer, which hangs at
  level load."* => **never enable 3D unconditionally; gate on slider.**
- `d1/arch/sdl/init.c`: on APT sleep/wake, `stereo_resume()` resets stereo state;
  otherwise *"stereo_hw_on is stale == 1 ... both screens stay black."* => **must
  handle sleep/wake or hit the same black-screen bug.**

---

## The architecture decision (proven by the references)

**The top screen must be GPU/gfx-owned, NOT presented by SDL.** Our spike proved
this empirically: with SDL still driving the top via `SDL_Flip`, my gfx
eye-buffer writes flashed/fought with SDL (you saw it alternate between the game
and my grid). So SDL must STOP presenting the top screen. Keep SDL only for the
bottom (automap).

This is a **video-layer refactor**, not a drop-in spike. The DXX-3DS split
(top=GPU, bottom=raw gfx, SDL=input only) is the template.

**What applies to US (software raycaster) vs DXX-3DS (GL/picaGL):**
- APPLIES: top screen GPU/gfx-owned; `gfxSet3D(true)` is the enabler; eye buffers
  GFX_LEFT/GFX_RIGHT; gate 3D on the slider; handle sleep/wake stereo reset.
- DOES NOT APPLY: picaGL's `pglSetStereo` only stereo-izes GL draw calls. We have
  no GL — so we render the scene TWICE manually (`Render_render` at angle±sep) and
  copy each into GFX_LEFT/GFX_RIGHT ourselves (the same manual eye-buffer write
  DXX-3DS does for GFX_BOTTOM, applied to GFX_TOP LEFT/RIGHT).

---

## Concrete implementation approach

### Phase 0 — Spike (DONE, validated the conflict)
- `gfxSet3D(true)` + fill GFX_LEFT/GFX_RIGHT + `gfxSwapBuffersGpu()` at init:
  proved the gfx 3D path executes (flash seen at launch). Per-frame version proved
  SDL and gfx FIGHT over the top screen. **Conclusion: disable SDL top present.**
- Spike code is currently in `SDL_Video.c` (`Stereo_spike3D`), per-frame call
  REVERTED (no longer fighting). Keep the function; gate behind a flag later.

### Phase 1 — Stop SDL from owning the top screen
- In `SDL_InitVideo` (3DS): after `SDL_Init`, call `gfxSetDoubleBuffering(
  GFX_TOP, ...)` / ensure SDL's top-surface present is suppressed. The clean way
  (per DXX-3DS): don't rely on SDL's 400×480 surface for the top; treat the top
  as gfx-owned. May require NOT using `SDL_DUALSCR` for the top, or ignoring
  SDL's top present and only presenting the bottom via SDL.
- Risk: SDL's N3DS port may re-present the top on `SDL_Flip`. Need to confirm we
  can present ONLY the bottom. Fallback: present bottom via raw gfx too (like
  DXX-3DS bottom_screen.c), dropping SDL's video surface for both and using SDL
  only for events/audio.

### Phase 2 — Dual-eye software render + transpose to eye buffers
- Allocate two CPU framebuffers (left/right), 400×240 (our logical res).
- Each frame (when `isUpdateView`): `Render_render(... angle-sep)` -> leftBuf,
  `Render_render(... angle+sep)` -> rightBuf.
- Transpose-copy each 400×240 buffer into `gfxGetFramebuffer(GFX_TOP, GFX_LEFT/
  RIGHT)` (240×400 RGBA8). Verify with a known test pattern (no shear/mirror).
- `gfxFlushBuffers()` + present both eyes.

### Phase 3 — 3D enable gated on slider + sleep/wake
- Read `osGet3DSliderState()` each frame; `sep = base * slider`. When slider==0,
  `gfxSet3D(false)` + render single eye to GFX_LEFT (flat). When >0,
  `gfxSet3D(true)` + two-eye render. **Lockstep**: never `gfxSet3D(true)` without
  the two-eye path active (DXX-3DS hang lesson).
- Add APT suspend/resume hook to reset stereo state (`stereo_resume` analog) so
  wake doesn't leave screens black.

### Phase 4 — Eye separation tuning + settings toggle
- `sep` scaled by slider; settings toggle disables 3D entirely (fall back to
  single-eye SDL top render if we kept that path, else single-eye gfx).

---

## Risks / open questions
- **R1 — SDL vs gfx top-screen ownership. CONFIRMED CONFLICT** (spike flashed).
  Fix = Phase 1 (disable SDL top present). This is the central architectural
  change. May need to present bottom via raw gfx too (DXX-3DS style).
- **R2 — Framebuffer transpose (central hard part).** gfx eye buffers 240×400
  RGBA8; our render 400×240 row-major. Transpose + RGBA8 + 240-byte pitch. Verify
  with test pattern.
- **R3 — Double render cost.** Two `Render_render`/move frame. Perf branch banked
  headroom. If slow, pull deferred internal-res scale (only if needed).
- **R4 — `Render_render` global state.** Point `render->pixels` at leftBuf/rightBuf
  per pass; second pass must not corrupt first. May need `Render_renderToBuffer`.
- **R5 — Sleep/wake stereo reset.** Must mirror DXX-3DS `stereo_resume()` or
  screens stay black after sleep.
- **R6 — Bottom (automap) ownership.** If we drop SDL video for the top, the
  bottom may need raw-gfx present too (DXX-3DS does this). Confirm automap still
  draws.

---

## Execution plan (per project rules: own commit per step, diff shown, on-device verify)
1. **Phase 1:** disable SDL top-screen present; prove top is gfx-owned and bottom
   (automap) still works. Resolves R1/R6.
2. **Phase 2:** dual-eye render + transpose to GFX_LEFT/RIGHT; test pattern.
   Resolves R2/R4.
3. **Phase 3:** slider-gated `gfxSet3D` + sleep/wake reset. Resolves R5.
4. **Phase 4:** separation tuning + toggle.

Each step: branch `stereo` off `perf` (done — all 4 perf commits are ancestors),
own commit, diff shown before approval, verified on-device before next. No
conversational filler, no AI-slop.

---

## Reference (verified)
- **libctru `gfx.h`**: https://libctru.devkitpro.org/gfx_8h.html
- **DXX-3DS (Descent I, 3DS, picaGL + stereo 3D)**:
  https://github.com/iNdioNicarao/dxx-3ds — `d1/arch/3ds/bottom_screen.c`,
  `d1/arch/ogl/gr.c` (stereo present + lockstep gfxSet3D/pglSetStereo),
  `d1/arch/sdl/init.c` (stereo_resume on wake).
- **picaGL-3ds** (GL->PICA200 layer): https://github.com/iNdioNicarao/picaGL-3ds
- **Heretic3DS** (GL/citro3d stereo reference): https://github.com/elhobbs/heretic3ds
- Toolchain has `gfx.h` + `gspgpu.h` at `/opt/devkitpro/libctru/include/3ds/`.
