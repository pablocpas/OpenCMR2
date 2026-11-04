#!/usr/bin/env python3
"""Build/run the cockpit window depth-write rendered-pixel regression from an existing
GL-enabled CMake build. Requires a usable SDL OpenGL video driver. Defaults
to SDL offscreen on Linux. No native Mac result is implied by execution on
another operating system."""
import argparse
import json
import os
import pathlib
import shlex
import subprocess
import sys

p = argparse.ArgumentParser()
p.add_argument('--build', required=True, type=pathlib.Path)
p.add_argument('--output', type=pathlib.Path)
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
build = a.build.resolve()
commands = json.loads((build / 'compile_commands.json').read_text())
entry = next(c for c in commands if c['file'].endswith('/render/gfx_gl.cpp'))
args = entry.get('arguments') or shlex.split(entry['command'])
out = build / 'cockpit_window_render'
result = []
i = 0
while i < len(args):
    v = args[i]
    if v in ('-o', '-c'):
        if v == '-o': i += 2; continue
        i += 1; continue
    if v == entry['file']: v = str(root / 'tests/cockpit_window_render.cpp')
    result.append(v); i += 1
sdl = list(build.rglob('libSDL3.so')) + list(build.rglob('libSDL3.dylib'))
if not sdl:
    sys.exit('No built SDL3 shared library found: supply a vendored-SDL GL-enabled build')
lib = sdl[0].resolve().parent
result += ['-fuse-ld=lld', '-L' + str(lib), '-Wl,-rpath,' + str(lib), '-lSDL3', '-o', str(out)]
subprocess.run(result, cwd=entry['directory'], check=True)
env = os.environ.copy()
if sys.platform.startswith('linux'): env.setdefault('SDL_VIDEODRIVER', 'offscreen')
subprocess.run([str(out)] + ([str(a.output.resolve())] if a.output else []), env=env, check=True)
