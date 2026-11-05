import re,sys
s=open('game/Race.cpp').read()
n=s.count('Windshield_ResetAll();')
assert n>=2,'Race.cpp must reset windshield on restart and session start (found %d)'%n
print('ok windshield reset wiring')
