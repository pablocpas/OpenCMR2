#!/usr/bin/env python3
"""Wiring checks for video.windshield_crack_overlay (default off, only when no interior glass part)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
wg = rd("game/WindshieldGlass.cpp"); wd = rd("game/WindshieldDirt.cpp"); en = rd("src/platform/enhancements.h"); rb = rd("rebuild.sh")
need('Sys_GetOption("video.windshield_crack_overlay", 0) == 0' in wg, "overlay must default off")
need("g_wgGlassPartCount[car] != 0" in wg, "overlay must only act when no interior glass part was selected")
need("WgFitGlassUV" in wg and "Gfx_SetDepthTest(TRUE)" in wg, "pane must use fitted exterior UVs and be depth tested")
need("WindshieldGlass_DrawCrackPane(" in wd and "Windshield_DrawCrackPane(bit)" in rd("game/Game.cpp"), "crack pane must be drawn from the 3D pass")
need("--windshield-crack-overlay" in en and "video.windshield_crack_overlay" in en, "CLI flag must map to the option")
need("--windshield-crack-overlay" in rb, "rebuild.sh must pass the flag through")
print("windshield crack overlay wiring ok")
