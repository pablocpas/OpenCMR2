# SPDX-License-Identifier: GPL-3.0-or-later
"""No original game artwork. All DDS/archives are generated synthetic fixtures."""
import gzip, hashlib, importlib.util, io, json, shutil, struct, subprocess, sys
import tempfile, unittest
from pathlib import Path
from PIL import Image, ImageDraw

HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location('patcher', HERE / 'convert.py')
patcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patcher)


def dds():
    image = Image.new('RGBA', (32, 32))
    draw = ImageDraw.Draw(image)
    draw.ellipse((4, 2, 28, 22), fill=(40, 160, 60, 255))
    draw.rectangle((14, 18, 18, 31), fill=(100, 70, 40, 255))
    data = io.BytesIO()
    image.save(data, format='DDS')
    return data.getvalue()


def bfl(entries):
    body = bytearray()
    directory = bytearray()
    for leaf, raw in entries:
        offset = len(body)
        body.extend(raw)
        name = leaf.encode('ascii')
        directory.extend(struct.pack('<III', len(raw), offset, len(name)))
        directory.extend(name)
        directory.extend(bytes((-len(name)) % 4))
    return gzip.compress(b'CMPR' + bytes(4) + body + directory + struct.pack('<I', len(body)))


class Tests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)
        self.input = self.base / 'game'
        self.input.mkdir()
        self.script = self.base / 'convert.py'
        shutil.copy(HERE / 'convert.py', self.script)
        self.output = self.base / 'private' / 'scenery-pack'
        self.raw = dds()
        self.index = {}
        categories = ['individual-tree', 'spectator', 'forest-strip', 'shadow-mask']
        for name, category in zip(['frahit', 'fralot', 'finhit', 'finlot'], categories):
            leaf = ('tre_md1' if category=='individual-tree' else name) + '.dds'
            (self.input / (name + '.bfl')).write_bytes(bfl([(leaf, self.raw)]))
            self.index[name + '.bfl/' + leaf] = {
                'sha256': hashlib.sha256(self.raw).hexdigest(),
                'width': 32, 'height': 32,
                'category': category, 'kind': 'tree' if category != 'spectator' else 'spectator'
            }
        self.write_index()
        self.before = {p.name: p.read_bytes() for p in self.input.iterdir()}

    def test_source_identity_records(self):
        r = subprocess.run([sys.executable, str(self.script), '--input-dir', str(self.input), '--output-dir', str(self.output)], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        lines = (self.output / 'source-identities.tsv').read_text().splitlines()
        self.assertEqual(len(lines), 2)
        for line in lines:
            key, size, digest = line.split('\t')
            self.assertEqual(int(size), len(self.raw))
            self.assertEqual(digest, hashlib.sha256(self.raw).hexdigest())
            self.assertIn(self.index[key]['category'], ['individual-tree', 'spectator'])

    def tearDown(self):
        self.tmp.cleanup()

    def write_index(self):
        self.script.with_name('source-index.json').write_text(json.dumps(self.index))

    def run_patch(self, *args, ok=True):
        current = {str(p.relative_to(self.input)): p.read_bytes() for p in self.input.rglob('*') if p.is_file()}
        run = subprocess.run([sys.executable, str(self.script), '--input-dir', str(self.input),
                              '--output-dir', str(self.output), *args], capture_output=True, text=True)
        self.assertEqual(run.returncode == 0, ok, run.stdout + run.stderr)
        self.assertEqual(current, {str(p.relative_to(self.input)): p.read_bytes() for p in self.input.rglob('*') if p.is_file()})
        return run

    def test_articulated_demo_fixed_feet_and_distinct_arm(self):
        # Synthetic artwork only. Exercise exact authored mask/rotation/plate route.
        im=Image.new('RGBA',(128,128),(0,0,0,0));d=ImageDraw.Draw(im)
        d.rectangle((78,15,104,127),fill=(10,30,90,255))
        d.polygon([(64,13),(69,8),(96,4),(98,11),(75,16),(70,20)],fill=(220,180,140,255))
        poses=patcher.articulated_spectator(im)
        self.assertEqual(len(poses),4)
        self.assertNotEqual(poses[0].tobytes(),poses[2].tobytes())
        for pose in poses:self.assertEqual(pose.crop((0,70,128,128)).tobytes(),im.crop((0,70,128,128)).tobytes())
        self.assertEqual(poses[1].tobytes(),poses[3].tobytes())

    def test_foreground_limbs_high_low_and_fixed_feet(self):
        for size in (64,128):
            for name,r in patcher.FOREGROUND_LIMBS.items():
                im=Image.new('RGBA',(128,128),(0,0,0,0));d=ImageDraw.Draw(im)
                d.rectangle((0,65,127,127),fill=(20,30,60,255))
                d.polygon([tuple(p) for p in r['polygon']],fill=(220,180,140,255))
                source=im.resize((size,size),Image.Resampling.NEAREST)
                poses=patcher.foreground_limb_poses(source,name)
                self.assertEqual(len(poses),4)
                self.assertEqual(poses[0].size,(size*2,size*2))
                self.assertNotEqual(poses[0].tobytes(),poses[2].tobytes())
                for pose in poses:
                    self.assertEqual(pose.crop((0,int(size*1.4),size*2,size*2)).tobytes(),poses[0].crop((0,int(size*1.4),size*2,size*2)).tobytes())

    def test_finland_route_uses_authored_frames(self):
        oldroot=getattr(patcher,'root',None)
        try:
            patcher.root=self.base/'authored';patcher.root.mkdir()
            source=self.base/'person.dds'
            im=Image.new('RGBA',(128,128));ImageDraw.Draw(im).rectangle((0,0,127,127),fill=(80,120,140,255));im.save(source)
            for name in patcher.FINLAND_LIMBS:
                frames=patcher.spectator({'path':str(source),'archive':'finhit.bfl','texture':name+'.dds'},Path(name))
                self.assertEqual(len(frames),4)
        finally:patcher.root=oldroot

    def test_full_output_and_feet(self):
        self.run_patch()
        rows = self.output.joinpath('manifest.tsv').read_text().splitlines()
        self.assertEqual(len(rows), 3)
        people = sorted(self.output.glob('spectators/*/*/*.tga'))
        self.assertEqual(len(people), 12)
        first = Image.open(people[0]).convert('RGBA')
        for frame in people:
            image = Image.open(frame).convert('RGBA')
            self.assertEqual(image.size, (64, 64))
            self.assertEqual(image.crop((0, 45, 64, 64)).tobytes(), first.crop((0, 45, 64, 64)).tobytes())
        objs=list(self.output.glob('trees/*/*/*.obj'));self.assertTrue(objs)
        self.assertTrue(objs[0].read_text().startswith('# opencmr2-source-canvas-1 32 32\n'))
        self.assertTrue('source-fitted-interpolated' in self.output.joinpath('coverage.json').read_text())
        self.assertFalse(list(self.output.rglob('*.dds')))
        self.assertFalse(list(self.output.rglob('*.bfl')))
        self.run_patch(ok=False)

    def test_reviewed_crossed_scope_and_shell_fallback(self):
        self.assertEqual(patcher.CROSSED_CANDIDATES,{
            ('finhit.bfl','tre_fir2'),('finlot.bfl','tre_fir2'),
            ('finhit.bfl','tre_fr3'),('finlot.bfl','tre_fr3'),
            ('frahit.bfl','tre_md2'),('fralot.bfl','tre_md2')})
        # Same verified synthetic full-canvas source, new mapping name.
        old='frahit.bfl/tre_md1.dds';rec=self.index.pop(old)
        self.index['frahit.bfl/tre_md2.dds']=rec
        (self.input/'frahit.bfl').write_bytes(bfl([('tre_md2.dds',self.raw)]))
        self.write_index();self.run_patch('--tree-geometry','shell')
        self.assertFalse(list(self.output.glob('trees/*/*/tree.obj')))
        shutil.rmtree(self.output);self.run_patch('--tree-geometry','crossed')
        objs=list(self.output.glob('trees/*/*/tree.obj'));self.assertEqual(len(objs),1)
        self.assertTrue(objs[0].read_text().startswith('# opencmr2-source-canvas-1 32 32'))
        coverage=json.loads((self.output/'coverage.json').read_text())
        self.assertEqual(next(x for x in coverage if x['texture']=='tre_md2.dds')['status'],'source-fitted-interpolated')

    def test_crossed_candidate(self):
        self.run_patch('--tree-geometry','crossed')
        metadata=json.loads(next(self.output.glob('trees/*/*/recipe.json')).read_text())
        self.assertEqual(metadata['triangles'],80)
        self.assertIn('not full3D',metadata['geometry'])
        self.assertIn('bottom6%',metadata['alpha_changes'])
        self.assertFalse(list(self.output.glob('trees/*/*/crown.tga')))
        before={str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()}
        shutil.rmtree(self.output);self.run_patch('--tree-geometry','crossed')
        self.assertEqual(before,{str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()})

    def test_tree_generation_is_deterministic(self):
        self.run_patch();before={str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()}
        shutil.rmtree(self.output);self.run_patch()
        self.assertEqual(before,{str(p.relative_to(self.output)):p.read_bytes() for p in self.output.rglob('*') if p.is_file()})

    def test_unsupported_tree_stays_original(self):
        key=next(iter(self.index));rec=self.index.pop(key)
        key='frahit.bfl/tre_fr1.dds';self.index[key]=rec;self.write_index()
        (self.input/'frahit.bfl').write_bytes(bfl([('tre_fr1.dds',self.raw)]))
        self.run_patch();self.assertFalse(list(self.output.glob('trees/*/*/*.obj')))
        self.assertEqual(len(self.output.joinpath('manifest.tsv').read_text().splitlines()),2)
        self.assertTrue('retained-original' in self.output.joinpath('coverage.json').read_text())

    def test_all_trees_card_without_stem_data_builds_crown_only(self):
        # A card with no opaque texels in the middle 40% of its stem rows has no stem data (Greece tre_40 in the native log).
        # --all-trees must not abort the pack; it builds the crown with no invented stem.
        image = Image.new('RGBA', (32, 32)); d = ImageDraw.Draw(image); d.ellipse((0, 0, 9, 31), fill=(40, 160, 60, 255)); d.ellipse((22, 0, 31, 31), fill=(40, 160, 60, 255))
        data = io.BytesIO(); image.save(data, format='DDS'); raw = data.getvalue()
        (self.input / 'frahit.bfl').write_bytes(bfl([('tre_xx9.dds', raw)]))
        self.index.pop('frahit.bfl/tre_md1.dds')
        self.index['frahit.bfl/tre_xx9.dds'] = {'sha256': hashlib.sha256(raw).hexdigest(), 'width': 32, 'height': 32,
                                                'category': 'individual-tree', 'kind': 'tree'}
        self.write_index()
        self.run_patch('--tree-geometry', 'shell', '--all-trees')
        self.assertIn('no-stem-data', self.output.joinpath('coverage.json').read_text())
        folder = next(self.output.glob('trees/*/tre_xx9'))
        self.assertTrue((folder / 'trunk.tga').is_file())
        self.assertNotIn('usemtl trunk\nf', (folder / 'tree.obj').read_text())

    def test_check_only(self):
        self.run_patch('--check-only')
        self.assertFalse(self.output.exists())

    def test_missing_duplicate_and_symlink(self):
        one = self.input / 'frahit.bfl'
        one.unlink()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        one.write_bytes(bfl([('tre_md1.dds', self.raw)]))
        nested = self.input / 'nested'
        nested.mkdir()
        shutil.copy(one, nested / one.name)
        self.run_patch(ok=False)
        (nested / one.name).unlink()
        one.unlink()
        one.symlink_to(self.base / 'outside.bfl')
        (self.base / 'outside.bfl').write_bytes(bfl([('tre_md1.dds', self.raw)]))
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())

    def test_wrong_hash_and_dimensions(self):
        key = next(iter(self.index))
        self.index[key]['sha256'] = '0' * 64
        self.write_index()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        self.index[key]['sha256'] = hashlib.sha256(self.raw).hexdigest()
        self.index[key]['width'] = 99
        self.write_index()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())

    def test_inside_and_nonempty_output(self):
        self.output = self.input / 'pack'
        self.run_patch(ok=False)
        self.output = self.base / 'pack'
        self.output.mkdir()
        self.output.joinpath('mine').write_text('retain')
        self.run_patch(ok=False)
        self.assertEqual(self.output.joinpath('mine').read_text(), 'retain')

    def test_corrupt_archive(self):
        for raw in [b'not gzip', gzip.compress(b'bad'),
                    gzip.compress(b'CMPR' + bytes(4) + struct.pack('<I', 99999)),
                    bfl([('../x', self.raw)]), bfl([('frahit.dds', self.raw), ('frahit.dds', self.raw)])]:
            path = self.base / 'broken.bfl'
            path.write_bytes(raw)
            with self.assertRaises(Exception):
                patcher.read_archive(path, 'frahit.bfl', self.index)

    def test_generation_failure_leaves_no_partial_output(self):
        # A valid transparent DDS passes input identity but has no tree silhouette.
        empty = io.BytesIO()
        Image.new('RGBA', (32, 32)).save(empty, format='DDS')
        raw = empty.getvalue()
        key = next(iter(self.index))
        self.index[key]['sha256'] = hashlib.sha256(raw).hexdigest()
        self.write_index()
        (self.input / 'frahit.bfl').write_bytes(bfl([('tre_md1.dds', raw)]))
        self.before['frahit.bfl'] = (self.input / 'frahit.bfl').read_bytes()
        self.run_patch(ok=False)
        self.assertFalse(self.output.exists())
        self.assertFalse(list(self.output.parent.glob('.cmr2-patch-*')))

    def test_sparse_versus_dense_crown(self):
        import numpy as np
        dense = np.zeros((256, 128), np.uint8)
        dense[10:180, 20:108] = 255            # solid crown block
        dense[180:256, 60:68] = 255            # trunk
        sparse = np.zeros((256, 128), np.uint8)
        for row in range(10, 180, 6):
            sparse[row:row + 2, 20:108] = 255  # thin branch rows, mostly gaps
        sparse[:, 62:66] = 255
        self.assertFalse(patcher.is_sparse(dense))
        self.assertTrue(patcher.is_sparse(sparse))
        cut, last = patcher.crown_profile(dense)
        self.assertTrue(0.5 < cut < 0.9, cut)

    def test_animation_gain_scales_limb_angle_and_defaults_to_one(self):
        self.assertEqual(patcher.ANIM_GAIN, 1.0)
        self.assertEqual(patcher.gained(20.0), 20.0)
        patcher.ANIM_GAIN = 3.0
        try:
            self.assertEqual(patcher.gained(20.0), 60.0)
            self.assertEqual(patcher.gained(-90.0), -110.0)   # clamped
        finally:
            patcher.ANIM_GAIN = 1.0



class GreeceIndexTests(unittest.TestCase):
    def test_greece_tre_sources_are_hash_pinned_and_opt_in(self):
        index=json.loads((HERE/'source-index.json').read_text())
        gre={k:v for k,v in index.items() if k.startswith(('grehit.bfl/','grelot.bfl/'))}
        self.assertEqual(len(gre),45)
        for key,rec in gre.items():
            self.assertRegex(rec['sha256'],'^[0-9a-f]{64}$')
            self.assertTrue(key.split('/')[1].startswith('tre_') and key.endswith('.dds'))
            self.assertIn(rec['category'],('individual-tree','cliff-card','hedge-card'))
        # Cliff/rock cards are pinned for identity only and never converted as trees.
        self.assertTrue(all(v['category']=='cliff-card' for k,v in gre.items() if '/tre_fal' in k))
        # Box-like hedge/strip cards (fr*, s1_b/c/z) are held: pinned, never converted.
        held={k for k,v in gre.items() if v['category']=='hedge-card'}
        self.assertEqual({k.split('/')[1][:-4] for k in held},{'tre_fra','tre_frb','tre_frc','tre_frz','tre_s1_b','tre_s1_c','tre_s1_z'})
        # No Greece mapping joins the reviewed default/crossed sets: Greece trees are --all-trees only.
        self.assertFalse([1 for a,_ in list(patcher.TREE_CANDIDATES)+list(patcher.CROSSED_CANDIDATES) if a.startswith('gre')])

if __name__ == '__main__':
    unittest.main()
