#!/usr/bin/env python3
"""Drone tuning rails: the TuningPanel rows and the CameraOperator runtime
clamps must carry the same widened ranges (user at the old 50/150/5/200
rails asked for even more radical; defaults unchanged)."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
panel = (r / "game" / "TuningPanel.h").read_text()
op = (r / "game" / "CameraOperator.cpp").read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
rows = {m[0]: m[1:] for m in re.findall(r'\{"[^"]*", "(video\.drone_[a-z_]+)", (\d+), (\d+), (\d+), (\d+)\}', panel)}
expect = {"video.drone_distance": (10, 300), "video.drone_height": (25, 400),
          "video.drone_lateral": (10, 400), "video.drone_spring": (1, 400),
          "video.drone_speed_drop": (0, 500)}
for key, (lo, hi) in expect.items():
    need(key in rows, f"tuning row missing for {key}")
    if key in rows:
        rlo, rhi, step, fallback = map(int, rows[key])
        need((rlo, rhi) == (lo, hi), f"{key} menu range {rlo}..{rhi} != {lo}..{hi}")
        need(lo <= fallback <= hi, f"{key} default {fallback} outside its range")
# runtime clamps must accept the full menu range
need('if(pct<10)pct=10;if(pct>300)pct=300' in op, "distance clamp must be 10..300")
need('Clamp(Sys_GetOption("video.drone_height",60)/100.f,.25f,4.f)' in op, "height clamp must be .25..4")
need('Clamp(Sys_GetOption("video.drone_lateral",70)/100.f,.1f,4.f)' in op, "lateral clamp must be .1..4")
need('Clamp(Sys_GetOption("video.drone_spring",150)/100.f,.01f,4.f)' in op, "spring clamp must be .01..4")
need('Clamp(Sys_GetOption("video.drone_speed_drop",150)/100.f,0.f,5.f)' in op, "drop clamp must be 0..5")
print("drone range wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
