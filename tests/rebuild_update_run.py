#!/usr/bin/env python3
"""Real disposable git remotes; fake compiler/game. No native build/network/assets."""
from pathlib import Path
import os, subprocess, tempfile, shutil, json
ROOT=Path(__file__).resolve().parents[1]
def git(p,*a):return subprocess.check_output(['git','-C',str(p),*a],stderr=subprocess.DEVNULL,text=True).strip()
def run(p,*a,env=None):return subprocess.run(['bash',str(p/'rebuild.sh'),*a],capture_output=True,text=True,env=env)
with tempfile.TemporaryDirectory(prefix='rebuild test ') as td:
 t=Path(td);seed=t/'seed';seed.mkdir();git(seed,'init','-b','main');git(seed,'config','user.email','test@example.invalid');git(seed,'config','user.name','Test');shutil.copy(ROOT/'rebuild.sh',seed/'rebuild.sh');(seed/'tools').mkdir();shutil.copy(ROOT/'tools/all-mods.flags',seed/'tools/all-mods.flags');git(seed,'add','.');git(seed,'commit','-qm','initial');remote=t/'remote';subprocess.run(['git','clone','--bare',str(seed),str(remote)],check=True,capture_output=True);repo=t/'checkout';subprocess.run(['git','clone',str(remote),str(repo)],check=True,capture_output=True);git(repo,'config','user.email','test@example.invalid');git(repo,'config','user.name','Test')
 data=t/'owned data';data.mkdir();mods=t/'owned ZIPs';mods.mkdir();log=t/'argv.jsonl';commands=t/'commands';commands.mkdir()
 # Fake build creates binary that records exact argv, including paths with spaces.
 (commands/'sysctl').write_text('#!/bin/sh\necho 2\n')
 (commands/'cmake').write_text('''#!/bin/sh
printf '%s\\n' "$*" >> "$BUILD_LOG"
if [ "$1" = --build ]; then
 mkdir -p build/macos/src
 cat > build/macos/src/opencmr2 <<'GAME'
#!/usr/bin/env python3
import sys,json,os
with open(os.environ['ARGS_LOG'],'a') as f:f.write(json.dumps(sys.argv[1:])+'\\n')
GAME
 chmod +x build/macos/src/opencmr2
fi
''')
 for f in commands.iterdir():f.chmod(0o755)
 env={**os.environ,'PATH':str(commands)+':'+os.environ['PATH'],'CMR2_DATA':str(data),'CMR2_MODS_DIR':str(mods),'ARGS_LOG':str(log),'BUILD_LOG':str(t/'build.log')}
 p=run(repo,'--all-mods','--run','--dry-run',env=env);assert p.returncode==0,p.stderr;assert not log.exists() and not (t/'build.log').exists();assert '--prepare-only' in p.stdout and 'run:' in p.stdout
 # Default unchanged: builds, no preparation or game, no fetch/switch.
 p=run(repo,env=env);assert p.returncode==0,p.stderr;assert not log.exists()
 p=run(repo,'--all-mods','--run',env=env);assert p.returncode==0,p.stderr;calls=[json.loads(x) for x in log.read_text().splitlines()];assert len(calls)==2;assert calls[0][-1]=='--prepare-only';assert calls[0][:-1]==calls[1];assert calls[1][:2]==['--data',str(data)];assert calls[1][calls[1].index('--mods-dir')+1]==str(mods);assert '--textures=uprez' in calls[1] and calls[1].index('--textures=uprez')>calls[1].index('--tunnel-lines')
 # Ignore build output, as real checkout does; untracked/modified input must block update.
 (repo/'.git/info/exclude').write_text('build/\n');(repo/'dirty').write_text('do not lose');before=git(repo,'rev-parse','HEAD');p=run(repo,'--update','--dry-run',env=env);assert p.returncode==2 and 'clean' in p.stderr;assert git(repo,'rev-parse','HEAD')==before;(repo/'dirty').unlink()
 with (repo/'rebuild.sh').open('a') as f:f.write('\n# local edit\n')
 p=run(repo,'--update',env=env);assert p.returncode==2;git(repo,'checkout','--','rebuild.sh')
 # Incoming script marker proves update reexecutes the newly fetched version.
 s=(seed/'rebuild.sh').read_text().replace('set -euo pipefail','set -euo pipefail\necho updated-script-marker',1);(seed/'rebuild.sh').write_text(s);git(seed,'add','.');git(seed,'commit','-qm','incoming');git(seed,'push',str(remote),'main');incoming=git(seed,'rev-parse','HEAD')
 p=run(repo,'--update','--all-mods','--run','--dry-run',env=env);assert p.returncode==0,p.stderr;assert git(repo,'rev-parse','HEAD')==before and 'updated-script-marker' not in p.stdout
 p=run(repo,'--update','--all-mods','--run',env=env);assert p.returncode==0,(p.stdout,p.stderr);assert git(repo,'rev-parse','HEAD')==incoming and 'updated-script-marker' in p.stdout
 # Diverged local main is refused without changing current branch or losing commits.
 git(repo,'checkout','-b','feature');git(repo,'checkout','main');(repo/'local').write_text('preserve');git(repo,'add','.');git(repo,'commit','-qm','local');local=git(repo,'rev-parse','HEAD');git(repo,'checkout','feature');p=run(repo,'--update',env=env);assert p.returncode==2 and 'ahead/diverged' in p.stderr;assert git(repo,'branch','--show-current')=='feature' and git(repo,'rev-parse','main')==local
 # A failed build must never launch.
 (commands/'cmake').write_text('#!/bin/sh\nexit 7\n');size=log.stat().st_size;p=run(repo,'--all-mods','--run',env=env);assert p.returncode==7 and log.stat().st_size==size
print('rebuild update/run: clean default, exact preset, quoting, dry-run, fast-forward/reexec, dirty/diverged protection and fail-stop pass')
