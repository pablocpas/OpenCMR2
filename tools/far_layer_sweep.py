#!/usr/bin/env python3
"""Sweep every stage for faults of the far layer (game/FarLayer.cpp).

stage_probe (OPENCMR2_PROBE_SWEEP) places the car along each stage's route,
facing along it and across it, and compares, at the same moment, the frame
without the layer (sky keyed) with the frame with the layer in marker colours:

- faults: layer pixels where the stage was. The layer is drawn behind the
  stage, so this must be 0.
- near_sky: layer pixels nearer than half the view distance where the sky
  was: the layer close to the car, seen through a gap or over it. Views with
  many are worth a look.
- far: layer pixels beyond, what the layer adds to the horizon.

Writes OUT/summary.csv (one line per view), OUT/index.html (the stages and
the views to look at, with the frames as drawn and marked) and the frames.

    tools/far_layer_sweep.py --probe build/linux/tests/stage_probe --data GAME --out DIR
"""
import argparse
import concurrent.futures
import csv
import html
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib

COUNTRIES = ["Finland", "Greece", "France", "Sweden", "Australia", "Kenya", "Italy", "United Kingdom"]
STAGES = 10
NEAR_SKY_LOOK = 500   # near pixels in the sky that make a view worth a look
FAULT = 200          # layer pixels over the stage that make a view faulty (particles drawn at random
                     # places each frame, snow or rain, account for a few dozen)


def run(probe, data, out, country, stage, spacing):
    work = out / f"{country}-{stage:02d}"
    (work / "user").mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, XDG_DATA_HOME=str(work / "user"), SDL_AUDIODRIVER="dummy",
               OPENCMR2_PROBE_SWEEP=str(spacing), OPENCMR2_PROBE_SWEEP_OUT=str(work),
               OPENCMR2_PROBE_UNLOCK_TRACKS="1", OPENCMR2_PROBE_CURSORS=f"36:{country},37:{stage}")
    # A probe now and then stalls before the race when several start at once;
    # it gets one more try.
    for attempt in range(2):
        try:
            result = subprocess.run([probe, data, str(work / "trace.jsonl"), "60", "100000"], env=env,
                                    capture_output=True, text=True, timeout=900)
            stderr = result.stderr
        except subprocess.TimeoutExpired:
            stderr = ""
        (work / "trace.jsonl").unlink(missing_ok=True)
        found = re.search(r"probe: country=(\d+) stage=(\d+)", stderr)
        rows = []
        if (work / "sweep.csv").exists():
            with (work / "sweep.csv").open() as file:
                rows = list(csv.DictReader(file))
        if found and (int(found.group(1)), int(found.group(2))) == (country, stage) and rows:
            return country, stage, work, rows
    return country, stage, work, None


def read_bmp(path):
    data = path.read_bytes()
    offset, = struct.unpack_from("<I", data, 10)
    width, height = struct.unpack_from("<ii", data, 18)
    rows = [data[offset + y * width * 4: offset + (y + 1) * width * 4] for y in range(abs(height))]
    return width, abs(height), rows if height < 0 else rows[::-1]


def write_png(path, bmp, scale=2):
    """The BMP at 1/scale size as a PNG."""
    width, height, rows = read_bmp(bmp)
    w, h = width // scale, height // scale
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        row = rows[y * scale]
        for x in range(w):
            b, g, r = row[x * scale * 4: x * scale * 4 + 3]
            raw += bytes((r, g, b))
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--probe", required=True)
    ap.add_argument("--data", required=True)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--spacing", type=float, default=200, help="metres between views along the route")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--countries", default="0-7", help="e.g. 0-7 or 2,5")
    ap.add_argument("--stages", help="only these, as country:stage counted from 1 (e.g. 0:1,6:9)")
    args = ap.parse_args()
    countries = []
    for part in args.countries.split(","):
        a, _, b = part.partition("-")
        countries += range(int(a), int(b or a) + 1)
    args.out.mkdir(parents=True, exist_ok=True)

    jobs = [(c, s) for c in countries for s in range(STAGES)]
    if args.stages:
        jobs = [(int(c), int(s) - 1) for c, s in (part.split(":") for part in args.stages.split(","))]
    results = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = [pool.submit(run, args.probe, args.data, args.out, c, s, args.spacing) for c, s in jobs]
        for future in concurrent.futures.as_completed(futures):
            country, stage, work, rows = future.result()
            results.append((country, stage, work, rows))
            state = "not loaded" if rows is None else f"{len(rows)} views, {sum(int(r['faults']) >= FAULT for r in rows)} with faults"
            print(f"{COUNTRIES[country]} {stage + 1}: {state}", flush=True)
    results.sort()

    with (args.out / "summary.csv").open("w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["country", "stage", "view", "node", "heading", "x", "z", "faults", "near_sky", "near", "far", "pixels"])
        for country, stage, _, rows in results:
            for r in rows or []:
                writer.writerow([COUNTRIES[country], stage + 1] + [r[k] for k in
                                ("view", "node", "heading", "x", "z", "faults", "near_sky", "near", "far", "pixels")])

    # The page: per stage its totals, then the views with faults or the layer
    # near in the sky, and one view of every stage for the look.
    images = args.out / "images"
    images.mkdir(exist_ok=True)
    lines = ["<!doctype html><meta charset=utf-8><title>Far layer sweep</title>",
             "<style>body{font:14px sans-serif;margin:16px}table{border-collapse:collapse}td,th{padding:2px 8px;"
             "border-bottom:1px solid #ccc;text-align:right}img{width:100%;max-width:576px}figure{display:inline-block;"
             "margin:4px;width:48%;min-width:280px}.bad{color:#b00}</style>",
             "<h1>Far layer sweep</h1><table><tr><th>stage<th>views<th>with faults<th>near in the sky<th>layer share</tr>"]
    shown = []
    for country, stage, work, rows in results:
        name = f"{COUNTRIES[country]} {stage + 1}"
        if rows is None:
            lines.append(f"<tr><td style=text-align:left>{html.escape(name)}<td colspan=4>not loaded</tr>")
            continue
        faults = [r for r in rows if int(r["faults"]) >= FAULT]
        near = [r for r in rows if int(r["near_sky"]) > NEAR_SKY_LOOK]
        share = sum(int(r["far"]) + int(r["near"]) for r in rows) / max(1, sum(int(r["pixels"]) for r in rows))
        lines.append(f"<tr><td style=text-align:left>{html.escape(name)}<td>{len(rows)}<td class={'bad' if faults else ''}>"
                     f"{len(faults)}<td>{len(near)}<td>{share:.1%}</tr>")
        sample = max(rows, key=lambda r: int(r["far"])) if rows else None
        for r in faults + near + ([sample] if sample else []):
            view = int(r["view"])
            if (work / f"marked-{view:04d}.bmp").exists() and (name, view) not in [(n, v) for n, v, *_ in shown]:
                shown.append((name, view, work, r))
    lines.append("</table>")
    for name, view, work, r in shown:
        label = f"{name}, view {view} ({'along' if r['heading'] == '0' else 'across'} the route at {r['x']}, {r['z']})"
        lines.append(f"<h3>{html.escape(label)}: faults {r['faults']}, near in the sky {r['near_sky']}, far {r['far']}</h3>")
        for kind in ("shown", "marked"):
            bmp = work / f"{kind}-{view:04d}.bmp"
            if bmp.exists():
                png = images / f"{work.name}-{kind}-{view:04d}.png"
                write_png(png, bmp)
                lines.append(f"<figure><img src=images/{png.name}><figcaption>{kind}</figcaption></figure>")
    (args.out / "index.html").write_text("\n".join(lines) + "\n")
    bad = sum(1 for *_, rows in results if rows and any(int(r["faults"]) >= FAULT for r in rows))
    print(f"{len(results)} stages, {bad} with faults; {args.out / 'index.html'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
