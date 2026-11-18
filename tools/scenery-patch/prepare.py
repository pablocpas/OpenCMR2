#!/usr/bin/env python3
"""Internal binary helper: verify and reuse/create private scenery, no game launch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

if sys.version_info < (3, 10):
    print('Enhancement preparation requires Python3.10+', file=sys.stderr)
    raise SystemExit(78)

HERE = Path(__file__).resolve().parent
FILES = ('convert.py', 'source-index.json', 'requirements.txt')


def run(argv):
    subprocess.run([str(x) for x in argv], check=True)


def extras_args(all_trees, gain):
    # Only non-default experimental options add converter flags; the default recipe stays byte-identical to before.
    out = []
    if all_trees:
        out.append('--all-trees')
    if gain != 1.0:
        out += ['--animation-gain', str(gain)]
    return out


def recipe_key(mode, all_trees=False, gain=1.0):
    h = hashlib.sha256(mode.encode())
    for extra in extras_args(all_trees, gain):
        h.update(extra.encode())
    for name in FILES:
        h.update((HERE / name).read_bytes())
    return h.hexdigest()[:16]


def python_command(cache, install):
    # Reuse source-checkout's environment first, never replace an existing venv.
    exe = 'Scripts/python.exe' if os.name == 'nt' else 'bin/python'
    candidates = [HERE / '.venv' / exe, cache / 'python' / exe]
    candidates += [Path(sys.executable)]
    for path in candidates:
        if path.is_file():
            p = subprocess.run([str(path), '-c', 'import PIL,numpy'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if p.returncode == 0:
                return [str(path)]
    uv = shutil.which('uv')
    if not uv:
        raise ValueError('Scenery needs Python with Pillow/numpy or uv. No software was installed. Install uv, then retry with --install-enhancement-deps to allow isolated setup.')
    if install:
        env = cache / 'python'
        if not env.exists():
            run([uv, 'venv', env, '--python', '3.12'])
        py = env / exe
        if not py.is_file():
            raise ValueError('Existing enhancement Python directory is invalid; not replacing it. Select a fresh cache directory.')
        run([uv, 'pip', 'install', '--python', py, '-r', HERE / 'requirements.txt'])
        return [str(py)]
    # Offline mode must not resolve/install dependencies as a side effect.
    raise ValueError('Scenery dependencies missing. Retry this same binary command with --install-enhancement-deps to permit isolated uv setup; originals and saves are unchanged.')


def inventory(folder):
    result = {}
    for p in sorted(folder.rglob('*')):
        if p.is_symlink():
            raise ValueError('Generated pack contains a symlink; refusing reuse')
        if p.is_file() and p.name != 'prepared.json':
            result[p.relative_to(folder).as_posix()] = hashlib.sha256(p.read_bytes()).hexdigest()
    return result


def prepare(data, cache, mode, install=False, all_trees=False, gain=1.0):
    data = data.resolve(strict=True)
    if cache.is_symlink():
        raise ValueError("Symlink cache is not supported")
    cache = cache.resolve()
    if cache == data or data in cache.parents:
        raise ValueError('Enhancement cache must be outside original game data')
    cache.mkdir(parents=True, exist_ok=True)
    if cache.is_symlink():
        raise ValueError('Symlink cache is not supported')
    command = python_command(cache, install)
    key = recipe_key(mode, all_trees, gain)
    extra = extras_args(all_trees, gain)
    pack = cache / ('scenery-' + mode + '-' + key)
    # Always reverify original archives, even when a cached derived pack exists.
    # check-only requires new/empty output, so use an absent path inside a temp.
    print('Verifying original game archives (hash check of owned data)...', flush=True)
    with tempfile.TemporaryDirectory(prefix='.verify-', dir=cache) as tmp:
        run(command + [HERE / 'convert.py', '--input-dir', data, '--output-dir', Path(tmp) / 'unused', '--tree-geometry', mode] + extra + ['--check-only'])
    if pack.is_symlink():
        raise ValueError("Symlink pack is not supported")
    if pack.exists():
        try:
            marker = json.loads((pack / 'prepared.json').read_text())
        except (OSError, ValueError) as e:
            raise ValueError('Existing pack lacks a valid completion record; not replacing it: ' + str(pack)) from e
        if marker != {'recipe': key, 'files': inventory(pack)}:
            raise ValueError('Cached pack has changed or is incomplete; not overwriting: ' + str(pack))
        print('Reusing verified private scenery:', pack)
    else:
        # Converter already stages atomically, then make helper completion atomic too.
        print(f'Generating scenery pack ({mode}, first run for this recipe; can take minutes)...', flush=True)
        with tempfile.TemporaryDirectory(prefix='.prepare-', dir=cache) as tmp:
            stage = Path(tmp) / 'pack'
            run(command + [HERE / 'convert.py', '--input-dir', data, '--output-dir', stage, '--tree-geometry', mode] + extra)
            (stage / 'prepared.json').write_text(json.dumps({'recipe': key, 'files': inventory(stage)}, sort_keys=True))
            # Never replace an existing destination, including a concurrent generation.
            if pack.exists():
                raise ValueError('Another run created the pack; retry safely: ' + str(pack))
            stage.rename(pack)
        print('Prepared private scenery:', pack)
    print("OPENCMR2_PACK=" + str(pack))
    return pack


def prepare_cars(mods, cache, choices):
    import zipfile
    index = json.loads((HERE / 'car-index.json').read_text())
    selected = choices.split(',')
    if not selected or any(x not in index for x in selected) or len(set(selected)) != len(selected):
        raise ValueError('Unknown/duplicate car alternative; see --help-enhancements')
    targets = {}
    payloads = {}
    labels = []
    for name in selected:
        rec = index[name]
        labels.append((rec['slot'], rec['display_name'], rec['short_name']))
        archive = mods / rec['archive']
        if not archive.is_file():
            raise ValueError('Missing owned ZIP ' + rec['archive'] + '; --mods-dir must contain original ZIP filenames, see --help-enhancements. Nothing downloaded.')
        if archive.is_symlink() or hashlib.sha256(archive.read_bytes()).hexdigest() != rec['sha256']:
            raise ValueError('Unsupported or modified owned ZIP: ' + rec['archive'])
        with zipfile.ZipFile(archive) as z:
            members = {}
            for item in z.infolist():
                leaf = Path(item.filename.replace('\\', '/')).name.lower()
                if leaf in members:
                    raise ValueError('Duplicate ZIP leaf')
                members[leaf] = item
            for f in rec['files']:
                target = Path(f['target']).name
                key = target.lower()
                if key in targets:
                    raise ValueError('Conflicting car alternatives: ' + targets[key] + ' and ' + name)
                item = members.get(key)
                if item is None or item.file_size != f['bytes']:
                    raise ValueError('Incomplete car ZIP: ' + name)
                raw = z.read(item)
                if hashlib.sha256(raw).hexdigest() != f['sha256']:
                    raise ValueError('Car member hash mismatch: ' + target)
                targets[key] = name
                payloads[target] = raw
    payloads['car-names.tsv'] = ('OPENCMR2-CAR-NAMES-1\n' + ''.join('\t'.join(row) + '\n' for row in sorted(labels))).encode('ascii')
    # Complete files stay together, only selected archive slots are published.
    key = hashlib.sha256((json.dumps(index, sort_keys=True) + ','.join(sorted(selected))).encode()).hexdigest()[:16]
    cache.mkdir(parents=True, exist_ok=True)
    out = cache / ('cars-' + key)
    expected = {name: hashlib.sha256(raw).hexdigest() for name, raw in payloads.items()}
    if out.is_symlink():
        raise ValueError("Symlink car cache is not supported")
    if out.exists():
        if inventory(out) != expected:
            raise ValueError('Existing car cache changed; not overwriting: ' + str(out))
    else:
        with tempfile.TemporaryDirectory(prefix='.cars-', dir=cache) as tmp:
            stage = Path(tmp) / 'cars'
            stage.mkdir()
            for name, raw in payloads.items():
                (stage / name).write_bytes(raw)
            if out.exists():
                raise ValueError('Concurrent car generation; retry')
            stage.rename(out)
    print('OPENCMR2_CARS=' + str(out.resolve()))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input-dir', type=Path, required=True)
    ap.add_argument('--cache-dir', type=Path, required=True)
    ap.add_argument('--mode', choices=['shell', 'crossed'])
    ap.add_argument('--cars')
    ap.add_argument('--mods-dir', type=Path)
    ap.add_argument('--install-deps', action='store_true')
    ap.add_argument('--all-trees', action='store_true')
    ap.add_argument('--animation-gain', type=float, default=1.0)
    args = ap.parse_args()
    try:
        data = args.input_dir.resolve(strict=True)
        if args.cache_dir.is_symlink():
            raise ValueError('Symlink cache is not supported')
        cache = args.cache_dir.resolve()
        if cache == data or data in cache.parents:
            raise ValueError('Cache must be outside original game data')
        if args.cars:
            if not args.mods_dir:
                raise ValueError('--cars requires --mods-dir with owned ZIPs')
            prepare_cars(args.mods_dir, cache, args.cars)
        elif args.mode:
            prepare(data, cache, args.mode, args.install_deps, args.all_trees, args.animation_gain)
        else:
            raise ValueError('No preparation mode selected')
    except (OSError, ValueError, subprocess.CalledProcessError) as e:
        ap.exit(1, str(e) + '\n')


if __name__ == '__main__':
    main()
