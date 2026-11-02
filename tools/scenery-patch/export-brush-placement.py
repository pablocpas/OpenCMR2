#!/usr/bin/env python3
"""Read-only export of where the gre01 roadside brush walls sit relative to the stage route.

Answers one question for game/SceneryPack.cpp (video.brush_volume): are the brush wall quads, taken
through their scene node matrices, in the same world space as the route nodes, and which side of
each wall faces the road. Writes a small JSON of numbers only (hashes, positions, matrices,
distances, directions). No textures, meshes or other game bytes are copied. Originals are read-only,
the output file must not exist, and Python standard library only.

It walks the documented C3D layout (game/Sector.cpp, src/port/disk_layouts.h: node 0x18c, mesh
0x120, stage object 0xa0, sector 0x88, portal 0x1c, triangle 76, vertex 48), finds the wall parts by
the same position/UV SHA-256 identity as game/Gre01BrushSources.inc, finds the scene node that
places each one, and compares with the route nodes (<stage>.cat, 0x2c bytes each, 16.16 x/z).
The `checks` block says whether the layout assumptions held on the real file; a failed check means
the numbers must not be trusted. UNVALIDATED on a real archive when this was written (no stage data
was available): only a synthetic fixture exercises it.
"""
import argparse, hashlib, importlib.util, json, math, re, struct
from pathlib import Path

_spec = importlib.util.spec_from_file_location('export_evidence', Path(__file__).with_name('export-evidence.py'))
_export = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_export)
NODE, MESH, OBJ, SECTOR, PORTAL, TRI, VERT = 0x18c, 0x120, 0xa0, 0x88, 0x1c, 76, 48

class Refused(Exception):
    """The file does not match the layout this tool understands; no numbers are written for it."""

def load_hashes(inc):
    return set(re.findall(r'"([0-9a-f]{64})"', Path(inc).read_text()))

def fix(b, o): return struct.unpack_from('<i', b, o)[0] / 65536.0

def names(b):
    tn = struct.unpack_from('<i', b, 0xc)[0]; out = []; i = 0
    while tn + (i + 1) * 260 <= len(b):
        s = b[tn + i * 260: tn + (i + 1) * 260].split(b'\x00')[0]
        if not s: break
        out.append(s.decode('ascii', 'replace').split('\\')[-1].lower()); i += 1
    return out

def matmul(a, b):   # row-vector convention, 4x4 as 16 floats: v' = v*A*B
    return [sum(a[r*4+k] * b[k*4+c] for k in range(4)) for r in range(4) for c in range(4)]

def pt(m, p): return [p[0]*m[c] + p[1]*m[4+c] + p[2]*m[8+c] + m[12+c] for c in range(3)]
def dr(m, d): return [d[0]*m[c] + d[1]*m[4+c] + d[2]*m[8+c] for c in range(3)]

def route_nodes(raw):
    n = len(raw) // 44; pos = []; last = n
    for i in range(n):
        pos.append((fix(raw, i*44), fix(raw, i*44+8)))
    for i in range(n - 1, -1, -1):   # RallyData trims after the last checkpoint-flag node
        if raw[i*44+0x18] & 1: last = i + 1; break
    return pos[:last]

def nearest(route, mx, mz):
    best = None
    for i in range(len(route)):
        ax, az = route[i]; px, pz = ax, az
        if i + 1 < len(route):
            dx, dz = route[i+1][0] - ax, route[i+1][1] - az; l2 = dx*dx + dz*dz
            if l2 > 1e-9:
                t = max(0.0, min(1.0, ((mx-ax)*dx + (mz-az)*dz) / l2)); px, pz = ax + dx*t, az + dz*t
        d = math.hypot(px - mx, pz - mz)
        if best is None or d < best[0]: best = (d, px, pz, i)
    return best

def analyse(c3d, route, hashes):
    b = c3d
    if b[:4] != b'PP_F': raise ValueError('not a C3D model')
    nn, nm, no, ns = struct.unpack_from('<4H', b, 24); tri_n = struct.unpack_from('<I', b, 32)[0]
    portals = struct.unpack_from('<H', b, 40)[0]
    mesh0 = 48 + nn * NODE; obj0 = mesh0 + (nm + no) * MESH; sec0 = obj0 + no * OBJ
    ts = sec0 + ns * SECTOR + portals * PORTAL; vs = ts + tri_n * TRI
    if vs + 4 > len(b) or ts > len(b): raise Refused('header counts exceed file size')
    tex = names(b)
    parts = []   # (mesh index, texture, [indices])
    for mi in range(nm + no):
        m = mesh0 + mi * MESH
        vo = struct.unpack_from('<I', b, m+12)[0]; to, tc = struct.unpack_from('<II', b, m+36)
        last = -99; runs = []
        for ti in range(tc):
            off = ts + to + ti * TRI
            t = struct.unpack_from('<i', b, off+4)[0]; ids = struct.unpack_from('<3H', b, off+64)
            if t != last: runs.append((t, [])); last = t
            runs[-1][1].extend(ids)
        for t, idx in runs:
            if len(idx) < 6 or len(idx) > 12000 or not 0 <= t < len(tex): continue
            if not tex[t].startswith('bsh_sh1'): continue
            h = hashlib.sha256()
            for j in idx:
                o = vs + vo + j * 48; h.update(b[o:o+12]); h.update(b[o+32:o+40])
            if h.hexdigest() in hashes:
                uniq = []
                for j in idx:
                    if j not in uniq: uniq.append(j)
                parts.append((mi, tex[t], h.hexdigest(), uniq, vo))
    nodes = []
    for ni in range(nn):
        o = 48 + ni * NODE
        pobj = struct.unpack_from('<I', b, o+0xc)[0]; par = struct.unpack_from('<I', b, o+8)[0]
        typ = struct.unpack_from('<i', b, o+0x178)[0]
        cur = [fix(b, o+0x98+k*4) for k in range(16)]
        nodes.append({'pobj': pobj, 'parent': par, 'type': typ, 'current': cur})
    if not route or len(route) % 44 or len(route) < 44 * 4: raise Refused('route (<stage>.cat) missing or not a whole number of 0x2c nodes')
    rt = route_nodes(route)
    out = []; checks = {'route_nodes': len(rt), 'node_count': nn, 'mesh_count': nm, 'object_mesh_count': no,
                        'wall_parts_found': len(parts), 'wall_hashes_expected': len(hashes)}
    for mi, texname, digest, uniq, vo in parts:
        placing = [n for n in nodes if n['type'] == 0 and n['pobj'] != 0xffffffff and n['pobj'] % MESH == 0 and n['pobj'] // MESH == mi]
        if len(placing) != 1: raise Refused('wall part in mesh %d has %d placing nodes (need exactly 1)' % (mi, len(placing)))
        rec = {'mesh': mi, 'texture': texname, 'hash': digest, 'placing_nodes': len(placing)}
        corners = {}
        for j in uniq:
            o = vs + vo + j * 48
            x, y, z = struct.unpack_from('<3f', b, o); u, v = struct.unpack_from('<2f', b, o+32)
            if u in (0.0, 1.0) and v in (0.0, 1.0): corners[int(u) + 2 * int(v)] = [x, y, z]
        rec['local_corners_uv00_10_01_11'] = [corners.get(k) for k in range(4)]
        if len(corners) != 4: raise Refused('wall part in mesh %d does not have four UV-corner vertices' % mi)
        if True:
            n = placing[0]; W = n['current']
            if n['parent'] != 0xffffffff: raise Refused('placing node of mesh %d has a parent node; parent transforms are not composed by this tool' % mi)
            rows = [math.sqrt(sum(W[r*4+c]**2 for c in range(3))) for r in range(3)]
            rigid = all(0.95 < r < 1.05 for r in rows)
            if not rigid: raise Refused('placing node of mesh %d has a non-unit rotation matrix' % mi)
            rec['node_matrix'] = W; rec['rows_unit'] = rigid
            wc = [pt(W, corners[k]) for k in range(4)]; rec['world_corners'] = wc
            e1 = [corners[1][i] - corners[0][i] for i in range(3)]; e2 = [corners[2][i] - corners[0][i] for i in range(3)]
            nl = [e1[1]*e2[2]-e1[2]*e2[1], e1[2]*e2[0]-e1[0]*e2[2], e1[0]*e2[1]-e1[1]*e2[0]]
            ln = math.sqrt(sum(v*v for v in nl)) or 1.0; nw = dr(W, [v/ln for v in nl])
            mx, mz = (wc[0][0] + wc[1][0]) / 2, (wc[0][2] + wc[1][2]) / 2
            rec['world_mid_xz'] = [mx, mz]; rec['world_normal'] = nw
            if rt:
                d, px, pz, seg = nearest(rt, mx, mz)
                hn = math.hypot(nw[0], nw[2]) or 1.0
                side = ((px - mx) * nw[0] + (pz - mz) * nw[2]) / hn
                rec.update({'nearest_route_distance': d, 'nearest_route_segment': seg,
                            'road_side_of_normal': side, 'away_world_xz': None if abs(side) < 0.05 else
                            [(-nw[0] if side > 0 else nw[0]) / hn, (-nw[2] if side > 0 else nw[2]) / hn]})
        out.append(rec)
    dists = [r['nearest_route_distance'] for r in out if 'nearest_route_distance' in r]
    checks.update({'parser_refusals': 0,
                   'nearest_route_distance_min_max': [min(dists), max(dists)] if dists else None,
                   'walls_within_60_units': sum(1 for d in dists if d <= 60.0)})
    checks['all_expected_found'] = len(parts) == len(hashes)
    if not parts: raise Refused('no wall parts matched the Gre01BrushSources hashes in this archive')
    return {'checks': checks, 'walls': out}

def export(data, output, inc):
    data = data.resolve(strict=True); output = output.absolute()
    if output.exists(): raise ValueError('output already exists; choose a new file path')
    hashes = load_hashes(inc); report = {'archives': []}
    for arch in sorted(p for p in data.rglob('*') if p.is_file() and p.name.lower() in ('gre01hi.bfl', 'gre01lo.bfl')):
        members = {n.lower(): bl for n, bl in _export.members(arch.read_bytes())}
        entry = {'archive': arch.relative_to(data).as_posix(), 'members': sorted(members)}
        obj = members.get('temp.obj')
        stage = re.sub(r'(hi|lo)\.bfl$', '', arch.name.lower()); cats = [stage + '.cat'] if stage + '.cat' in members else []   # loader: sprintf("%s.cat", stage code)
        entry['route_member_candidates'] = cats
        try:
            if not obj or obj[:4] != b'PP_F': raise Refused('temp.obj missing or not a C3D model')
            if not cats: raise Refused('route member %s.cat not found (exact stage-code name required)' % stage)
            entry['analysis'] = analyse(obj, members[cats[0]], hashes)
        except (Refused, struct.error, IndexError) as e:
            entry['refused'] = str(e); report['refused'] = True
        report['archives'].append(entry)
    if not report['archives']: raise ValueError('no gre01hi.bfl / gre01lo.bfl found under data')
    output.write_text(json.dumps(report, indent=1))
    return report

if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--data', type=Path, required=True, help='owned game data folder (read-only)')
    ap.add_argument('--output', type=Path, required=True, help='new JSON file, outside the game data')
    ap.add_argument('--sources', type=Path, default=Path(__file__).resolve().parents[2] / 'game' / 'Gre01BrushSources.inc')
    a = ap.parse_args(); r = export(a.data, a.output, a.sources)
    code = 0
    for e in r['archives']:
        print(e['archive'], 'route member:', e['route_member_candidates'])
        if 'refused' in e: print('  REFUSED:', e['refused'])
        else: print('  checks:', json.dumps(e['analysis']['checks']))
    raise SystemExit(2 if r.get('refused') else 0)
