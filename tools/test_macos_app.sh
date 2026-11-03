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
echo "--- helpers present and importable from the bundle"
R="$A/Contents/Resources/enhancements/scenery-patch"
for f in prepare.py uprez_prepare.py finland1_roads.py france1_prepare.py road_prepare.py convert.py requirements.txt; do [ -f "$R/$f" ] || fail "missing helper $f"; done
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
