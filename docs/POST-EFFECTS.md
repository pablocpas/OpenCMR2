# Post-effect polish

This changes the existing optional DoF, god-ray and color-grading implementation.
It does not introduce MetalFX, force effects on, or replace any game assets.

## What was repaired

- DoF previously moved colors toward gray without sampling adjacent pixels. It
  now uses 16 golden-angle disk samples, a smooth focus transition and foreground
  rejection. Flat colors keep their brightness; focused pixels keep their detail.
- Rays previously brightened the whole frame, ran twice, and started from the raw
  scene, discarding earlier effects. One normalized 32-step pass now samples
  bright, unoccluded sky, decays along the ray and uses a restrained screen blend
  to avoid clipped white haze. It consumes the previous effect's output.
- There was no grading LUT. The HSL conversion changed colors even at neutral
  settings. Analytic RGB balance and luminance-based saturation now preserve
  neutral settings. The existing settings remain, including saturation/contrast
  as percentages (100 = neutral). Initial values and bypass checks now use those
  same units. Temperature/tint preserve black rather than adding colored fog.
- Depth effects need a sampleable depth texture. D32_FLOAT is selected only when
  the device reports support for both depth-target and sampler use. If unavailable,
  those effects are skipped while ordinary rendering and grading remain.
- New effect targets are released on resize before recreation. Two redundant
  ping-pong buffers are removed and a distinct grading target prevents feedback.
  Lens grime also avoids sampling its output and supplies its full uniform block.

## Atmosphere fog addition

An optional fog pass (video.fog, default off) now runs between lens grime and
DoF in the chain so blur and god rays stack on the fogged scene. It is a single
full-screen scene+depth pass with distance fog, a ground-mist gradient, a sky
cap and a sun-ward glow - screen-space, not occluded volumetric shafts. It is
deliberately heavy at amount100 by request and meant to be dialed down. Full
details, controls and evidence: docs/ATMOSPHERE.md.

## Motion blur keeps the car sharp

Motion blur (video.motion_blur / video.motion_blur_amount) blends the frame with
the previous one, which smeared the followed car along with the world. With
video.motion_blur_car (default 1, per the user's request) the SDL_GPU renderer
scales the blend to zero inside the followed car's screen rectangle, from the
car's own projected vertices (the same region DoF whole-scene mode uses,
render/car_region.h): the car - and the road inside the rectangle - stays sharp
while the world blurs. The rectangle is a feathered bounding box, not a
per-pixel silhouette. In the interior (cockpit) view the gate stays off: the exterior
windscreen mesh keeps the followed car's objectId there, so the game tells the renderer
the camera type (Gfx_SetPlayerInterior, camera type 3) and the blend is the old
whole-frame one. With no followed-car draw in front at all, the gate is off as well.
The OpenGL renderer records no car region and keeps the old behaviour there.

Cost: two history textures ping-pong (render/motion_blur_history.h) - the blend
writes the slot it did not read and that slot is next frame's history, so a frame
with motion blur on costs one full-screen blend pass and nothing else (one seed
copy on the first frame after any toggle, resize or non-race frame).

## Taste and limits

The intent is less white fog, actual soft defocus and a subtle grade, rather than
turning the picture into a heavily processed screenshot. Suggested starting
values, not forced defaults: temperature 15, tint 0, saturation 105, contrast 103;
keep DoF modest during driving and rays low. Retain the user's existing settings.

Focus still uses the game's nonlinear normalized depth (0..1), not meters or a
physical lens model. The sun is an artistic fixed screen position (0.85, 0.15),
not a world-space sun projected per camera, and rays are a screen-space effect,
not volumetric scattering. Occlusion uses the depth buffer's far/sky region.
Actual sky depth and focus behavior must be checked with the game's scenes on
Mac. DoF includes HUD text; no HUD separation is claimed. Skybox depth conventions,
transparent foliage and weather may need further scene-specific work.

## Evidence and acceptance

The optional `post_effects_pixels` test compiles and executes the actual fragment
shaders through SPIR-V/GLSL on Mesa llvmpipe. It checks neutral grading, in-focus
preservation, real out-of-focus blur, flat-color preservation, fully occluded and
zero-intensity ray identity, and alpha. It fails on the old neutral grading and
passes on this implementation. Optional dependencies and invocation match
`tests/spatial_aa_shader.py`; run:

```sh
python tests/post_effects_shader.py --output /tmp/effects.png
```

The comparison is synthetic test scenery, not game footage. It was visually
inspected for smoother defocus, unchanged focused foreground, restrained rays and
subtle color. Complete Linux build and Metal MSL translation are not proof of
native Metal runtime correctness. Before merge, check actual Mac gameplay and
menus, both isolated and stacked effects, resize/fullscreen, sky depth, haloing,
text readability, and warmed CPU/GPU frame time. No FPS improvement is claimed.

Sources are the existing fork's shaders and renderer. Changes preserve its
options, not a quoted suggestion's assumption that a LUT or a true DoF blur
kernel already existed.

## DoF depth convention (PR "DoF coc fix")

The depth texture holds hardware depth, `depth = far/(far-near) * (1 - near/z)`. With the usual
camera near plane of 0.1 (0x1999) the old `smoothstep(|depth - focus|)` was already 1.0 for every
pixel beyond about 0.5 units (checked numerically for near 0.1/0.5/1.0 and far 150/400), so blur was
uniform and the tunnel-vision peripheral term could add nothing.
When the renderer sees a perspective projection it now recovers near/far from `_33`/`_43`
(near = -B/A, far = B/(A-1)), rebuilds the view distance z and measures focus on a unit-free log
scale: 0 = near plane, 1 = far plane (`dof_focus` 50 = geometric mean of near and far). No metres are assumed.
Behaviour change applies only when depth_of_field is on (off by default is unchanged); if no
projection is recognised the old linear rule is used. Not natively verified. A sharp screen
centre is not a car mask: the car is only sharp while it is within focus range and inside the clear tunnel zone.
Which projection feeds the pass: every back-buffer, untransformed draw that tests and writes depth votes
with its recovered near/far pair; the pair with the most draws is used and Sys_Log reports the number of
distinct pairs and the dominant share ("options: DoF projection from depth-writing draws"). If the log shows more than one
pair (split screen, mirror, sky with its own projection), the pass falls back to the old raw-depth rule for that frame (so DoF looks as before the fix). With one pair it is a candidate, not proven. Votes reset every rendered frame.
The log-scale focus is a new mapping chosen here (no original intended focal distance was recovered): dof_focus/dof_range now
mean positions on a near-to-far log scale instead of raw hardware depth, so existing saved values give a different look
when depth_of_field is on. Math checks: depth/z round-trip for near 0.1/0.5/1.0 with far 150/400; dof_focus 50 lands at
z of about 3.9 / 8.7 / 12.2 units (near 0.1/0.5/1.0, far 150). Nothing here is native verified.
