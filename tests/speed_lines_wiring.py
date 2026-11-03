#!/usr/bin/env python3
"""Speed lines: header/shader parity, shared strength (no second knob), time and strength fed on both backends."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
h, f = rd("src/render/speed_lines.h"), rd("src/render/shaders/speedstreak.frag")
for hn, fn in (("kSpeedLinesInner", "LINES_INNER"), ("kSpeedLinesFade", "LINES_FADE"), ("kSpeedLinesRays", "LINES_RAYS"), ("kSpeedLinesOnset", "LINES_ONSET"), ("kSpeedLinesFull", "LINES_FULL"), ("kSpeedLinesOpacity", "LINES_OPACITY")):
    hv = re.search(hn + r"\s*=\s*([0-9.]+)f;", h); fv = re.search(r"const float " + fn + r"\s*=\s*([0-9.]+);", f)
    need(hv and fv and float(hv.group(1)) == float(fv.group(1)), fn + " differs between header and shader")
need("speedLines(c * vec2(settings.y, 1.0), settings.z, 0.0, 1.0)" in f, "lines must use the aspect corrected radius and the time uniform")
need("settings.w" in f, "lines amount must come from settings.w")
need("previous" not in f, "no previous frame")
need(f.count("sampler2D") == 2, "scene plus depth for vapor trail occlusion")
g = rd("src/render/gfx_gpu.cpp")
need("uniforms[2] = speedStreakClock;" in g and "? speedStreakSmooth : 0.0f;" in g, "SDL_GPU must feed time and strength (strength gated by the video.speed_lines toggle)")
x = rd("src/render/gl_effects_executor.h")
need("speedStreakClock, speedStreakSmooth}" in x, "GL must feed time and strength")
need("speedStreakClock" in rd("src/render/gfx_gl.cpp"), "GL member missing")
need('"video.speed_lines", 0, 1, 1, 0' in rd("game/TuningPanel.h"), "one on/off row for the lines, default off per the tuned defaults (user asked for an independent toggle)")
print("speed lines wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
