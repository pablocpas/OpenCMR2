#!/usr/bin/env python3
"""Generated-cap loader integration: convert.py's cap material and cap.tga must survive the
runtime loader path. Runs the native SceneryPackFormat parser on a freshly generated tree and
applies the loader's material rule (SceneryPack.cpp:165-171): every usemtl group needs a
map_Kd entry, and every group resolves to a texture file that exists in the pack dir."""
import importlib.util, subprocess, sys, tempfile
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
fmt_test = Path(sys.argv[1])
spec = importlib.util.spec_from_file_location('patcher', HERE.parent / 'tools/scenery-patch/convert.py')
patcher = importlib.util.module_from_spec(spec); spec.loader.exec_module(patcher)

fails = 0
def need(cond, msg):
    global fails
    if not cond: print('FAIL', msg); fails += 1

with tempfile.TemporaryDirectory() as d:
    root = Path(d)
    patcher.root = root / 'out'; patcher.root.mkdir()
    src = root / 'tre_cap.dds'
    im = Image.new('RGBA', (128, 128), (0, 0, 0, 0)); dr = ImageDraw.Draw(im)
    dr.ellipse((8, 4, 120, 100), fill=(40, 160, 60, 255))
    dr.rectangle((58, 80, 70, 127), fill=(90, 60, 30, 255))
    im.save(src)
    rel = patcher.tree({'path': str(src), 'archive': 'finhit.bfl', 'texture': 'tre_cap.dds'},
                       Path('trees/tre_cap'), adaptive=True)
    tree = patcher.root / rel
    run = subprocess.run([str(fmt_test), str(tree)], capture_output=True, text=True)
    need(run.returncode == 0, 'native OBJ parser accepts the generated cap tree: ' + run.stdout + run.stderr)
    materials = [line.split(' ', 1)[1] for line in run.stdout.splitlines() if line.startswith('material ')]
    need('cap' in materials, 'cap group parsed (got %r)' % materials)
    # Loader rule from SceneryPack.cpp: every group name must resolve through tree.mtl map_Kd
    # to a texture inside the asset directory, or the tree is rejected.
    maps = {}
    cur = None
    for line in tree.with_name('tree.mtl').read_text().splitlines():
        t = line.split()
        if not t: continue
        if t[0] == 'newmtl': cur = t[1]
        elif t[0] == 'map_Kd': maps[cur] = t[1]
    for m in materials:
        need(m in maps, 'material %s has a map_Kd entry' % m)
        need(m in maps and (tree.parent / maps[m]).exists(), 'texture for %s exists in the pack dir' % m)
    cap_tga = tree.parent / 'cap.tga'
    need(cap_tga.exists(), 'cap.tga written next to tree.obj')
    if cap_tga.exists():
        arr = np.array(Image.open(cap_tga).convert('RGBA'))
        need(arr.shape == (256, 256, 4), 'cap.tga is 256x256 RGBA')
        need((arr[:, :, 3] > 0).any(), 'cap.tga has opaque canopy pixels')
print('ok generated cap tree loads with cap material' if not fails else 'FAILED')
sys.exit(0 if not fails else 1)
