# Changelog

All notable changes to OpenCMR2 are documented here.

## [Unreleased]
### Added
- Scenery patcher: hash-pinned source index for the 45 Greece `tre_*` cards
  (grehit/grelot), converted only with `--all-trees`; cliff cards are pinned but
  never converted. No assets bundled; native look unverified.
### Changed
- Default fog amount is now 65 percent (was 100; same 0..200 percent scale, menu and F10
  0..100). Only unset values change: a saved `video.fog_amount` is kept. The
  `--all-mods`/atmosphere preset also starts at 65 (a preset launch applies its own
  values over saved ones, as before). `--fog-amount N` still wins.
- The "What's new" changelog no longer pops up over the main menu (its OK
  row could sit on top of the menu behind it). It opens from a "What's
  new..." row at the bottom of the main Options menu instead.
### Performance
- Tree wind (sway and rustle) now fades out with distance and is skipped
  entirely beyond a radius (video.tree_wind_radius, default 150 world units,
  0 = unlimited). Dense forest views keep full near-tree motion while the
  far treeline stops paying per-vertex wind math every frame.
### Fixed
- Replay chase camera orbit (arrow keys) no longer dies for the rest of the
  session after the F10 tuning panel is used once: the input-capture gate now
  follows the OPEN adjust panel instead of the sticky "engaged" preview flag.
- Fog lights whose lamps sit at/behind the near plane (bonnet and bumper
  cameras) are no longer dropped: the wedge anchors at the near-plane
  crossing and the beam lights the fog ahead again.
- The fog light depth gate now uses a raw depth-buffer reference computed
  from the game's own projection, so the lamp glow stays off the dashboard
  in cockpit view on both renderers (the gate previously ran only on
  SDL_GPU and only when the projection vote was unambiguous).
- Fog light beams crossing the near plane (rear lamps in chase view,
  headlights in R look-back) no longer draw as a streak toward the
  upper-left corner; the beam wedge is clipped at the near plane and keeps
  its true direction. Beam appearance and intensity are unchanged.

- Fog light beams no longer jump direction frame to frame in replays:
  the wedge now follows the beam's transverse view direction (continuous
  across near-plane crossings) with its length bounded by the foreshortened
  beam, so near-axial beams read as a glow at the lamp instead of a cone
  flipping across the sky. The lamp glow bubble anchors a few units forward
  along the beam - it floats in the lit air in front of the car, and the
  bonnet/dash occlude it via the depth gate instead of being painted over.

- Greece1 roadside brush walls (the flat bsh_sh1 cards near checkpoints)
  get optional draw-time depth (video.brush_relief, default 100): the flat
  quad gains interior alpha relief plus a taller, darkened back layer from
  the same verified cutout sheet, so the straight card silhouette breaks and
  the wall has parallax. Exact-source gated (verified gre01 part identity +
  cutout); originals, contact and collision are untouched. Note: alpha-driven
  displacement is an artistic heuristic (cutout coverage, not recovered
  geometry); the back layer is derived, not original, artwork.

### Added
- Windscreen dirt and wipers (cockpit view): dirt spatter from driving
  builds up on the actual glass area instead of the old flat fullscreen
  tint, rain wets it, and the 3D wiper blades now clear the dirt along
  their real sweep, smearing it to the edge of the arc while wet. The old
  white 2D wiper bar overlay is gone - the animated 3D blades are the
  wipers. Dirt only decays slowly in dry weather, matching the parked
  blades.

- **Precipitation density dial** (`video.weather_particle_amount`, 0..300,
  default 200): rain and snow draw up to 3x the original particle count via a
  port-owned sidecar pool simulated and drawn through the same wrap/streak/
  splash/sway/glint paths. The original 400-slot array, weather records and
  replay rand stream are untouched; 0 hides precipitation (draw skip only),
  100 is exactly the original behaviour. Density scales uniformly across the
  wrap volume (no clustering toward the car).

### Fixes

- **Launch crash on the main menu** (native SIGSEGV, `EXC_BAD_ACCESS` at
  0x8 in `FrontendAnimation_EvaluatePathSpline`, stack from the
  2026-11-01-000237 .ips): the boot render state can draw the
  freshly-entered main menu before its first update ran, and the letter/dot
  animation dereferenced the still-NULL menu path. The menu enter now
  guarantees a path (shared `FrontendMenuPath_Ensure` inline, exercised by
  `tests/frontend_menu_path.cpp`); the update's own branches stay the sole
  writers afterwards, so morphs and the cheat path are unchanged.

- **Native macOS build**: `frontend_whatsnew_runtime_test` used the GNU-only
  `-Wl,--allow-multiple-definition`, which Apple ld rejects, breaking the
  default test build. The interposition isolation test is now behind the
  same GNU/Linux gate as the --wrap isolation tests (substitute-glyph
  evidence harness; no game change).
### Scenery

- **Greece 1 bush/tree sheet relief** (opt-in, `--textures uprez`): the Greek
  stage's flat bush and small-tree cards (bsh_sh1*/tre_s1_*) get the same
  visual-only per-quad relief as the Finland1/France1 forest strips. New
  exact-source 2x cutouts come from the owned grehit/grelot archives through
  the existing deterministic pipeline (12 verified recipes); 51 strip-part
  identities come from the owned gre01hi/gre01lo stage models via the new
  tools/scenery-patch/strip_sources.py extractor (validated to reproduce all
  58 existing Finland1 tresh hashes byte-for-byte). Originals, collision and
  placement untouched; native appearance/performance unverified.

### Camera

- **Wider drone tuning rails** (defaults unchanged): distance 10-300% (was
  50-150), height 25-400% (was 50-150), lateral follow 10-400% (was
  25-200), accel/brake spring 1-400% (was 5-200), speed height drop 0-500%
  (was 0-200). Native tuning screenshots showed distance, height, spring
  and drop pinned at the old rails. Menu rows and runtime clamps move
  together (tests/drone_range_wiring.py).
ed guard:
  chip budget caps live chips at 60% of the pool, the plume keeps a 64-slot
  reserve, and air/speed motes stay evict-first, so the original effects
  keep their space at the defaults. Emission stays on the fixed 25 Hz
  effects step (frame-rate independent, pinned by executable simulation in
  tests/wheel_particle_emission.cpp). Rendered-pixel evidence in
  tests/dust_plume_render.cpp: denser cloud covers more screen and lifts it
  further, dirt tint stays warm, snow stays bright; sampled draw cost is
  printed by the harness (sandbox software GL, relative only - native perf
  unverified). Remaining emitters (air/speed motes, snow glints, weather
  rain/snow, exhaust, wheel splash/spray, skid smoke) keep their original
  amounts. What's New revision 5 (stacks after #222's revision 4).

### Effects

- **Overdrivable tyre debris and dust/snow plume amounts**: the tyre debris
  dial (`--tire-debris`, tuning panel, `[video] tire_debris`) now runs
  0..2000% with a new 800% default (8 chips per wheel per effects step -
  twice the old 400% ceiling), and a new `video.dust_plume_amount` dial
  (tuning panel, `[video] dust_plume_amount`, 0..300%, default 200%)
  scales the rooster-tail emission rate and live cloud cap (96 -> 192 at
  200%). Particle pool 400 -> 1200 (~+86 KiB Win32) with a composed guard:
  chip budget caps live chips at 60% of the pool, the plume keeps a 64-slot
  reserve, and air/speed motes stay evict-first, so the original effects
  keep their space at the defaults. Emission stays on the fixed 25 Hz
  effects step (frame-rate independent, pinned by executable simulation in
  tests/wheel_particle_emission.cpp). Rendered-pixel evidence in
  tests/dust_plume_render.cpp: denser cloud covers more screen and lifts it
  further, dirt tint stays warm, snow stays bright; sampled draw cost is
  printed by the harness (sandbox software GL, relative only - native perf
  unverified). Remaining emitters (air/speed motes, snow glints, weather
  rain/snow, exhaust, wheel splash/spray, skid smoke) keep their original
  amounts. What's New revision 5 (stacks after #222's revision 4).

### Effects

- **Overdrivable wheel spray and exhaust smoke amounts**: `video.spray_amount`
  (0..300%, default 200%) scales the per-wheel spray emission interval
  (interval halved at 200%, never below one step, 0 disables spray), and
  `video.exhaust_smoke_amount` (0..300%, default 200%) repeats the exhaust
  smoke puff per step (1 -> 2 at 200%, capped at 3, 0 disables). Both stay
  on the fixed 25 Hz effects step (frame-rate independent) and inside the
  existing bounded particle pool - no pool growth. Backfire sparks, splash
  particles, skid smoke, weather precipitation (an original fixed 400-slot
  array), air/speed motes and snow glints are unchanged. What's New
  revision 6.

### Effects

- **Particle overdo round 3**: `video.splash_amount` (0..300%, default 200%)
  repeats the wheel splash spawn per wheel-step on standing water;
  `video.backfire_amount` (0..300%, default 200%) repeats the backfire
  spark per step. Existing dials move to overdone defaults per the user's
  overdo-first instruction: air motes amount 100 -> 200 and snow glints
  100 -> 200 (both previously native-tuned to 100; tone down in the
  tuning panel or the Atmosphere menu). All emission stays on the fixed
  25 Hz effects step inside the existing bounded pool. Weather
  precipitation remains original (sidecar pool under investigation).
  What's New revision 7.

### Interface

- **Atmosphere menu directly under Graphics options**: the fog/air
  motes/snow glint rows move up from Advanced graphics > Effect amounts, so
  the fog layer is one click from Graphics options. What's New revision 3
  notes the move. The move exposed a draw bug - the new row's value fell
  into the Graphics options draw switch's default case and rendered with no
  label; `FrontendMenu_DrawGraphicsOptions` now draws it (case 0x10, same
  idiom as the advanced page's case 0x30). Evidence: new
  `tests/graphics_menu_render.cpp` drives the real Graphics options
  build/draw and the real Atmosphere submenu draw through the shared menu
  machine, asserting labels and on-frame bounds and dumping pixel frames
  plus drawn-line dumps (substitute glyphs/fonts - layout, labels and
  wiring proof, not native fonts or native pixels).

### Fixes

- **Broken lamps stop emitting light (both sides)**: the shared type-8 car
  light channel was never damage-gated, so its lamps kept glowing - and
  lighting the fog - with every lamp glass broken. The channel now goes
  dark when both sides' lamp glass is broken. Single-side behaviour is
  unchanged (the channel carries one shared level; per-side gating could
  extinguish an intact lamp), and the original per-side head/brake/reverse
  gating is preserved. Rear lamp glass has no break state in the damage
  model, so brake-light gating remains the original channel behaviour.

### Atmosphere

- **Fog light beams + cockpit fog fixes** (`video.fog_lights`, new
  `video.cockpit_glass_fog` default on): the fog-pass car-light response is
  now a directional wedge from each lamp along its projected beam direction
  (brightest at the lamps, decaying along the beam, with a small halo for
  foreshortened lamps) instead of a radial glow parked at the horizon. A 30%
  floor decouples the beam from the local fog factor, so the near road and
  the cockpit windscreen region show beams; a soft shoulder plus a headroom
  term keep an already bright horizon from clipping to white. The producer
  no longer drops lamps whose source projects off-screen when their beam
  still crosses the frame (cockpit view: your own headlights), keeping the
  behind-camera drop. Interior (cockpit) model meshes draw their
  semi-transparent parts without depth writes (`MESH_FLAG_GLASS_NO_ZWRITE`,
  per-part alpha scan; opaque interior parts unchanged), so the world depth
  behind windscreen glass survives for the fog pass (Atmosphere
  menu row included here so the control never ships without its
  implementation). The wedge is a
  screen-space approximation of lit air, not volumetric road illumination.
  Pure maths in render/fog_math.h, one fog.frag for both renderers;
  pixel-verified on the GL executor harness (beam direction/decay/floor,
  headroom vs bright scenes, no clip at mid-beam, review frames).

### Atmosphere (earlier)

- **Fog responds to car lights** (`video.fog_lights`, 0-200%, default 100):
  head and rear lights visibly cut the fog - radial in-scatter around each
  projected car light in the fog pass, tinted per light and coupled to the
  local fog density, with a depth gate off the car's own body (SDL_GPU; the
  OpenGL renderer keeps the glow unoccluded). Atmosphere menu row, launch
  flag `--fog-lights`, and the `atmosphere` test preset enable it.

- **Snow glints** (`video.snow_glint`, 0-200%, default 100): snow sparkles in
  the light. Snowbanks throw short-lived glint particles where the half-vector
  between sun and view aligns with a jittered micro-facet of the true ground
  normal (deterministic per ground position, so glints stick to the ground
  and flicker as the view moves), and falling flakes twinkle with their own
  sway phase, strongest looking toward the sun (forward scatter). Car head
  and rear lights boost both from the same light field the fog response uses,
  so flakes and banks brighten inside the beams. Render-only; nothing feeds
  physics or the rand stream. Atmosphere menu row, launch flag
  `--snow-glint`, and the `atmosphere` test preset enable it.

- **Weather and motes respond to light** (`video.snow_glint` / `video.air_motes`):
  the screen-space snowfall overlay (both renderers) twinkles toward the sun
  (forward scatter) and brightens inside projected car light beams, tinted per
  light; the floating air motes do the same in world space - sun forward
  scatter computed per step from the view and the stage sun, beam boosts from
  the shared light field. The `atmosphere` test preset is now the durable
  "accepted atmosphere + effects under test" bundle: fog 100 with full sun
  glow, near clear 60, spatial pockets, clearing toward the finish, fog light
  response, snow glints, 500% air motes and god rays 80. Explicit flags still
  override; the experimental `video.fog_cull` stays opt-in.

- **Snow surface material** (`video.snow_material`, 0-200%, default 0 = off;
  prototype): the snow ground itself responds to light. Visible terrain
  triangles are tagged snow only on collision-mesh evidence (the raw surface
  ids the wheel particles use at the triangle centroid, an on-ground height
  tolerance and an upward face normal), a vertex shared with a non-snow
  triangle is never lifted, and tagged vertices get a sun half-vector glint
  plus a cone boost inside car light beams, recomputed from the saved
  original colours every frame. Launch flag `--snow-material`. Terrain beyond
  the collision corridor gets no response; the per-frame vertex-buffer
  refresh cost is unmeasured, hence off by default.

### Rendering enhancements ("juice")

All changes below are optional and gated behind `[video]` settings in
`opencmr2.ini`, exposed as rows in the Advanced graphics menu, so the original
look stays available.

- **Draw distance** — new `video.draw_distance` setting (25–400% of the stage's
  far plane) scales the far plane in `View_SetPlayerProjection`, easing between
  the route's view distances instead of snapping.
- **Debug HUD** — per-frame draw calls / vertices / indices / triangles counters
  plus FPS on the gameplay overlay, controlled by `video.debug_draw` (also the
  debug flag bit 2).
- **Texture filtering** — anisotropic filtering now defaults to 8x (was off as in
  the original) via `video.anisotropy` (Off/2x/4x/8x/16x), implemented in both
  the SDL GPU (Vulkan/D3D12/Metal) and OpenGL renderers with a runtime
  `Gfx_SetAnisotropy()` setter.
- **Breakable trees** — a hard hit sets `STAGE_OBJECT_BROKEN`; the tree stops
  colliding and is no longer drawn. Controlled by `video.tree_break` (default
  on).
- **Camera effects** — dirt on lens (accumulates on shake, decays), additive lens
  flare at screen centre driven by sun visibility, and handheld camera-operator
  jitter in `ChaseCamera_BuildMatrix`. Gated by `video.camera_effects`.
- **Animated bitmaps** — idle scenery (crowds/trees) cycles through its bitmap
  frames behind the video when `video.animated_scenery` is on (default off).
  `STAGE_OBJECT_ANIMATED` flag bit, per-object frame/timer state cleared on
  stage unload, applied to both immediate and deferred draw paths.
- **Motion blur** — blends each frame into the previous one for motion
  ghosting, controlled by `video.motion_blur` (Off/On; blend strength fixed at
  a modest level). Implemented as a full-screen blend pass in the SDL_GPU
  renderer.
- **Bloom** — bright pixels bleed into their neighbours with a soft glow,
  controlled by `video.bloom` (Off/On). Bright-pass extract, separable Gaussian
  blur and additive composite, all in the SDL_GPU renderer.
- **Particles** — rain or snow falling through the scene, controlled by
  `video.particles` (Off/Rain/Snow). Simulated and rendered entirely in the
  port layer, no game-code changes required.
- **Directional shadows** — a shadow map from a directional sun light darkens
  occluded surfaces, controlled by `video.shadows` (Off/On) with a depth bias.
  Rendered into a 1024x1024 depth texture before the normal pass and sampled
  with PCF in the fixed-function fragment shader (SDL_GPU only).
- **Advanced graphics menu** — new rows for Draw Distance, Debug HUD, Texture
  Filtering, Breakable Trees, Camera Effects, Animated Scenery, Motion Blur,
  Bloom, Particles and Shadows, all saved to `opencmr2.ini`.
- **Drone tuning ranges** — the F10 tuning panel's Drone accel/brake spring row
  now goes down to 5% (was 25%) and Drone speed height drop up to 200% (was
  100%), per native testing feedback. Selected values and defaults (100%) are
  unchanged; the spring's critically damped ratio holds at the new floor, and
  the speed drop keeps its 2.2-unit height floor. `--drone-speed-drop` accepts
  0..200 to match.

- **What's new screen** — a short changelog of shipped changes shows once on
  the main menu after an update (tracked in `ui.whatsnew_seen`) and stays
  reachable from Options > Graphics > Screen. Content is curated in
  `game/WhatsNew.h`; `kWhatsNewRevision` is bumped with new entries.

### Related visual work in this batch

- Fog the sky and backdrop of foggy stages (`video.fog`-adjacent far-plane work).
- Dissolve scenery into the sky/backdrop toward the far plane.
- Car reflections drawn only from the sectors they reach; sorted and blended by
  their own distance; background layer drawn without depth after reflections.
- Vectorised arcade minimaps, dotted circuit-selection maps, menu flags, and
  co-driver/menu icons drawn from resolution-independent geometry.
- Animated menu flags and selection arrows preserved.

### Build & platform

- Baseline build fixed on macOS (zlib `Byte` typedef under `TARGET_OS_MAC`).
- Build is clean on the macOS preset
  (`-DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON`).

### Testing

- 30 tests pass (22 original + `graphics_options`, `render_stats`, `tree_break`,
  `camera_effects`, `animated_scenery`, `motion_blur`, `bloom`, `shadows`).
