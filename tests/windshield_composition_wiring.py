#!/usr/bin/env python3
"""Pin composition order to the actual game draw, not the prep callback."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
g=(r/'game/Game.cpp').read_text(); d=(r/'game/RallyData.cpp').read_text()
a=g.index('int Game_DrawSceneViewport(SceneNode *pRoot');g=g[a:]
assert g.index('Gfx_EndWorldEffects();') < g.index('Windshield_DrawOverlay(') < g.index('ScreenLine2D_Draw(3);')
assert 'Windshield_DrawOverlay(maskedView' not in d
assert 'if (glassType == 3)' in g
assert 'flag != 0 && bit >= 0 && bit < 2 && pCamera == g_viewNodes[bit]' in g
print('PASS windshield composes after world effects and before HUD, cockpit only')
