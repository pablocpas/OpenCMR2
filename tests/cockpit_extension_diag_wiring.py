#!/usr/bin/env python3
"""Wiring checks for the replay cockpit visibility diagnostics:
the extension probe fires per cockpit-view entry after the transforms settle
(not once per launch on the first, unsettled frame), the extension draw logs
why it is not on screen per session (mode 4 = replay), and the windshield
glass state transition log carries the stage clock and game mode."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
wg = rd("game/WindshieldGlass.cpp"); wd = rd("game/WindshieldDirt.cpp")
need(wg.count("WindshieldGlass_ArmCockpitExtensionProbe") >= 3,
     "probe armed from the option's rising edge AND from each fresh registration (re-arms per session)")
need("s_probeSettle" in wg and "--s_probeSettle" in wg or "s_probeSettle--" in wg or "s_probeSettle -=" in wg,
     "probe fires only after the settle countdown, never on the arming frame")
need("cockpit extension: car %d mode %d not drawn (%s)" in wg and "cockpit extension: car %d mode %d drawing" in wg,
     "draw transition logs with game mode present")
need("passenger profile unmatched" in wg and "texture missing" in wg and "no strip geometry" in wg,
     "each skip reason named")
need("clock %d mode %d" in wd, "glass state transition log carries stage clock and game mode")
need("StageTiming_GetClockTime" in wd, "transition log reads the stage clock")
clear = wg.split("void WindshieldGlass_ClearCockpitExtensions(void)", 1)[1].split("}", 1)[0]
need("g_wgExtDrawState[i] = 0" in clear and "g_wgExtSkipReason[i] = NULL" in clear,
     "ClearCockpitExtensions must reset the draw-transition state so the next session logs its own verdict")
print("cockpit extension diag wiring ok")
