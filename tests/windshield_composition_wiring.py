#!/usr/bin/env python3
"""Pin the windshield dirt to the glass: a depth-tested pane in the 3D pass, never a screen-space layer."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
g=(r/'game/Game.cpp').read_text(); d=(r/'game/RallyData.cpp').read_text()
w=(r/'game/WindshieldDirt.cpp').read_text(); gl=(r/'game/WindshieldGlass.cpp').read_text()
a=g.index('int Game_DrawSceneViewport(SceneNode *pRoot');g=g[a:]
# The old full-screen 2D overlay is gone everywhere.
for name,t in (('Game.cpp',g),('RallyData.cpp',d),('WindshieldDirt.cpp',w)):
    assert 'Windshield_DrawOverlay' not in t, name
# The crack/dirt panes are drawn inside the 3D pass, before world effects end and the HUD.
assert g.index('Windshield_DrawCrackPane(bit)') < g.index('Gfx_EndWorldEffects();') < g.index('ScreenLine2D_Draw(3);')
assert 'if (paneType == 3)' in g
# The dirt pane is part of the cockpit pane draw and registered to the glass mesh.
cp=w[w.index('void Windshield_DrawCrackPane(int view)'):]
assert 'Windshield_DrawDirtPane(mview)' in cp
pane=gl[gl.index('int WindshieldGlass_DrawDirtPane('):]
pane=pane[:pane.index('\n}\n')]
assert 'WindshieldGlass_GetFrontFrame' in pane and 'WgCollectDirtTriangles' in pane
assert 'Gfx_SetDepthTest(TRUE)' in pane and 'Gfx_SetDepthWrite(FALSE)' in pane
assert 'WsGlassFrame_UV' in pane
assert 'sx' not in pane and 'resX' not in pane   # no screen-space coordinates
# Without a glass mesh nothing is drawn (no screen-layer fallback).
assert 'return 0;   // no glass mesh to register to' in pane
print('PASS windshield dirt is a glass-registered, depth-tested pane in the 3D pass')
