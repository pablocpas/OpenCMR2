import hashlib, importlib.util, json, struct, tempfile, unittest
from pathlib import Path
from test_convert import bfl

spec = importlib.util.spec_from_file_location('bp', Path(__file__).with_name('export-brush-placement.py'))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def fixed(v): return int(round(v * 65536))

IDENT = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]

def build_c3d(world, objects=1, matrix=IDENT, as_node=False):
    """temp.sht-style model: no nodes, no ordinary meshes, `objects` stage objects pointing at one
    bsh_sh1a wall mesh whose vertices are already world-space (the layout seen in a real gre01hi.bfl)."""
    verts = [(world[k], (k & 1, k >> 1)) for k in range(4)]
    vbytes = b''.join(struct.pack('<3f', *p) + bytes(20) + struct.pack('<2f', float(u), float(v)) + bytes(8) for p, (u, v) in verts)
    tris = b''
    for ids in ((0, 1, 2), (1, 3, 2)):
        t = bytearray(76); struct.pack_into('<i', t, 4, 0); struct.pack_into('<3H', t, 64, *ids); tris += bytes(t)
    mesh = bytearray(0x120); struct.pack_into('<I', mesh, 12, 0); struct.pack_into('<II', mesh, 36, 0, 2)
    objs = b''
    for _ in range(objects):
        o = bytearray(0xa0); struct.pack_into('<I', o, 0xc, 0)       # pMesh = (meshIndex - meshCount) * 0x120 = 0
        for k, v in enumerate(matrix): struct.pack_into('<i', o, 0x18 + k * 4, fixed(v))
        objs += bytes(o)
    body = bytes(mesh) + objs + tris + vbytes
    names = b'bsh_sh1a.dds'.ljust(260, b'\0')
    head = bytearray(48)
    head[:4] = b'PP_F'; struct.pack_into('<I', head, 0xc, 48 + len(body))
    struct.pack_into('<4H', head, 24, 0, 0, objects, 0); struct.pack_into('<I', head, 32, 2)
    return bytes(head) + body + names

def cat(points):
    out = b''
    for i, (x, z) in enumerate(points):
        n = bytearray(44); struct.pack_into('<i', n, 0, fixed(x)); struct.pack_into('<i', n, 8, fixed(z)); n[0x18] = 1 if i == len(points) - 1 else 0
        out += bytes(n)
    return out

def hash_of(local):
    h = hashlib.sha256()
    for ids in ((0, 1, 2), (1, 3, 2)):
        for j in ids:
            h.update(struct.pack('<3f', *local[j])); h.update(struct.pack('<2f', float(j & 1), float(j >> 1)))
    return h.hexdigest()

class T(unittest.TestCase):
    def test_world_space_side_and_checks(self):
        local = [(100, 4, 41.5), (100, 4, 58.5), (100, 1, 41.5), (100, 1, 58.5)]
        c3d = build_c3d(local)
        route = cat([(104, 0), (104, 50), (104, 100), (104, 150)])
        r = m.analyse(c3d, route, {hash_of(local)})
        self.assertEqual(r['checks']['wall_parts_found'], 1)
        w = r['walls'][0]
        self.assertAlmostEqual(w['world_mid_xz'][0], 100.0, 3)
        self.assertTrue(w['runtime_extrudes'])
        self.assertAlmostEqual(w['nearest_route_distance'], 4.0, 3)
        self.assertLess(w['away_world_xz'][0], -0.99)   # road at +x, away = -x
        self.assertEqual(r['checks']['walls_within_60_units'], 1)

    def test_export_is_read_only_and_numbers_only(self):
        local = [(100, 4, 41.5), (100, 4, 58.5), (100, 1, 41.5), (100, 1, 58.5)]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            (data / 'gre01hi.bfl').write_bytes(bfl([('temp.sht', build_c3d(local)), ('gre01.cat', cat([(104, 0), (104, 50), (104, 100), (104, 150)]))]))
            inc = root / 's.inc'; inc.write_text('"%s",' % hash_of(local))
            before = (data / 'gre01hi.bfl').read_bytes(); out = root / 'o.json'
            rep = m.export(data, out, inc)
            self.assertEqual(before, (data / 'gre01hi.bfl').read_bytes())
            self.assertEqual(rep['archives'][0]['route_member_candidates'], ['gre01.cat'])
            self.assertNotIn('refused', rep['archives'][0])
            with self.assertRaises(ValueError): m.export(data, out, inc)

    def test_fail_closed(self):
        local = [(100, 4, 41.5), (100, 4, 58.5), (100, 1, 41.5), (100, 1, 58.5)]
        c3d = build_c3d(local)
        with self.assertRaises(m.Refused): m.analyse(c3d, b'', {hash_of(local)})
        with self.assertRaises(m.Refused): m.analyse(c3d, cat([(1, 1)]), {hash_of(local)})
        with self.assertRaises(m.Refused): m.analyse(c3d, cat([(104, 0)] * 5), {'0' * 64})   # no wall matched
        r5 = cat([(104, 0)] * 5)
        with self.assertRaises(m.Refused): m.analyse(build_c3d(local, objects=2), r5, {hash_of(local)})   # two placing objects
        with self.assertRaises(m.Refused): m.analyse(build_c3d(local, matrix=[3] + IDENT[1:]), r5, {hash_of(local)})   # scaled
        with self.assertRaises(m.Refused): m.analyse(build_c3d(local, matrix=[1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]), r5, {hash_of(local)})   # not orthogonal
        ordinary = bytearray(c3d); struct.pack_into('<4H', ordinary, 24, 0, 1, 0, 0)   # same mesh now an ordinary mesh
        with self.assertRaises(m.Refused): m.analyse(bytes(ordinary), r5, {hash_of(local)})
        with tempfile.TemporaryDirectory() as tmp:   # route under another name is not accepted
            root = Path(tmp); data = root / 'g'; data.mkdir()
            (data / 'gre01hi.bfl').write_bytes(bfl([('temp.sht', c3d), ('other.cat', cat([(104, 0)] * 5))]))
            inc = root / 's.inc'; inc.write_text('"%s",' % hash_of(local))
            rep = m.export(data, root / 'o.json', inc)
            self.assertIn('route member', rep['archives'][0]['refused']); self.assertTrue(rep['refused'])

    def test_member_rules(self):
        local = [(100, 4, 41.5), (100, 4, 58.5), (100, 1, 41.5), (100, 1, 58.5)]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'g'; data.mkdir()
            both = bfl([('temp.sht', build_c3d(local)), ('temp.obj', build_c3d(local)), ('gre01.cat', cat([(104, 0)] * 5))])
            (data / 'gre01hi.bfl').write_bytes(both)
            inc = root / 's.inc'; inc.write_text('"%s",' % hash_of(local))
            rep = m.export(data, root / 'o.json', inc)
            self.assertTrue(rep['refused'])   # walls in two members: ambiguous

    def test_route_trim_matches_runtime(self):
        r = cat([(1, 0), (2, 0), (3, 0), (4, 0), (5, 0), (6, 0)])
        raw = bytearray(r); raw[5 * 44 + 0x18] = 0; raw[3 * 44 + 0x18] = 1   # last checkpoint flag at node 3: nodes 4,5 trimmed
        self.assertEqual(len(m.route_nodes(bytes(raw))), 4)
        raw2 = bytearray(r); raw2[0x18] = 1; raw2[5 * 44 + 0x18] = 0   # only node 0 flagged: runtime never tests node 0, keeps all
        self.assertEqual(len(m.route_nodes(bytes(raw2))), 6)

if __name__ == '__main__': unittest.main()
