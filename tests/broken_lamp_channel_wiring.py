#!/usr/bin/env python3
"""Broken lamp glass must gate the shared type-8 light channel: with both
sides' lamp glass broken no lamp may still emit light. The gate is
both-sides only - the channel carries one shared level, so single-side
gating could extinguish an intact lamp (no per-side data exists)."""
import pathlib, sys
s = pathlib.Path(__file__).resolve().parents[1] / "game" / "StageObjects.cpp"
t = s.read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0 || broken1 == 0, broken0 == 0 || broken1 == 0, 0x10);" in t,
     "type-8 channel must be masked off when both sides' lamp glass is broken")
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 1);" in t,
     "original per-side headlight gating must be preserved")
need("CarLight_SetChannelMasks(LIGHT_CAR, broken0 == 0, broken1 == 0, 2);" in t,
     "original per-side brake gating must be preserved")
need("CarLight_SetChannelMasks(LIGHT_CAR, 1, 1, 0x10);" not in t,
     "ungated always-on type-8 mask must be gone")
print("broken lamp channel wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
