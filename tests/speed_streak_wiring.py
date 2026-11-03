#!/usr/bin/env python3
"""Source wiring and shader/header constant parity for the speed streak (default off; no physics; not a car mask)."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
def rd(p): return (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
h, f = rd("src/render/speed_streak.h"), rd("src/render/shaders/speedstreak.frag")
def hv(n): return float(re.search(n + r"\s*=\s*([0-9.]+)f?;", h).group(1))
def fv(n): return float(re.search(r"const (?:float|int) " + n + r"\s*=\s*([0-9.]+);", f).group(1))
need(hv("kSpeedStreakInner") == fv("INNER"), "INNER differs between header and shader")
need(hv("kSpeedStreakRamp") == fv("RAMP"), "RAMP differs")
need(hv("kSpeedStreakMaxLen") == fv("MAXLEN"), "MAXLEN differs")
need(int(hv("kSpeedStreakTaps")) == int(fv("TAPS")), "TAPS differs")
need("aspect" in h or "settings.y" in f, "shader must aspect-correct the radius")
need(re.search(r"length\(c \* vec2\(settings\.y, 1\.0\)\)", f) is not None, "radius must be aspect corrected")
need("previous" not in f, "the streak must not read a previous frame")
need("speedstreak.frag" in rd("src/CMakeLists.txt"), "shader not built")
g = rd("src/render/gfx_gpu.cpp")
need("SHADER_CODE(speedstreak_frag)" in g and "encoded|=256" in g, "pass not wired")
need(re.search(r"SpeedStreakPipeline\(\), gradingTarget", g) is not None, "streak must write gradingTarget")
need("streakApplied ? godRaysBuf1 : gradingTarget" in g, "grading must not write its own input")
need("Gfx_SetSpeedStreak(SpeedStreak_Strength(" in rd("game/Race.cpp") and 'Sys_GetOption("video.speed_streak", 0)' in rd("game/Race.cpp"), "per-tick feed missing or default not 0")
need('"video.speed_streak", 0, 100, 10, 100' in rd("game/TuningPanel.h"), "panel row missing or default not 0")
need("Speed streak:" in rd("game/RallyData.cpp"), "overlay line missing")
x = rd("src/render/gl_effects_executor.h")
need("g_speedstreak_frag_glsl" in x and '"SpeedStreakUniforms"' in x and "run(10, 0, u, sizeof(u))" in x, "GL streak pass not wired")
bits=re.search(r"bits\[\]=\{([^}]+)\}",x)
need(bits is not None and [int(v) for v in bits.group(1).split(",")][10]==256,"GL streak encoded bit missing")
table=re.search(r"effectPrograms\[(\d+)\]",rd("src/render/gfx_gl.cpp"))
need(table is not None and int(table.group(1))>10,"GL program table too small for streak index10")
need("void SetSpeedStreak" in rd("src/render/gfx_gl.cpp"), "GL SetSpeedStreak missing")
print("speed streak wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
