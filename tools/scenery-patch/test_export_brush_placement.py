import hashlib, importlib.util, json, struct, tempfile, unittest
from pathlib import Path
from test_convert import bfl

spec = importlib.util.spec_from_file_location('bp', Path(__file__).with_name('export-brush-placement.py'))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def fixed(v): return int(round(v * 65536))

def build_c3d(local, tx, tz):
    """One object mesh (bsh_sh1a quad, 2 triangles) placed by one node: yaw 90 + translate (tx, tz)."""
    verts = [(local[k], (k & 1, k >> 1)) for k in range(4)]   # corners in UV order
    vbytes = b''.join(struct.pack('<3f', *p) + bytes(20) + struct.pack('<2f', float(u), float(v)) + bytes(8) for p, (u, v) in verts)
    assert len(vbytes) == 4 * 48
    tris = b''
    for ids in ((0, 1, 2), (1, 3, 2)):
        t = bytearray(76); struct.pack_into('<i', t, 4, 0); struct.pack_into('<3H', t, 64, *ids); tris += bytes(t)
    mesh = bytearray(0x120); struct.pack_into('<I', mesh, 12, 0); struct.pack_into('<II', mesh, 36, 0, 2)
    node = bytearray(NODE := 0x18c)
    struct.pack_into('<I', node, 0xc, 0); struct.pack_into('<I', node, 8, 0xffffffff); struct.pack_into('<i', node, 0x178, 0)
    # right=(0,0,-1) up=(0,1,0) forward=(1,0,0) position=(tx,0,tz): local x->world -z, local z->world +x
    mat = [0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, tx, 0, tz, 1]
    for k, v in enumerate(mat): struct.pack_into('<i', node, 0x98 + k * 4, fixed(v))
    body = bytes(node) + bytes(mesh) + bytes(0xa0) + tris + vbytes
    names = b'bsh_sh1a.dds'.ljust(260, b'\0')
    head = bytearray(48)
    head[:4] = b'PP_F'; struct.pack_into('<I', head, 0xc, 48 + len(body))
    struct.pack_into('<4H', head, 24, 1, 0, 1, 0); struct.pack_into('<I', head, 32, 2)
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
        local = [(-8.5, 4, 0), (8.5, 4, 0), (-8.5, 1, 0), (8.5, 1, 0)]
        c3d = build_c3d(local, 100.0, 50.0)
        route = cat([(104, 0), (104, 50), (104, 100), (104, 150)])
        r = m.analyse(c3d, route, {hash_of(local)})
        self.assertEqual(r['checks']['wall_parts_found'], 1)
        self.assertEqual(r['checks']['walls_without_exactly_one_placing_node'], 0)
        w = r['walls'][0]
        self.assertAlmostEqual(w['world_mid_xz'][0], 100.0, 3)
        self.assertAlmostEqual(w['nearest_route_distance'], 4.0, 3)
        self.assertLess(w['away_world_xz'][0], -0.99)   # road at +x, away = -x
        self.assertEqual(r['checks']['walls_within_60_units'], 1)

    def test_export_is_read_only_and_numbers_only(self):
        local = [(-8.5, 4, 0), (8.5, 4, 0), (-8.5, 1, 0), (8.5, 1, 0)]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game'; data.mkdir()
            (data / 'gre01hi.bfl').write_bytes(bfl([('temp.obj', build_c3d(local, 100.0, 50.0)), ('gre01.cat', cat([(104, 0), (104, 50), (104, 100), (104, 150)]))]))
            inc = root / 's.inc'; inc.write_text('"%s",' % hash_of(local))
            before = (data / 'gre01hi.bfl').read_bytes(); out = root / 'o.json'
            rep = m.export(data, out, inc)
            self.assertEqual(before, (data / 'gre01hi.bfl').read_bytes())
            self.assertEqual(rep['archives'][0]['route_member_candidates'], ['gre01.cat'])
            with self.assertRaises(ValueError): m.export(data, out, inc)

if __name__ == '__main__': unittest.main()
