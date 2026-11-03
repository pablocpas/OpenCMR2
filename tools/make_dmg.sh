#!/bin/sh
# Wrap the macOS release zip (tools/package_release.py) in a versioned DMG and verify it.
# macOS only (hdiutil). The DMG holds the same files as the zip: the executable, SDL3, helper
# scripts and licences. No game assets. Usage: tools/make_dmg.sh ZIP VERSION OUTPUT_DIR
set -eu
zip=$1; version=$2; out=$3
[ -f "$zip" ] || { echo "missing zip: $zip" >&2; exit 1; }
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
unzip -q "$zip" -d "$work/src"
folder=$(ls "$work/src"); [ "$(echo "$folder" | wc -l)" -eq 1 ] || { echo "expected one top folder" >&2; exit 1; }
dmg="$out/OpenCMR2-macOS-arm64-$version.dmg"
[ ! -e "$dmg" ] || { echo "exists: $dmg" >&2; exit 1; }
mkdir -p "$out"
hdiutil create -volname "OpenCMR2 $version" -srcfolder "$work/src/$folder" -fs HFS+ -format UDZO -ov "$dmg"
hdiutil verify "$dmg"
mnt=$(mktemp -d)
hdiutil attach "$dmg" -mountpoint "$mnt" -nobrowse -readonly -quiet
trap 'hdiutil detach "$mnt" -quiet || true; rm -rf "$work" "$mnt"' EXIT
echo "--- DMG contents"; (cd "$mnt/$folder" && find . -type f | sort)
echo "--- binary"; file "$mnt/$folder/opencmr2"; lipo -archs "$mnt/$folder/opencmr2" || true
codesign -dv "$mnt/$folder/opencmr2" 2>&1 || true
otool -L "$mnt/$folder/opencmr2" | head -20
if find "$mnt/$folder" -type f \( -iname '*.bfl' -o -iname '*.bff' -o -iname '*.mp3' -o -iname '*.avi' -o -iname '*.bik' \) | grep -q .; then
  echo "game-asset-like file found in DMG" >&2; exit 1
fi
shasum -a 256 "$dmg" | sed "s#$out/##" > "$dmg.sha256"; cat "$dmg.sha256"
