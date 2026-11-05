# Fullscreen SDL_GPU attachment declarations

Six dedicated fullscreen pipelines (lens grime, speed streak, DoF, god rays,
fog and color grading) declared a depth attachment even though their shared
PostProcessPass begins a color-only pass. Sampling scene depth in a shader
does not require attaching that texture as a depth target. SDL's Metal backend
uses the declaration to set the pipeline's depthAttachmentPixelFormat, so the
pipeline and pass descriptions disagreed.

Set those six declarations to false, matching the generic postprocess and
motion-blur pipelines. Geometry and shadow passes retain their depth targets.
No shader, effect amount, ordering, asset, camera or saved option changes.

The render-pass wiring test now audits every explicit pipeline constructor,
checks all dedicated fullscreen routes to the shared pass, and checks that the
shared pass omits the depth descriptor. The executable postprocess helper test
and Linux build also pass. These are contract/build checks, not native Metal
pixel acceptance.

This is independent of the November 5 native screenshots: that run used
c437126, before the dirt/shell/passenger repairs merged. This repair does not
claim to fix any particular reported visible failure without native review.
