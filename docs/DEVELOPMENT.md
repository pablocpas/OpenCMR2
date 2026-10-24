# Development workflow

Build requirements and platform presets are in the root [README](../README.md).
CI builds each supported platform using the same presets.

## Build and test

On Linux, run the native build and 32-bit reference build:

```sh
cmake --preset linux
cmake --build --preset linux
ctest --preset linux

cmake --preset linux-x86
cmake --build --preset linux-x86
ctest --preset linux-x86
```

The 32-bit build needs i386 libraries. On Ubuntu 24.04 (the CI environment),
SDL3 is built from source:

```sh
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install clang g++-multilib ninja-build glslang-tools pkg-config \
    libx11-dev:i386 libxext-dev:i386 libxrandr-dev:i386 libxcursor-dev:i386 \
    libxi-dev:i386 libxss-dev:i386 libxtst-dev:i386 libxkbcommon-dev:i386 \
    libwayland-dev:i386 wayland-protocols libegl1-mesa-dev:i386 \
    libasound2-dev:i386 libudev-dev:i386 libdbus-1-dev:i386
export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig
cmake --preset linux-x86 -DOPENCMR2_VENDORED_SDL=ON
```

On Fedora (`SDL3-devel.i686` is used when it is 3.4 or later):

```sh
sudo dnf install clang gcc-c++ glibc-devel.i686 libstdc++-devel.x86_64 \
    libstdc++-devel.i686 SDL3-devel.i686 ninja-build glslang
```

To include real-stage physics, runtime, lighting and movie regressions,
configure with your own original game data, then rebuild and test:

```sh
cmake --preset linux -DOPENCMR2_BINK_TEST_DATA_DIR="$HOME/cmr2game"
cmake --build --preset linux
ctest --preset linux
```

Use the same option for `linux-x86`. LAN game tests are registered for native
builds when game data is supplied; network tests require loopback UDP access.
See [PHYSICS_PARITY.md](PHYSICS_PARITY.md) and [BASELINE.md](BASELINE.md) for
the fixture scope and recording commands.

The `game_data_setup` test uses synthetic archive placeholders and simulated
desktop dialogs with SDL's dummy video driver. It covers first-run selection,
missing files, reading archives and writing saves under UTF-8 paths, remembered
settings, cancellation and a `Data` folder beside the executable. It also
checks selecting subfolders or parent folders, ambiguous installations and
symlink cycles on every CI platform, without original assets or a display
server.

## Far layer sweep

The far layer (`game/FarLayer.cpp`) draws the stage beyond its view distance
behind the stage itself. `tools/far_layer_sweep.py` checks it on every stage
with your game data: the probe places the car along each route, facing along
and across it, and compares the frame without the layer (sky keyed green) with
the frame with the layer in marker colours, at the same moment.

```sh
tools/far_layer_sweep.py --probe build/linux/tests/stage_probe --data "$HOME/cmr2game" --out sweep
```

A view has a fault when layer pixels show where the stage was; there must be
none, and the script exits with an error if there are. Layer pixels nearer
than half the view distance where the sky was mean the layer fills a gap close
to the car, which is worth a look. `sweep/index.html` lists the stages and
shows those views and one view of each stage, as drawn and marked.

## Sanitizers

The `linux-debug` preset builds with AddressSanitizer and UBSan. With the
vendored SDL, the shared ASan runtime must be linked into both SDL and the
game, and found at run time:

```sh
cmake --preset linux-debug -DOPENCMR2_VENDORED_SDL=ON \
    -DOPENCMR2_BINK_TEST_DATA_DIR="$HOME/cmr2game" \
    "-DCMAKE_SHARED_LINKER_FLAGS=-shared-libasan -fsanitize=address,undefined" \
    "-DCMAKE_EXE_LINKER_FLAGS=-shared-libasan" \
    "-DCMAKE_CXX_FLAGS=-fsanitize-recover=address" "-DCMAKE_C_FLAGS=-fsanitize-recover=address"
cmake --build --preset linux-debug --target stage_probe
LD_LIBRARY_PATH="$(dirname "$(clang -print-file-name=libclang_rt.asan.so)")" \
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 \
    python3 tests/runtime_scenarios.py --probe build/linux-debug/tests/stage_probe --data "$HOME/cmr2game"
```

Each scenario's `run.log` holds the reports. Some of the original's code
reads a dword and keeps its low byte, running past its variable without
changing the value, or ends a loop at `&a[n]` and indexes the members after
an array; `tools/sanitizer-ignorelist.txt` lists those functions with the
access, and UBSan's function type check is off because the game calls its
callback tables through generic types. The scenarios run without reports, so
any new one is worth a look. The 60-second scenario timeout may
need raising under the sanitizers.

## Source sync and generated files

`game/` follows CMR2Decomp; `UPSTREAM` records the imported commit.
Use `tools/sync_upstream.py`, inspect its changes and resolve any conflicts.
Port adaptations and tests must remain valid after a sync. Port code keeps the
original's floating-point conventions: after a sync that touches it, run
`tools/ftoi_rewrite.py --port` (float-to-integer conversions) and
`tools/x87s_rewrite.py --port` (operations the original ran at x87 single
precision); both need `clang-devel`, and a second run finds nothing.

When compatibility-sensitive structures change, check the disk-layout header:

```sh
python3 tools/gen_disk_layouts.py --check
```

Regenerate it with `python3 tools/gen_disk_layouts.py` when needed and review
the diff. The raw-access audit requires `clang-devel` and writes its detailed
JSON and plugin caches under `build/`:

```sh
python3 tests/raw_access_audit.py --plugin
python3 tools/audit_raw_access.py --json build/raw-access-current.json \
  --markdown docs/64bit-raw-access.md --jobs 4
```

The Markdown audit is generated; update it with the tool. Individual reviews
in `tools/raw_access/reviewed.json` retain source guards and automatic evidence.

## Files to keep

- Keep guides in `docs/` and link them from its index. Results of a past
  investigation belong in the commit message, not in a new document.
- Keep frozen regression inputs, accepted comparisons and provenance in
  `tests/reference/`. Its recordings and audit logs support reproducible tests.
- Put temporary plans, logs, captures, build products and downloaded
  toolchains under `build/`. It is ignored by Git, as are `out/`, `dist/`,
  Python caches and local `CMakeUserPresets.json`.
- Never commit original game files. Release packages contain only the
  executable, its libraries and the licences.

Before committing, review `git status --short`, `git diff --check` and the staged
diff.
