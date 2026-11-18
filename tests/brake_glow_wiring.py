#!/usr/bin/env python3
"""Brake glow heat must tick per consumed physics step (sim-rate governed, render-rate
independent), and the draw must never index the car table after Car_FreeTable: the
order count outlives the table and menu scenes reuse the 3D pass."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
car = rd("game/Car.cpp")
need("BrakeGlow_TickStep(pList[i], 25.0f / pCar->simulationRateHz);" in car,
     "heat tick must run from Car_DecrementContactTimers with the car's real step duration")
need("Car_UpdateWheelMeshStates(void)" in car and "BrakeGlow_UpdateCar" not in car,
     "heat tick must not run from the per-render-frame wheel mesh refresh")
dec = car.split("void Car_DecrementContactTimers")[1]
need(dec.index("simulationStepsRemaining--") < dec.index("BrakeGlow_TickStep"),
     "heat tick must be inside the steps-remaining branch")
glow = rd("game/BrakeGlow.cpp")
need("g_carBuffer == NULL || g_carCount <= 0 || g_carOrderCount <= 0" in glow,
     "draw must guard the freed car table (Car_FreeTable leaves the order count set)")
need("car >= g_carCount" in glow, "draw must validate the car index against g_carCount")
need("Gfx_DestroyTexture(tex);" in glow and "s_texFailed" in glow, "texture create+lock failure must not leak one texture per frame")
need("BrakeGlow_DiscRadius(s_extent[car][w])" in glow and "MeshVertexF" in glow, "glow quad must scale off the measured wheel mesh extent, not a fixed radius")
need("BrakeGlow_FillMissingExtents(s_extent[car])" in glow,
     "an unreadable wheel mesh must borrow a sibling's measured extent, not the oversized fixed fallback")
print("brake glow wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
