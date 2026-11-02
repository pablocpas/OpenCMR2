#!/usr/bin/env python3
"""Exercise the real host, arcade ghost, replay and enlarged preliminary map paths."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

SCENARIOS = {
    'saved-game-create': (500, {'OPENCMR2_PROBE_SAVE_SESSION': '1',
                                'OPENCMR2_PROBE_EXPECT_MODE': '2'}),
    'saved-game-load': (500, {'OPENCMR2_PROBE_CURSORS': '88:0,218:1',
                              'OPENCMR2_PROBE_EXPECT_MODE': '2'}),
    'preliminary-map': (500, {'OPENCMR2_PROBE_EXPAND_MAP': '1'}),
    'arcade-time-trial': (500, {'OPENCMR2_PROBE_CURSORS': '0:2,88:2,29:0',
                              'OPENCMR2_PROBE_EXPECT_CARS': '2',
                              'OPENCMR2_PROBE_EXPECT_MODE': '7'}),
    'italy-arcade-championship': (2500, {'OPENCMR2_PROBE_CURSORS': '0:2,88:0',
                                      'OPENCMR2_PROBE_EXPECT_CARS': '6',
                                      'OPENCMR2_PROBE_EXPECT_MODE': '5',
                                      'OPENCMR2_PROBE_HEADLIGHT_CHEAT': '1'}),
    'multiplayer-host': (500, {'OPENCMR2_PROBE_CURSORS': '0:3,33:2,60:0,53:1,61:6,19:2,62:3',
                             'OPENCMR2_PROBE_EXPECT_MODE': '8',
                             'OPENCMR2_PROBE_SESSION_NAME': 'TEST'}),
    'finland-championship-results': (1500, {'OPENCMR2_PROBE_CURSORS': '88:0',
                                           'OPENCMR2_PROBE_EXPECT_MODE': '0',
                                           'OPENCMR2_PROBE_FINISH_AT_TICK': '500'}),
    'finland-results': (1500, {'OPENCMR2_PROBE_CURSORS': '88:1',
                              'OPENCMR2_PROBE_EXPECT_MODE': '1',
                              'OPENCMR2_PROBE_FINISH_AT_TICK': '500'}),
    'recorded-replay': (1000, {'OPENCMR2_PROBE_REPLAY_AT_TICK': '500'}),
}
for mode, cursor, cars in [('single-race', 1, 6), ('time-trial', 2, 2)]:
    for track, country in enumerate(['italy', 'sweden', 'greece', 'australia',
                                      'finland', 'france', 'kenya', 'united-kingdom']):
        SCENARIOS[f'arcade-{mode}-{country}'] = (500, {
            'OPENCMR2_PROBE_CURSORS': f'0:2,88:{cursor},29:1,37:{track}',
            'OPENCMR2_PROBE_EXPECT_CARS': str(cars),
            'OPENCMR2_PROBE_EXPECT_MODE': '6' if cursor == 1 else '7',
            'OPENCMR2_PROBE_EXPECT_ARCADE_TRACK': str(track),
            'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
        })
# The two-car super special stages, where the start table index is negative.
for country, name in [(1, 'greece'), (3, 'sweden'), (5, 'kenya'), (7, 'united-kingdom')]:
    SCENARIOS[f'super-special-{name}'] = (500, {
        'OPENCMR2_PROBE_CURSORS': f'88:2,36:{country},37:10',
        'OPENCMR2_PROBE_EXPECT_CARS': '2',
        'OPENCMR2_PROBE_EXPECT_COUNTRY': str(country),
        'OPENCMR2_PROBE_EXPECT_STAGE': '10',
        'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
    })
# The CPU opponent of each super special, from both start sides (the race
# clock seeds the side), must replay its recorded run to the end: it replays
# controls open loop, so any difference from the x87 single precision the run
# was recorded with sends it off its line within the first lap.
for country, name in [(1, 'greece'), (3, 'sweden'), (5, 'kenya'), (7, 'united-kingdom')]:
    for side, clock in [('a', 100000), ('b', 101000)]:
        SCENARIOS[f'super-special-{name}-cpu-run-{side}'] = (3600, {
            'OPENCMR2_PROBE_CURSORS': f'88:2,29:1,36:{country},37:10',
            'OPENCMR2_PROBE_EXPECT_CARS': '2',
            'OPENCMR2_PROBE_EXPECT_COUNTRY': str(country),
            'OPENCMR2_PROBE_EXPECT_STAGE': '10',
            'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
            'OPENCMR2_PROBE_RACE_CLOCK': str(clock),
            'OPENCMR2_PROBE_OPPONENT': '1',
        })
# The Rally Challenge is a knockout on the Japan super special (country 8).
for side, clock in [('a', 100000), ('b', 101000)]:
    SCENARIOS[f'rally-challenge-japan-cpu-run-{side}'] = (3600, {
        'OPENCMR2_PROBE_CURSORS': '88:4',
        'OPENCMR2_PROBE_EXPECT_CARS': '2',
        'OPENCMR2_PROBE_EXPECT_COUNTRY': '8',
        'OPENCMR2_PROBE_EXPECT_STAGE': '10',
        'OPENCMR2_PROBE_EXPECT_MODE': '4',
        'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
        'OPENCMR2_PROBE_RACE_CLOCK': str(clock),
        'OPENCMR2_PROBE_OPPONENT': '1',
    })
# The results of a Rally Challenge race with a profile that holds stage
# records, then Restart: the challenge's ghost name came from past the records.
SCENARIOS['rally-challenge-results-restart'] = (1500, {
    'OPENCMR2_PROBE_CURSORS': '88:4',
    'OPENCMR2_PROBE_EXPECT_CARS': '2',
    'OPENCMR2_PROBE_EXPECT_COUNTRY': '8',
    'OPENCMR2_PROBE_EXPECT_STAGE': '10',
    'OPENCMR2_PROBE_EXPECT_MODE': '4',
    'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
    'OPENCMR2_PROBE_STAGE_RECORDS': '1',
    'OPENCMR2_PROBE_FINISH_AT_TICK': '400',
    'OPENCMR2_PROBE_RESTART': '1',
})
# The results menu after a Rally Challenge race: every Down press must move
# its cursor (presses read on the opponent's steps alone were lost). At 60 FPS,
# where the original lost about a third of them.
SCENARIOS['rally-challenge-results-menu'] = (1100, {
    'OPENCMR2_PROBE_CURSORS': '88:4',
    'OPENCMR2_PROBE_EXPECT_CARS': '2',
    'OPENCMR2_PROBE_EXPECT_COUNTRY': '8',
    'OPENCMR2_PROBE_EXPECT_STAGE': '10',
    'OPENCMR2_PROBE_EXPECT_MODE': '4',
    'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
    'OPENCMR2_PROBE_FINISH_AT_TICK': '400',
    'OPENCMR2_PROBE_RESULTS_PRESSES': '30',
    'fps': '60',
})
# High display rates must preserve each full press as well. Finish the
# shorter sequence before the next automatic results fade blocks menu input.
_count, _settings = SCENARIOS['rally-challenge-results-menu']
SCENARIOS['rally-challenge-results-menu-144fps'] = (
    _count, dict(_settings, fps='144', OPENCMR2_PROBE_RESULTS_PRESSES='20'))
# The bracket lives in the race state machine, where the frontend's menu
# scaling wrapper does not run. Check its real render path and restoration
# of the physical display size before continuing into a race. The layout is
# the display's shape at 480 lines with the 640 textures (below 1024 pixels
# wide) and at 768 with the 1024 ones, divided by the menu scale.
for label, size, scale, layout in [
        ('native', '640x480', '100', '640x480'),
        ('1080p', '1920x1080', '100', '1365x768'),
        ('ultrawide', '2560x1080', '100', '1820x768'),
        ('larger', '1920x1080', '125', '1092x614'),
        ('legacy', '1920x1080', '0', '1920x1080')]:
    SCENARIOS[f'rally-challenge-layout-{label}'] = (500, {
        'OPENCMR2_PROBE_CURSORS': '88:4',
        'OPENCMR2_PROBE_EXPECT_MODE': '4',
        'OPENCMR2_PROBE_EXPECT_CARS': '2',
        'OPENCMR2_PROBE_EXPECT_COUNTRY': '8',
        'OPENCMR2_PROBE_EXPECT_STAGE': '10',
        'OPENCMR2_PROBE_UNLOCK_TRACKS': '1',
        'OPENCMR2_PROBE_CHALLENGE_LAYOUT': size,
        'menu_scale': scale,
        'challenge_layout': layout,
    })
# Every car of the selection menu (51) with the all-cars cheat; the bonus cars
# index tables past the six championship teams.
for car, name in [(12, 'lancia-integrale'), (20, 'ford-puma'), (21, 'ford-escort')]:
    SCENARIOS[f'time-trial-{name}'] = (500, {
        'OPENCMR2_PROBE_CURSORS': f'0:2,88:2,51:{car}',
        'OPENCMR2_PROBE_EXPECT_CARS': '2',
        'OPENCMR2_PROBE_EXPECT_MODE': '7',
        'OPENCMR2_PROBE_UNLOCK_CARS': '1',
    })
OPPONENT = re.compile(r'opponent tick=(\d+) rate=\S+ playing=(-?\d+) started=\d+ type=\d+ lane=\S+ frame=(\d+)/(-?\d+)')


def verify_cpu_run(log, records):
    # The run keeps feeding controls to a car that is stuck, so check the car:
    # it may stop only at the end of its run (the finish), never for ten
    # seconds while most of the run is still to come.
    samples = [tuple(map(int, match.groups())) for match in OPPONENT.finditer(log)]
    progress = [(tick, frame / length) for tick, playing, frame, length in samples if playing and length > 0]
    if not progress:
        raise ValueError('the CPU run never played')
    def played(tick):
        return max((f for t, f in progress if t <= tick), default=0.0)
    still = 0
    for record in records:
        if record['tick'] < progress[0][0]:
            continue
        car = next(c for c in record['cars'] if c['id'] == 1)
        still = still + 1 if car['speed'] < 8000 else 0
        if still == 250 and played(record['tick']) < 0.95:
            raise ValueError(f'the CPU car stopped at tick {record["tick"] - 249} in sector {car["sector"]} '
                             f'with {played(record["tick"]):.0%} of its run to go')
    best = max(f for _, f in progress)
    if best < 0.95:
        raise ValueError(f'the CPU run reached only {best:.0%}')
    return {'run_played': round(best, 3)}


REPLAY = re.compile(r'probe: replay tick=(\d+) playing=(-?\d+) lane=(-?\d+) frame=(-?\d+) length=(-?\d+)')


def verify_replay(log):
    samples = [tuple(map(int, match.groups())) for match in REPLAY.finditer(log)]
    playback = [s for s in samples if s[0] >= 500 and s[1]]
    finished = [s for s in samples if s[0] > 500 and not s[1]]
    # frame counts compressed control packets, not 25 Hz simulation steps.
    cursors = [(s[2], s[3]) for s in playback]
    if len(set(cursors)) < 3 or cursors != sorted(cursors):
        raise ValueError(f'replay did not advance continuously: {playback}')
    if not finished or finished[0][2] < 1 or finished[0][0] <= playback[-1][0]:
        raise ValueError(f'replay did not finish normally: {samples}')
    return {'distinct_packet_cursors': len(set(cursors)),
            'last_playing_tick': playback[-1][0], 'first_finished_tick': finished[0][0]}


def verify(probe, data, output, cpu_runs=False):
    report = {}
    for name, (count, settings) in SCENARIOS.items():
        # The long CPU run scenarios form their own test.
        if ('-cpu-run-' in name) != cpu_runs:
            continue
        run = output / name
        (run / 'user').mkdir(parents=True)
        # The menus' animations move in pixels, so their screen sets when the
        # races start: the scenarios keep the original's.
        settings = dict(settings)
        menu_scale = settings.pop('menu_scale', '0')
        challenge_layout = settings.pop('challenge_layout', None)
        (run / 'user' / 'opencmr2.ini').write_text(f'[video]\nmenu_scale = {menu_scale}\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('OPENCMR2_PROBE_')}
        env.update(OPENCMR2_HEADLESS='1', SDL_VIDEODRIVER='dummy',
                   SDL_AUDIODRIVER='dummy', XDG_DATA_HOME=str(run / 'user'))
        if name == 'saved-game-load':
            env['XDG_DATA_HOME'] = str(output / 'saved-game-create' / 'user')
        fps = settings.pop('fps', '30')
        env.update(settings)
        log_path = run / 'run.log'
        with log_path.open('w') as log:
            subprocess.run([str(probe), str(data), str(run / 'trace.jsonl'), fps, str(count)],
                           env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
        log = log_path.read_text(encoding='latin-1')
        if not re.search(rf'probe: {count} ticks, [1-9][0-9]* driving ticks, .*; complete', log):
            raise ValueError(f'{name}: no completed race: {log_path}')
        records = [json.loads(line) for line in (run / 'trace.jsonl').read_text().splitlines()]
        if len(records) != count or records[-1]['tick'] != count - 1:
            raise ValueError(f'{name}: incomplete trace')
        report[name] = {'ticks': count, 'driving_ticks': records[-1]['driving_tick']}
        if challenge_layout:
            expected = f'probe: challenge layout {challenge_layout} on {settings["OPENCMR2_PROBE_CHALLENGE_LAYOUT"]}'
            if expected not in log:
                raise ValueError(f'{name}: incorrect bracket scaling; expected {expected}')
            report[name]['menu_layout'] = challenge_layout
        if name == 'saved-game-create' and 'probe: saved garage session' not in log:
            raise ValueError('real garage session was not saved')
        if name == 'saved-game-load' and not all(
                f'probe: menu 218 ({count} entries)' in log for count in (2, 1)):
            raise ValueError('saved session was not selected through the Load menu')
        if name == 'preliminary-map' and 'probe: preliminary map fully expanded' not in log:
            raise ValueError('preliminary map never fully expanded')
        if name in ('finland-results', 'finland-championship-results'):
            match = re.search(r'probe: Finland stage 2 driving ticks=(\d+)', log)
            if not match or int(match[1]) < 100:
                raise ValueError('Finland results did not continue to stage 2')
            report[name]['next_stage_driving_ticks'] = int(match[1])
        if name == 'recorded-replay':
            report[name]['replay'] = verify_replay(log)
        if 'OPENCMR2_PROBE_RESULTS_PRESSES' in settings:
            match = re.search(r'probe: results menu (\d+) presses, (\d+) moves', log)
            if not match or match[1] != settings['OPENCMR2_PROBE_RESULTS_PRESSES'] or match[1] != match[2]:
                raise ValueError(f'{name}: results menu presses lost: {match and match.group(0)}')
            report[name]['results_menu_moves'] = int(match[2])
        if '-cpu-run-' in name:
            report[name]['cpu_run'] = verify_cpu_run(log, records)
        print(f'{name}: PASS ({count} ticks)', flush=True)
    (output / 'acceptance.json').write_text(json.dumps(report, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--cpu-runs', action='store_true',
                        help='run the scenarios whose CPU opponent replays a recorded run instead')
    args = parser.parse_args()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        verify(args.probe.resolve(), args.data.resolve(), args.output.resolve(), args.cpu_runs)
    else:
        with tempfile.TemporaryDirectory(prefix='opencmr2-runtime-') as directory:
            verify(args.probe.resolve(), args.data.resolve(), Path(directory), args.cpu_runs)


if __name__ == '__main__':
    main()
