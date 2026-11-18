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

    def test_progress_output_during_prepare(self):
        # The pack step ran silently for minutes on a real Mac and looked hung: it must narrate phases.
        verify = self.run_patch('--check-only')
        self.assertIn('Reading originals 1/', verify.stdout)
        full = self.run_patch('--tree-geometry', 'crossed')
        self.assertIn('Reading originals 1/', full.stdout)
        self.assertIn('Converting assets 1/', full.stdout)
        self.assertLess(full.stdout.index('Converting assets 1/'), full.stdout.index('Created'))
        # The slow step is the per-asset conversion: the in-progress line must name the
        # asset being worked on BEFORE it runs, so a long conversion never leaves the
        # previous asset's name on screen (review finding on PR #386).
        self.assertIn('Converted 1/', full.stdout)
        self.assertLess(full.stdout.index('Converting assets 1/'), full.stdout.index('Converted 1/'))

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
        # Devin #385: crossed trees must not carry the shell-only textures tree() wrote first.
        self.assertFalse(list(self.output.glob('trees/*/*/cap.tga')))
        self.assertFalse(list(self.output.glob('trees/*/*/core.tga')))
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


class SolidCrownTests(unittest.TestCase):
    """Hollow-crown fix: the adaptive shell must be a closed outer volume with a
    nested inner core, so alpha holes show foliage, not sky through the crown."""
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
        self.script.with_name('source-index.json').write_text(json.dumps(self.index))

    def tearDown(self):
        self.tmp.cleanup()

    def test_crown_shell_closed_with_inner_core(self):
        run = subprocess.run([sys.executable, str(self.script), '--input-dir', str(self.input),
                              '--output-dir', str(self.output)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        obj = next(self.output.glob('trees/*/*/tree.obj'))
        verts, faces = [], []
        material = None
        for line in obj.read_text().splitlines():
            f = line.split()
            if not f: continue
            if f[0] == 'v': verts.append(tuple(round(float(x), 6) for x in f[1:4]))
            elif f[0] == 'usemtl': material = f[1]
            elif f[0] == 'f' and material in ('crown', 'core', 'cap'):
                faces.append(tuple(int(x.split('/')[0]) - 1 for x in f[1:4]))
        import numpy as np
        from collections import Counter
        pos = lambda i: verts[i]
        P = np.array([pos(i) for f_ in faces for i in f_])
        cx, cz = float(P[:, 0].mean()), float(P[:, 2].mean())
        rmax = max(((v[0]-cx)**2 + (v[2]-cz)**2) ** .5 for v in verts)
        inward = 0
        edges = Counter()
        for a, b, c in faces:
            pa, pb, pc = np.array(pos(a)), np.array(pos(b)), np.array(pos(c))
            n = np.cross(pb - pa, pc - pa)
            cen = (pa + pb + pc) / 3
            radial = np.array([cen[0] - cx, 0.0, cen[2] - cz])
            if np.linalg.norm(n) > 1e-12 and n @ radial < -1e-12:
                inward += 1
            for e in ((a, b), (b, c), (c, a)):
                edges[tuple(sorted((pos(e[0]), pos(e[1]))))] += 1
        self.assertTrue(inward > 0, 'no inner core: crown has no inward-facing shell')
        boundary = [(e, ((e[0][0]+e[1][0])/2 - cx, (e[0][2]+e[1][2])/2 - cz))
                    for e, n in edges.items() if n != 2]
        outer_open = [e for e, (ex, ez) in boundary if (ex*ex + ez*ez) ** .5 > 0.8 * rmax]
        self.assertEqual(outer_open, [],
                         'outer crown shell is open (hollow): %d boundary edges at the outer radius' % len(outer_open))

    def test_inner_core_shades_darker_by_texture(self):
        # The renderer disables lighting for scenery packs (SceneryPack.cpp State: SetLighting FALSE),
        # so reversed normals cannot darken the core; the depth cue must come from darker texels.
        run = subprocess.run([sys.executable, str(self.script), '--input-dir', str(self.input),
                              '--output-dir', str(self.output)], capture_output=True, text=True)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        obj = next(self.output.glob('trees/*/*/tree.obj'))
        self.assertIn('usemtl core', obj.read_text(), 'inner core faces need their own material')
        self.assertIn('map_Kd core.tga', obj.with_name('tree.mtl').read_text())
        import numpy as np
        crown = np.array(Image.open(obj.with_name('crown.tga')).convert('RGBA')).astype(float)
        core = np.array(Image.open(obj.with_name('core.tga')).convert('RGBA')).astype(float)
        opaque = crown[:, :, 3] >= 128
        self.assertEqual(crown.shape, core.shape)
        self.assertTrue((core[:, :, 3] == crown[:, :, 3]).all(), 'core alpha must match the crown atlas')
        self.assertLess(core[:, :, :3][opaque].mean(), crown[:, :, :3][opaque].mean() * 0.75,
                        'core texels must be visibly darker than the crown')


class CrownCapUV(unittest.TestCase):
    def setUp(self):
        self.base = Path(tempfile.mkdtemp())

    def tearDown(self):
        shutil.rmtree(self.base)

    def test_cap_uvs_have_area_and_share_planar_coordinates(self):
        # Olli 2026-11-18: "these trees look like a swirl from below". The cap fan gave every
        # vertex the same texture row (center (0,v), ring (theta,v)): the underside sampled one
        # atlas line swept radially. Cap triangles must have 2D UV area and agree at shared points.
        oldroot = getattr(patcher, 'root', None)
        try:
            patcher.root = self.base / 'out'; patcher.root.mkdir()
            src = self.base / 'tre_cap.dds'
            im = Image.new('RGBA', (128, 128), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
            d.ellipse((8, 4, 120, 100), fill=(40, 160, 60, 255))
            d.rectangle((58, 80, 70, 127), fill=(90, 60, 30, 255))
            im.save(src)
            rel = patcher.tree({'path': str(src), 'archive': 'finhit.bfl', 'texture': 'tre_cap.dds'}, Path('trees/tre_cap'), adaptive=True)
            self.assertIn('map_Kd cap.tga',(patcher.root/rel).with_name('tree.mtl').read_text())
            cut=round(im.height*patcher.crown_profile(__import__('numpy').array(im)[:,:,3])[0])
            expected=im.crop((im.width//4,cut//4,3*im.width//4,3*cut//4)).resize((256,256),Image.Resampling.LANCZOS)
            self.assertEqual(Image.open((patcher.root/rel).with_name('cap.tga')).tobytes(),expected.tobytes())
            vs, vts, mat, caps, bad = [], [], None, 0, 0
            shared={}
            for line in (patcher.root / rel).read_text().splitlines():
                t = line.split()
                if not t: continue
                if t[0] == 'v': vs.append(tuple(float(x) for x in t[1:4]))
                elif t[0] == 'vt': vts.append(tuple(float(x) for x in t[1:3]))
                elif t[0] == 'usemtl': mat = t[1]
                elif t[0] == 'f' and mat in ('crown','cap'):
                    idx = [int(k.split('/')[0]) - 1 for k in t[1:4]]
                    if len({round(vs[i][1], 6) for i in idx}) == 1:  # flat crown face = cap fan
                        caps += 1
                        uv=[vts[i] for i in idx]
                        area=abs((uv[1][0]-uv[0][0])*(uv[2][1]-uv[0][1])-(uv[1][1]-uv[0][1])*(uv[2][0]-uv[0][0]))
                        if area < 0.001: bad += 1
                        for i in idx:
                            key=tuple(round(x,6) for x in vs[i]); value=vts[i]
                            if key in shared: self.assertEqual(shared[key],value,'cap UVs disagree at a shared point')
                            shared[key]=value
            self.assertGreater(caps, 0)
            self.assertEqual(bad, 0, f"{bad}/{caps} cap triangles sample a single atlas row (swirl from below)")
        finally:
            patcher.root = oldroot

if __name__ == '__main__':
    unittest.main()
