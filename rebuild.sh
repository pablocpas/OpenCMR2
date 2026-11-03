#!/usr/bin/env bash
#
# rebuild.sh - clean rebuild of OpenCMR2 for macOS (Apple silicon), with optional private asset preparation.
#
# No options: wipes build/macos/, reconfigures with the "macos" CMake preset, builds everything (game + tests). Same as before.
#
# Asset options. They run the game binary's own helper with --prepare-only after the build, so the
# private pack is made the same way as at launch: originals are verified by hash and never written, the cache lives under enhancements/ in your
# profile, an old cache is never overwritten, nothing is downloaded (unless you add --install-enhancement-deps), no selection is saved to the INI.
#   --update                    Fast-forward origin/main first (clean checkout only); never resets/stashes
#   --all-mods                  Current audiovisual preset (not every effect on); needs data and owned car ZIPs.
#                               Includes the EXPERIMENTAL Greece1 brush-wall extrusion (--brush-volume 1): visual only,
#                               collision unchanged, extrusion direction not verified natively; only part of the walls change
#                               (offline count on one recovered gre01hi: 19 of 38, the rest stay flat)
#   --run                       Launch after a successful build/preparation
#   --mods-dir DIR              Owned car ZIP folder for --all-mods (or CMR2_MODS_DIR; default mods)
#   --data DIR                  Your owned game data folder (or set CMR2_DATA). Required with any asset option.
#   --trees crossed|shell       3D tree replacements (experimental, three verified tree cards)
#   --spectators animated       Articulated spectator poses
#   --textures original|france1|uprez
#                               Exact-source 2x ground texture filtering (no new detail). uprez is the DEFAULT on
#                               runs that prepare or launch with a data folder configured (--data or CMR2_DATA):
#                               it covers the owned Finland1/France1/Greece1 stages; france1 filters France1 only;
#                               original opts out (unfiltered originals). Bare build-only runs stay build-only.
#   --roads curved              Experimental Ballon curved road
#   --driver-names real         Real driver names (display only)
#   --install-enhancement-deps  Allow uv to set up an isolated Python with Pillow/numpy (network); default is no downloads
#   --assets-only               Skip the rebuild and prepare with the existing binary
#   --test-preset overdo|atmosphere|off
#                               Diagnostic preset for the launch; with --all-mods it is added to the baked overdo
#                               (e.g. --test-preset atmosphere gives overdo,atmosphere: the usual look plus exaggerated fog/motes)
#   --fog-clear N               Near-field fog clear distance 0-200 (0 = off): launch tuning passed straight to the game
#   --fog-cull 0|1              Experimental fog culling past the far draw distance (default 0 = off); silhouettes may pop
#   --fog-spatial 0|1           Fog pockets along the stage route (default 0 = off)
#   --fog-lights 0..200        Fog response to head/rear lights, percent (default 100)
#   --fog-evolve -100..100      Fog trend start to finish: negative clears, positive thickens (default 0)
#   --brush-volume 0|1          Greece1 roadside brush-wall extrusion away from the road (default 0 = original flat quads;
#                               --all-mods sets 1; explicit value wins). Experimental, launch-only passthrough
#   --cockpit-door-extend N     Experimental, default off: extend the Subaru Impreza 555 cockpit driver-door card rearward by N/100 model units (0-80),
#                               to cover the see-through cut edge; matches only the original Subaru interior geometry, other cars are untouched. Native look unverified
#   --windshield-crack-overlay 0|1  Experimental, default off: in cockpits whose interior model has no glass part (the Subaru), redraw the car's exterior
#                               front-glass triangles with the damaged/broken texture, depth tested against the dash. Native look unverified
#   --fog-amount N / --fog-glow N / --air-motes-amount N / --snow-glint N / --snow-material N
#                               Other atmosphere dials (docs/ATMOSPHERE.md), also launch-only passthrough
#   --export-brush-placement    Read-only export of Greece1 roadside wall placement vs the stage route to a NEW JSON
#                               (numbers only; needs --data). Standalone: no build, no launch, no pack, and it does
#                               NOT enable video.brush_volume or any extrusion (needs the temp.sht exporter fix, PR #296, to read real archives). Stops with a reason if the checks fail.
#   --brush-placement-output F  Where to write it (must not exist; default: $TMPDIR or /tmp, gre01-brush-placement-<time>.json)
#   --dry-run                   Check the inputs and print the plan; build nothing, write nothing
#   -h, --help                  This text
# Missing or unsupported inputs stop the script with a message; nothing is skipped silently. Generated packs are derived game art:
# they stay on your Mac and are never committed. Native appearance and speed of every option remain unverified.
set -euo pipefail

cd "$(dirname "$0")"

usage() { sed -n '2,/^set -euo/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; }
die() { echo "rebuild.sh: $*" >&2; exit 2; }

DATA="${CMR2_DATA:-${OPENCMR2_DATA:-}}"
MODS="${CMR2_MODS_DIR:-mods}"
UPDATE=0
ALL_MODS=0
RUN=0
REEXEC_ARGS=()
for arg in "$@"; do [ "$arg" = --update ] || REEXEC_ARGS+=("$arg"); done
DRY=0
TEXTURES_VALUE=
ASSETS_ONLY=0
FLAGS=()
TUNING=()
TEST_PRESET=
EXPORT_BRUSH=0
BRUSH_OUT=
need_value() { [ "$#" -ge 2 ] && [ -n "$2" ] || die "$1 needs a value (see --help)"; }
while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        --update) UPDATE=1; shift ;;
        --all-mods) ALL_MODS=1; shift ;;
        --run) RUN=1; shift ;;
        --mods-dir) need_value "$@"; MODS="$2"; shift 2 ;;
        --data) need_value "$@"; DATA="$2"; shift 2 ;;
        --trees) need_value "$@"; case "$2" in crossed|shell) ;; *) die "--trees takes crossed or shell, got '$2'" ;; esac; FLAGS+=(--trees "$2"); shift 2 ;;
        --spectators) need_value "$@"; [ "$2" = animated ] || die "--spectators takes animated, got '$2'"; FLAGS+=(--spectators "$2"); shift 2 ;;
        --textures) need_value "$@"; case "$2" in original|france1|uprez) ;; *) die "--textures takes original, france1 or uprez, got '$2'" ;; esac; TEXTURES_VALUE="$2"; shift 2 ;;
        --roads) need_value "$@"; [ "$2" = curved ] || die "--roads takes curved, got '$2'"; FLAGS+=(--roads "$2"); shift 2 ;;
        --driver-names) need_value "$@"; [ "$2" = real ] || die "--driver-names takes real, got '$2'"; FLAGS+=(--driver-names "$2"); shift 2 ;;
        --install-enhancement-deps) FLAGS+=(--install-enhancement-deps); shift ;;
        --test-preset) need_value "$@"; TEST_PRESET="$2"; shift 2 ;;
        --assets-only) ASSETS_ONLY=1; shift ;;
        --export-brush-placement) EXPORT_BRUSH=1; shift ;;
        --brush-placement-output) need_value "$@"; BRUSH_OUT="$2"; shift 2 ;;
        --dry-run) DRY=1; shift ;;
        --fog-amount|--fog-glow|--fog-clear|--fog-lights|--air-motes-amount|--fog-evolve|--snow-glint|--snow-material|--cockpit-door-extend) need_value "$@"; TUNING+=("$1" "$2"); shift 2 ;;
        --fog-cull|--fog-spatial|--brush-volume|--windshield-crack-overlay) need_value "$@"; case "$2" in 0|1) ;; *) die "$1 takes 0 or 1, got '$2'" ;; esac; TUNING+=("$1" "$2"); shift 2 ;;
        --fog-amount=*|--fog-glow=*|--fog-clear=*|--fog-lights=*|--air-motes-amount=*|--fog-evolve=*|--snow-glint=*|--snow-material=*|--cockpit-door-extend=*) TUNING+=("${1%%=*}" "${1#*=}"); shift ;;
        --fog-cull=*|--fog-spatial=*|--brush-volume=*|--windshield-crack-overlay=*) case "${1#*=}" in 0|1) ;; *) die "${1%%=*} takes 0 or 1" ;; esac; TUNING+=("${1%%=*}" "${1#*=}"); shift ;;
        *) die "unknown option '$1' (see --help)" ;;
    esac
done

# Update before using preset definitions: re-enter the newly fetched script, without
# --update, so options/launch settings are taken from the same source being built.
if [ "$UPDATE" = 1 ]; then
    [ -z "$(git status --porcelain)" ] || die "--update needs a clean checkout (including untracked files); save your work first"
    if [ "$DRY" = 1 ]; then
        echo "==> Dry run: would fetch origin main, switch to main and fast-forward only; no git changes"
    else
        git fetch origin main
        git show-ref --verify --quiet refs/heads/main || die "local main missing; create it from origin/main first"
        git merge-base --is-ancestor main origin/main || die "local main is ahead/diverged; stopped without switching or resetting"
        git switch main
        git merge --ff-only origin/main
        exec bash ./rebuild.sh "${REEXEC_ARGS[@]+"${REEXEC_ARGS[@]}"}"
    fi
fi
if [ "$ALL_MODS" = 1 ] && [ -n "$TEXTURES_VALUE" ]; then
    # An explicit --textures always wins over the preset's baked uprez:
    # original opts out, france1 narrows to France1, uprez is the same choice.
    ALL_MODS_TEXTURES="$TEXTURES_VALUE"
    TEXTURES_VALUE=
fi
if [ "$ALL_MODS" = 1 ]; then
    [ "${#FLAGS[@]}" -eq 0 ] || die "--all-mods cannot be combined with individual asset/dependency options"
    # Baked diagnostic preset is overdo; --test-preset atmosphere extends it, off replaces it.
    ALL_MODS_PRESET=overdo
    if [ -n "$TEST_PRESET" ]; then
        if [ "$TEST_PRESET" = off ]; then ALL_MODS_PRESET=off; else ALL_MODS_PRESET="overdo,$TEST_PRESET"; fi
    fi
    [ -d "$MODS" ] || die "owned car ZIP folder not found: $MODS (use --mods-dir or CMR2_MODS_DIR)"
    # The preset lives in tools/all-mods.flags (shared with the macOS app launcher).
    FLAGS=()
    while IFS= read -r tok; do
        case "$tok" in ""|"#"*) continue ;; "@MODS@") tok="$MODS" ;; "@PRESET@") tok="$ALL_MODS_PRESET" ;; esac
        FLAGS+=("$tok")
    done < tools/all-mods.flags
    # Textures after the preset: explicit choice wins; default is uprez.
    # "original" must be passed explicitly: the game's own --asset-upgrades
    # sets uprez, and only a later explicit flag overrides it.
    FLAGS+=(--textures="${ALL_MODS_TEXTURES:-uprez}")
fi


# Read-only brush placement export: standalone, never builds, launches or prepares anything.
if [ -n "$BRUSH_OUT" ] && [ "$EXPORT_BRUSH" = 0 ]; then die "--brush-placement-output needs --export-brush-placement"; fi
if [ "$EXPORT_BRUSH" = 1 ]; then
    if [ "$ALL_MODS" = 1 ] || [ "$RUN" = 1 ] || [ "$ASSETS_ONLY" = 1 ] || [ "${#FLAGS[@]}" -gt 0 ] || [ -n "$TEXTURES_VALUE" ] || [ -n "$TEST_PRESET" ] || [ "${#TUNING[@]}" -gt 0 ]; then
        die "--export-brush-placement is standalone; run it without build, launch or asset options"
    fi
    [ -n "$DATA" ] || die "--export-brush-placement needs --data DIR (or CMR2_DATA)"
    [ -d "$DATA" ] || die "game data folder not found: $DATA"
    command -v python3 >/dev/null 2>&1 || die "python3 not found (needed for the exporter)"
    [ -n "$BRUSH_OUT" ] || BRUSH_OUT="${TMPDIR:-/tmp}"
    case "$BRUSH_OUT" in *.json) ;; *) BRUSH_OUT="${BRUSH_OUT%/}/gre01-brush-placement-$(date +%Y%m%d-%H%M%S).json" ;; esac
    [ ! -e "$BRUSH_OUT" ] || die "output already exists: $BRUSH_OUT (choose a new path)"
    case "$(cd "$(dirname "$BRUSH_OUT")" 2>/dev/null && pwd -P)/" in "$(cd "$DATA" && pwd -P)/"*) die "output must be outside the game data folder" ;; esac
    if [ "$DRY" = 1 ]; then
        echo "==> Dry run: nothing is built or written"
        printf "    export (read-only):"; printf " %q" python3 tools/scenery-patch/export-brush-placement.py --data "$DATA" --output "$BRUSH_OUT"; printf "\n"
        exit 0
    fi
    echo "==> Exporting Greece1 brush placement (read-only; does not enable the extrusion or change game files)"
    python3 tools/scenery-patch/export-brush-placement.py --data "$DATA" --output "$BRUSH_OUT" || die "export refused or failed (exit $?); no usable JSON, see the message above"
    echo "==> Wrote $BRUSH_OUT (numbers only). Send that file back for the direction check."
    exit 0
fi

# An explicit non-original choice becomes a real flag here (all-mods handled above).
[ -z "$TEXTURES_VALUE" ] || [ "$TEXTURES_VALUE" = original ] || FLAGS+=(--textures "$TEXTURES_VALUE")

# Default: exact-source uprez ground textures for the owned Finland/France/Greece
# stages, on runs that prepare or launch (--run or an asset option) with a data
# folder to build them from. --textures original (or france1) overrides the
# default. A bare build-only run stays build-only, even with CMR2_DATA set.
if [ "$ALL_MODS" = 0 ] && [ -z "$TEXTURES_VALUE" ]; then
    if [ -n "$DATA" ] && [ -d "$DATA" ] && { [ "$RUN" = 1 ] || [ "${#FLAGS[@]}" -gt 0 ]; }; then
        FLAGS+=(--textures uprez)
        echo "==> Default: texture uprez on (exact-source 2x filter, owned Finland/France/Greece stages; opt out: --textures original)"
    elif [ "$RUN" = 1 ] || [ "${#FLAGS[@]}" -gt 0 ]; then
        echo "==> Note: no --data folder; default texture uprez skipped, build only"
    fi
fi

PREPARE=0
for f in "${FLAGS[@]+"${FLAGS[@]}"}"; do case "$f" in --trees|--spectators|--textures|--roads|--driver-names) PREPARE=1 ;; esac; done
[ "$ALL_MODS" = 0 ] || PREPARE=1
[ "$PREPARE" = 1 ] || { [ "${#FLAGS[@]}" -eq 0 ] || die "--install-enhancement-deps needs an asset option"; [ "$ASSETS_ONLY" = 0 ] || die "--assets-only needs an asset option"; }

# Launch-only passthrough, not an asset option: after the asset-option guard above.
if [ "$ALL_MODS" != 1 ] && [ -n "$TEST_PRESET" ]; then
    FLAGS+=(--test-preset "$TEST_PRESET")
fi
# Launch-only atmosphere tuning (game flags, not asset options): same guard placement as --test-preset,
# so these combine with --all-mods instead of tripping its "no individual options" check.
[ "${#TUNING[@]}" = 0 ] || FLAGS+=("${TUNING[@]}")

BIN=build/macos/src/opencmr2
if [ "$PREPARE" = 1 ]; then
    [ -n "$DATA" ] || die "asset options need --data DIR (or CMR2_DATA)"
    [ -d "$DATA" ] || die "game data folder not found: $DATA"
    if [ "$ALL_MODS" = 0 ]; then
    case " ${FLAGS[*]} " in
        *" --roads "*) n=$(find "$DATA" -iname fra01hi.bfl 2>/dev/null | wc -l | tr -d ' ')
            [ "$n" = 1 ] || die "--roads curved needs exactly one fra01hi.bfl (Ballon) under $DATA, found $n" ;;
    esac
    case " ${FLAGS[*]} " in
        *" --trees "*|*" --spectators "*)
            for b in frahit.bfl fralot.bfl finhit.bfl finlot.bfl; do
                n=$(find "$DATA" -iname "$b" 2>/dev/null | wc -l | tr -d ' ')
                [ "$n" = 1 ] || die "need exactly one $b under $DATA, found $n (trees and spectators are fitted to the original release)"
            done ;;
    esac
    # Textures last: an explicit asset option names its own archive first, then
    # the (possibly defaulted) texture choice checks its sources.
    case " ${FLAGS[*]} " in
        *" --textures france1"*) n=$(find "$DATA" -iname frahit.bfl 2>/dev/null | wc -l | tr -d ' ')
            [ "$n" = 1 ] || die "--textures france1 needs exactly one frahit.bfl under $DATA, found $n" ;;
        *" --textures uprez"*) n=$(find "$DATA" -iname frahit.bfl -o -iname finhit.bfl -o -iname grehit.bfl 2>/dev/null | wc -l | tr -d ' ')
            [ "$n" -ge 1 ] || die "texture uprez needs at least one owned stage archive (frahit/finhit/grehit.bfl) under $DATA, found none (opt out: --textures original)" ;;
    esac
    fi
fi
if [ "$RUN" = 1 ] && [ -n "$DATA" ]; then
    [ -d "$DATA" ] || die "game data folder not found: $DATA"
fi

if [ "$DRY" = 1 ]; then
    echo "==> Dry run: nothing is built or written"
    if [ "$ASSETS_ONLY" = 1 ]; then echo "    skip rebuild (--assets-only), use $BIN"; else echo "    rebuild: rm -rf build/macos; cmake --preset macos; cmake --build --preset macos"; fi
    if [ "$PREPARE" = 1 ]; then echo "    prepare: $BIN --data \"$DATA\" ${FLAGS[*]} --prepare-only"; echo "    data folder checked; --all-mods source hashes and car ZIPs are validated by the game helper during preparation"; else echo "    no asset options: no preparation"; fi
    if [ "$RUN" = 1 ]; then printf "    run:"; printf " %q" "$BIN"; if [ -n "$DATA" ]; then printf " %q" --data "$DATA"; fi; printf " %q" "${FLAGS[@]+"${FLAGS[@]}"}"; printf "\n"; fi
    exit 0
fi

if [ "$ASSETS_ONLY" = 0 ]; then
    echo "==> Removing build/macos/"
    rm -rf build/macos

    echo "==> Configuring (preset: macos)"
    cmake --preset macos

    echo "==> Building"
    cmake --build --preset macos -j"$(sysctl -n hw.ncpu)"
    echo "==> Built: build/macos/src/OpenCMR2"
fi

if [ "$PREPARE" = 1 ]; then
    [ -x "$BIN" ] || die "$BIN not found; run without --assets-only first"
    echo "==> Preparing private asset pack (originals are not modified)"
    "$BIN" --data "$DATA" "${FLAGS[@]}" --prepare-only
    echo "==> Pack prepared. Launch with the same --data and flags to use it."
fi
if [ "$RUN" = 1 ]; then
    [ -x "$BIN" ] || die "$BIN not found"
    echo "==> Launching"
    if [ -n "$DATA" ]; then exec "$BIN" --data "$DATA" "${FLAGS[@]+"${FLAGS[@]}"}"; else exec "$BIN" "${FLAGS[@]+"${FLAGS[@]}"}"; fi
fi
echo "==> Done"
