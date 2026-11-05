#!/usr/bin/env python3
"""GL tunnel vision: the OpenGL executor must feed the same 8-float DoFUniforms as SDL_GPU, ease the speed at 0.8 s, and gate on race frames."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
x, l = rd("src/render/gl_effects_executor.h"), rd("src/render/gfx_gl.cpp")
need("void SetSpeedFocus(float speed) override" in l, "GL SetSpeedFocus must not be the base no-op")
need("exp(-dt / 0.8f)" in x, "GL tunnel easing must match SDL_GPU (0.8 s)")
need("float u[20]" in x and "speedFocusSmooth, aspect, kTunnelRadiusPerHeight * (float)backHeight" in x, "GL DoF uniforms must carry tunnel, aspect and radius")
need("u[16] = tunnelFocusU" in x and "u[17] = 1.0f - tunnelFocusV" in x, "GL DoF uniforms must carry the tunnel tracking focus (v pre-flipped for GL)")
need("float u[260]" in x and "u[256] = tunnelFocusU" in x, "GL streak uniforms must carry the tunnel tracking focus")
need("bool tunnelOn = effectsRaceFrame &&" in x, "tunnel must be race-frame gated")
need("depthBlurOn ? dofBlur * 0.04f : 0.0f" in x, "tunnel-only must have zero depth blur")
need("Gfx_SetDofStatus(GfxDofStatus{5," in x, "overlay status must be reported on GL")
g = rd("src/render/gfx_gpu.cpp")
need(re.search(r"kTunnelRadiusPerHeight \* \(float\)backHeight", g) is not None, "SDL_GPU radius constant moved: keep GL in sync")
need("Tunnel vision: n/a (OpenGL backend)" in rd("game/RallyData.cpp") and "bool gl=false" in rd("game/RallyData.cpp"), "overlay must no longer be n/a on GL")
print("gl tunnel wiring ok" if ok else "FAILED");

# The road projection must happen inside this frame's world draw, after camera
# blending/projection setup, not from the earlier simulation update.
race=rd("game/Race.cpp"); game=rd("game/Game.cpp")
need("Race_ApplyTunnelFocus();" not in race[race.index("void Race_UpdateFrameEffects(int param_1)"):race.index("void Race_UpdateFrameEffects(int param_1)")+2000],"no stale simulation focus feed")
need(game.index("CGraphics::SetProjection(0, 0, farPlane, 0);") < game.index("Race_ApplyTunnelFocus();") < game.index("Gfx_EndWorldEffects(); // world FX"),"focus feed belongs to completed live projection before world effects")
need("TunnelRoad_ProjectPoint(&farPoint" in race,"runtime must use tested 16.16 projection adapter")
sys.exit(0 if ok else 1)
