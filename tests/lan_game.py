#!/usr/bin/env python3
"""Independent game processes (two by default, --players up to 8): LAN discovery,
join, ready and stage load.

Accepts native probes or the Wine wrapper used by physics_parity.py. Both
processes use isolated profiles and the same real UDP protocol as LAN play.
"""
import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import tempfile


def verify(host_probe, guest_probe, data, output, players=2, seconds=40):
    roles = ['host'] + ([f'guest{i}' for i in range(1, players)] if players > 2 else ['guest'])
    processes = []
    logs = []
    # Reserve distinct ports until profiles are written.
    sockets = [socket.socket(socket.AF_INET, socket.SOCK_DGRAM) for _ in roles]
    try:
        for s in sockets:
            s.bind(('127.0.0.1', 0))
        ports = [s.getsockname()[1] for s in sockets]
        host_port = ports[0]
        for index, (role, port) in enumerate(zip(roles, ports)):
            probe = host_probe if role == 'host' else guest_probe
            run = output / role
            user = run / 'user'
            user.mkdir(parents=True)
            (user / 'opencmr2.ini').write_text(
                f'[net]\nport={port}\npeers=127.0.0.1:{host_port}\nbroadcast=0\ntailscale=off\n')
            env = {k: v for k, v in os.environ.items() if not k.startswith('OPENCMR2_PROBE_')}
            browser = 1 if role == 'host' else 0
            env.update(OPENCMR2_HEADLESS='1', SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                       XDG_DATA_HOME=str(user), OPENCMR2_PROBE_LAN_ROLE='host' if role == 'host' else 'guest',
                       OPENCMR2_PROBE_LAN_GUESTS=str(players - 1), OPENCMR2_PROBE_LAN_SECONDS=str(seconds),
                       OPENCMR2_PROBE_CURSORS=f'0:3,33:2,60:0,53:{browser},61:6,19:2,62:3',
                       OPENCMR2_PROBE_EXPECT_CARS=str(players), OPENCMR2_PROBE_EXPECT_MODE='8',
                       OPENCMR2_PROBE_SESSION_NAME='LAN TEST')
            logs.append((run / 'run.log').open('w'))
            sockets[index].close()
            processes.append(subprocess.Popen(
                [str(probe), str(data), str(run / 'trace.jsonl'), '30', '500'],
                env=env, stdout=logs[-1], stderr=subprocess.STDOUT))
        for role, process in zip(roles, processes):
            code = process.wait(timeout=seconds + 20 + 15 * players)
            if code:
                raise RuntimeError(f'{role} failed ({code}): {output / role / "run.log"}')
        report = {}
        for role in roles:
            records = [json.loads(line) for line in (output / role / 'trace.jsonl').read_text().splitlines()]
            if len(records) != 500 or records[-1]['driving_tick'] <= 0:
                raise ValueError(f'{role}: no complete {players}-player race')
            # Every other car moves on every machine.
            positions = {}
            for record in records:
                if record['state'] == 8:
                    for car in record['cars']:
                        positions.setdefault(car['id'], set()).add(tuple(car['position']))
            if len(positions) != players:
                raise ValueError(f'{role}: {len(positions)} cars, expected {players}')
            remote = {car: len(seen) for car, seen in positions.items() if car != 0}
            if min(remote.values()) < 50:
                raise ValueError(f'{role}: a remote car did not advance ({remote})')
            report[role] = {'ticks': len(records), 'driving_ticks': records[-1]['driving_tick'],
                            'distinct_remote_positions': remote}
        if 'probe: LAN guest ready' not in (output / 'host/run.log').read_text():
            raise ValueError('host started without receiving guest readiness')
        (output / 'acceptance.json').write_text(json.dumps(report, indent=2) + '\n')
        print(f'LAN game: {players} machines joined and drove 500 ticks with {players} cars')
    finally:
        for process in processes:
            if process.poll() is None:
                process.kill()
            process.wait()
        for log in logs:
            log.close()
        for s in sockets:
            s.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-probe', type=Path, required=True)
    parser.add_argument('--guest-probe', type=Path)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--players', type=int, default=2, choices=range(2, 9))
    parser.add_argument('--seconds', type=int, default=40, help='wall time per process (more under the sanitizers)')
    args = parser.parse_args()
    host = args.host_probe.resolve()
    guest = (args.guest_probe or args.host_probe).resolve()
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        verify(host, guest, args.data.resolve(), args.output.resolve(), args.players, args.seconds)
    else:
        with tempfile.TemporaryDirectory(prefix='opencmr2-lan-') as directory:
            verify(host, guest, args.data.resolve(), Path(directory), args.players, args.seconds)


if __name__ == '__main__':
    main()
