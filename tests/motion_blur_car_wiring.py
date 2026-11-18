#!/usr/bin/env python3
"""Motion blur car-keep wiring: the followed car stays sharp while the world blurs
(user request: motion blur must exclude the car). One shader gate fed by the car's
own projected vertices (shared with DoF whole-scene mode), one packer for both
executors, SDL_GPU only, on by default per the user's request, panel row to tune."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
s = rd("src/render/shaders/motionblur.frag"); g = rd("src/render/gfx_gpu.cpp")
need("float carKeep;" in s and "vec4 carRect;" in s, "shader uniforms")
need("m *= 1.0 - inU * inV;" in s, "gate scales the mix to 0 inside the car rectangle")
need("if (carKeep > 0.5)" in s, "gate off keeps the old whole-frame blend")
need("MotionBlurUniforms_Pack" in g and "render/motion_blur_uniforms.h" in g, "SDL_GPU uses the shared packer")
need('Sys_GetOption("video.motion_blur_car", 1)' in g, "default on per the user's request")
need("CarRegion_AddDraw(carRegion" in g, "car region from the car's own vertices")
need(g.index("CarRegion carRegion;") < g.index("// Motion blur: blend"), "region is computed before the motion blur pass (shared with DoF)")
need('Sys_GetOption("video.blur_mode", 0)' in g, "DoF whole-scene mode still shares the region")
# cockpit: the exterior windscreen keeps the followed car's objectId, so the region is VALID in the
# interior view - the gate must be forced off there, and the game must tell the renderer the view type.
need("mbp.interiorView =" in g and "interiorView" in rd("src/render/motion_blur_uniforms.h"), "interior view feeds the packer, which resolves the gate off")
need("bool interiorView" in rd("src/render/gfx_internal.h"), "world-end marker carries the interior flag")
need("Gfx_SetPlayerInterior" in rd("src/render/gfx_core.cpp") and "Gfx_SetPlayerInterior" in rd("src/port/gfx.h"), "renderer API")
need("Gfx_SetPlayerInterior" in rd("game/Game.cpp") and "View_GetActiveCameraMode" in rd("game/Game.cpp"), "game marks the interior view (camera type 3)")
gl = rd("src/render/gl_effects_executor.h")
need("MotionBlurUniforms_Pack" in gl and "keepCar = 0" in gl, "GL has no car region: gate off, full block uploaded")
need('{"Motion blur keeps the car sharp (0/1)", "video.motion_blur_car", 0, 1, 1, 1}' in rd("game/TuningPanel.h"), "panel row, default 1")
need("video.motion_blur_car" in rd("docs/POST-EFFECTS.md"), "documented option")
# history ping-pong: the steady-state blend writes the slot it did not read - no full-screen copy per frame
need("render/motion_blur_history.h" in g and "MotionBlurHistory_Commit" in g, "ping-pong state machine in use")
blend = g.index("PostProcessPass2(cmd, MotionBlurPipeline()")
need("SDL_CopyGPUTextureToTexture" not in g[blend:g.index("MotionBlurHistory_Commit", blend)], "no copy pass in the steady-state blend")
# MRT mirror: the blend also lands in the back buffer when its input is elsewhere, so a terminal
# stack presents with no HUD-handoff blit (the old code paid a full-screen history copy instead).
need("layout(location = 1) out vec4 outPresent;" in s and "outPresent = outColor;" in s, "shader mirrors the blend to a second output")
need("presentTarget = presentSource != backColor" in g, "second target is the back buffer only when the blend input is not")
need("num_color_targets = 2" in g, "blend pipeline declares two color targets")
need("MotionBlurHistory_Seed" in g, "first frame after a reset still seeds once")
need(g.count("MotionBlurHistory_Reset()") >= 5, "toggle, resize, disabled, bypass and non-race paths all reset")
print("motion blur car-keep wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
