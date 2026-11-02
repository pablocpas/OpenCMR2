#!/usr/bin/env python3
"""Read-only export of where the gre01 roadside brush walls sit relative to the stage route.

Answers one question for game/SceneryPack.cpp (video.brush_volume): where are the brush wall quads
in world space, how far is each from the stage route, and which side faces the road, so we can tell
which walls the runtime will extrude. Writes a small JSON of numbers only (hashes, positions,
matrices, distances, directions). No textures, meshes or other game bytes are copied. Originals are
read-only, the output file must not exist, and Python standard library only.

Validated layout (a real gre01hi.bfl): the 38 wall parts are STAGE OBJECT meshes in temp.sht (not
temp.obj, not scene-node placed). Each is placed by one stage-object record (0xa0: position 0x0,
pMesh 0xc, 4x4 fix matrix 0x18); in that archive the matrix is identity and the vertices are already
world-space. The tool finds the parts by the position/UV SHA-256 identity of
game/Gre01BrushSources.inc, requires the matches in exactly one of temp.sht/temp.obj/temp.c3d, exactly
one stage object per wall mesh and a rigid matrix, and refuses anything else (ordinary meshes,
scene-node placement, missing or ambiguous items). Route nodes come from <stage>.cat (0x2c bytes each,
16.16 x/z). `runtime_extrudes` mirrors game/BrushRelief.h (route distance <= 60, road-facing ratio
>= 0.7, extrusion must not approach any road point). Only gre01hi was examined; gre01lo and other
archives are unchecked. Whether the ambient world transform at draw time is identity is read live by
the runtime, which declines if it is not rigid; it has not been confirmed natively.
"""
import argparse, hashlib, importlib.util, json, math, re, struct
from pathlib import Path

_spec = importlib.util.spec_from_file_location('export_evidence', Path(__file__).with_name('export-evidence.py'))
_export = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_export)
NODE, MESH, OBJ, SECTOR, PORTAL, TRI, VERT = 0x18c, 0x120, 0xa0, 0x88, 0x1c, 76, 48
MAX_ROUTE_DISTANCE, MIN_ROAD_FACING, EXTRUSION_DEPTH = 60.0, 0.7, 0.8 * 4 + 0.9   # same constants as game/SceneryPack.cpp and game/BrushRelief.h

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
    for i in range(n - 1, 0, -1):   # RallyData (game/RallyData.cpp ~9083) trims after the last checkpoint-flag node; node 0 is never tested there
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
    node_meshes = set()
    for ni in range(nn):
        pobj = struct.unpack_from('<I', b, 48 + ni * NODE + 0xc)[0]
        if pobj != 0xffffffff and pobj % MESH == 0: node_meshes.add(pobj // MESH)
    if not route or len(route) % 44 or len(route) < 44 * 4: raise Refused('route (<stage>.cat) missing or not a whole number of 0x2c nodes')
    rt = route_nodes(route)
    out = []; checks = {'route_nodes': len(rt), 'route_nodes_in_file': len(route) // 44, 'node_count': nn, 'mesh_count': nm, 'object_mesh_count': no,
                        'wall_parts_found': len(parts), 'wall_hashes_expected': len(hashes)}
    for mi, texname, digest, uniq, vo in parts:
        # The walls are STAGE OBJECT meshes (loader: pMesh = pMeshes + meshCount + n*0x120). A stage object
        # record (0xa0): position fix at 0x0, pMesh at 0xc, 4x4 fix matrix at 0x18 (the matrix the draw uses).
        if mi < nm or mi in node_meshes: raise Refused('wall part in mesh %d is not a plain stage-object mesh (ordinary mesh or scene-node placed); not handled' % mi)
        placing = [oi for oi in range(no) if struct.unpack_from('<I', b, obj0 + oi * OBJ + 0xc)[0] == (mi - nm) * MESH]
        if len(placing) != 1: raise Refused('wall part in mesh %d has %d stage objects placing it (need exactly 1)' % (mi, len(placing)))
        orec = obj0 + placing[0] * OBJ
        W = [fix(b, orec + 0x18 + k * 4) for k in range(16)]
        rec = {'mesh': mi, 'texture': texname, 'hash': digest, 'stage_object': placing[0], 'placing_objects': 1,
               'object_record_position': [fix(b, orec + k * 4) for k in range(3)]}
        corners = {}
        for j in uniq:
            o = vs + vo + j * 48
            x, y, z = struct.unpack_from('<3f', b, o); u, v = struct.unpack_from('<2f', b, o+32)
            if u in (0.0, 1.0) and v in (0.0, 1.0): corners[int(u) + 2 * int(v)] = [x, y, z]
        rec['local_corners_uv00_10_01_11'] = [corners.get(k) for k in range(4)]
        if len(corners) != 4: raise Refused('wall part in mesh %d does not have four UV-corner vertices' % mi)
        if True:
            rows = [math.sqrt(sum(W[r*4+c]**2 for c in range(3))) for r in range(3)]
            rigid = all(0.95 < r < 1.05 for r in rows)
            if not rigid: raise Refused('stage object matrix of mesh %d has a non-unit rotation' % mi)
            dots = [sum(W[i*4+c]*W[j*4+c] for c in range(3)) for i, j in ((0,1),(0,2),(1,2))]
            if any(abs(d) > 0.05 for d in dots): raise Refused('stage object matrix of mesh %d is not orthogonal' % mi)
            rec['object_matrix'] = W; rec['rows_unit'] = rigid
            wc = [pt(W, corners[k]) for k in range(4)]; rec['world_corners'] = wc
            e1 = [corners[1][i] - corners[0][i] for i in range(3)]; e2 = [corners[2][i] - corners[0][i] for i in range(3)]
            nl = [e1[1]*e2[2]-e1[2]*e2[1], e1[2]*e2[0]-e1[0]*e2[2], e1[0]*e2[1]-e1[1]*e2[0]]
            ln = math.sqrt(sum(v*v for v in nl)) or 1.0; nw = dr(W, [v/ln for v in nl])
            mx, mz = (wc[0][0] + wc[1][0]) / 2, (wc[0][2] + wc[1][2]) / 2
            rec['world_mid_xz'] = [mx, mz]; rec['world_normal'] = nw
            rp = rec['object_record_position']; rec['wall_mid_to_record_position_xz'] = math.hypot(mx - rp[0], mz - rp[2])
            if rt:
                d, px, pz, seg = nearest(rt, mx, mz)
                hn = math.hypot(nw[0], nw[2]) or 1.0
                side = ((px - mx) * nw[0] + (pz - mz) * nw[2]) / hn
                rec.update({'nearest_route_distance': d, 'nearest_route_segment': seg,
                            'road_side_of_normal': side, 'away_world_xz': None if abs(side) < 0.05 else
                            [(-nw[0] if side > 0 else nw[0]) / hn, (-nw[2] if side > 0 else nw[2]) / hn],
                            'road_facing_ratio': abs(side) / d if d > 0 else 0.0})
                ok = bool(d <= MAX_ROUTE_DISTANCE and abs(side) >= 0.05 and abs(side) >= MIN_ROAD_FACING * d and math.hypot(nw[0], nw[2]) >= 0.3)   # runtime also needs a vertical wall
                gain = None
                if ok:
                    a = rec['away_world_xz']
                    gain = nearest(rt, mx + a[0] * EXTRUSION_DEPTH, mz + a[1] * EXTRUSION_DEPTH)[0] - d
                    rec['distance_gain_at_full_extrusion'] = gain
                    if gain < -1e-3: ok = False   # the extrusion would approach another road section
                rec['runtime_extrudes'] = ok
        out.append(rec)
    dists = [r['nearest_route_distance'] for r in out if 'nearest_route_distance' in r]
    checks.update({'parser_refusals': 0,
                   'nearest_route_distance_min_max': [min(dists), max(dists)] if dists else None,
                   'walls_within_60_units': sum(1 for d in dists if d <= 60.0),
                   'walls_runtime_would_extrude': sum(1 for r in out if r.get('runtime_extrudes')),
                   'walls_declined_too_far': sum(1 for d in dists if d > MAX_ROUTE_DISTANCE),
                   'walls_declined_not_facing_road_or_approach_road': sum(1 for r in out if r.get('nearest_route_distance', 1e9) <= MAX_ROUTE_DISTANCE and not r.get('runtime_extrudes'))})
    checks['all_expected_found'] = len(parts) == len(hashes)
    if not parts: raise Refused('no wall parts matched the Gre01BrushSources hashes in this member')
    return {'checks': checks, 'walls': out}

def export(data, output, inc):
    data = data.resolve(strict=True); output = output.absolute()
    if output.exists(): raise ValueError('output already exists; choose a new file path')
    hashes = load_hashes(inc); report = {'archives': []}
    for arch in sorted(p for p in data.rglob('*') if p.is_file() and p.name.lower() in ('gre01hi.bfl', 'gre01lo.bfl')):
        members = {n.lower(): bl for n, bl in _export.members(arch.read_bytes())}
        entry = {'archive': arch.relative_to(data).as_posix(), 'members': sorted(members)}
        # The wall parts live in temp.sht (verified on a real gre01hi.bfl); temp.obj and temp.c3d are searched too and must not also match.
        cand = [n for n in ('temp.sht', 'temp.obj', 'temp.c3d') if members.get(n, b'')[:4] == b'PP_F']
        obj = None
        stage = re.sub(r'(hi|lo)\.bfl$', '', arch.name.lower()); cats = [stage + '.cat'] if stage + '.cat' in members else []   # loader: sprintf("%s.cat", stage code)
        entry['route_member_candidates'] = cats
        try:
            if not cand: raise Refused('no temp.sht / temp.obj / temp.c3d C3D model in the archive')
            if not cats: raise Refused('route member %s.cat not found (exact stage-code name required)' % stage)
            hits = []
            for n in cand:
                try: hits.append((n, analyse(members[n], members[cats[0]], hashes)))
                except Refused as e:
                    if not str(e).startswith('no wall parts matched'): raise
            entry['searched_members'] = cand
            if len(hits) != 1: raise Refused('wall parts matched in %d members of %s (need exactly 1)' % (len(hits), ', '.join(cand)))
            entry['wall_member'] = hits[0][0]; entry['analysis'] = hits[0][1]
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
        else: print('  wall member:', e['wall_member']); print('  checks:', json.dumps(e['analysis']['checks']))
    raise SystemExit(2 if r.get('refused') else 0)
