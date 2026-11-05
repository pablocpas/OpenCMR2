#!/bin/bash
# Build OpenCMR2.app from the macOS release zip (tools/package_release.py), sign it (Developer ID when
# APPLE_SIGNING_IDENTITY is set, otherwise ad-hoc), wrap it in a versioned DMG and verify the DMG. macOS only. The app holds the executable, SDL3, the preparation
# helpers, the launch preset and the licences. No game assets. Usage: tools/make_dmg.sh ZIP VERSION OUT_DIR
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
zip=$1; version=$2; out=$3
[ -f "$zip" ] || { echo "missing zip: $zip" >&2; exit 1; }
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
unzip -q "$zip" -d "$work/src"
folder=$(ls "$work/src"); [ "$(echo "$folder" | wc -l)" -eq 1 ] || { echo "expected one top folder" >&2; exit 1; }
src="$work/src/$folder"
dmg="$out/OpenCMR2-macOS-arm64-$version.dmg"
[ ! -e "$dmg" ] || { echo "exists: $dmg" >&2; exit 1; }
mkdir -p "$out"

stage="$work/stage"; app="$stage/OpenCMR2.app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
cp "$src/opencmr2" "$app/Contents/MacOS/opencmr2-game"
cp "$src"/libSDL3*.dylib "$app/Contents/MacOS/"
cp "$ROOT/tools/macos-app/launcher.sh" "$app/Contents/MacOS/OpenCMR2"
chmod +x "$app/Contents/MacOS/OpenCMR2" "$app/Contents/MacOS/opencmr2-game"
cp -R "$src/enhancements" "$src/Licenses" "$app/Contents/Resources/"
cp "$ROOT/tools/all-mods.flags" "$app/Contents/Resources/all-mods.flags"
minos=$(otool -l "$app/Contents/MacOS/opencmr2-game" | awk '/LC_BUILD_VERSION/{f=1} f&&/minos/{print $2; exit}')
sed -e "s/@VERSION@/$version/g" -e "s/@MINOS@/${minos:-14.0}/" "$ROOT/tools/macos-app/Info.plist.in" > "$app/Contents/Info.plist"
plutil -lint "$app/Contents/Info.plist"
identity="${APPLE_SIGNING_IDENTITY:-}"
if [ -n "$identity" ]; then
  codesign --force --deep --timestamp --sign "$identity" "$app"   # Developer ID, NOT notarized
  signing_note="- The app is Developer ID signed, not notarized: macOS may still warn on first open."
else
  codesign --force --deep --sign - "$app"                         # ad-hoc: valid structure, NOT notarized
  signing_note="- The app is ad-hoc signed, not notarized: macOS will warn on first open."
fi
codesign --verify --deep --strict --verbose=2 "$app"
cat > "$stage/READ ME FIRST.txt" <<TXT
OpenCMR2 $version (macOS, Apple silicon)

Drag OpenCMR2.app to Applications and open it.
- It asks once for the folder of YOUR OWN Colin McRae Rally 2.0 installation and for your own car mod ZIP folder.
  No game files are included here and your originals are never modified.
- It prepares a private asset pack under ~/Library/Application Support/OpenCMR2 (first run can take several minutes).
- Needs Python 3.10 or newer (Homebrew or python.org) once, plus internet once for Pillow and numpy.
$signing_note
- Logs: ~/Library/Logs/OpenCMR2/launcher.log. To be asked for the folders again, delete
  ~/Library/Application Support/OpenCMR2/launcher.conf.
- Gameplay on macOS is not yet verified by the project.
TXT
ln -s /Applications "$stage/Applications"
hdiutil create -volname "OpenCMR2 $version" -srcfolder "$stage" -fs HFS+ -format UDZO -ov "$dmg"
if [ -n "$identity" ]; then
  codesign --force --timestamp --sign "$identity" "$dmg"
  codesign --verify --verbose=2 "$dmg"
fi
hdiutil verify "$dmg"
shasum -a 256 "$dmg" | sed "s#$out/##" > "$dmg.sha256"; cat "$dmg.sha256"
