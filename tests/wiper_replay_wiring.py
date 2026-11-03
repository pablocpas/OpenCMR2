#!/usr/bin/env python3
"""Wiring check: the live wiper key is ignored while a replay plays."""
import sys
s = open("game/RallyData.cpp", encoding="utf-8").read()
a = s.index("void WindscreenWiper_UpdateInput(void)")
body = s[a:a + 900]
if "Race_GetPlayerRecordField4(0) != 0" not in body or body.index("Race_GetPlayerRecordField4(0)") > body.index("WindscreenWiper_Toggle"):
    print("FAIL: wiper input must return early during replay"); sys.exit(1)
print("wiper replay wiring ok")
