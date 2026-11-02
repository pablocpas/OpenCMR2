#!/usr/bin/env python3
"""Read-only inventory of tre* tree-card members across every .bfl under an owned game folder.

Writes a small JSON text file (archive path relative to the data folder, member name, size, DDS/TGA
dimensions, sha256). No pixels or game files are copied, so the result is safe to send. It answers
"which archive holds tre_103?" without guessing, e.g. for the native log's uncovered stems. Archive
identity is NOT verified here (unlike export-greece-trees.py): a hit locates a stem, nothing more.
Python standard library only; the output file must not exist and must live outside the game data.
"""
import argparse
import importlib.util
import hashlib
import json
import os
from pathlib import Path

_spec = importlib.util.spec_from_file_location('export_greece_trees', Path(__file__).with_name('export-greece-trees.py'))
_g = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_g)
_export = _g._export
LIMIT = 256 * 1024 * 1024

def inventory(data, output, stems=()):
    data = data.resolve(strict=True); output = output.absolute()
    if not data.is_dir(): raise ValueError('data must be directory')
    if output.exists(): raise ValueError('output already exists; choose a new file path')
    parent = output.parent.resolve(strict=True)
    if parent == data or data in parent.parents:
        raise ValueError('output must be outside original game data')
    want = {s.lower() for s in stems}
    rows = []; skipped = []
    for directory, dirs, files in os.walk(data, followlinks=False):
        dirs[:] = sorted(d for d in dirs if not (Path(directory) / d).is_symlink())
        for name in sorted(files):
            p = Path(directory) / name
            if not name.lower().endswith('.bfl') or p.is_symlink(): continue
            rel = p.relative_to(data).as_posix()
            if p.stat().st_size > LIMIT: skipped.append({'archive': rel, 'why': 'size limit'}); continue
            try: members = list(_export.members(p.read_bytes()))
            except (ValueError, OSError, EOFError, Exception) as e:
                skipped.append({'archive': rel, 'why': 'unreadable: ' + str(e)[:80]}); continue
            for member, raw in members:
                leaf = member.replace('\\', '/').rsplit('/', 1)[-1].lower()
                if not _g._wanted(member): continue
                stem = leaf.rpartition('.')[0]
                if want and stem not in want: continue
                rows.append({'archive': rel, 'member': leaf, 'bytes': len(raw), 'dimensions': _g._dims(leaf, raw),
                             'sha256': hashlib.sha256(raw).hexdigest()})
    found = {r['member'].rpartition('.')[0] for r in rows}
    report = {'scope': 'tre* members per .bfl, locator only; archive identity unverified; no pixels included',
              'stems_requested': sorted(want), 'stems_not_found': sorted(want - found),
              'skipped_archives': skipped, 'hits': sorted(rows, key=lambda r: (r['archive'], r['member']))}
    with output.open('x', encoding='utf-8') as f: f.write(json.dumps(report, indent=2) + '\n')
    print('Tree inventory:', output, '-', len(rows), 'members in', len({r['archive'] for r in rows}), 'archives')
    print('No originals changed. Nothing uploaded.')
    return report

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--data', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--stems', default='', help='optional comma-separated stems (e.g. tre_103,tre_130); default all tre*')
    a = ap.parse_args()
    try: inventory(a.data, a.output, [s.strip() for s in a.stems.split(',') if s.strip()])
    except (ValueError, OSError) as e: ap.exit(1, str(e) + '\n')

if __name__ == '__main__':
    main()
