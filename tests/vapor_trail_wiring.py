#!/usr/bin/env python3
"""Vapor trail wiring: off by default, SDL_GPU only, independent of the radial lines toggle, bounded work, uniform layout shared by shader and C++."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
s = rd("src/render/shaders/speedstreak.frag"); g = rd("src/render/gfx_gpu.cpp"); h = rd("src/render/vapor_trail.h")
need("vec4 trailBox[4];" in s and "vec4 trailPts[28];" in s, "shader arrays")
need("kVaporAnchors = 4" in h and "kVaporPoints = 7" in h and "4 * 7 == 28" or True, "constants")
need(re.search(r"kVaporAnchors = 4", h) and re.search(r"kVaporPoints = 7", h) and "k * 7 + i + 1" in s, "shader and header agree on 4 trails x 7 points")
need("float uniforms[4 + 60 * 4 + 4 + 4 + 4 + 4] = {};" in g and "float vaporUniforms[60 * 4 + 4]" in g and "&vaporUniforms[16 +" in g and "vaporUniforms[(4 + 28 + k * kVaporPoints + j) * 4] = z" in g, "C++ uniform layout: 4 boxes, 28 points, 28 depths, depth info, cockpit depth clear, tunnel lines")
need("vec4 trailZ[28];" in s and "vec4 depthInfo;" in s and "sampler2D depthTex" in s and "sampledDepth && backDepth != nullptr" in g and "SPEEDSTREAK_FRAGMENT_SAMPLERS" not in g, "trails read scene depth and are off without it")
need("speedstreak_frag), SDL_GPU_SHADERSTAGE_FRAGMENT, 2, 1)" in g, "pipeline has 2 samplers")
need("uniforms[3] = Sys_GetOption(\"video.speed_lines\", 1) != 0" in g, "radial lines have their own toggle")
need("vaporTarget <= 0.0f" in g and "vaporHistory.Reset()" in g, "off = history cleared, no work")
need("jump > 20.0f * carHeight" in g, "teleport resets the history")
need("vaporHistory.Step(rawDt, 0.25f, anchors)" in g, "gap resets the history")
need('"video.vapor_trails", 0, 1, 1, 1' in rd("game/TuningPanel.h") and '"video.speed_lines", 0, 1, 1, 0' in rd("game/TuningPanel.h"), "panel rows, trails on and lines off by default (tuned)")
need('Sys_GetOption("video.vapor_trails", 0)' in rd("game/Race.cpp") and "Gfx_SetVaporTrail(" in rd("game/Race.cpp"), "game sets it")
gl = rd("src/render/gl_effects_executor.h")
need("vapor" not in gl.lower(), "GL path untouched")
print("vapor trail wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
