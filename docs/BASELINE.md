# Stage baseline and CPU profiling

These tools record a real stage with controlled clocks and inputs, replay it
at different render cadences and measure CPU frame timings. They compare the
port with itself; the comparison with the original executable is in
[ORIGINAL_REFERENCE.md](ORIGINAL_REFERENCE.md) and
[PHYSICS_PARITY.md](PHYSICS_PARITY.md).

Normal play has no observer unless `OPENCMR2_PROFILE` is set. The fake clock and
keyboard overrides are enabled only by the separate `stage_probe` executable.

## Capture and playback

Build the 32-bit reference preset, then run:

```sh
python3 tools/stage_baseline.py run --data ~/cmr2game --output build/baseline-first
```

The driver loads Finland stage 1 through the real frontend and garage, with
fresh default settings, one default 4WD car and automatic transmission. It
isolates settings/saves in a temporary `XDG_DATA_HOME` for every run and uses
SDL's dummy video/audio drivers and the null graphics executor. Game-side
drawing, loading, audio decisions and the original tick body still run.

All runs bootstrap at the same 33 ms cadence and fixed Unix seed. After entering
race mode, the clock advances at the requested cadence using integer millisecond
samples. The clock sampling points are retained; the scheduler now evaluates
both rounded boundaries in double precision to prevent duplicate steps. The
variable cadence repeats 7/13/33/5/22/48/9/23 ms intervals.

The default 500 ticks cover approximately 20 simulation seconds: 125 countdown
ticks and 375 driving ticks. The script accelerates, steers left/right, coasts,
brakes and briefly applies the handbrake. Controls are assigned at the existing
boundary after device mapping and before the car update, indexed by simulation
tick. They are captured separately from the resulting car state.

The output includes:

- `reference.jsonl`: one explicit state record per completed tick, with no raw
  structure dumps, pointer values or padding. Fixed-point values retain their
  exact integers.
- `inputs.csv`: resolved digital/analog/shift controls before physics. `playback`
  reads this file at 60 FPS; all cross-cadence runs read the same file.
- `repeat.jsonl`: an independent run of the same scripted scenario. Both repeat
  and playback must match every recorded field.
- `fps-*.jsonl`: playback at 30/60/120/144/240 FPS and variable cadence.
- `*.frames.csv`: CPU timings on the real monotonic clock; virtual test time is
  never used for these measurements.
- `*.log` and `report.json`: scenario checks, binary/trace SHA-256, and the first
  differing tick/field, with a separate first car-state difference.

Use a new output directory for each capture; existing baselines are protected
against overwriting. Keep the same original assets when comparing builds. The
report identifies the probe binary, but does not fingerprint the whole asset
installation. It does not establish equivalence with an original Windows
executable.

To compare a later build against a saved trace:

```sh
python3 tools/stage_baseline.py compare \
    build/baseline-first/reference.jsonl build/baseline-next/reference.jsonl
```

`compare` returns 1 for a difference and 2 for invalid input. `run` fails when
repeat/playback is not identical or loading fails. Cross-cadence differences are
reported as baseline findings; add `--require-cadence` to make them failures.
Use `--ticks` or `--rates` to change the experiment. For direct probe use, set
`XDG_DATA_HOME` to a disposable directory:

```sh
XDG_DATA_HOME=/tmp/cmr2-probe SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
OPENCMR2_HEADLESS=1 build/linux-x86/tests/stage_probe \
    ~/cmr2game /tmp/stage.jsonl 60 500 build/baseline-first/inputs.csv
```

## Coverage and results

The trace covers selected fields of the ordered car: position/history, body
axes, velocities, controls, contact triangles and surfaces, suspension, wheel
loads/slip/torque, steering filter, engine/gear state, body damage grid,
per-part damage values/intensity, checkpoint indices and split times. It also
records the shared MSVC RNG state without advancing it.

It is a regression reference for this scenario, not a complete authoritative
state dump. AI, moving collision objects, every damage/debris node, device input
filters, full replay stream state, ghosts, multiple cars, pause/focus transitions,
resizing and race completion still need broader fixtures. Injecting resolved
controls deliberately bypasses hardware mapping/latency tests.

Validated on 2026-10-09 with the Linux x86 build and local original assets:

| Comparison | Recorded car/control/damage/checkpoint fields | Shared RNG |
| --- | --- | --- |
| 60 FPS repeat and recorded-input playback | Identical for all 500 ticks | Identical |
| 30, 120, 144, 240 FPS and variable cadence | Identical for all 500 ticks | Different from tick 0 |

At 60 FPS the probe also observed RNG changes in 704 race frames that performed
no simulation step; at 240 FPS there were 4304 such frames. These observations
establish frame-dependent use of the shared RNG, but do not identify all callers
or prove a resulting vehicle difference in this short scenario. Audit those
callers and their ownership before separating visual randomness or claiming
full cadence independence. Preserve these traces when implementing that boundary.

`car_scheduler` in CTest calls the existing scheduler directly. It covers ten
seconds at all five cadences and variable cadence, equal-rate car copying,
stored 50 Hz rates, zero elapsed time, multiple steps, the five-step cap and
discarded debt, menu/transition rebasing, and 32-bit millisecond wrap.

## Measure a normal session

```sh
OPENCMR2_PROFILE="$HOME/opencmr2-profile.csv" \
    build/linux-x86/src/opencmr2 --data ~/cmr2game
python3 tools/profile_summary.py ~/opencmr2-profile.csv
```

Close the game normally to write the CSV. Measurements are kept in memory,
capped at one million frames, so no CSV writes occur during live frame updates.
The profiler does not change the clock, controls, RNG or scheduling policy.

The CSV contains frame-start intervals, CPU time inside the frame callback,
total tick CPU time/count, and time in the presentation executor including CPU
submission/waits. Sound-slot servicing and event handling are outside callback
work time but can contribute to the next frame-start interval. Internal flushes
and readbacks remain part of callback/tick work; they are not separately timed.
The columns are observations of nested scopes, not separate budgets to add.

The summary prints median, p95, p99, maximum, frames above the chosen budget
and mean tick cost. Use `--budget-ms 6.944` for a 144 Hz budget and
`--from-frame N` to exclude startup/loading. Frame intervals measure callback
cadence, not actual monitor scanout. Blocking movies can occupy a single
callback with many internal presentations. Startup, menus and races should be
examined separately.

`OPENCMR2_FRAME_HASH=file` writes a hash of the back buffer of every presented
frame, and `OPENCMR2_FRAME_DUMP=dir` saves every 600th frame (or every
`OPENCMR2_FRAME_DUMP_EVERY`) as a BMP. Run `stage_probe` with a real video
driver instead of the null executor to compare renderer changes: the same
scenario must produce the same hashes. Each hashed frame waits for the GPU, so
measure timings in a separate run.

`OPENCMR2_PROBE_TIMELINE=1` writes `<trace>.timeline.csv`: the virtual time of
every presented frame (`P`) and every simulation step (`D` while driving, `T`
otherwise). `OPENCMR2_PROBE_STEER_AT_TICK=N` steers fully left for 12 steps
from driving step N, where the scripted controls steer neither way. With
frame hashes, a race with it and one without give the first frame that shows
the input: at 25 Hz a control is read by the next step, on average 20 ms
after it changes, and that step's frame already shows it.

GPU execution time, actual display intervals and input-to-display latency are
not measured yet. Headless probe timings include trace overhead and no GPU;
use a normal session on the target machine to investigate real stutter. The
CSV path must have an existing parent directory. A crash/forced termination
does not flush the in-memory capture.
