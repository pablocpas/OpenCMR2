#!/bin/bash
# Verify a built DMG: mount, structure, signature, launch path (no gameplay, no game data).
# Usage: tools/test_macos_app.sh DMG   (macOS only; needs network once for pip)
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
dmg=$1
mnt=$(mktemp -d); t=$(mktemp -d)
trap 'hdiutil detach "$mnt" -quiet || true; rm -rf "$t"' EXIT
hdiutil attach "$dmg" -mountpoint "$mnt" -nobrowse -readonly -quiet
app="$mnt/OpenCMR2.app"
fail() { echo "FAIL: $*" >&2; exit 1; }
echo "--- contents"; (cd "$mnt" && find . -type f | sort | head -80)
[ -d "$app" ] && [ -L "$mnt/Applications" ] || fail "app or Applications link missing"
plutil -lint "$app/Contents/Info.plist"; plutil -p "$app/Contents/Info.plist" | head -20
codesign --verify --deep --strict "$app" || fail "signature"
echo "--- binary"; file "$app/Contents/MacOS/opencmr2-game"; lipo -archs "$app/Contents/MacOS/opencmr2-game"; otool -L "$app/Contents/MacOS/opencmr2-game" | head
if find "$mnt" -type f \( -iname '*.bfl' -o -iname '*.bff' -o -iname '*.mp3' -o -iname '*.avi' -o -iname '*.bik' -o -iname '*.zip' \) | grep -q .; then fail "game-asset-like file in DMG"; fi
cp -R "$app" "$t/OpenCMR2.app"   # run from a writable copy, as after drag-and-drop
A="$t/OpenCMR2.app"
echo "--- game binary starts (help)"; "$A/Contents/MacOS/opencmr2-game" --help 2>&1 | head -5
mkdir -p "$t/data" "$t/mods"; : > "$t/mods/x.zip"
export OPENCMR2_LAUNCHER_SUPPORT="$t/support" OPENCMR2_LAUNCHER_LOGDIR="$t/logs" OPENCMR2_LAUNCHER_NO_DIALOG=1
echo "--- launcher dry run vs rebuild.sh preset"
OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS="$t/mods" OPENCMR2_LAUNCHER_DRY_RUN=1 "$A/Contents/MacOS/OpenCMR2" 2>/dev/null | sed -n 's/^run: [^ ]* //p' > "$t/launcher-run.txt"
bash "$ROOT/rebuild.sh" --data "$t/data" --all-mods --mods-dir "$t/mods" --run --dry-run | sed -n 's/^    run: [^ ]* //p' > "$t/rebuild-run.txt"
[ -s "$t/launcher-run.txt" ] && cmp "$t/launcher-run.txt" "$t/rebuild-run.txt" || { diff "$t/launcher-run.txt" "$t/rebuild-run.txt" || true; fail "launcher flags differ from rebuild.sh --all-mods"; }
echo "launcher flags identical to rebuild.sh --all-mods ($(wc -w < "$t/launcher-run.txt") words)"
echo "--- car ZIPs are optional"
none="$(OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS=none OPENCMR2_LAUNCHER_DRY_RUN=1 "$A/Contents/MacOS/OpenCMR2" 2>/dev/null)"
echo "$none" | grep -q -- "--cars" && fail "--cars present although car mods skipped"
echo "$none" | grep -q -- "--mods-dir" && fail "--mods-dir present although car mods skipped"
echo "$none" | grep -q -- "--brush-volume" || fail "preset lost its other flags when cars skipped"
mkdir "$t/emptymods"
OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS="$t/emptymods" OPENCMR2_LAUNCHER_DRY_RUN=1 "$A/Contents/MacOS/OpenCMR2" 2>/dev/null | grep -q -- "--cars" && fail "empty ZIP folder still passes --cars"
echo "ok: preset runs without car mods (none chosen, or no ZIPs in the folder)"
echo "--- clear failures (log + dialog path, no silent exit)"
fake="$t/oldpython"; printf '#!/bin/sh\nexit 1\n' > "$fake"; chmod +x "$fake"
for cand in /nonexistent "$fake"; do
  set +e
  OPENCMR2_LAUNCHER_SUPPORT="$t/s-py" OPENCMR2_LAUNCHER_LOGDIR="$t/l-py" OPENCMR2_LAUNCHER_PYTHON_CANDIDATES="$cand" OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS=none "$A/Contents/MacOS/OpenCMR2" > /dev/null 2>&1; rc=$?
  set -e
  [ "$rc" -ne 0 ] && grep -q "ALERT: Python 3.10" "$t/l-py/launcher.log" || fail "missing/old Python did not fail clearly ($cand)"
done
echo "ok: missing or too-old Python stops with an explanatory alert and a log line; no Homebrew or account change"
set +e
OPENCMR2_LAUNCHER_SUPPORT="$t/s-pip" OPENCMR2_LAUNCHER_LOGDIR="$t/l-pip" PIP_INDEX_URL=http://127.0.0.1:9/simple PIP_RETRIES=0 PIP_DEFAULT_TIMEOUT=2 OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS=none "$A/Contents/MacOS/OpenCMR2" > /dev/null 2>&1; rc=$?
set -e
[ "$rc" -ne 0 ] && grep -q "ALERT: Could not set up the Python environment" "$t/l-pip/launcher.log" || fail "failed dependency download did not fail clearly"
[ ! -e "$t/s-pip/launcher-venv" ] || fail "half-made venv left behind"
echo "ok: failed dependency download stops with an alert, leaves no half-made environment, and a retry starts clean"
echo "--- visual evidence (best effort; screenshots go to \$OPENCMR2_TEST_SHOTS)"
shots="${OPENCMR2_TEST_SHOTS:-$t/shots}"; mkdir -p "$shots"
open "$mnt" >/dev/null 2>&1 || true; sleep 4; screencapture -x "$shots/01-dmg-finder-window.png" 2>&1 || echo "screencapture unavailable"
osascript -e 'tell application "Finder" to close every window' >/dev/null 2>&1 || true
( OPENCMR2_LAUNCHER_SUPPORT="$t/s-vis1" OPENCMR2_LAUNCHER_LOGDIR="$t/l-vis1" OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS=none OPENCMR2_LAUNCHER_NO_DIALOG=0 OPENCMR2_LAUNCHER_PYTHON_CANDIDATES=/nonexistent "$A/Contents/MacOS/OpenCMR2" >/dev/null 2>&1 & echo $! > "$t/vis1.pid" )
sleep 6; screencapture -x "$shots/02-error-dialog-python-missing.png" 2>&1 || true; pkill osascript 2>/dev/null || true; sleep 1
( OPENCMR2_LAUNCHER_SUPPORT="$t/s-vis2" OPENCMR2_LAUNCHER_LOGDIR="$t/l-vis2" OPENCMR2_LAUNCHER_NO_DIALOG=0 "$A/Contents/MacOS/OpenCMR2" >/dev/null 2>&1 & )
sleep 6; screencapture -x "$shots/03-first-run-game-folder-dialog.png" 2>&1 || true; pkill osascript 2>/dev/null || true; sleep 1
ls -la "$shots"
echo "--- helpers present and importable from the bundle"
R="$A/Contents/Resources/enhancements/scenery-patch"
for f in prepare.py uprez_prepare.py finland1_roads.py france1_prepare.py road_prepare.py convert.py requirements.txt; do [ -f "$R/$f" ] || fail "missing helper $f"; done
echo "--- helper discovery inside the bundle (synthetic placeholder game folder, no real game data)"
fx="$t/fixture"; mkdir -p "$fx/Game/Bigfiles" "$fx/FrontEnd" "$fx/Sounds" "$fx/CountrySpecific" "$fx/SetupRep"
# The game only accepts non-empty files and non-empty folders; the content is a placeholder byte.
for f in Game/Bigfiles/common.bfl FrontEnd/common.bfl Sounds/surface.bfl CountrySpecific/placeholder SetupRep/placeholder; do printf x > "$fx/$f"; done
set +e
OPENCMR2_LAUNCHER_SUPPORT="$t/s-fx" OPENCMR2_LAUNCHER_LOGDIR="$t/l-fx" OPENCMR2_LAUNCHER_DATA="$fx" OPENCMR2_LAUNCHER_MODS=none "$A/Contents/MacOS/OpenCMR2" > /dev/null 2>&1; rc=$?
set -e
echo "fixture launch exit code: $rc"; tail -40 "$t/l-fx/launcher.log"
grep -q "helper missing beside executable" "$t/l-fx/launcher.log" && fail "game did not find the helpers inside the app bundle"
grep -q "Enhancement helper exited\|Scenery preparation failed\|OPENCMR2_PACK=" "$t/l-fx/launcher.log" || fail "no evidence the bundled helper was started"
echo "ok: the game found and started the bundled helper scripts from inside OpenCMR2.app (it rejects placeholder files, as it must)"
echo "--- real launch path with empty data folder (venv setup, helper discovery, expected refusal)"
set +e
OPENCMR2_LAUNCHER_DATA="$t/data" OPENCMR2_LAUNCHER_MODS="$t/mods" "$A/Contents/MacOS/OpenCMR2" > "$t/launch.out" 2>&1; rc=$?
set -e
echo "launcher exit code: $rc"; echo "--- launcher log"; cat "$t/logs/launcher.log" | tail -60
[ -x "$t/support/launcher-venv/bin/python3" ] || fail "venv was not created"
"$t/support/launcher-venv/bin/python3" -c "import PIL,numpy;print('venv has Pillow',PIL.__version__,'numpy',numpy.__version__)" || fail "venv missing Pillow/numpy"
(cd "$R" && "$t/support/launcher-venv/bin/python3" -c "import prepare,uprez_prepare,finland1_roads,france1_prepare,road_prepare,convert;print('helper modules import')") || fail "helper import"
grep -q "helper missing beside executable" "$t/logs/launcher.log" && fail "game did not find helpers inside the app bundle"
[ "$rc" -ne 0 ] || fail "launcher succeeded on an empty data folder (it must refuse)"
echo "OK: launch path checks passed (no gameplay, no game data)"
