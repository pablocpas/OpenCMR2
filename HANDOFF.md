# Android effects screen (1 November 2026)

Lane: feat/android-effects-gui, Android-only (android/) plus one workflow
change, no game code, physics, originals or desktop files touched. User
request (voice, 1 November 09:17-09:19, phonemsg-01M6AZHQT84GHJS1F2GGQVXPE6
+ follow-ups): "wire each one of them up to the GUI", favourites being the
tunnel-vision blur ring with speed lines, "the air strips that the car
draws" (vapor trails), "the camera angle changing", then maybe fog, and
"definitely the 3D trees" plus "obviously the animated people"; he also
floated disabling F10 on Android and asked what we think.

Design: a launcher Effects screen (EffectsActivity + EffectSettings) stores
tri-state choices (Default/On/Off, Default passes nothing) in their own
SharedPreferences and hands them to the game as launch flags in
GameActivity.getArguments. CLI flags win over opencmr2.ini by design
(Sys_GetOption), so the ini and the desktop GUI are untouched and every
change applies on the next game start, which is every launch from the
launcher. v1 rows: tunnel vision strength 0..200 (off = game default 130), speed FOV
widening 0..100 (default-on-enable 40, matching the atmosphere preset - the
grounded reading of "the camera angle changing", the dynamic-FOV favourite
from the same list; corner tilt/roll is the alternative reading and stays
out until he corrects), radial tunnel lines, vapor trails, fog. Flag syntax
matches src/platform/enhancements.h exactly; the pure argument builder has
JVM unit tests and a Robolectric screenshot test renders the screen to a
PNG that CI uploads as the OpenCMR2-Android-UI artifact (first Android-side
tests; junit/robolectric/compose-ui-test added as test dependencies).

NOT in v1, deliberately: 3D trees and animated spectators need a prepared
scenery pack, and preparing one requires the desktop build (the platform
layer runs Python tooling; without a pack the game refuses to start), so
exposing those toggles on Android would break launch until a pack delivery
route exists. That route is now the scenery-pack import on the effects screen
(lane feat/android-scenery-import): the user prepares the pack with the
desktop build, copies the folder to the device, imports it in the launcher,
and GameActivity passes --scenery-dir. Read-only, save-independent, never
combined with the automatic preparation flags. Camera-angle changes were an ambiguous transcript item and
stay out until the user confirms which cameras. F10 assessment: the tuning
panel is keyboard-driven and unreachable on touch-only Android. Nothing is
removed: launch flags and live tuning are different things - these rows set
starting values, they do not replace adjusting a live race, and the exact
replacement UX for that on Android is not approved yet.

CI: the Android leg now also runs on pull requests that touch android/**
(new android-changes gate job; other PRs skip it, so no extra build time),
and the leg runs the new unit tests. Before this, Kotlin changes were first
compiled after merge. Kotlin was NOT compiled locally (no SDK in the
authoring environment); the PR's android leg is the first compile. Native
on-device appearance and effect behaviour remain UNVERIFIED.

# Fog-light beam/glow geometry repair (1 November 2026)

Update (1 Nov 2026, later): glow bubbles REMOVED on user steering
("keep just the beams"; native screenshots showed the lamp glows still read
as silly bubbles). The fog shader drops the halo term entirely; the wedge
owns the beam from the lamp (0.01 onset ramp replacing the 0.05 halo-owned
gap). CPU-side, halo uniform slots are pinned to the lamp anchor with the
lamp's own gate - layout unchanged for both render paths (single fog.frag
feeds SPIR-V and MSL via spirv-cross). Scene-matrix sweep still CLEAN;
render-test bubble pixel assertions removed with the effect. Native visual
acceptance of beams-only remains OUTSTANDING. Lens flares were raised by the
user as a possible separate optional effect - NOT in this lane.


Lane: fix/fog-beam-true-projection (PR #256), render-only (game/FogBeamProjection.h,
game/LightField.h, tests/). No physics, gameplay, originals, assets or shader
changes; the 35-vec4 FogUniforms layout is untouched.

Player report with 8 screenshots: "headlights still pointing all over, as well
as backlights" - glow bubbles floating above the car in chase view, detached
glows mid-corner, red rear glows adrift, no beam in cockpit.

Root causes (confirmed by an executed scene matrix driving the production
projection with the real race FOV over 6 views x yaw -40..40 x pitch -12..12):
the e65a48d wedge-length formula re-amplified any beam past ~2 degrees off the
view axis to a full-screen wedge up/down/sideways; the 3-unit forward halo
anchor put rear-lamp halos ~1 unit from the chase camera so the gate reference
went negative and the shader silently disabled occlusion; the near-plane clip
landed at exactly z=0x10000 and a 1-ulp round-trip loss turned bonnet/bumper
lamp uv's into the (-100,-100) sentinel.

Fix: wedge length is now the true foreshortened projected beam end (direction
stays the continuous transverse component; ~6-degree axial fade band), halo
anchors sit 1.25 units along the beam with view-depth clamping so the depth
gate is always valid, and the clip margin is 0x10100. Regression coverage:
tests/fog_beam_scene_matrix.cpp (CPU-only, in ctest) fails 508 checks on main,
0 here; tests/fog_beam_render.cpp assertions re-derived from executed values.

Verification state: CPU/synthetic scene geometry only - the shader math is
unchanged but the NEW UNIFORMS have NOT been rendered natively; Metal and GL
share only the CPU-side uniform computation, rendered equivalence is not
verified. The GL pixel harness (fog_beam_render.py) needs a real SDL3/GL
environment and is not wired into CI. Native visual acceptance on the user's
Mac is OUTSTANDING - this is not a completed repair until the user confirms
the look.

# Adjustable Android touch controls (1 November 2026)

Lane: feat/touch-controls-adjust, Android-only (android/), no game code,
physics, originals or desktop files touched. Player-reported pain (upstream
issue pablocpas/OpenCMR2#37, echoed on fork PR #239): pads held off the screen
sides by display-cutout padding, buttons small, no customization.

Adds a touch layout section to ControllerActivity: control size (0.8-1.6x),
distance from screen edges and from the bottom (0-25% each), opacity
(0.2-1.0), a camera/cutout keep-clear switch (default on, the original
behavior) and reset. Settings live in the controllers SharedPreferences as
plain values (touch_scale, touch_margin_x, touch_margin_y, touch_opacity,
touch_avoid_cutout) so a future non-Android overlay can reuse the model.
TouchControls applies them live via a preference listener; pads shrink before
they would overlap on narrow screens. Cutout handling moved from whole-overlay
padding to per-pad side margins, same effect at defaults.

Design follows shipped precedents: Lemuroid (same RadialGamePad library:
scale/margin/rotation settings), PPSSPP (layout editor, opacity) and the
Android 48dp touch-target guidance. Per-button drag placement (PPSSPP-style)
is NOT in this lane: it means replacing the RadialGamePad views with a custom
overlay. No upstream submission. No WhatsNew/CHANGELOG entry, matching the
earlier Android-only commits.

Hit-area floor: exact geometry from the RadialGamePad 2.0.0 source (primary
circle radius = (view - 16dp)/2.652; four buttons partition the whole circle
into nearest-anchor touch wedges, so the real hit area is the wedge, not the
visual circle). At the 0.8 minimum on a 1080p phone (2.625 density) the wedge
is ~51dp - at/above the 48dp guidance; at 0.6 it fell to ~44dp, so 0.8 is the
floor. Visual button diameter is ~36.5dp at the floor on 1080p phones: the
radial four-in-circle layout cannot show 48dp visuals there at any reduced
scale; the wedge, not the visual, is the target. Default geometry is
bit-identical to the old code in landscape (view size and origins, any
asymmetric cutout); portrait clamps differently by design (old code let the
pads overlap; the game is landscape-locked so this is unreachable in-game).

Native on-device appearance and usability remain UNVERIFIED: local
Robolectric pixel renders only, no device or emulator run.

# Integrated asset candidate (30 October 2026)

Claim: feat/runtime-uprez-finland1, current main plus PR182 render-time drone fix.
`--asset-upgrades` selects supported broader tree/bush replacements, animated 2D
spectators, exact-source opaque ground2x textures and Finland1 visual-only curves.
No originals, saves or settings are rewritten. GitHub contains code/metadata only.
All changes remain PR-only; human-led merges. No CI dispatch, release or artifacts.

Golden goal: add audiovisuals/feel, never change physics/gameplay. Finland1
retains original HPC contact, FIN barriers, AI route and surface data. It never
enables the exact-edge physics experiment. Legacy `--roads curved` Ballon is
still a separate unaccepted paired-contact experiment, not part of this preset.

Finland hi/lo kept separate by exact original C3D+quartet hashes. Generated
render retains original vertices, lights, texture tail, sectors, mesh counts,
and visual-only triangles. New points projected onto original terrain; outside
footprint candidates discarded. Private tests: 1036 new points, no outside
points, <=0.009mm height quantization error at those vertices. Changed-triangle centers measured max3.65cm visual/contact height difference;
original collision remains untouched. Native appearance/performance unverified.

Runtime2D is a safe first subset: 82 Finland hi/lo ground sources plus4 France
road sources, exact source/output hashes, complete mips, original UV dimensions.
No whole-atlas foliage/people/font filtering. Those remain original except the
existing dedicated scenery recipes. This is interpolation, not recovered detail.
Existing broader helper generated132 mappings:84 trees/bushes,48 spectators;
76 multi-tree strips and6shadow masks remain original. Barrier/hedge conversion
and source-specific atlas uprez remain open. Greece source recovery remains open.

Local build succeeds; full140suite has135passes and5previous Zig safety aborts
(net_session, service_park_weather, gamepad_layout, frontend_language,
net_frontend). Real owned-source helper dispatch, upload/reset checks and native
C3D relocation/parts/bounds/free probes pass. Native Mac Metal driving, culling,
reflections, appearance and frame time remain unverified. No full-pack completion.

# Handoff - OpenCMR2 rendering enhancements


## Current graphics coordination (29 October 2026)

Owner: current optional graphics work. This section is the coordination record
for these lanes; the completed batch below is historical, not a claim that all
current experiments have shipped. Read this file and `docs/DEVELOPMENT.md`
before changing these areas. No AGENTS.md, CODEX.md, board, roadmap, TODO or
bugs coordination file was present at this revision. Preserve other owners' work.
All patches are PRs. Merges remain human-led. Never commit original game bytes.

| Lane | Current state and next handoff |
| --- | --- |
| ELF wrap isolation gates | Claimed on fix/elf-wrap-test-platform-gate, main3a094f7 after merged PR50/51. Windows Clang is not MSVC, but its COFF lld-link cannot implement GNU ELF --wrap/Itanium symbol interception. Replace compiler-brand guesses with explicit Linux GNU/Clang gate across all eight affected blocks. Keep native Windows portable tests; unsupported isolation harness coverage remains a documented gap. No game change. |
| Tree test standard header | Claimed on fix/tree-test-string-header, main1e2a625 after merged PR49. Windows now links scenery test and game, then tree collision test fails std::to_string because it omitted string header. Add direct standard include, no behavior change. Linux/Android pass, Mac remains queued. |
| Windows scenery test linkage | Claimed on fix/windows-scenery-test-linkage, based merged PR48/main4dd2c90. Windows advanced to scenery_pack_runtime link failure: private-to-public macro changes MSVC access-qualified static symbol names. Replace macro with narrow friend test accessor, preserving real private linkage. No runtime behavior change. |
| Windows dust test follow-up | Claimed on fix/windows-dust-recording-env, based human-merged PR47/main00b4ca8. New first Windows error in dust_volume_recording.cpp: POSIX unsetenv. Use Windows _putenv_s for that test, retaining Unix path. No other unguarded POSIX environment calls found in current tests. Linux native/ARM/x86 and Android CI pass after PR47; Mac pending; Windows requires follow-up CI. No rendering/runtime/default changes. |
| CI portability repair | Claimed on fix/portable-ci-regressions, current main b74c8065. Failed run40840862203: missing optional Pillow in enhancement preparation test, macOS temporary-root symlink fixture, Windows POSIX setenv, x86 stale Particle size assertion after runtime-only dust tag. Repair tests/fixtures only; no native acceptance or workflow/release changes. Validation: isolated full79/79CTests; prepare8pass/1skip without optional Pillow and9pass with it; road contract10pass with symlink-ancestor refusal; actual32-bit Particle header compile-only probe passes. Mac/Windows/fullx86 CI still required; no native gameplay claim. |
| Curved road CLI | PR #45 merged by the owner. `--roads original` and the default retain original geometry. Experimental `curved` is exact-source Ballon only, jointly selects render/contact buffers, and preserves original FIN. Isolated full suite: 79/79 tests; actual binary prepare-only and loader/session probes passed. Native Mac graphics, driving, culling, barrier alignment and frame-time acceptance remain unverified. Do not treat Linux probes or merge as native acceptance. |
| Reactive crowd/person layers | Claimed, local inactive research only. Existing accepted forearm animation stays unchanged. Source-authored transparent person seam and lossless rest partition checked privately. Head tilt/hop needs extra canvas and matched UV/geometry; neck joins, source-cell layout, single-update integration, car-relative orientation, occlusion, reflection and performance remain open. Single-view art cannot supply true head yaw. No runtime activation or ready-to-merge claim. |
| Scene-only grading | Claimed, local inactive research only. Pure snapshot/transaction and compiled shader checks cover viewport bounds, alpha, failure refusal and later synthetic HUD composition. No game caller. Native SDL_GPU/Metal execution/copy failures, stage/weather eligibility and performance remain open. Preserve original full-frame effects and default settings. |
| Bitmap upscale | Retained private candidates only, no runtime installation. Road seam/mip periodic filtering and exact source identity integration remain open. Foliage, people and fonts stay quarantined from generic atlas filtering. Do not duplicate asset-owner processing or publish owned art. |
| Trees/font source gaps | Preserve accepted tree baseline and human-merged PR #44. Missing exact source exports remain separate; no guessed replacement bytes or blanket acceptance. |
| Atmosphere (fog + air motes + beams + fog lights) | PR #202 merged (fog + air motes). Native feedback follow-ups: PR #203 (fog gated to race frames - it washed the Bink splash/menus because both render paths skipped the raceWorld gate; plus PR #201 libstdc++ build fix), PR #204 (docs: fog_amount is a look dial, not a perf mitigation), PR #205 (atmosphere preset now enables god rays at 80 - the occlusion-masked shafts were built but never in the preset; new video.fog_clear near-field clear distance 0..200 world units, global not road-only, default off; motes alpha 130->190; then native 'smaller and more plentiful, like small bugs': size 0x3800->0x2000->0x1000 (native 'much smaller' after testing the 0x2000 build), amount ceiling 300->500 (cap 144->240, pool budget half->three-quarters, motes still evicted first so original particles keep priority), per-speck overdraw down, count up, net FPS unmeasured; then native 'bugs or dust specs that float and fly around on their own instead of looking like black spots we drive into': mote texture swapped from dark exhaust smoke to bright snow cloud (the black-spot look against bright fog), per-mote seeded wander flight (own turn rate/speed/bob, no sink) on top of a shared car-following drift, deterministic per seed, render-only), plus experimental opt-in video.fog_cull far-plane clamp (99.5 percent fog shell, 150-unit and 1.25x clear-distance floors, sector culling keys off the far plane, route easing preserved, render-only; 15 percent far-plane silhouette band caveat documented). Dynamic fog (video.fog_spatial pockets along the route, video.fog_evolve start-to-finish trend by route progress - both per direct user request, deterministic per stage, replay-stable, reversible; fog_cull keys off the clearest reach so clear stretches cannot pop in; prototype pending native look). rebuild.sh passes the atmosphere dials through to the game (--fog-amount/--fog-glow/--fog-clear/--fog-cull/--air-motes-amount, combining with --all-mods; the gap bit a user launch command before this). Correction to the earlier 'beams never enabled' note: --all-mods sets --effect god_rays=on at the default amount 39, so combined launches had weak rays; the atmosphere preset raises the amount to 80. fog.frag verified with real pixels on llvmpipe incl. clear-distance and rays-occlusion assertions; suite 151/152 with only the pre-existing replay_controls_wiring failure. Native Mac Metal pixels, FPS and mote/beam appearance remain unverified. Open: native FPS measurement (user A/B, minimal specific ask once build ready). Fog lights (video.fog_lights): car head/rear lights cut the fog - light field recorded from the glow update, projected per view, radial in-scatter in fog.frag coupled to local fog, depth gate on SDL_GPU (GL unoccluded approximation); suite 153+/154 with only the pre-existing replay_controls_wiring failure; fog.frag re-verified to translate to MSL with the project's spirv-cross flags. Snow glints (video.snow_glint): snowbanks sparkle via a true micro-facet half-vector model on the untouched collision normals, flakes twinkle with forward scatter, car light beams boost both from the shared light field; suite 154/155 with only the pre-existing replay_controls_wiring failure; native pixels unverified. Weather overlay glints + mote light response (video.snow_glint/video.air_motes): port-layer flakes and air motes twinkle toward the sun and brighten in car beams, shared light field; pixel-verified on llvmpipe via the GL executor harness (beam/sun/disable assertions pass; the harness's older god-rays occluder assertion fails in this sandbox on parent commits too - llvmpipe environment issue to recheck natively, not a regression); atmosphere preset is now the durable accepted+under-test bundle (fog_clear 60, spatial 1, evolve -40 folded in; fog_cull stays opt-in). Details: docs/ATMOSPHERE.md. Snow surface material (video.snow_material, prototype, default off): the snow GROUND itself responds to light - visible terrain triangles are tagged snow only on collision-mesh evidence (raw surface ids 0x17/0x29/0x18 at the triangle centroid, on-ground height tolerance, upward face normal; misses and non-snow stay untouched, so banks beyond the collision corridor and trees/cliffs/buildings never lift), a vertex shared with any non-snow triangle is never lifted (tagged triangles cannot bleed into neighbours), and tagged vertices get a sun half-vector glint (merged SnowGlint maths, weather-gated) plus a car-beam cone boost from the light field, written into the per-vertex diffuse channel and refreshed per frame; colours recompute from a saved original every frame (no accumulation) and restore on option-off/stage exit. Rendered-pixel verified on the GL executor (tests/snow_material_render.cpp/.py): quads coloured by the same SnowMaterial.h inlines the game calls, drawn through the real gfx_core -> GL fixed3d path and read back - sun-facing lifts by the computed response, beam boost lifts toward peak, option-off and view-facing-away are exact identities, the shared-edge case glows only on the snow side, restore returns the baseline exactly; review frames dumped. MeshVertexF/GfxVertex layout is also static_assert-locked, and both backends compile the same fixed3d.vert that shades color1 (snow_material_wiring); native Mac Metal/OpenGL pixels remain unverified. Range 0..200 like the other amount dials; no perf claims (whole-mesh refresh cost unmeasured, hence default off). Suite 157/159 with only the pre-existing replay_controls_wiring failure (+ net_session sandbox flake, passes isolated); native Mac Metal/OpenGL pixels unverified. Particle overdo (in review, feat/particle-overdo): per direct user request to overdo ALL particle FX first and tone down later - tyre debris dial 0..2000% default 800% (8 chips/wheel/step, 2x the old 400% ceiling; count, not size) and new video.dust_plume_amount 0..300% default 200% (plume rate + live cloud cap 96->192). Pool 400->1200 (~+86KiB Win32, Particle=108B pinned); composed guard: chips 60% budget, plume 64-slot reserve, motes evict-first; emission on the fixed 25Hz step (frame-rate independent, executable-simulation pinned). Rendered-pixel harness tests/dust_plume_render.cpp/.py (GL executor, white-texture substitute for the cloud art): density, warm dirt tint, bright snow pins pass, sampled draw cost printed (sandbox-relative). Remaining emitters (air/speed motes, snow glints, weather, exhaust, splash/spray, skid smoke) untouched - scope inventory in the PR. Native perf/pixels unverified. Merged as #223. Follow-up (in review, feat/more-particle-overdo): wheel spray interval scaling (video.spray_amount 0..300 default 200, min one step, 0 off) and exhaust smoke puff repeat (video.exhaust_smoke_amount 0..300 default 200, cap 3, 0 off) - both 25Hz-step-based, pool unchanged, backfire sparks/splash/skid/weather-precipitation (original fixed 400-slot array, enlarging it is an original-data-layout change, deferred) untouched. What's New rev 6. Merged as #226. Round 3 (in review, feat/particle-overdo-3): splash spawn repeat + backfire spark repeat dials (same 0..300 default-200 shape), air motes and snow glint defaults raised 100->200 (previously native-tuned to 100; overdo-first instruction wins, tone-down path documented). No distinct skid-smoke emitter exists beyond spray/trail coverage - reported, not invented. Weather precipitation: original fixed 400-slot array; port-owned sidecar/overflow approach under investigation to avoid the ABI change. Rendered evidence for the round-2 rate dials in tests/particle_rate_render.cpp (PR #227). What's New rev 7. |
| Atmosphere GUI (fog menu on Graphics options + fog light beams) | PR #217 (feat/fog-light-beams, open): directional fog light beams - the fog light response becomes a wedge-shaped beam in front of head/rear lamps (axial falloff, never shines backward, halo near the lamp, 30 percent of the previous radial response as floor), producer keeps lamps whose projected beam crosses the frame (cockpit own-headlights fix), and MESH_FLAG_GLASS_NO_ZWRITE (0x100000) plus a per-part semi-transparent alpha scan skip z-writes on cockpit glass so the fog pass does not black out interior transparency (video.cockpit_glass_fog, ini-only, default on - the real-asset glass attribution stays a labeled hypothesis; the synthetic harness proves the mechanism, not that real car interiors use offending glass polys). Atmosphere menu glass row ships inside #217. Pixel evidence: GL executor road frames, beam corridor lifts, outside-beam unchanged. PR #218 (feat/fog-gui-menu, open): Atmosphere submenu moves from Advanced graphics > Effect amounts onto the Graphics options page (user request; 9 rows, under the 22-item layout cap; the advanced menu is exactly at cap so the fog toggle could not join it). Building the required render harness caught a real bug: the new row's value hit the draw switch default and rendered BLANK - fixed via FrontendMenu_DrawGraphicsOptions case 0x10 (same idiom as the advanced page's case 0x30). tests/graphics_menu_render.cpp renders the real Graphics options page and the real Atmosphere submenu through the shared menu machine with label/bounds assertions and pixel + drawn-line dumps; block-glyph and DejaVu-substitute-font frames reviewed (substitute fonts - not native fonts or native pixels). No merge-order dependency: #218 has no glass row, #217 carries it. Suite 161/162 with only the pre-existing replay_controls_wiring failure. Native Mac menu pixels and cap/readability acceptance remain unverified. |

## Replay chase arrows dead after F10 (1 November 2026)

Claim: fix/replay-chase-arrows, based main 00e2a5f, todo replay arrow-key
input (user screenshot: type5 drone chase replay, arrows dead). ROOT CAUSE:
RallyData.cpp zeroed the orbit input whenever TuningPanel_PreviewActive() -
but that predicate returns g_tuneEngaged, STICKY after the first F10 press of
the session ("the tuned look stays live"). One F10 use killed replay chase
arrows until restart; the user's on-screen debug readout proves tuning was
engaged. The orbit machinery itself was always fine (replay_chase_runtime
drives mode 5 drone variant green - the harness calls View_ReplayChaseFrame
directly and never exercised the gate). FIX: both gates (KeyIsFreeCamKey and
the Update zero-path) now use ReplayChaseOrbit::InputCaptured(menuOpen,
TuningPanel_IsOpen()) - the OPEN adjust panel still captures arrows, matching
Input.cpp's own Tuning_FilterKeys(IsOpen) precedent; the sticky preview no
longer does. tests: replay_chase_orbit gains InputCaptured policy checks;
replay_controls_wiring.py had TWO stale substring expectations (the
View_ReplayChaseReset addition and the Input.cpp:748 filter mechanism) -
updated to pin the current correct mechanism, so the long-standing permanent
failure is genuinely resolved: full suite 170/170, ZERO failures (first
green). LOCAL ONLY per main 1:26; WhatsNew rev 11 collides with
#235/#239/motes rev 11, renumber at survivor rebase.

## Tree wind distance falloff (1 November 2026)

Claim: feat/tree-wind-radius, based main 00e2a5f, todo rainy replay 14-22 fps
(user's rainy chase replay: scenery CPU 17.7 ms, wind math 11.17 ms,
2,018,592 rustled verts/frame, executor encode+submit 26.2 ms, swapchain wait
17.1 ms). Cause boundary: the replacement-tree path has NO distance handling -
every replaced tree draws at full detail and pays per-vertex wind math every
frame; dense Finland forest pushes 672,864 tree tris/frame. The fix adds
video.tree_wind_radius (world units, default 150, 0 = unlimited): wind
amplitude (WindFrame.ampScale, scales BOTH sway offset and gust rustle) fades
linearly from full at 60% of the radius to zero at the radius, where the tree
takes the existing STATIC draw path and skips WindPart entirely - no feature
removal, near trees move exactly as before, no sway pop (fade band).
SceneryPack_BeginBatch now takes the camera eye from the Game.cpp caller
(g_viewNodes stays out of SceneryPack so tests link it standalone).
tree_rustle/tree_wind/tree_wind_radius remain config-only options, no menu
row (advanced page layout-locked at 22 rows, amounts page capped at 20, both
full). Counters unchanged; the rustled-verts OSD line now shows the drop.
tests/tree_wind_radius green; suite 170/171 with only the pre-existing
replay_controls_wiring failure. One frame does not prove the whole slowdown;
executor encode (2.3M verts) and GPU swapchain wait point at total tree tri
volume as the next lever (distance LOD to original cards), deliberately NOT
bundled here.

## What's New changelog (31 October 2026)

PRs #211/#212/#213 (fog lights, snow glints, weather/mote light response +
durable atmosphere preset) merged by the owner at 21:33 EET; main is 673cb4a.
feat/whats-new (#215) and feat/drone-tuning-range (#214) rebased onto it;
suites 157/158 and 155/156 with only the pre-existing replay_controls_wiring
failure.

Claim: feat/whats-new, based main 673cb4a. A "What's new" screen shows once
per content revision when the main menu is entered (ui.whatsnew_seen in
opencmr2.ini; recorded when queued so a crash cannot loop it) and stays
reachable from Options > Graphics > Screen ("What's new..." row, localized
labels). Content is curated literal lines in game/WhatsNew.h - bump
kWhatsNewRevision and add lines whenever user-visible changes ship; only
merged, truthful changes. Screen options items are looked up by value, so the
added row disturbs nothing; the auto-show hook is one marked line in
FrontendMenu_EnterMain using the same Menu_SetNextAction path as attract
mode. Pure-logic pins in tests/whats_new.cpp; suite 153/154 with only the
pre-existing replay_controls_wiring failure (net_session flakes in the full
run, passes isolated). Menu wiring, fonts and the transition have no native
or GL coverage - needs a native look. 

Update the relevant lane with tests, blockers and next handoff as it changes.
Do not overlap these claimed lanes silently or combine inactive experiments into
a road follow-up. Changes to eligibility/defaults need their own reviewed scope.

## Fog light beams + cockpit fog (31 October 2026)

Claim: feat/fog-light-beams, based main d8c25d0. Replaces the radial,
fog-factor-coupled car-light glow in fog.frag (the horizon blowout + dark
near-road mechanism) with a directional screen-space wedge per lamp
(brightest at the lamps, axial ramp so it never shines backward, halo for
foreshortened lamps), a 30% floor decoupling beams from local fog (near
road + cockpit glass show beams), soft shoulder + headroom so a bright
horizon cannot clip to white. Producer projects each beam (pos+dir*30 world
units) and keeps lamps whose beam crosses the frame (fixes own-headlights
drop in cockpit view; behind-camera drop kept). Interior cockpit meshes get
MESH_FLAG_GLASS_NO_ZWRITE (bit 20, port-side) and the parts draw paths skip
z-writes only for parts with real semi-transparent vertices (per-index alpha
scan; opaque parts and interior self-occlusion unchanged), gated by ini-only
video.cockpit_glass_fog (default 1) - the windscreen-glass poly cause stays
HYPOTHETICAL until verified against real a5/c5 interior assets natively.
The wedge is a screen-space approximation of lit air, NOT volumetric or
world-space road illumination. FogUniforms grows to 23 vec4 (beams at
offset 76) on both backends; GL keeps viewZ=0/no depth gate. Tests:
tests/fog_math.cpp beam/shoulder/headroom pins; gl_effects_runtime drives
the production fog.frag - beam brightens along direction, decays, no
backward shine, floor lights clean air, headroom dims bright scenes,
mid-beam stays unclipped, review frames frame-fog-beam.ppm +
frame-fog-road.ppm (4th frame = beams) dumped and inspected. Suite passes
with only the pre-existing replay_controls_wiring failure and the known
llvmpipe god-rays occluder environment failure (fails on parent commits
too). Native Mac Metal pixels unverified; interior transparency/sorting
regression check needs a native cockpit run with real assets.

## France1 first asset test

Claim: feat/france1-asset-test. User selected France1 first. Exact hi/lo quartets
recovered and verified; hi matches existing Ballon hash, lo differs and currently
fails old unique-contact-triplet converter guard. Keep lo road original, do not
relax it. Opaque t_1a/b2x periodic-filter/mips only, exact four high/low source
hashes, CLI --textures original|france1, stage-name+originalC3D gate, originals
preserved. No restored detail claim. People/tree motion is now mergedPR54.
Hedge/bank depth proposal accepted; regional strips are not
yet bound to screenshot's actual mesh. Native appearance/performance unverified.

## Current request: full supported scenery motion

Claim: feat/stronger-supported-scenery, based main7efec63. Keep accepted source-fit
3D tree geometry and authored arm frames. All verified spectator placements now
animate (remove Finland65%placement gate), plus fixed-feet geometry sway moves
all people on each supported card rather than one arm on a multi-person atlas.
Tree crown amplitude increases1.5%to4.5%height, applies to fitted replacement
geometry too, with fixed base and immutable source/cached vertices. Same stage
simulation clock and deterministic phase; no render-clock or physics RNG use.
This does not claim every game's asset/track is source-covered or add unrelated
object motion. Native culling/reflections/appearance/performance remain open.

## Deliberate custom-camera operator

Claim: feat/deliberate-camera-operator on main15308db. Existing camera_effects
is shake, not anticipation. New opt-in Human operator row in actual custom
camera menu saves separately on accept, Cancel does not save. Only custom
mode4 gets bounded, acceleration/slew-limited orientation lead from heading
trend and actual travel direction. No position movement, random jitter or
extra spring lag. Existing height/distance/pitch/lag and terrain-clearance
paths retained; operator suppresses old chase jitter only while active.
State per camera index, reset start/restart/disabled/discontinuous heading.
Source and actual update/apply harness tests pass; full83/83 Linux tests on
merged main ddeec08. Native feel/menu/seek/split-screen acceptance unverified.

## Historical completed batch

## Summary
User-adjustable draw distance, in-GUI debug stats (FPS + draw calls + vertices), expanded Advanced graphics options ("juice"), breakable trees, camera effects (dirt on lens, lens flare, human-operator shake), animated idle bitmaps (crowds/trees), motion blur, bloom, rain/snow particles, and directional shadows are all implemented. Everything is gated behind `[video]` settings in `opencmr2.ini` and marked `// OpenCMR2:` per the README.

## Status
- Baseline build fixed (macOS zlib `Byte` typedef was undefined under `TARGET_OS_MAC`; `game/zlib/zconf.h`).
- All 27 tests pass (22 original + `graphics_options`, `render_stats`, `tree_break`, `camera_effects`, `animated_scenery`).
- Build is clean on macOS preset (`-DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON`).
- Handoff complete: nothing in this batch remains outstanding.

## Completed changes
| Area | Files | What |
|------|-------|------|
| Draw distance | `game/FixedPoint.cpp`, `game/FixedPoint.h` (removed), `game/GraphicsOptions.h` | `video.draw_distance` 25–400% clamp, scales far plane in `View_SetPlayerProjection` |
| Debug stats | `src/render/gfx_core.cpp`, `src/port/gfx.h`, `game/Graphics.cpp/.h`, `game/Game.cpp`, `game/RallyData.cpp` | per-frame draw calls/vertices/indices/triangles; `Graphics_DebugDrawWanted()` (debug flag bit 2 OR `video.debug_draw`); FPS overlay now honors setting; draw calls + vertices on gameplay overlay |
| Texture filtering | `src/render/gfx_gpu.cpp`, `src/render/gfx_gl.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h` | anisotropic default 1→8, always requests feature, `Gfx_SetAnisotropy()` runtime setter |
| Breakable trees | `game/StageObjects.cpp`, `game/Game.cpp` | hard hit (>0x20000 speed) sets `STAGE_OBJECT_BROKEN` flag bit; tree stops colliding and is no longer drawn; default on via `video.tree_break` |
| Camera effects | `game/FixedPoint.cpp`, `game/RallyData.cpp`, `game/Race.cpp`, `game/GraphicsOptions.h` | dirt on lens (accumulates on shake, decays), additive lens flare at screen centre (sun visibility), handheld camera jitter in `ChaseCamera_BuildMatrix`; gated by `video.camera_effects` |
| Advanced graphics menu | `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | rows: Draw Distance, Debug HUD, Texture Filtering (Off/2x/4x/8x/16x), Breakable Trees, Camera Effects, Animated Scenery, Motion Blur, Bloom, Particles, Shadows; saved to `opencmr2.ini` |
| Pure helpers | `game/GraphicsOptions.h` | clamp/round-trip helpers for draw distance, anisotropy, lens dirt, shake amplitude, flare brightness, motion-blur mix, bloom intensity, shadow bias |
| Tests | `tests/graphics_options.cpp`, `tests/render_stats.cpp`, `tests/tree_break.cpp`, `tests/camera_effects.cpp`, `tests/animated_scenery.cpp`, `tests/motion_blur.cpp`, `tests/bloom.cpp`, `tests/shadows.cpp` | TDD for clamp mappings, renderer counters, broken flag, dirt/shake/flare helpers, anim frame advance + texture indexing, blur/bloom/bias mappings |
| Animated bitmaps | `game/GraphicsOptions.h`, `game/Game.cpp`, `game/Sector.cpp`, `game/Graphics.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | `video.animated_scenery` Off/On row in Advanced graphics (default off). `STAGE_OBJECT_ANIMATED` flag bit; per-object frame/timer state in a global table (cleared on stage unload); `AnimObject_Texture()` swaps the current cycle frame into the draw path (immediate and deferred); menu row saved to `opencmr2.ini`. |
| Motion blur | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/motionblur.*`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/motion_blur.cpp` | `video.motion_blur` Off/On row; full-screen blend of current frame into previous frame (mix factor from strength); SDL_GPU implementation, OpenGL stub |
| Bloom | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/brightpass.frag blur.frag composite.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/bloom.cpp` | `video.bloom` Off/On row; bright-pass extract, separable Gaussian blur, additive composite into the scene; SDL_GPU implementation, OpenGL stub |
| Particles | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/particle.vert particle.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | `video.particles` Off/Rain/Snow row; simulated and rendered entirely in the port layer (screen-space billboards), no game-code changes; SDL_GPU implementation, OpenGL stub |
| Directional shadows | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/common.glsl fixed.frag fixed3d.vert fixedtl.vert shadow.vert shadow.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/shadows.cpp` | `video.shadows` Off/On row + depth bias; 1024x1024 shadow map from a directional sun light rendered before the normal pass, sampled with PCF in the fragment shader; SDL_GPU implementation, OpenGL stub |

## Key references
- `StageObject` struct: `game/Sector.h:36` (disk record 0xa0 bytes; `pMesh` at 0xc, `flags` at 0x10)
- `SectorObjectEntry`: `game/SectorObjectEntry.h:20` (runtime; `pObject`, `pBounds`)
- Collision/draw share `StageObject` instances (RallyData.cpp:7422 draw list build, 7458 collision entry)
- Draw loop: `Game_DrawStaticStageObjects` Game.cpp:2321; object draw via `Graphics_DrawMeshLOD` or queued `StageObjectDraw`
- `g_sunVisibility` (StageObjects.cpp:6156, set at 6814) for flare brightness
- Settings: `Sys_GetOption`/`Sys_SetOption` (src/platform/sys.cpp:1204)

## Build/test commands
```
cmake --preset macos -DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON
cmake --build build/macos -j 8
ctest --test-dir build/macos
```

## CI execution efficiency handoff

Claim: ci/deduplicate-native-checks-final. Preserve six native build targets,
existing tag release dependencies and artifacts. Remove feature-push/PR duplicate
execution, ignore docs-only automatic changes, cancel obsolete PR checks only,
and bound jobs/individual CTests. Manual full checks remain available. No new
cache/storage/upload permissions. Six policy tests and actionlint pass; revised
workflow execution remains unverified. Existing credential cannot write workflow
files, so this change is a user-applied patch on a separate branch/PR, not a
published workflow. Windows portability repairs remain separate PRs. Revert
this one commit through a PR if the execution policy needs to be rolled back.


## Current requested steering

Claim: feat/wiper-w-toggle. User requests stronger people/tree motion, all-track
rounding, W wiper toggle and actual in-app remapping. Wiper change is isolated:
physical W default, saved explicit key retained, rising-edge continuous toggle
independent of dirt strength. Does not clear any driving bindings. W may conflict
with user WASD presets; current source-only review cannot inspect their saved
controller file. In-app original driving reconfiguration exists through Controls
and device configuration, but wiper is outside its nine actions. Controls now adds a separate wiper-key capture row, with release-before-capture,
Escape cancel and refusal of driving-key conflicts. Implicit W is disabled when
active keyboard driving uses W, rather than stealing acceleration; explicit saved
choices are retained. Native menu pixels/navigation remain unverified.
All-track rounding remains source-gated Ballon only until paired source/layout
and FIN/native validation can extend safely. No arbitrary object animation.

## Cockpit W toggle failure follow-up

Claim: fix/cockpit-wiper-toggle. Native owner report: 3D blades always on in
rain and W has no effect. Confirmed source mismatch: PR53 toggles the added
screen-dirt overlay; CarInterior_UpdateWiperSweep independently re-enables the
actual cockpit blade modes from rain and ignores that option. Fix the shared
control path and test actual rain/press/held/release/off/park/on transitions,
not only a pure bool helper. Preserve rain-dependent speed, CIN blade limits,
remapping and driving conflicts. Native behavior remains failed acceptance
until a new build is tried. Dust cost regression is the other current priority;
hedge/bush/grass additions remain saved and paused, not lost or complete.
## Driving remapping menu reachability

Claim: fix/driving-bindings-menu. Native report says only Wiper rebind is
available. Source reproduction: Controls exposes two opaque controller rows;
the device configuration's sole link to ten existing binding actions is labeled
Back (localized 0x67). Selecting apparent Back actually enters bindings, so code
existence was not a usable UI. Make that action explicitly Rebind driving
controls, label the bindings save action and player rows. Test constructed menu
links and actual action traversal/capture/save. Native pixels/navigation still
need owner's check; do not tell them an unverified hidden path is usable.
## Heavy dust cost follow-up

Claim: perf/dust-shared-detail-noise. Native screenshots show lower FPS during
heavy dust; varying camera/time and recording mean this is not a controlled
bottleneck measurement. Inspect actual kernel and reduce repeated work without
changing density/shape first. Shared world detail noise is independent of lobe
and now computed once per ray sample before the lobe loop. The lazy branch
variant was byte-identical but slightly slower on llvmpipe, so was rejected. Preserve depth/HUD,
fallbacks, sample counts/defaults and compare actual shader pixels, not source
syntax alone. Mac same-camera/weather frame-time acceptance remains required.
Wiper PR57 and Controls PR58 stay separate; scenery additions paused.

Local synthetic240x160 draw-query evidence, not Mac gameplay FPS: interleaved
40timed samples each, dense32lobes original median6.410ms vs shared5.157ms;
sparse3smalllobes original.0943ms vs shared.0856ms (noisy short draws).
These measure llvmpipe/software rasterization only.120 before/after actual
compiled-kernel outputs byte-identical across dense/empty/occluded/orbit cases.
No sample-count, shape, pool, compositor, default or fallback change. Native
same-camera/weather heavy-dust comparison remains open.

## Helicopter camera operator refinement

Claim: fix/helicopter-camera-framing. Owner screenshot/steering asks stable
helicopter following, occasional late reframing after a turn and slight
overshoot, not continuous anticipation or shake. Test bypass of original spring
only while customized operator mode is on; never overwrite saved lag. Native
label loses uppercase H/O; inspect font glyph path, use menu-consistent lowercase
and verify actual pixels once exact owned font pair is available. Native feel,
projection and menu acceptance stay open. Dust59 and cockpit57 remain separate.

Refinement validation:86/86Linux tests. Actual ChaseCamera_Update confirms custom
operator spring bypass and immediate original-lag restoration while keeping saved
lag unchanged; temporary isolated INI only. State fixture confirms mirrored turns,
cut/NaN/dt resets, per-view independence, bounds, return to stable framing and a
small single overshoot. Synthetic .3rad/s turn after calm produces -8.4deg lag,
+.32deg overshoot, then zero; not a native gameplay claim. Label regression calls
actual Font_DrawText with a deliberately lowercase-only glyph map: old uppercase
H/O vanish, new lowercase label emits both. Original font pair unavailable, so
this is a source-path regression, NOT native-font/pixel acceptance. Loader slot1
is Fonts/general/hel_15pt in frontend, hel_12pt in-race (.pcf+.tga). Native
menu and helicopter feel must still be checked in owner build.

## PR59 CI repair follow-up
All failed Linux job logs show recipe tests missing PIL. Exact desktop workflow
requirements-install patch held separately because credential lacks workflow
scope. Linux CI dependency gap remains open, not fixed by skipping tests.
Windows enhancements_test link failure came from redefining private as public,
changing static member mangling. Use existing public SetGameHDPath, same save
assertions. Explicit string headers repair wiper and camera Windows tests.
Both human-merged camera/wiper and dust notes retained above. Native CI pending;
no merges, dispatches or reruns performed.

## Accepted graphics CLI options follow-through
Claim: fix/graphics-cli-options. User command used --textures=france1, silently
passed through rather than selecting texture preparation. Accept equals syntax
for recognized valued enhancement flags, preserving separate-token form. AA
CLI overrides were accepted/logged but GPU constructor read saved-only settings;
use the same override-aware startup selector in constructor and runtime tests.
No saved defaults/profile change. Test full user's combined flags with synthetic
helpers, not proprietary artwork or a claimed native renderer acceptance.
Validation87/87Linuxpass: full user's combined argument form through actual binary
with isolated synthetic helper outputs dispatches textures/roads/scenery/cars.
Invalid/empty equals values fail. Actual Platform_Init per-launch AA request feeds
same GPUStartupOptions::Read used by GPU constructor, saved-only reader stays0,
INI remains byte-identical and subsequent no-flag launch restores saved0.
No native frame or active AA/MetalFX success claim. MetalFX still needs compatible
upscale/extension and reports encoded/fallback; input render above output is not
an upscale recommendation. Font174/174 vectorized log covers existing glyph cells,
not proof of uppercase code coverage; native lowercase label check remains open.

## Native camera rejection: world-space inertia
Claim: fix/world-inertial-helicopter-camera. User rejected rear-attached aiming;
requires independent camera inertia, car can turn before camera follows, distant
helicopter/long lens, wider turns/tighter fast straights. Previous spring bypass
and orientation-only offset were wrong. Replace with simulation-owned world
position/velocity, trailing heading, damped aim and actual projection zoom. Off
restores untouched original settings. Verify real built records, not graph only;
synthetic visuals cannot establish native feel or terrain clearance acceptance.
Validation87/87Linuxpass, finalfocused4/4 after saved-tilt regression. Real
ChaseCamera_Update/BuildMatrix now shows car yaw30deg without immediate camera
world orbit/forward rotation, viewpoint farther back and projection zoom wider
with steering. Repeated draws don't step world physics. Original spring path
restored, saved lag untouched/off returns original custom path; per-view state,
teleport/cut/bad-input resets covered. Saved custom tilt offset relative to
original default carried into independent look-at basis. Synthetic geometry/path
pixels inspected (world trajectory and lens1.46turn vs1.85straight fixture), NOT
native game/terrain/collision/feel proof. Steering is an input cue before body
rotation, not route knowledge or guaranteed anticipation of an unseen corner.
Native rejection remains open until owner tests this distinct architecture.
## Wiper binding beside driving actions
Claim: feat/wiper-driving-control-row. Move shared wiper-key row from Controls
landing page into the actual driving bindings list beside camera/look-back.
Retain extension INI key, never enlarge nine-key packed save layout. Both player
binding pages expose same global wiper binding; not independent new per-player
wiper actions. Preserve release-before-capture, Escape/conflict behavior and
actual merged cockpit update. Include current unsaved driving edits in conflict
checks. Tighten list spacing32 vs36 to retain Save row, verify actual draw layout.
Verified actual Menu_Update selection into W capture from both player pages,
release-before-capture, unsaved edited driving-key rejection, Escape, preserved
nine-key tables. 88/88 Linux tests pass. Actual draw-call layout fixture confirms
keyboard11rows+joystick12rows fit480p at32px spacing (last baseline442 for12).
Synthetic preview inspected; native atlas/Mac pixels still acceptance gate.

## Tracking-drone framing and terrain follow-up

The optional human-operator camera still has its independent world position,
velocity, orbit heading and steering-sensitive lens. It now follows closer
(9-14 nominal units, height 7-10, 1.3x transient tether), aims ahead, and keeps
normal-projection player framing near the lower third. Saved custom pitch/FOV
can intentionally change that framing. Other camera modes and saved settings
are not changed.

Simulation update checks collision-ground height along two 24-sample corridors
(player to camera and forward aim to camera), plus the predicted camera point.
A bank can pull the camera closer along its inertial direction before requiring
more altitude. Rise clearance is immediate, descent limited to 2 units/second.
Grounded corridor endpoints prevent a forward aim buried in a crest from
creating runaway altitude. Previous lift is removed before advancing the free
world spring, then reapplied as a constraint. Draws never advance it.

This is heightfield terrain avoidance, not a general scene raycast. Off-mesh
samples are unknown, not a clear-view certificate. Very thin geometry, vertical
faces and tree/hedge billboards outside collision terrain are not guaranteed.
Native Mac validation on the user's bank/crest/fast-curve case remains required.
The synthetic framing probe is a diagnostic, not game footage.
## Shared graphics-strength interfaces (work in progress)
MacOpenGL+Metal requested. GLexecutor owner uses existing effect setters plus
SetSunScreenPosition(x,y,visible), normalized top-left screen coordinates, and
SetShadowQuality(resolution,softness), softness0..100 maps PCFradius0..4texels.
Sharedfixedshadowshader uses shadowParams.z as texelradius and sampler7.
Mac builds now default OpenGL inclusion ON, Metal remains default primary.
This interface step alone does not claim working new controls or Macnativeproof.

## Metal-first effect activation checkpoint
Saved/per-launch requests now call actual SDL_GPU setters after device creation,
and Detail Apply uses the same reader. No INI/profile writes at startup. Nonzero
grade requests use restrained non-neutral temperature/tint/saturation/contrast;
zero blend is neutral. Added bounds and explicit amount keys (menu work separate).
Motion history is seeded before first blend, reset on toggle/resize. Bloom blur
now supplies a real integer tap count in a padded block, not float0 in that slot.
Shadow target/viewport/scissor honor selected resolution and PCF texel radius.
Rays use projected visible stage sun, not fixed screen location; offscreen skips.
Per-frame state changes log encoded passes and submission acceptance separately.
Neither encoding nor submission claims completed GPU work or native pixel proof.
MetalFX remains fallback on unsupported/downscaling presentations. Grounded
native case 2214x1440 -> 1280x832 is a downscale, not an upscale success.
GL executor/controls are a separate upcoming patch, not claimed by this one.
Native Mac stage/gameplay and frame time still require owner test.

Actual generated fullscreen vertex UV test catches quarter-image sampling:
old p*.5 did not span source. Fixed to p; GL-translated real shader test passes.

## Route-aware lateral drone refinement
Native feedback likes base inertia but requests physically moving outside a bend
for road visibility, including left of car in supplied right bends. Read actual
selected-stage .cat route and each car's race-progress segment. Preview12/30units
ahead across at most16nodes; don't nearest-search across hairpin legs or use
body yaw during a slide. Upcoming bend drives bounded6unit lateral target in the
existing world spring, route tangent steers orbit, route point steers damped aim.
Ground corridor also checks farther route point. Missing/degenerate/far/stage-end
preview falls back to existing inertia. No always-left rule, rigid attachment,
route rewriting or car physics change. Mirrored bends, drift-opposite-heading,
loops/finish/missing data and actual runtime matrix wiring covered.89/89Linux
pass; synthetic projection inspected, native road/vegetation visibility remains
acceptance gate. Route is coarse and not a full render-geometry visibility solver.
## Launch choices and runtime effects OSD
OSD has Off/FPS/Detail choices. FPS remains one line. Detail adds sanitized
launch choices (trees, crowd, wind, roads, textures, names, car-skin presence and
CLI effect override count), never absolute paths or raw command line. Requested
vs encoded states come from previous actual SDL_GPU frame, including precipitation,
submission acceptance and MetalFX fallback. Encoded does not mean completed pixels.
Mode/reset clears stale status; unsupported/unwired executor reports unknown.
Layout width expands for detailed diagnostics, bounded at640/480 and higher.
Tests cover source-summary privacy, CLI override count, status roundtrip and
13line glyph bounds. Native font/pixels remain required. GL status joins with
its separate executor integration, not falsely reported active in this patch.

## OpenGL effect executor integration
Integrate reviewed GL-only executor atop human-merged Metal66/camera67/OSD68.
No SDL_GPU file changes. GL startup and Detail Apply now use same bounded reader;
entering menu no longer forces effects off. Bloom, temporal history, depth blur,
projected sun rays, analytic grade, added rain/snow, opaque-only shadow path and
quality/softness setters have real execution. First history frame seeds without
blending; toggle/resize resets. GL3.3-safe fullscreen source orientation/whole UV,
std140 padding, feedback avoidance and complete state-cache invalidation covered.
OSD encoded bits follow successful GL calls, not option requests. Shadow excludes
alpha foliage; depth includes HUD and is nonlinear; fixed shadow origin/direction
remain limited. AA request honestly skips in GL, MetalFX not applicable.
Actual synthetic runtime pixels inspected and tests pass forced GL3.3/GLSL330
llvmpipe. Full Linux build/tests pass. Native Mac GL compile, game pixels and frame
time remain acceptance gates, not inferred from software Linux tests.

## GL spatial AA omission corrected
The AA pass in PR69 was omitted, not impossible on OpenGL. Reuse generated
spatial_aa.frag GLSL330 via existing post framework at effect8, before bloom.
Read override-aware option at device creation, keep defaultoff. Separate target,
linear clamp, reciprocal resolution and padded UBO; success sets encoded bit1.
Failure retains source, never claim executed from request. Actual GL3.3 executor
pixel tests cover changed edge pixels, alpha, exact flat colors, off identity,
stacked effects/ordering, resize and normal draw state restoration. Full90tests
pass. Native Mac output/performance acceptance remains open.
## Strength-controls checkpoint
Effect amounts subpage18rows/3pages, bounded22itemMenu capacity. Uses actual
keyboard/controller menu navigation, all-size draw tests and no writes on Enter.
Real shared effect strengths and shadowquality, conservative35%blurUIcap. Split
camera shake/lens dirt/flare with legacyfallback; scaleactualRGBadditiveflare
insteadineffectivealpha. Windscreendirtfractionalaccumulation scaled, dustvolume
density+fallbackopacityseparatefromweather/debris, cubeStage/64/128/256reloadquality.
No profile/save migration. BothGL/Metalruntimefoundationshumanmerged69. Actual
menu-coordinateillustrationinspected, notnativeatlas. Lensoverlaysretainexisting
projectedchallengepath, nofalsegameplayclaim. Auto-weather/autofocus/reflection
blend/distance+shadowgroupingnotinthischeckpoint; see docs/EFFECT-AMOUNTS.md.
## Session checkpoint (PRs #137-#145)
Merged to main, all source-only and not native-verified on the Mac: #137 camera readout, per-type camera move, F10 panel, tyre debris, tyre-track length/history; #138 `--all-trees`, `--tree-coverage`, `--spectator-gain`; #139/#140 bush index entries; #141 test-only CI fix; #142 F10 panel drawn on sprite layer 3 (layer 4 draws before the 3D scene); #143 `--tree-rustle 0..300` (default 100) and `--spectator-irregular 0|1` (default 1), texture-coordinate shift only.
#144 caches the crowd mesh template (counts logged, no timing claim). Bush converter path: no stem when the source canopy reaches the ground, crossed planes for the Finland leaf-strip bushes.
Open: whole-stage tyre-track persistence, Hi/Lo and other-rally exporter coverage, bsh_st1/forest/hedge strips, drone FPS profile (needs native profile), curved-road generalisation, native confirmation of #137/#142/#143, macOS CI never started.

## Gameplay-first drone follow (30 October 2026)

Claim: feat/drone-gameplay-follow, based on the batch-9 CI-fix lineage.
The clarified voice request supersedes the independent route-tangent orbit:
steering readability wins, with light inertia rather than a detached TV camera.
Normal continuous-turn orbit lag stays near two degrees; abrupt inputs may
briefly exceed that rather than snap onto a hard angular clamp. A substepped
world-position/velocity spring bounds acceleration, with angular-velocity
feed-forward for fast lateral tracking. Route preview still supports lift,
but no longer selects an outside-bend orbit. Acceleration creates a small
fall-back; braking approach settles without overtaking the car. A behind-car
emergency boundary remains for extreme discontinuous inputs, not normal stops.
Existing terrain, reset, opt-in/classic and per-camera paths
remain. No new setting or default enablement.

Four focused local CTests pass, including actual ChaseCamera_Update and
CameraOperator_Update/Apply wiring. New fixtures cover 25/60/120Hz, turns,
reversals, heading wrap, left/right symmetry, emergency stopping and settling.
Old tests requiring a detached orbit were replaced because they contradict
the clarified request, not relaxed as a substitute for intended behavior.
Native Mac gameplay/feel/graphics/performance remain unverified. No hosted
run or publication until the approved CI limits are merged and verified.

Broad local build was attempted, not green: unchanged SceneryMotion.h lacks
a direct size_t declaration under this compiler (animated_scenery tests).
A standalone compile of the base header reproduces it. No suppression or
unrelated fix is included in this camera patch.

Continuity follow-up: removed the initial hard angular and fore/aft clamps.
World-position finite-difference acceleration stays below 850 units/s^2 in
25/60/120Hz discontinuous heading-wrap, reversal and braking fixtures; normal
trace max orbit 1.73 degrees, view 2.31 degrees, lag -0.44..0.035 world units.
Approximate gameplay feel is not a literal two-degree invariant. Adjacent
SceneryMotion cstddef fix has its own commit with standalone red/green proof.
Full build now succeeds; full CTest run is 125/130, with five aborts in
network, frontend and controller paths (Zig runtime safety checks in unchanged
code). Those failures are not suppressed and baseline reruns remain needed.

Exact baseline comparison completed: all files changed by this candidate were
restored to 3861bc9, retaining ONLY the isolated cstddef build prerequisite;
the same compiler, runtime configuration and five test targets were used. All
five failures reproduced at the same unchanged code paths. Candidate source
was then restored and four focused camera tests passed again. The acceleration
assertion now derives from the actual 800-unit/s^2 integration cap plus 5%
finite-difference/float tolerance, not an arbitrary margin above one trace.
That cap is an unvalidated gameplay tuning choice, not physical proof of feel.
Emergency braking is mathematically continuous but still rapid in the trace.

## Current native-regression candidate (30 October 2026, 16:11 EET)

Integrated merged batch9 ef25e57. User screenshots show drone with tunnel and
streak active; View_IsDroneCamera incorrectly used player index for arrays
indexed by alternating camera-record slot. It now maps active slot + player*2
and requires actual type5. Linked regression covers both players/slots, stale
record/classic rejection and speed-FOV bypass; old query fails, fix passes.
Space replay helper was already fixed in batch9; added linked actual raw-key
update pause/hold/release/resume/reset test, passing without runtime code edit.
Terrain samples could raise by 8 units immediately but descend slowly. Added
alternating sample red/green test; limit rise to3units/s, preserve2units/s
descent. Ground corridor remains heuristic: abrupt true obstacles may clip.
Nine focused CTests pass. Native screenshot's FPS drop remains open: nested
scenery/tree/wind timers cannot be added or subtracted into a GPU conclusion.
Camera roll is separate future opt-in work, not in this candidate. All patches
reviewable, merges owner-led, CI intentionally skipped on old stack workflow.

Startup identity: compiled Git SHA/branch/exact-tag/tracked-dirty status now
prints before startup and with --version. Always-run CMake generator refreshes
on rebuild (write only on change), no runtime Git dependency. Source archives
explicitly report metadata unavailable. Temp-repo regression covers commit,
branch, exact tag, dirty edit, detached checkout and stable unchanged output.
Current full build succeeds; 127/132 tests pass, same five previously reproduced
local Zig runtime baseline aborts. Nine focused camera/replay checks pass.


## Fixed Finland1 strips and cutout texture follow-up (30 October2026)

Separate from pinned PR184208ffae, never changing that delivery. Added82
full-canvas2x cutout variants:76forest-strip variants across Finland/France
hi/lo source archives, six Finland wooden-fence variants. Source/output hashes,
rectangular dimensions and full mip chain are checked. Premultiplied RGB and
nearest-achievable alpha-test coverage mips; interpolation, not invented detail.
No original or derived artwork is included. Six shadow masks stay original.

Fixed Finland1forest material runs take a separate exact original position/UV
hash allowlist and strict four-corner planar/full-canvas fit. Ninefold
triangulation adds shallow relief and perimeter/feet-fixed optional wind, all
clamped within original quad axis-aligned bounds. Original mesh/collision
records, physics, placement and material/light/fog binding are untouched.
Two degenerate-axis parts cannot sway; one nonplanar part stays original.
This is NOT individual3Dtrees or completed universal geometry coverage. Fence
work is texture-only. Other track geometry is unchanged. Greece sources arrived 1 Nov
(gre01hi/gre01lo/grehit/grelot, verified CMPR + content-identified). Greece1
bush/tree sheet relief is on feat/greece1-brush-strips: 51 strip-part
identities (hi==lo geometry) extracted by the new committed
tools/scenery-patch/strip_sources.py (validated 58/58 against the Finland1
tresh hashes), 12 exact-source cutout recipes in uprez-textures.json,
stage/stem gates extended (gre01 temp.c3d hashes, bsh_sh1/tre_s1).
Screenshot wall mapping: bsh_sh1* are the roadside bush cards by texture
review; native visual acceptance still owed. Existing command opts in; --trees original disables this
geometry, --textures original disables these texture and strip paths.

Full local build passes;136/141tests pass, same five known baseline Zig
aborts. All144Finland indexed textures passed actual runtime upload/all-mip
comparisons and stage/identity/reset guards. Actual original temp.sht probe
accepted785material runs, retained one, submitted43,740triangles;783runs show
wind motion and all source bytes stay immutable. Runtime executor stubbed,
not native Metal acceptance. CPU-rasterized submitted vertices/source pixels
inspected privately. Native appearance/performance/culling remain open.
Actual binary --asset-upgrades --prepare-only resolves all three private
cache roots from owned archives. No merge, hosted run or release requested.


## France1fixed hedge/tree-wall follow-up (30 October2026)

Separate candidate off185bef5ab3, prior pinned deliveries unchanged. Live owned
France1hi/lo originals recovered; stage identities match runtime gates. Original
fixed `bsh_st1a-f` hedges and `tre_w2a-d`/`tre_wla` walls added to exact
position/UV allowlist. Most source quads are warped, so preserve both original
triangle domains/diagonal, barycentric mapping and edge coordinates rather than
relaxing full-canvas billboard planar fit. Warped support only after exact source
gate; Finland stays strict planar. Source vertices/physics/collision untouched.
All generated positions remain within original quad AABBs. Material/world/live
light/fog states remain original. This is shallow relief, not a solid3Dhedge.

Added12hedge and6France barbed-wire texture variants. Total186indexed private
2xoutputs, all actual hi/lo runtime upload/all-mip comparisons, stage/identity/
reset guards pass. Six shadow masks original. Both France hi/lo actual temp.sht
runtime probes accept749material runs, submit50,868triangles;748parts move,
one degenerate-axis part stays still; source bytes immutable and bounds pass.
Finland regression remains785accepted/1nonplanar retained/43,740triangles/
783moving. Runtime executor stubbed, native Metal/culling/FPS unverified.
Private CPU raster of actual submitted France hedge/wall vertices inspected.
Greece still source-blocked, no repeated ask. All-track/solid reconstruction
remains open. No human merge, hosted run or release requested.


## France1opaque terrain2x follow-up (30 October2026)

Separate off186b527069, no prior pin changed. Actual original France1temp.c3d
material/UV/alpha inventory found51uncovered opaque terrain materials after
excluding bridge/cropped/alpha materials. New hi/lo102outputs bring exact-source
2xmanifest to288. Explicit edge-clamp filtering follows original material clamp
routing, UV dimensions and originals unchanged. No new detail/physics claim.
Runtime manifest bound raised32KiBto128KiB: expanded manifest exceeded old bound;
real upload test caught the all-textures refusal before publication. Added test
for runtime metadata synchronization, unique output names and51-per-archive
coverage/policy. Six focused recipe tests pass, full build succeeds and136/141
normal tests pass with same5baseline Zig aborts. All288owned-source runtime
uploads/all-mips/stage/identity/reset checks pass, including rectangular terrain.
Inspected actual uploaded mip0vsoriginal pixels privately, no native GPU claim.
Original broad644scope not claimed; object/atlas texture coverage, other stages,
solid geometry and Greece originals remain open. No merges, tags, dispatches,
releases or artifact writes requested. Exact commands/source PR for human test.


## Cohesive terrain/trackside2x follow-up (30 October2026)

Build on187cd91bdb (not yet delivered to user; user last has186). Add50original
fixed temp.shtmaterial hi/lo variants:10Finland banner variants,40France wall/
guardrail/banner/static-ribbon variants. Full-canvas source UV usage verified
from actual stage meshes.338indexed2xoutputs total including187terrain.
Texture-only, no source/geometry/physics change. All338actual runtime owned-source
upload/all-mip/stage/identity/reset checks pass. Premultiplied alpha/mip histogram
optimization byte-identical across all338payloads plus manifest; avoids repeated
pixel scans. Seven recipe tests pass; full136/141same5baseline Zig aborts. Actual
uploaded output pixels inspected privately, not native gameplay. Scope still
excludes unknown object/atlas textures, other stages, solid reconstruction,
shadow masks and Greece source dependency. Prior184-187pins unchanged. New
cohesive command should replace186for user, including held187terrain coverage.

## Native338texture preparation repair (31 October2026)

User mainb17749cc8aa6build succeeded, then source-checked texture preparation
refused a reviewed output mismatch. Exact native ARM cause not reproduced.
Old338recipes pass local Pillow10.4/11.3/12.3. Avoid assuming cache corruption.
Portable integer-only resampling replaces native float Lanczos for stage-uprez;
all338digests regenerated/reviewed and runtime upload/all-mip/stage/reset tested.
11helper tests pass. Full application build,136/141CTest with5same known Zig
baseline aborts. Native ARM/gameplay/FPS still unverified. This repair targets
main338directly;190opaque-object expansion stays separate and unchanged.

## Native frontend camera readout lifetime crash (31 October2026)

User ARM report pins crash to View_GetCameraCarOffset Car.cpp8419, via debug
readout during frontend challenge scene after world replacement. Null-only
Car_Getchecks permit stale freed slots/unspawned matrix pointers. Presentation
lookup now checks current buffer/count, exact slot membership and embedded
physicsMatrix identity before reading. Also guards adjacent speed-FOV/move
presentation readers. No legacy allocator/physics/source geometry changes.
Regression uses freed pointer poison, exact reported invalid pWorld, new table
before spawn, smaller replacement allocation and both valid views. Old main
aborts the regression on invalid-pointer dereference (Zig null-page check);
repair passes. Full build and137/142CTest,
same5baseline Zig aborts. Native Mac stage transition retest still pending.

## User reference settings (2026-11-01, native M2, NOT defaults - respect in fog/light work)
From four user screenshots (Finland1 snow, chase cam type 4, Subaru):
- fog: encoded amount 70, glow 100; fog evolve 60 (100 neutral clears thickens); fog light response 100
- air motes on, amount 500; snow glints 100; snow material OFF (video.snow_material=0)
- speed fov 100; speed streak 100; tunnel vision 130; motion blur 60; dof off; radial speed lines off; vapor trails on
- tyre debris 400; dust/snow plume 200; wheel spray 200; exhaust smoke 200; wheel splash 200; backfire sparks 200; rain/snow amount 200; cockpit speed fov 25
- camera chase type 4 (move family 0.05 units:0); corner tilt 30; drone distance 50, height 100
- fog culling row NOT visible on tuning pages 1/43, 19/43, 43/43 - exposure unresolved, do not invent a value

## Known bug: fog beam wedge direction (rear lamps) - fix in progress
Beam end 30wu along lamp dir can sit behind the camera (rear lamps in chase view);
FixMatrix_ProjectWorldPointToView returns sentinel (-100,-100)px, producing a wedge
drawn toward the upper-left corner = red streak up the left snowbank (user report
2026-11-01). Fix: clip lamp->beamEnd segment at the near plane in view space
(StageObjects.cpp fog-light projection block) before projecting.

## Fog light camera fixes (1 November 2026)

Lamp-side near-plane clip (bonnet/bumper cams no longer drop lamps within 1u when the beam crosses into view; anchor at the crossing, no depth gate on a clipped anchor) and projection-independent depth gate: GfxFogLight.viewZ (world units, SDL_GPU-only, vote-dependent) replaced by depthRef (raw depth-buffer value 1 world unit in front of the lamp, from CGraphics::m_projection33/43 game-side; GL executor remaps into its half-range buffer), so the gate runs on BOTH renderers even when the near/far vote is ambiguous - the cockpit dash-spots failure mode. Rendered-pixel evidence: tests/fog_beam_render.cpp scenarios (near-plane-crossing rear beam, occluded-lamp gate-off, near-plane lamp wedge) through the real GL executor + fog.frag. Residual: if a real cockpit dash does not write depth, the gate cannot see it - needs native verification. R look-back glow: harness scenario added (camera 180 degrees, rear beam toward the camera) - the wedge keeps its true screen side and never mirrors to the upper-left; the pre-#233 sentinel geometry is consistent with the glow beside the left spoiler support, and the corrected mechanism is pinned by pixel assertions; the user's exact screenshot-specific cause and real-asset lamp alignment stay unverified natively.

## Windscreen dirt and real wiper sweep (1 November 2026)

Branch feat/windshield-wipe. Replaces the old flat fullscreen windscreen
tint with a per-glass dirt map that only the real 3D wiper blades clear.

- game/WindshieldDirt.{h,cpp}: 96x64 per-view dirt + wetness maps, a
  trapezoid glass mask (v 0.04..0.66 with pillar taper - approximate; exact
  per-poly glass mapping is a native follow-up), driving spatter hooked
  into the two existing Race.cpp dirt-accumulation sites, and a wipe that
  removes dirt along the blade's arc band: wet sweeps smear part of the
  removed dirt to the trailing rim, dry sweeps leave up to 1/8 residue.
  Wetness follows StageObject_GetViewWeatherField54; dirt decays over a
  64-frame sweep so it does not accumulate forever.
- Clearing is driven by the REAL blade state (g_carWiperStates +
  g_carInteriorProfilePointers, cockpit slots only, direction-reversed
  handled): in dry weather the blades stay parked and dirt only decays.
  This is a behavior change from the old 2D white-bar overlay, which moved
  regardless of the 3D blades and is removed from CameraEffects_DrawOverlays.
  The 3D blade animation itself is original behavior, untouched.
- The overlay draws a 96x64 BGRA grime texture as one alpha-blended TL quad
  through gfx_core, so it runs on GL and SDL_GPU alike; cockpit only
  (camera type 3). The old g_windscreenDirt global is now write-only
  (kept; removing it is a separate cleanup).
- Evidence: windshield_dirt unit test (masking, determinism, wetness,
  wet/dry wipe, decay) and windshield_render, which builds a real dirt map
  and runs the real Windshield_DrawOverlay through gfx_core -> GL with
  readback: glass darkened toward the grime tint, the wiped arc stays
  clean, the dash band untouched. Harness GL, not native Mac Metal/OpenGL.
- Pivot position and arc band are an approximation of the original blade
  sweep (profile maxAngle as the band); exact blade-tip tracking against
  the interior model is a follow-up.

## Fog light beam stability and forward halo (1 November 2026, late)

Branch fix/fog-light-axial-halo (stacked on #235). User replay report on
clean main 00e2a5f (WITH #233's clip): "lights jump all over, pointing up
one second and down the next" plus bonnet/body glow overlap.

- Mechanism (source-grounded, then pixel-pinned): the wedge direction came
  from the projected beam END; when the end crosses the near plane the
  projection is noise, so near-axial beams flipped side frame to frame and
  stretched across the sky. Now: direction = the beam's transverse
  view-space component (continuous), length = foreshortened world length at
  the lamp depth, capped at 0.6 of the screen; exactly axial beams collapse
  to the halo. The praised near-axial look (reverse lights in dust,
  phonemsg-01M6A3C88QJ0VEGFFAC0PATE3M) is preserved: toward-camera beams
  keep a bounded shaft toward their closest-approach side.
- Halo: the glow bubble anchors 3 world units forward along the beam
  (haloX/haloY/haloRef in GfxFogLight; FogUniforms 27->35 vec4 = 140
  floats, single shared packer; GL remaps haloRef like depthRef). The
  bubble floats in the lit air in front of the car; bodywork occludes it.
  The wedge gate reference moved from 1 unit in front of the lamp to half
  a unit behind it so the car's own panels (within ~1 unit) occlude.
- Harness (tests/fog_beam_render.cpp): 6 scenarios incl. axial collapse,
  20-degree yawed transverse wedge, camera-cut side/length continuity,
  bonnet forward halo, gated occlusion, look-back halo. Rendered-pixel
  asserted and frame-inspected. GL on Linux, not native Mac Metal.
- Native attribution: user's frames were on 00e2a5f (pre-#235); the
  residual direction/body-overlap defects existed there. This branch +
  #235 are UNVERIFIED natively; no "fixed for you" claim until a native
  run on a build containing both.
- FixMatrix_RotateVector moved unchanged into FixedMatrixCore.cpp (same
  pattern as #234) so the harness links the production primitive.

## Greece1 brush walls: why #232 could not fix them, and the depth path (1 November 2026, late)

User report on clean main 00e2a5f (with #232 merged): "greece 1 flat bush near
1st checkpoint is still flat bush". Investigation against the user's own
verified gre01 archives (stage + texture), using the repo's generator
(tools/scenery-patch/strip_sources.py) and the SceneryPack runtime gate:

- #232's FixedStrip relief targets temp.sht strip parts. gre01's temp.sht has
  ZERO strip-eligible parts; the roadside brush lives in temp.sht as stage-object (corrected: an earlier note said temp.obj)
  parts (bsh_sh1a x12, bsh_sh1c x16, bsh_sh1z x10 = 38 wall parts; tre_s1_*
  trees separate). 0/38 part hashes appear in game/FixedStripSources.inc, so
  the relief could never activate for them. The .inc comment naming Greece1
  overstated coverage; #232's Greece effect was texture cutouts only.
- The brush walls are literally single flat vertical quads (~17x3 units)
  with an alpha-cutout sheet: any perimeter-preserving relief keeps the
  straight-card silhouette (probe-verified, decision-grade only).
- fix branch feat/greece1-brush-depth: BrushRelief (game/BrushRelief.h) draws
  each wall quad tessellated with interior alpha displacement (heuristic:
  alpha = cutout coverage, not physical depth) plus a taller, darkened back
  layer from the same verified sheet ("skybreak") that breaks the silhouette
  and adds parallax. Gated: video.scenery_overrides/tree_overrides/
  texture_uprez + verified cutout + gre01 part hash + video.brush_relief
  (0-100, default 100; Atmosphere submenu row). GL+Metal via the shared port
  draw path. Unit test tests/brush_relief.cpp; game-render frames pending.

## Cross-effect light coverage matrix (todo for the shared principle)

Responds to light today: fog (fog.frag), falling snowflakes, snow-surface
glints, air motes. NO light response: rain streaks/splashes (flat grey),
dust plumes, wheel spray, gravel/debris (fixed particle colour+shade). The
praised reverse-lights-in-dust screenshot shows the fog/light overlay beams;
the dust particles themselves are unlit.

## CI test repairs (branch fix/ci-test-platform-repairs, 1 Nov 2026)
- tests/fog_uniforms.cpp: guard-page allocation now uses VirtualAlloc/PAGE_NOACCESS
  on Windows (was POSIX-only sys/mman.h; broke the windows-x64 build leg at
  compile time after the fog-uniforms test merged). Same guard-page coverage
  on both platforms.
- tools/scenery-patch/test_uprez_prepare.py: temp roots resolved
  (Path(tmp).resolve()) because macOS runner TMPDIR lives under /var ->
  /private/var, and the prepare() symlink guard rejected the symlinked
  parents. Tamper/reuse coverage unchanged; 11/11 pass locally.
## Shared SDL touch controls (branch feature/sdl-touch-controls, 1 Nov 2026)

One touch implementation for every SDL platform (iPhone/iPad and a browser
build first; Android migrates when its Kotlin overlay is retired).
src/input/TouchLayout.* is the pure, SDL-free logic: floating circles
(driving = steer+accel+brake; brake circle = brake/handbrake/both, neutral
at touch-down, direction-based states with brake->handbrake reversal),
aggregate brake across pointers (only the last release ends braking),
handedness mirror, classic fixed pads as a selectable mode. Settled
semantics come from the user's 2026-11-01 messages; every size/threshold in
TouchConfig and the "most recent driving pointer steers" ownership rule are
PROVISIONAL placeholders pending the user's design decisions.
src/input/input_sdl.cpp feeds SDL finger events (routed from
platform/sys.cpp) into it and merges the result into the keyboard state the
game polls, under the default arrow/Space bindings, so no game-code changes
are needed. input.touch_controls auto/on/off (auto = touch device exists,
except Android where the Kotlin overlay owns the screen), input.touch_mode,
input.touch_handed. tests/touch_layout.cpp covers the settled invariants
(7 cases). NOT DONE: on-screen rendering of the pads (they are invisible
until drawn), analog steering via joystick bindings, settings GUI entries,
Android overlay retirement. No CHANGELOG/WhatsNew entry: nothing user-
visible until the pads render.
## Merge state + follow-ups (1 Nov 2026, late morning)
- #243 (SDL touch foundation) and #244 (CI repairs) are MERGED (main e0a7710).
  #244 landed at 1a692b0; its final two windows test fixes are in #245
  (fix/test-leg-followups): replay_controls_wiring reads UTF-8 (README gained
  curly quotes in 73fc51f; Windows cp1252 default) and tuning_runtime closes
  its ini readback handle before later saves/cleanup (Windows: rename over an
  open file is denied, remove_all then throws -> abort 0xc0000409; Linux
  unlinks open files, hence windows-only) + explicit <string> for libc++.
- iOS PoC (poc/ios-sim-build): Stage 1 compile green; Stage 2 = unsigned sim
  .app boot smoke in CI (marker from main() + no crash signature; no Data in
  CI, so process-start only, no menu/gameplay claims) + documented Documents
  route via SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS) (NSDocumentDirectory on
  Cocoa). First smoke run failed on device-type pick (iPhone-6s-Plus is
  refused by current runtimes; now highest-numbered iPhone type).
- rebuild.sh: --textures accepts original|france1|uprez (game vocabulary) and
  uprez is the default when --data is given (#246); opt-out --textures
  original. Uprez coverage: owned Finland1/France1/Greece1 archives only.
- What's new (#247): no more main-menu popup; localized row at the bottom of
  the main Options menu (above Back); full log listed; rev 16.

## Greece1 brush volume (EXPERIMENTAL, video.brush_volume=1, default 0)
With video.brush_volume=1, gre01 brush walls (38 bsh_sh1a/c/z parts) get depth only AWAY from the road: the road-facing face stays the original quad, then relief, the back card and 4 slices (0.8 units apart) stack away. Side = nearest stage route segment within 60 units vs the quad normal (never winding). Default off keeps the previous relief + back card behaviour. Collision untouched.
Coordinate contract (CORRECTED from a real gre01hi.bfl; earlier statements that the walls are temp.obj object meshes placed by a scene node worldF were wrong, caused by a stride bug in strip_sources.py): the 38 brush walls are stage-object meshes in temp.sht (no scene nodes, no ordinary meshes). Each stage-object record has its position at 0x0, pMesh at 0xc and a 4x4 fix matrix at 0x18; in that archive the matrix is identity and the vertices are already world-space. Game_DrawStaticStageObjects draws them with Graphics_DrawMeshLOD and sets no world matrix of its own, so the ambient WORLD transform applies (expected identity, not proven natively). The runtime reads Gfx_GetTransform(WORLD), requires it rigid, takes the quad and normal to world space, picks the side against the route polyline, declines unless the wall is within 60 units, faces the road (|side|/dist >= 0.7) and the extrusion would not approach any road point, and maps the away direction back to local space.
Real-data result (gre01hi, 506 route nodes after the RallyData trim): 23/38 walls are within 60 units of the route (nearest 14.5 to 136.7); with the facing and approach guards the runtime extrudes 19, declines 15 as too far and 4 as not facing the road/approaching it. The quad winding normal is unreliable (faces the road for 22, away for 16), so the guard uses geometry, not winding. Offline harness (software, not native): the runtime matches the exporter on all 38 walls and extruded perimeter vertices stay on the fixed road-facing edge with slices displaced only away from the road. STILL UNPROVEN natively: that the draw-time world transform is identity, and the look/direction in the game. gre01lo and the user's other archives were not examined.

Default now: gre01 brush walls draw as the original flat quads (the earlier relief + back card pushed along the quad-winding normal, which was never checked against the road side, so it is opt-in only via video.brush_relief_legacy=1). video.brush_volume=1 is the route-grounded experiment.
Real-data check (no game bytes copied, JSON numbers only): `python3 tools/scenery-patch/export-brush-placement.py --data "/path/to/owned/game-data" --output /tmp/gre01-brush-placement.json` reads gre01hi.bfl/gre01lo.bfl read-only, locates the 38 wall parts by hash (in temp.sht), their stage objects and the route (.cat), and prints a `checks` summary including how many walls the runtime would extrude. Validated on one real gre01hi.bfl.
