#!/usr/bin/env python3
"""The cockpit windshield glass is resolved at interior attach, which runs before CarEffects_Init fills g_carWindowVerts. CarEffects_CarWindowVertices must therefore read the CIN glass section itself when the table is still empty, or the feature latches off with 'no glass profile'."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
ce = rd("game/CarEffects.cpp")
m = re.search(r"const FixVector \*CarEffects_CarWindowVertices\(int car\)\s*\{(.*?)\n\}", ce, re.S)
need(m is not None, "CarEffects_CarWindowVertices not found")
body = m.group(1) if m else ""
need("g_carWindowVerts[car] != NULL" in body, "must prefer the table entry once CarEffects_Init has filled it")
need("CarInfo_GetSection(pCar, CAR_INFO_GLASS)" in body, "must fall back to the CIN glass section when the table is empty")
need("CarInfo_GetDirectory(pCar->index) == NULL" in body, "fallback must not dereference an unloaded CIN directory")
st = rd("game/StageTiming.cpp")
need(st.find("    View_SetupCameras();") < st.find("    StageObjects_Init();"), "ordering assumption changed: re-check whether the fallback is still needed")
print("windshield glass profile order wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
