#!/usr/bin/env python3
"""External replay cameras drop tunnel vision and the speed streak at once; inside-car views and driving keep them. Source wiring only, not a native check."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
race = rd("game/Race.cpp")
pol = rd("game/SpeedLookPolicy.h")
m = re.search(r"bool SpeedLook_ExternalReplayView\(int viewType, bool replayPlaying\)\s*\{(.*?)\n\}", pol, re.S)
need(m is not None, "classifier missing")
body = m.group(1) if m else ""
need("if (!replayPlaying) return false;" in body, "driving must never be classified external")
need("return viewType != 3;" in body, "only the interior view (type 3) may keep the effects during replay")
need("SpeedLook_ScreenEffectsOff(viewType, Race_GetPlayerRecordField4(0) != 0, View_IsDroneCamera(0), Sys_GetOption(\"video.speed_effects\", 1) != 0)" in race and "Gfx_ClearSpeedEffects();" in race, "must clear on external replay views")
need("Gfx_SetVaporTrail(" in race.split("Gfx_ClearSpeedEffects();")[1].split("return;")[0], "vapor trails must stay on the world-space path")
need("virtual void ClearSpeedEffects()" in rd("src/render/gfx_internal.h"), "executor hook missing")
for f in ("src/render/gfx_gpu.cpp", "src/render/gfx_gl.cpp"):
    need(re.search(r"void ClearSpeedEffects\(\) override \{ speedFocusTarget = speedFocusSmooth = 0\.0f; speedStreakTarget = speedStreakSmooth = 0\.0f;", rd(f)) and "tunnelFocusTargetValid = 0;" in rd(f), f + " must zero targets and eased values (incl. the tunnel tracking focus)")
need("Gfx_ClearSpeedEffects" in rd("src/port/gfx.h") and "Gfx_ClearSpeedEffects" in rd("src/render/gfx_core.cpp"), "C entry point missing")
print("replay external wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)

# Render-time defence is independent of the earlier physics update ordering.
fixed=rd("game/FixedPoint.cpp")
projection=fixed[fixed.index("void View_SetPlayerProjection(unsigned int player)"):fixed.index("// Layout of one force-feedback slot")]
need("Gfx_SetSpeedEffectsSuppressed(View_IsDroneCamera(player));" in projection,"drone exclusion at actual rendered view")
for backend in ("src/render/gfx_gl.cpp","src/render/gfx_gpu.cpp"):
    b=rd(backend)
    need("speedEffectsSuppressed=v;if(v)ClearSpeedEffects();" in b,"clear eased blur/streak immediately")
    need("speedFocusTarget = speedEffectsSuppressed ||" in b and "speedStreakTarget = speedEffectsSuppressed ||" in b,"reject late speed writes for drone")
