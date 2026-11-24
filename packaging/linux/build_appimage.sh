#!/usr/bin/env bash
# Build a portable Linux AppImage from a configured and built CMake directory.
#
# Usage:
#   packaging/linux/build_appimage.sh \
#     --build-dir build/linux \
#     --output dist/OpenCMR2-Linux-x86_64.AppImage \
#     [--arch x86_64|aarch64] \
#     [--appimagetool /path/to/appimagetool] \
#     [--runtime-file /path/to/runtime]
set -euo pipefail

APP_ID="io.github.pablocpas.OpenCMR2"
BUILD_DIR=""
OUTPUT=""
ARCH_ARG=""
APPIMAGETOOL=""
RUNTIME_FILE=""

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --output)
            OUTPUT="$2"
            shift 2
            ;;
        --arch)
            ARCH_ARG="$2"
            shift 2
            ;;
        --appimagetool)
            APPIMAGETOOL="$2"
            shift 2
            ;;
        --runtime-file)
            RUNTIME_FILE="$2"
            shift 2
            ;;
        -h|--help)
            sed -n '2,12p' "$0" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

if [ -z "$BUILD_DIR" ] || [ -z "$OUTPUT" ]; then
    echo "Error: --build-dir and --output are required." >&2
    exit 1
fi

if [ -z "$ARCH_ARG" ]; then
    ARCH_ARG="$(uname -m)"
fi
case "$ARCH_ARG" in
    x86_64|amd64) ARCH_ARG="x86_64" ;;
    aarch64|arm64) ARCH_ARG="aarch64" ;;
    *)
        echo "Error: unsupported AppImage architecture '$ARCH_ARG'" >&2
        exit 1
        ;;
esac

if [ -z "$APPIMAGETOOL" ]; then
    APPIMAGETOOL="$(command -v appimagetool || true)"
fi
if [ -z "$APPIMAGETOOL" ] || [ ! -x "$APPIMAGETOOL" ]; then
    echo "Error: appimagetool not found or not executable." >&2
    echo "Pass --appimagetool /path/to/appimagetool-${ARCH_ARG}.AppImage" >&2
    exit 1
fi

APPDIR="$(mktemp -d "${TMPDIR:-/tmp}/opencmr2-appdir.XXXXXX")"
trap 'rm -rf "$APPDIR"' EXIT

DESTDIR="$APPDIR" cmake --install "$BUILD_DIR" --prefix /usr

if [ ! -x "$APPDIR/usr/bin/opencmr2" ]; then
    echo "Error: $APPDIR/usr/bin/opencmr2 was not installed." >&2
    exit 1
fi

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
exec "$HERE/usr/bin/opencmr2" "$@"
EOF
chmod 0755 "$APPDIR/AppRun"

ln -sf "usr/share/applications/${APP_ID}.desktop" "$APPDIR/${APP_ID}.desktop"
ln -sf "usr/share/icons/hicolor/256x256/apps/${APP_ID}.png" "$APPDIR/${APP_ID}.png"
ln -sf "${APP_ID}.png" "$APPDIR/.DirIcon"

mkdir -p "$(dirname "$OUTPUT")"
rm -f "$OUTPUT" "${OUTPUT}.sha256"

TOOL_ARGS=(--no-appstream)
if [ -n "$RUNTIME_FILE" ]; then
    TOOL_ARGS+=(--runtime-file "$RUNTIME_FILE")
fi

# Extract-and-run lets appimagetool execute in containers without FUSE.
ARCH="$ARCH_ARG" APPIMAGE_EXTRACT_AND_RUN="${APPIMAGE_EXTRACT_AND_RUN:-1}" \
    "$APPIMAGETOOL" "${TOOL_ARGS[@]}" "$APPDIR" "$OUTPUT"

chmod 0755 "$OUTPUT"

(
    cd "$(dirname "$OUTPUT")"
    sha256sum "$(basename "$OUTPUT")" > "$(basename "$OUTPUT").sha256"
)

echo "Created $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
