# Atmosphere: fog and air motes

Optional, default off. Requested as "more atmosphere - motes, fog, light cutting
through it", deliberately overdone first and then brought down. Everything here
is visual only: no physics, gameplay, saves, original assets or default settings
change. Both effects work on the SDL_GPU (Metal) and GL paths.

## What it is

- **Fog** (`video.fog`): one extra full-screen post pass (scene + depth in) that
  runs before DoF and god rays so those stack on the fogged scene. Exponential
  distance fog, a ground-mist height gradient (0.55x at the top of the frame,
  1.45x at the bottom), a sky cap (the sky hazes but never becomes a flat wall,
  max 85 percent blend) and a sun-ward glow that brightens and warms the fog
  around the projected sun position. This is the "light cutting through" dial.
  It is screen-space glow in the fog pass, **not** occluded volumetric light
  shafts - geometry does not carve beams out of it. Existing god rays remain the
  occluded-shaft effect and stack on top.
- **Fog light response** (`video.fog_lights`): head and rear lights cut the
  fog. The per-step car-light update records each enabled car-light glow
  (world position, intensity, tint - the same values the glow sprites use)
  into a small light field (game/LightField.h); the per-frame view draw
  projects up to 8 of them to the screen and the fog pass adds radial
  in-scatter around each, coupled to the local fog density so only foggy air
  lights up, tinted per light (warm headlights, red rear lights). A depth
  gate keeps the glow off the car's own body (SDL_GPU path; the GL renderer
  has no near/far recovery and keeps the glow unoccluded - documented
  approximation). 0..200 percent, default 100.
- **Snow glints** (`video.snow_glint`): snow responds to light. Ground
  samples on snow/ice surface cells spawn short glint billboards (a new
  particle type, 24 live at most, 0.4 s life) when the half-vector between
  the rotated stage sun and the view aligns with the cell's micro-facet -
  the true ground normal from the untouched collision mesh, jittered by a
  deterministic per-position seed (game/SnowGlint.h). Sun strength follows
  the stage's weather light intensity, so overcast snowfall glints less
  than clear-air sun. The same light field as the fog response adds a cone
  boost: samples and flakes inside a head/rear light beam brighten with the
  beam. Falling flakes twinkle per flake from their sway phase (brightest
  toward the sun, forward scatter) and inside beams. Amount scales glint
  rate and flake brightness 0..200 percent, default 100; 0 disables both.
  Render-only: own deterministic seeds, gameplay rand stream untouched.
- **Weather overlay glints** (`video.snow_glint`): the port-layer snowfall
  overlay (OpenGL RenderWeather and SDL_GPU RenderParticles, one shared maths
  header src/render/weather_glint.h) twinkles per flake toward the sun's
  screen position - forward scatter, where real snow glints - and brightens
  inside projected car light beams, tinted per light. 0 disables the overlay
  response; the game-side flake and bank glints share the option.
- **Air motes** (`video.air_motes`): small bug-like specks flying on their
  own around the viewed car, a new particle type in the existing plume pool.
  Base cap 48 billboards at 100 percent, 240 at 500. Each mote gets a
  spawn-time seed (stamped by an init callback) that drives its own smooth
  wander loop - turn rate, speed and a gentle vertical bob - on top of a
  shared drift (light wind plus most of the viewed car's velocity, so the
  swarm travels along instead of the car driving into stationary spots).
  No sink. They use the bright snow-cloud texture, not the dark exhaust
  smoke: the smoke read as black spots against bright fog on native. Spawn/
  step math is pure and unit-tested; they render through the existing
  particle draw path.

## Controls

All settings are [video] options, off unless enabled:

| Setting | Range | Default | Meaning |
| --- | --- | --- | --- |
| video.fog | 0/1 | 0 | fog pass |
| video.fog_amount | 0..200 | 65 | density; default 65 (100 is the heavy look) (23 units ~50 percent fog, 60 ~84, 150 ~99) |
| video.fog_glow | 0..100 | 60 | sun-ward glow strength |
| video.fog_clear | 0..200 | 0 | near-field clear distance in world units: fog ramps from none at the camera to the full curve at this distance, so the road ahead stays readable while distant haze keeps its density. Global in every direction, not a road-only mask |
| video.fog_probe | 0/1 | 0 | one-shot diagnostic (SDL_GPU only): on the 0->1 edge the next fogged frame reads back the fog pass input, output and raw depth over a windscreen-centred region and logs per-cell depth vs expected fog factor vs actual color shift, with a verdict separating residual depth pollution from a weak density curve from a Metal pass/compositing failure. Verdict also rides on the F10 "Fog depth" line. The single armed frame stalls on a readback fence; zero cost while off. Toggle off and on to re-run |
| video.fog_cull | 0/1 | 0 | experimental: shorten the far plane to the fully fogged shell (99.5 percent at the weakest gradient, never closer than 150 units or 1.25x the clear distance). Render-only, physics untouched. Caveats: the far-plane sky cap keeps culled-edge silhouettes 15 percent visible as a haze band, distant trees stop occluding god-ray shafts near the horizon, and any performance saving is unmeasured - this trades draw work for a look change, it is not a proven speedup. With dynamic fog on, culling keys off the CLEAREST stretch the curves can reach, never the current value, so clearing fog cannot pop culled geometry back in |
| video.fog_spatial | 0/1 | 0 | fog pockets along the stage route: dense patches open into clearer stretches (+/-50% of the amount), two fixed sine waves over route progress with a per-stage phase - each stage keeps its own layout, every run and replay of a stage matches |
| video.fog_evolve | 0..200 | 100 | stage-wide trend from start to finish by ROUTE PROGRESS (distance, not elapsed race time - slow driving changes nothing): 100 = neutral, below clears toward the end, above thickens, +/-50% at the extremes |
| video.air_motes | 0/1 | 0 | drifting motes |
| video.air_motes_amount | 0..500 | 100 | count/rate scale (500 = cap 240, "small bugs" look) |

CLI: `--effect fog --fog-amount 100 --fog-glow 60 --fog-clear 60 --fog-cull 1
--fog-spatial 1 --fog-evolve -40 --effect air_motes --air-motes-amount 100`
(--fog-evolve is signed on the command line: negative clears toward the finish,
positive thickens; stored as 0..200 with 100 neutral). The exaggerated starting look is one
command: `--test-preset atmosphere` (fog 100/100 + motes 500 + god rays at
80, per launch, nothing saved; explicit flags win, e.g. `--fog-amount 40`).
God rays are the occlusion-masked light shafts: geometry between the camera
and the sun blocks the shaft, so beams read as cutting through the fog. It is deliberately
separate from `--test-preset overdo`, so the default `--all-mods` launch
keeps its established look. rebuild.sh passes the atmosphere dials straight
through to the game: --fog-amount, --fog-glow, --fog-clear, --fog-cull and
--air-motes-amount (space or = form; they combine with --all-mods). To try it
on top of the usual run:
`./rebuild.sh --update --all-mods --run --test-preset atmosphere` (passed
to the game as `--test-preset overdo,atmosphere`: the usual overdo look
plus the exaggerated atmosphere; presets combine comma-separated).

Menus: Graphics options > Atmosphere... has the fog/mote rows (previously under Advanced graphics > Effect amounts)
(its own submenu because Menu::items is layout-locked at 22 entries). The F10
tuning panel adds the fog clear distance row and applies live. The OSD effect line and
startup log show the active fog/mote state.

## Performance shape (unmeasured)

Fog is one full-screen pass over scene + depth - the same cost class as the
existing grade/god-ray passes, no volumetric marching, no extra scene render.
Motes are at most 48 billboards at 100 percent through the existing particle
path. Actual frame-time impact on the M2 is **not measured**; check the F10
FPS readout. Note the fog pass costs the same at any amount (one full-screen
pass, intensity is just a blend factor) - if it costs too much, the mitigation
is `--effect fog=off` (or the menu/F10 toggle), not a lower amount. Lower
amounts are for the look, not for speed.

## Renderer notes

The SDL_GPU path recovers view distance from depth with the real near/far (same
formula as dof.frag). The GL executor does not have near/far at the post stage,
so it uses a documented raw-depth ramp stand-in (fog_math.h
Fog_FactorRawDepth); the fog band there covers roughly 9 world units to the far
plane. Fog needs a sampleable depth texture, like DoF. Pure maths reference for
both paths: src/render/fog_math.h, mirrored by src/render/shaders/fog.frag and
covered by tests/fog_math.cpp.

## Evidence and acceptance

- tests/fog_math.cpp: density anchors, viewZ round trip, height gradient, sky
  cap, sun glow falloff, raw-depth fallback.
- tests/air_mote_spawn.cpp: ring bounds, uniform disc density, spawn rate
  scaling, drift/wander step bounds, per-seed determinism, fade, pool caps.
- tests/air_mote_pool.cpp: the real production spawn path (Particle_Spawn in
  Graphics.cpp) with a full 400-slot pool: 48 base cap at amount 100, gameplay
  rand stream untouched, wander-seed stamping and drift update through the
  registered callbacks,
  original particles never evicted by motes, motes evicted first by other
  spawns. tests/billboard_queue_wiring.py pins the queue acceptance counters.
  Actual mote pixels in a rendered stage remain unverified.
- tests/gl_effects_runtime.cpp (manual, `python3 tests/gl_effects_runtime.py
  --build build/linux --output out`): production fog.frag through the real GL
  executor on llvmpipe. Off = exact identity, near/cockpit-depth pixels
  unfogged, distant pixels fogged, ground-mist gradient, sky cap keeps 15
  percent of the sky, sun-ward glow localized, amount scaling, alpha preserved,
  plus stacked off/100/200 review frames (out-fog-road.ppm). llvmpipe pixels
  prove appearance and math, not native Metal FPS.
- tests/weather_glint.cpp: phase determinism, sun-proximity falloff and
  aspect correction, twinkle bands, tinted beam boost. Pixel-pinned through
  the real GL executor on llvmpipe in tests/gl_effects_runtime.cpp: same
  flake with a projected beam on vs off, sun on vs off, and snow_glint 0
  disabling the response. (The harness's older god-rays occluder assertion
  fails in this llvmpipe sandbox on this branch and its parent alike -
  environment sensitivity, not a regression from this work.)
- tests/snow_glint.cpp: facet-seed determinism and cell quantization,
  half-vector lobe (peak on alignment, dead off-axis, opposed sun/view
  zero), cone boost (in-beam, behind, past reach, zero direction, zero
  intensity), flake twinkle bounds and determinism, fade envelope.
- Menu wiring covered by tests/advanced_menu.cpp; F10 rows by
  tests/tuning_panel.cpp and tests/tuning_runtime.cpp.

Not proven here: native Mac pixels and frame time, mote appearance in a real
stage, fog interaction with real skybox/weather depths, Metal MSL output beyond
compilation. Those need on-device driving, then dial the amounts down to taste.
