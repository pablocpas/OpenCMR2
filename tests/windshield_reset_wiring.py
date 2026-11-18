from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'game/Race.cpp').read_text()
n=s.count('Windshield_ResetAll();')
assert n>=2,'Race.cpp must reset windshield on restart and session start (found %d)'%n
# Replay playback re-accrues damage from the recorded inputs; the live part
# table must reset at the replay-viewing boundary - Race_StartPlayerAndOpponentReplays,
# the race-end fade callback that starts stream playback. It must NOT sit in the
# pre-race/restart transitions (mode 4 is also live knockout; flags 26/27 are race
# configuration) nor in the generic lane init (ghost playback uses it during live
# sessions). Any later live stage rebuilds the table in stage setup.
# anchor on the definition (signature + opening brace), not the prototype
fn=s.split('void Race_StartPlayerAndOpponentReplays(int player)\n{')[1]
assert 'StageTiming_ResetPartDamageForReplay();' in fn, 'replay damage reset must run at the replay-viewing entry'
assert fn.index('StageTiming_ResetPartDamageForReplay();') < fn.index('Replay_SelectAndInitializeLane('), 'reset must precede the first lane init'
assert s.count('StageTiming_ResetPartDamageForReplay();') == 1, 'exactly one reset call site (found %d)'%s.count('StageTiming_ResetPartDamageForReplay();')
so=(Path(__file__).resolve().parents[1]/'game/StageObjects.cpp').read_text()
fn=so.split('int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car)\n{')[1]
assert 'StageTiming_ResetPartDamageForReplay' not in fn, 'generic lane init must stay live-safe (ghost playback passes it)'
st=(Path(__file__).resolve().parents[1]/'game/StageTiming.cpp').read_text()
fn=st.split('void StageTiming_ResetPartDamageForReplay(void)')[1]
assert 'StageTiming_RebuildDamagedPartMeshes(pCar)' in fn, 'replay reset must restore the exterior appearance (meshes, visibility)'
assert 'CarPartDamage_ClearAccumulated(' in fn, 'replay reset must clear flags AND damage values (accrual re-derives flags from values)'
print('ok windshield reset wiring')
