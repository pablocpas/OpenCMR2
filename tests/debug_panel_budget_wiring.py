#!/usr/bin/env python3
"""F10 panel budget wiring: the sprite queue and the panel background must cover the whole
diagnostic panel. Two native truncations motivate this: the 1023-sprite layer cap killed the
panel mid-row ("fla"), and the row-count expression undercounted by 7 so late rows had no panel
behind them. The queue must fit the worst case (every row at its full 159-character buffer)."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False

rdc = rd("game/RallyData.cpp")
sh = rd("game/Sprite.h"); sc = rd("game/Sprite.cpp"); gc = rd("game/Game.cpp")

# Panel row accounting: the line() lambda rows between the row counter and the end of the panel.
row_m = re.search(r"int row=spectator\?(\d+):(\d+);", rdc)
need(row_m, "row counter 'int row=spectator?H:h;' present")
start = row_m.start() if row_m else rdc.index("int row=spectator?")
end = rdc.index("DebugOverlays_RequestView", start)
block = rdc[start:end]
sites = len(re.findall(r"\bline\(", block))  # call sites only (the lambda declaration is "line=[")
# A for-loop over N rows from one call site (launch summary) expands the count.
loops = sum(int(n) - 1 for n in re.findall(r"for\s*\([^)]*<\s*(\d+)\s*;[^)]*\)\s*line\(", block))
rows = sites + loops
m = re.search(r"DebugOverlayLayout::For\(g_pGraphics->resX,g_pGraphics->resY,\(spectator\?(\d+):(\d+)\)\+(\d+)\)", rdc)
need(m, "panel budget expression (spectator?H:h)+N")
need(m and int(m.group(3)) == rows, f"budget covers the line() rows: {rows} counted ({sites} call sites + {loops} from loops), budget says {m.group(3) if m else '?'}")
# The row counter and the layout base must use the same header counts, or rows drift off the panel.
need(m and row_m and (m.group(1), m.group(2)) == (row_m.group(1), row_m.group(2)),
     f"row counter headers ({row_m.group(1) if row_m else '?'}:{row_m.group(2) if row_m else '?'}) match the layout base ({m.group(1) if m else '?'}:{m.group(2) if m else '?'})")
need("header rows + one entry per line() row below" in rdc, "budget keeps the sync comment")

# Queue capacity: worst case is spectator (its header count from the layout expression) with every row at its full text[160] buffer.
cap = int(re.search(r"#define SPRITE_LAYER_MAX (0x[0-9a-fA-F]+|\d+)", sh).group(1), 0)
spectator_headers = int(m.group(1)) if m else 0
worst = (spectator_headers + rows) * 159
need(cap >= worst, f"sprite layer cap {cap} fits the worst-case panel: {spectator_headers}+{rows} rows x 159 glyphs = {worst}")
need(cap >= 0x1fff, "cap raised past the original 0x3ff that truncated the panel at 'fla'")

# The flush loop writes 4 vertices per sprite with no bounds check: the vertex buffers must scale with the cap.
need("1024 * 4" not in sc and sc.count("[(SPRITE_LAYER_MAX + 1) * 4]") == 4, "all four sprite vertex buffers scale with SPRITE_LAYER_MAX")
need("loggedFull" in sc and "sprite layer" in sc, "a full layer logs once instead of dropping silently")

# Exterior window gate engagement is visible in the panel itself (no log grep needed).
need("unsigned g_cockpitWindowGate[2] = {0, 0};" in gc, "gate engagement counters defined")
need('g_cockpitWindowGate[0]++;' in gc and 'g_cockpitWindowGate[1]++;' in gc, "counters counted on the view-tied car path")
need("Window gate: exterior window draws %u | no depth write %u" in rdc, "panel shows the gate counters")
print("debug panel budget wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
