#!/usr/bin/env python3
"""Broken lamp glass must gate the light channels of its own end: break
slots 4/5 hold the rear lamp pair (brake/reverse) and 6/7 the front pair
(headlight/shared). The shared channel carries one level for both sides, so
it goes dark only when BOTH front lamps are broken - single-side gating
could extinguish an intact lamp (no per-side data exists). The behavioural
pin lives in broken_lamp_gating.cpp; this file guards the exact wiring."""
import pathlib, sys
s = pathlib.Path(__file__).resolve().parents[1] / "game" / "StageObjects.cpp"
t = s.read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("CarLight_SetChannelMasks(LIGHT_CAR, pParts->partBroken[6] == 0 || pParts->partBroken[7] == 0, pParts->partBroken[6] == 0 || pParts->partBroken[7] == 0, 0x10);" in t,
     "shared channel must be masked off when both front lamps (slots 6/7) are broken")
need("CarLight_SetChannelMasks(LIGHT_CAR, pParts->partBroken[6] == 0, pParts->partBroken[7] == 0, 1);" in t,
     "headlight channel must be gated per side on the front lamp pair (slots 6/7)")
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 2);" in t,
     "original per-side brake gating on the rear pair (slots 4/5) must be preserved")
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 8);" in t,
     "original per-side reverse gating on the rear pair (slots 4/5) must be preserved")
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 1);" not in t,
     "headlight gating on the rear pair must be gone")
need("CarLight_SetChannelMasks(LIGHT_CAR, 1, 1, 0x10);" not in t,
     "ungated always-on shared-channel mask must be gone")
print("broken lamp channel wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
