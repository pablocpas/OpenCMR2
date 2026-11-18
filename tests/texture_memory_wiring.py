#!/usr/bin/env python3
"""Texture memory overlay wiring: the F10 panel shows live texture retention from
Gfx_GetTextureMemoryStats so the 10 GB memory report can be attributed. The panel
budget test owns row accounting; this pins the data path."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
rdc = (r / "game/RallyData.cpp").read_text()
gfx_h = (r / "src/port/gfx.h").read_text()
core = (r / "src/render/gfx_core.cpp").read_text()
# Deliberately loose: the behavioural C++ test owns correctness; this only checks the
# panel actually calls the stats function, not formatting or loop shape (review feedback).
need("Gfx_GetTextureMemoryStats(" in rdc, "panel reads the texture memory stats")
need("Gfx_GetTextureMemoryStats" in gfx_h, "stats function declared in gfx.h")
need("Gfx_GetTextureMemoryStats" in core, "stats implemented over the live texture registry")
print("ok texture memory wiring" if ok else "FAILED")
sys.exit(0 if ok else 1)
