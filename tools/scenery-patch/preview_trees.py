#!/usr/bin/env python3
"""Software review contact sheet for exported tree-card candidates.

Reads a ZIP produced by export-greece-trees.py, runs each candidate through
convert.py's real shell/crossed generators (same adaptive/sparse selection
rule as the pack build), renders the generated OBJ at 0/30/60/90 degrees with
a software z-buffered rasterizer, alpha-tested at ref 127 like the runtime
replacement path (game/SceneryPack.cpp), and writes one labeled contact-sheet
PNG. Standard library + numpy + Pillow only.

SCOPE: this is a geometric and alpha-coverage review aid. It is NOT a GPU
render, NOT the game's lighting/fog/LOD path, and NOT native appearance or
performance evidence. The earlier Finland/France additions were reviewed with
offscreen-GPU renders; this software sheet does not substitute for that bar
where it is required - it catches silhouette/stem/sparsity/alpha failures
before any GPU or native look. No game files are read or written; nothing is
uploaded.
"""
import importlib.util, io, json, math, sys, tempfile, zipfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

CONVERT = Path(__file__).with_name('convert.py')
_spec = importlib.util.spec_from_file_location('convert', CONVERT)
cv = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(cv)

VIEW_W, VIEW_H = 220, 300
ANGLES = (0, 30, 60, 90)

def load_obj(obj_path):
    verts, uvs, tris = [], [], []
    mat = None
    for line in obj_path.read_text().splitlines():
        f = line.split()
        if not f: continue
        if f[0] == 'v': verts.append([float(x) for x in f[1:4]])
        elif f[0] == 'vt': uvs.append([float(f[1]), float(f[2])])
        elif f[0] == 'usemtl': mat = f[1]
        elif f[0] == 'f':
            idx = [int(x.split('/')[0]) - 1 for x in f[1:4]]
            tris.append((idx, mat))
    return np.array(verts), np.array(uvs), tris

def render(verts, uvs, tris, textures, angle_deg):
    th = math.radians(angle_deg)
    c, s = math.cos(th), math.sin(th)
    # orthographic: screen x = rotated horizontal, y = height, depth = rotated depth
    rx = verts[:, 0] * c + verts[:, 2] * s
    rz = -verts[:, 0] * s + verts[:, 2] * c
    ry = verts[:, 1]
    margin = 8
    xmin, xmax = float(rx.min()), float(rx.max())
    span = max(xmax - xmin, 1e-6)
    scale = min((VIEW_W - 2 * margin) / span, (VIEW_H - 2 * margin) / max(float(ry.max()), 1e-6))
    cx = (xmin + xmax) / 2
    sx = (rx - cx) * scale + VIEW_W / 2
    sy = VIEW_H - margin - ry * scale
    img = np.zeros((VIEW_H, VIEW_W, 3), np.uint8)
    img[:] = (96, 96, 96)  # neutral mid-gray
    # bright sky band behind the crown, dim ground below - silhouette readability
    img[: VIEW_H // 3] = (170, 175, 190)
    img[VIEW_H - VIEW_H // 6 :] = (70, 60, 50)
    zb = np.full((VIEW_H, VIEW_W), -1e18)
    yy, xx = np.mgrid[0:VIEW_H, 0:VIEW_W]
    for idx, mat in tris:
        tex = textures.get(mat)
        if tex is None: continue
        ta, tw, thh = tex
        p = np.stack([sx[idx], sy[idx]], 1)
        d = rz[idx]
        t = uvs[idx].copy(); t[:, 1] = 1 - t[:, 1]
        x0 = max(int(p[:, 0].min()) - 1, 0); x1 = min(int(p[:, 0].max()) + 2, VIEW_W)
        y0 = max(int(p[:, 1].min()) - 1, 0); y1 = min(int(p[:, 1].max()) + 2, VIEW_H)
        if x0 >= x1 or y0 >= y1: continue
        a = p[1] - p[0]; b = p[2] - p[0]
        den = a[0] * b[1] - a[1] * b[0]
        if abs(den) < 1e-9: continue
        gx = xx[y0:y1, x0:x1] - p[0][0]; gy = yy[y0:y1, x0:x1] - p[0][1]
        w1 = (gx * b[1] - gy * b[0]) / den
        w2 = (a[0] * gy - a[1] * gx) / den
        w0 = 1 - w1 - w2
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not inside.any(): continue
        depth = w0 * d[0] + w1 * d[1] + w2 * d[2]
        zwin = inside & (depth > zb[y0:y1, x0:x1])
        if not zwin.any(): continue
        tu = (w0 * t[0][0] + w1 * t[1][0] + w2 * t[2][0])
        tv = (w0 * t[0][1] + w1 * t[1][1] + w2 * t[2][1])
        ti = (np.clip(tv, 0, 1) * (thh - 1)).astype(int), (np.clip(tu, 0, 1) * (tw - 1)).astype(int)
        px = ta[ti]
        keep = zwin & (px[:, :, 3] >= 128)  # runtime replacement alpha test (ref 127)
        if not keep.any(): continue
        zb[y0:y1, x0:x1][keep] = depth[keep]
        img[y0:y1, x0:x1][keep] = px[:, :, :3][keep]
    return img

def build_and_render(archive, member, raw, work):
    leaf = member.replace('\\', '/').rsplit('/', 1)[-1].lower()
    stem = leaf.rsplit('.', 1)[0]
    work.mkdir(parents=True, exist_ok=True)
    src = work / leaf
    src.write_bytes(raw)
    try:
        img = Image.open(io.BytesIO(raw)).convert('RGBA')
    except Exception as e:
        return None, 'undecodable: %s' % e
    a = {'path': str(src), 'texture': leaf, 'archive': archive}
    rel = Path(stem)
    mode = 'adaptive-shell'
    try:
        alpha = np.array(img)[:, :, 3]
        if cv.is_sparse(alpha):
            mode = 'crossed-sparse'
            cv.crossed_tree(a, rel)
        else:
            cv.tree(a, rel, adaptive=True)
    except ValueError as e:
        try:
            mode = 'crossed-fallback(%s)' % str(e)[:40]
            cv.crossed_tree(a, rel)
        except Exception as e2:
            return None, 'rejected: %s' % str(e2)[:60]
    obj = cv.root / rel / 'tree.obj'
    textures = {}
    for mat, fname in (('crown', 'crossed.tga'), ('crown', 'crown.tga'), ('core', 'core.tga'), ('trunk', 'trunk.tga')):
        p = cv.root / rel / fname
        if mat not in textures and p.exists():
            t = np.array(Image.open(p).convert('RGBA'))
            textures[mat] = (t, t.shape[1], t.shape[0])
    verts, uvs, tris = load_obj(obj)
    return [render(verts, uvs, tris, textures, ang) for ang in ANGLES], '%s %dtris' % (mode, len(tris))

def main(argv=None):
    argv = argv if argv is not None else sys.argv[1:]
    if len(argv) != 2:
        raise SystemExit('usage: preview_trees.py EXPORT.zip OUT.png')
    with tempfile.TemporaryDirectory() as tmp:
        cv.root = Path(tmp) / 'out'
        rows = []
        with zipfile.ZipFile(argv[0]) as z:
            manifest = json.loads(z.read('manifest.json'))
            for hit in manifest['hits']:
                raw = z.read(hit['export_path'])
                views, note = build_and_render(hit['archive'], hit['member'], raw, Path(tmp) / 'src')
                label = '%s %s %s %s' % (hit['archive'], hit['member'].replace('\\', '/').rsplit('/', 1)[-1],
                                         hit['sha256'][:10], note)
                rows.append((label, views))
        cell_w, cell_h = VIEW_W, VIEW_H + 18
        sheet = Image.new('RGB', (cell_w * (len(ANGLES) + 0), (cell_h) * max(1, len(rows)) + 4), (24, 24, 24))
        draw = ImageDraw.Draw(sheet)
        for r, (label, views) in enumerate(rows):
            draw.text((4, r * cell_h + 2), label, fill=(255, 255, 160))
            for k, v in enumerate(views or []):
                sheet.paste(Image.fromarray(v), (k * cell_w, r * cell_h + 18))
        sheet.save(argv[1])
        print('sheet:', argv[1], 'candidates:', len(rows))

if __name__ == '__main__':
    main()
