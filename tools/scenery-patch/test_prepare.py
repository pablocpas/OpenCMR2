"""Synthetic safety tests for binary preparation helper. No game artwork/network."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location('prepare', HERE / 'prepare.py')
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)


class PrepareTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.data = self.root / 'owned game with spaces'
        self.data.mkdir()
        self.cache = self.root / 'cache'
        self.original = self.data / 'untouched'
        self.original.write_bytes(b'original')
        self.calls = []

    def tearDown(self):
        self.assertEqual(self.original.read_bytes(), b'original')
        self.temp.cleanup()

    def fake_run(self, argv):
        self.calls.append([str(x) for x in argv])
        if '--check-only' not in argv:
            out = Path(argv[argv.index('--output-dir') + 1])
            out.mkdir()
            (out / 'manifest.tsv').write_text('synthetic')
            (out / 'tree.obj').write_text('synthetic tree')

    def prep(self, mode='shell'):
        with patch.object(p, 'python_command', return_value=['fake python']), patch.object(p, 'run', side_effect=self.fake_run):
            return p.prepare(self.data, self.cache, mode)

    def test_repeat_and_modes(self):
        a = self.prep()
        before = p.inventory(a)
        self.assertEqual(self.prep(), a)
        self.assertEqual(before, p.inventory(a))
        self.assertEqual(sum('--check-only' not in x for x in self.calls), 1)
        self.assertNotEqual(self.prep('crossed'), a)
        self.assertTrue(a.is_dir())

    def test_prepare_narrates_steps(self):
        import contextlib, io
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            self.prep()
        out = buf.getvalue()
        self.assertIn('Verifying original', out)
        self.assertIn('Generating scenery pack', out)
        self.assertLess(out.index('Verifying original'), out.index('Generating scenery pack'))

    def test_changed_pack_not_overwritten(self):
        a = self.prep()
        (a / 'tree.obj').write_text('edited')
        with self.assertRaisesRegex(ValueError, 'changed'):
            self.prep()
        self.assertEqual((a / 'tree.obj').read_text(), 'edited')

    def test_source_failure_keeps_pack(self):
        a = self.prep()
        with patch.object(p, 'python_command', return_value=['fake']), patch.object(p, 'run', side_effect=ValueError('bad hash')):
            with self.assertRaisesRegex(ValueError, 'bad hash'):
                p.prepare(self.data, self.cache, 'shell')
        self.assertTrue((a / 'manifest.tsv').exists())

    def test_input_cache_refused(self):
        with self.assertRaisesRegex(ValueError, 'outside'):
            p.prepare(self.data, self.data / 'cache', 'shell')

    def test_symlink_pack_refused(self):
        self.cache.mkdir()
        (self.cache / ('scenery-shell-' + p.recipe_key('shell'))).symlink_to(self.data)
        with self.assertRaisesRegex(ValueError, 'Symlink'):
            self.prep()

    def test_no_implicit_download(self):
        # No dependency or uv command should execute without explicit permission.
        with patch.object(p.shutil, 'which', return_value='/fake/uv'), patch.object(p.subprocess, 'run') as run:
            run.return_value.returncode = 1
            with self.assertRaisesRegex(ValueError, 'install-enhancement-deps'):
                p.python_command(self.cache, False)
            self.assertTrue(all('-c' in call.args[0] for call in run.call_args_list))

    def cars_fixture(self):
        mods = self.root / 'mods'
        mods.mkdir()
        idx = {}
        for alias in ['one', 'two']:
            archive = mods / (alias + '.zip')
            raw = ('asset ' + alias).encode()
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('folder/205a1.bfl', raw)
            idx[alias] = {'slot': '205', 'display_name': 'Peugeot 205 T16 EVO Dakar 1987', 'short_name': '205 T16 Dakar 1987', 'archive': archive.name, 'sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
                          'files': [{'target': 'Game/Cars/205a1.bfl', 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}]}
        helper = self.root / 'helper'
        helper.mkdir()
        (helper / 'car-index.json').write_text(json.dumps(idx))
        return mods, helper

    @unittest.skipUnless(importlib.util.find_spec("PIL"), "optional Pillow not installed; converter tests require it")
    def test_real_converter_helper_repeat(self):
        import shutil
        import test_convert
        # Reuse the actual synthetic DDS/BFL fixture class, not original art.
        cls = next(v for v in vars(test_convert).values() if isinstance(v, type) and issubclass(v, unittest.TestCase) and v is not unittest.TestCase)
        fixture = cls()
        fixture.setUp()
        try:
            helper = fixture.base
            shutil.copy(HERE / 'requirements.txt', helper / 'requirements.txt')
            with patch.object(p, 'HERE', helper):
                out = p.prepare(fixture.input, fixture.base / 'cache', 'crossed')
                self.assertTrue((out/'manifest.tsv').exists())
                self.assertEqual(out, p.prepare(fixture.input, fixture.base / 'cache', 'crossed'))
                self.assertEqual(len((out/'manifest.tsv').read_text().splitlines()), 3)
        finally:
            fixture.tearDown()

    def test_car_hash_reuse_conflicts_and_tamper(self):
        mods, helper = self.cars_fixture()
        with patch.object(p, 'HERE', helper):
            a = p.prepare_cars(mods, self.cache, 'one')
            self.assertEqual(a, p.prepare_cars(mods, self.cache, 'one'))
            self.assertEqual((a/'car-names.tsv').read_text(), 'OPENCMR2-CAR-NAMES-1\n205\tPeugeot 205 T16 EVO Dakar 1987\t205 T16 Dakar 1987\n')
            with self.assertRaisesRegex(ValueError, 'Conflicting'):
                p.prepare_cars(mods, self.cache, 'one,two')
            (a / '205a1.bfl').write_bytes(b'edit')
            with self.assertRaisesRegex(ValueError, 'changed'):
                p.prepare_cars(mods, self.cache, 'one')
            (mods / 'two.zip').write_bytes(b'not the zip')
            with self.assertRaisesRegex(ValueError, 'modified'):
                p.prepare_cars(mods, self.cache, 'two')

    def test_unknown_and_incomplete_car(self):
        mods, helper = self.cars_fixture()
        with patch.object(p, 'HERE', helper):
            with self.assertRaisesRegex(ValueError, 'Unknown'):
                p.prepare_cars(mods, self.cache, 'missing')
            with self.assertRaisesRegex(ValueError, 'Missing owned ZIP'):
                p.prepare_cars(mods/'absent', self.cache, 'one')
            idx = json.loads((helper / 'car-index.json').read_text())
            idx['one']['files'][0]['bytes'] += 1
            (helper / 'car-index.json').write_text(json.dumps(idx))
            with self.assertRaisesRegex(ValueError, 'Incomplete'):
                p.prepare_cars(mods, self.cache, 'one')

    def test_experimental_options_change_recipe_only_when_set(self):
        base = p.recipe_key('shell')
        self.assertEqual(base, p.recipe_key('shell', False, 1.0))
        self.assertNotEqual(base, p.recipe_key('shell', True, 1.0))
        self.assertNotEqual(base, p.recipe_key('shell', False, 2.0))
        self.assertNotEqual(p.recipe_key('shell', False, 2.0), p.recipe_key('shell', False, 3.0))
        self.assertEqual(p.extras_args(False, 1.0), [])
        self.assertEqual(p.extras_args(True, 2.5), ['--all-trees', '--animation-gain', '2.5'])

if __name__ == '__main__':
    unittest.main()
