#!/usr/bin/env python3
"""Blur mode wiring: one speed curve (the tunnel strength), a real car region from vertices, SDL_GPU only, off by default, visible in the panel and overlay."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
s = rd("src/render/shaders/dof.frag"); g = rd("src/render/gfx_gpu.cpp")
need("vec4 carRect;" in s and "vec4 carMode;" in s, "shader uniforms")
need("peripheral = tunnel * (1.0 - inU * inV * inZ)" in s, "whole-scene mode uses the same tunnel strength, minus the car region")
need("if (carMode.x > 0.5)" in s, "mode 0 must keep the old path")
need("float uniforms[20]" in g and "CarRegion_AddDraw(carRegion" in g and "blurShown" in g, "SDL_GPU computes the region from vertices")
need("if (!blurShown) for (int k = 8; k < 16; ++k) uniforms[k] = 0.0f;" in g, "no region = zeros = old look")
need('Sys_GetOption("video.blur_mode", 0)' in g, "default off")
need('"video.blur_mode", 0, 1, 1, 0' in rd("game/TuningPanel.h"), "panel row, default 0")
need("Blur mode:" in rd("game/RallyData.cpp"), "overlay line")
gl = rd("src/render/gl_effects_executor.h")
need("carMode" not in gl and "blur_mode" not in gl, "GL path untouched (peripheral only)")
print("blur mode wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
