import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from test_convert import bfl

spec = importlib.util.spec_from_file_location('tree_inventory', Path(__file__).with_name('export-tree-inventory.py'))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

def dds(w, h, payload=b'x'):
    return b'DDS ' + struct.pack('<II', 124, 0) + struct.pack('<II', h, w) + payload

class InventoryTests(unittest.TestCase):
    def test_locates_stems_across_archives_without_pixels(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); data = root / 'game' / 'Tracks' / 'Elsewhere'; data.mkdir(parents=True)
            (data / 'abchit.bfl').write_bytes(bfl([('tre_103.dds', dds(64, 128)), ('tre_130.tga', struct.pack('<12xHH', 32, 32)), ('private.dds', b'secret')]))
            (data / 'abclot.bfl').write_bytes(bfl([('tre_103.dds', dds(32, 64)), ('tre_x.dds', dds(8, 8))]))
            (data / 'broken.bfl').write_bytes(b'not an archive')
            before = {p.name: p.read_bytes() for p in data.iterdir()}
            out = root / 'inv.json'
            report = m.inventory(root / 'game', out, ['tre_103', 'tre_999'])
            self.assertEqual([(h['archive'], h['member'], h['dimensions']) for h in report['hits']],
                [('Tracks/Elsewhere/abchit.bfl', 'tre_103.dds', [64, 128]), ('Tracks/Elsewhere/abclot.bfl', 'tre_103.dds', [32, 64])])
            self.assertEqual(report['stems_not_found'], ['tre_999'])
            self.assertEqual([s['archive'] for s in report['skipped_archives']], ['Tracks/Elsewhere/broken.bfl'])
            text = out.read_text(); self.assertNotIn('secret', text); self.assertNotIn('private', text)
            self.assertEqual(before, {p.name: p.read_bytes() for p in data.iterdir()})
            with self.assertRaises(ValueError): m.inventory(root / 'game', out)
            with self.assertRaises(ValueError): m.inventory(root / 'game', root / 'game' / 'inv.json')

if __name__ == '__main__':
    unittest.main()
