#!/usr/bin/env python3
"""Wiring checks for video.windshield_dirt_fade_cracked (default off: cracked glass can show the dirt film dimmed)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
wd = rd("game/WindshieldDirt.cpp"); wh = rd("game/WindshieldDirt.h"); en = rd("src/platform/enhancements.h"); rb = rd("rebuild.sh")
need("Windshield_DirtAlphaScale" in wh and "Windshield_DirtAlphaScale" in wd, "alpha scale helper must exist and be used in the draw")
need('Sys_GetOption("video.windshield_dirt_fade_cracked", 0)' in wd, "option must default off")
need("if (glassState == 2) return 0;" in wh, "broken glass must always hide the dirt overlay")
need("if (glassState == 1 && fadeCracked) return 128;" in wh, "cracked glass fades to half only when enabled")
need("--windshield-dirt-fade-cracked" in en and "video.windshield_dirt_fade_cracked" in en, "CLI flag must map to the option")
assert en.count("--windshield-dirt-fade-cracked") >= 3, "flag needs the whitelist, parser and help entries"
need("--windshield-dirt-fade-cracked" in rb, "rebuild.sh must pass the flag through")
print("windshield dirt fade wiring ok")
