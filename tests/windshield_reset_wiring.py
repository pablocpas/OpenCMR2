from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'game/Race.cpp').read_text()
n=s.count('Windshield_ResetAll();')
assert n>=2,'Race.cpp must reset windshield on restart and session start (found %d)'%n
print('ok windshield reset wiring')
