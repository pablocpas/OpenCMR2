#!/bin/bash
# OpenCMR2.app launcher. Reproduces `./rebuild.sh --all-mods --run` from the installed app:
# asks once for the user's OWN game folder and car ZIP folder, prepares a private asset pack
# under ~/Library/Application Support/OpenCMR2 (originals are never modified, nothing is bundled
# or uploaded), then starts the game with the same options rebuild.sh uses.
# Test hooks: OPENCMR2_LAUNCHER_DATA / _MODS (a folder or 'none') / _PRESET / _PYTHON_CANDIDATES, OPENCMR2_LAUNCHER_NO_DIALOG=1,
# OPENCMR2_LAUNCHER_DRY_RUN=1 (print the commands, run nothing), OPENCMR2_LAUNCHER_SUPPORT=DIR.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
RES="$HERE/../Resources"
GAME="$HERE/opencmr2-game"
SUPPORT="${OPENCMR2_LAUNCHER_SUPPORT:-$HOME/Library/Application Support/OpenCMR2}"
LOGDIR="${OPENCMR2_LAUNCHER_LOGDIR:-$HOME/Library/Logs/OpenCMR2}"
CONF="$SUPPORT/launcher.conf"
mkdir -p "$SUPPORT" "$LOGDIR"
LOG="$LOGDIR/launcher.log"
say() { printf '%s %s\n' "$(date '+%F %T')" "$*" >> "$LOG"; printf '%s\n' "$*" >&2; }
alert() {   # alert "text": says what happened and offers the log; never a silent exit
    say "ALERT: $1"
    [ "${OPENCMR2_LAUNCHER_NO_DIALOG:-0}" = 1 ] && return 0
    r="$(osascript -e 'on run argv' -e 'button returned of (display dialog (item 1 of argv) with title "OpenCMR2" buttons {"Show Log", "OK"} default button "OK" with icon caution)' -e 'end run' "$1" 2>/dev/null)"
    [ "$r" = "Show Log" ] && open -R "$LOG" >/dev/null 2>&1
    return 0
}
note() { [ "${OPENCMR2_LAUNCHER_NO_DIALOG:-0}" = 1 ] || osascript -e 'on run argv' -e 'display notification (item 1 of argv) with title "OpenCMR2"' -e 'end run' "$1" >/dev/null 2>&1; say "$1"; }
choose() {  # choose "prompt"  -> prints a folder path, or nothing when cancelled
    [ "${OPENCMR2_LAUNCHER_NO_DIALOG:-0}" = 1 ] && return 0
    osascript -e 'on run argv' -e 'POSIX path of (choose folder with prompt (item 1 of argv))' -e 'end run' "$1" 2>/dev/null | sed 's#/$##'
}
getconf() { [ -f "$CONF" ] && sed -n "s/^$1=//p" "$CONF" | tail -1; }
setconf() { { [ -f "$CONF" ] && grep -v "^$1=" "$CONF"; printf '%s=%s\n' "$1" "$2"; } > "$CONF.tmp" && mv "$CONF.tmp" "$CONF"; }

DATA="${OPENCMR2_LAUNCHER_DATA:-$(getconf DATA_DIR)}"
MODS="${OPENCMR2_LAUNCHER_MODS:-$(getconf MODS_DIR)}"
PRESET_EXTRA="${OPENCMR2_LAUNCHER_PRESET:-$(getconf TEST_PRESET)}"

if [ -z "$DATA" ] || [ ! -d "$DATA" ]; then
    DATA="$(choose 'Choose your own Colin McRae Rally 2.0 game folder')"
    [ -n "$DATA" ] || { alert "No game folder chosen. OpenCMR2 needs your own installation; nothing was changed."; exit 1; }
    setconf DATA_DIR "$DATA"
fi
# Car mod ZIPs are optional: without them the preset runs without the extra cars.
if [ -z "$MODS" ]; then
    r="skip"
    if [ "${OPENCMR2_LAUNCHER_NO_DIALOG:-0}" != 1 ]; then
        r="$(osascript -e 'on run argv' -e 'button returned of (display dialog (item 1 of argv) with title "OpenCMR2" buttons {"Skip car mods", "Choose folder..."} default button "Choose folder...")' -e 'end run' 'Optional: do you have your own car mod ZIP files (used by the all-mods preset)? Without them OpenCMR2 runs with the original cars.' 2>/dev/null)"
    fi
    MODS=none
    if [ "$r" = "Choose folder..." ]; then
        m="$(choose 'Choose your own car mod ZIP folder')"; [ -n "$m" ] && MODS="$m"
    fi
    setconf MODS_DIR "$MODS"
fi
[ -d "$DATA" ] || { alert "Game folder not found: $DATA"; exit 1; }
SKIPCARS=0
if [ "$MODS" = none ]; then SKIPCARS=1
elif [ ! -d "$MODS" ]; then say "Car ZIP folder not found ($MODS): running without car mods"; SKIPCARS=1
elif ! find "$MODS" -maxdepth 1 -iname '*.zip' 2>/dev/null | grep -q .; then say "No ZIP files in $MODS: running without car mods"; SKIPCARS=1
fi

# Test preset, the same rule as rebuild.sh --all-mods.
case "$PRESET_EXTRA" in "") PRESET=overdo ;; off) PRESET=off ;; *) PRESET="overdo,$PRESET_EXTRA" ;; esac
FLAGS=()
skipnext=0
while IFS= read -r tok; do
    case "$tok" in ""|"#"*) continue ;; esac
    if [ "$skipnext" = 1 ]; then skipnext=0; continue; fi
    if [ "$SKIPCARS" = 1 ]; then case "$tok" in --cars|--mods-dir) skipnext=1; continue ;; esac; fi
    case "$tok" in "@MODS@") tok="$MODS" ;; "@PRESET@") tok="$PRESET" ;; esac
    FLAGS+=("$tok")
done < "$RES/all-mods.flags"
FLAGS+=(--textures=uprez)

# Python 3.10+ with Pillow and numpy for the asset preparation helpers (not bundled).
pick_python() {
    local cands="${OPENCMR2_LAUNCHER_PYTHON_CANDIDATES:-/opt/homebrew/bin/python3 /usr/local/bin/python3 /Library/Frameworks/Python.framework/Versions/Current/bin/python3 /usr/bin/python3}"
    for p in $cands; do
        [ -n "$p" ] && [ -x "$p" ] || continue
        "$p" -c 'import sys,venv;sys.exit(0 if sys.version_info>=(3,10) else 1)' 2>/dev/null && { printf '%s' "$p"; return 0; }
    done
    return 1
}
VENV="$SUPPORT/launcher-venv"
REQ="$RES/enhancements/scenery-patch/requirements.txt"
stamp="$(shasum -a 256 "$REQ" | cut -c1-16)"
if [ "${OPENCMR2_LAUNCHER_DRY_RUN:-0}" != 1 ]; then
    if [ ! -x "$VENV/bin/python3" ] || [ "$(cat "$VENV/.req-stamp" 2>/dev/null)" != "$stamp" ]; then
        PY="$(pick_python)" || { alert "Python 3.10 or newer is required for the one-time asset preparation and was not found (looked in Homebrew, python.org and /usr/bin). Install Python 3.10+ and open OpenCMR2 again."; exit 1; }
        note "First run: setting up the asset preparation environment (needs internet once)."
        rm -rf "$VENV"
        { "$PY" -m venv "$VENV" && "$VENV/bin/python3" -m pip install --disable-pip-version-check -r "$REQ"; } >> "$LOG" 2>&1 \
            || { rm -rf "$VENV"; alert "Could not set up the Python environment (Pillow/numpy). Check your internet connection."; exit 1; }
        printf '%s' "$stamp" > "$VENV/.req-stamp"
    fi
    export PATH="$VENV/bin:/usr/bin:/bin:/usr/sbin:/sbin"
fi

if [ "${OPENCMR2_LAUNCHER_DRY_RUN:-0}" = 1 ]; then
    printf 'prepare:'; printf ' %q' "$GAME" --data "$DATA" "${FLAGS[@]}" --prepare-only; printf '\n'
    printf 'run:'; printf ' %q' "$GAME" --data "$DATA" "${FLAGS[@]}"; printf '\n'
    exit 0
fi
note "Preparing your private asset pack (first run can take several minutes; originals are not modified)."
"$GAME" --data "$DATA" "${FLAGS[@]}" --prepare-only >> "$LOG" 2>&1 \
    || { alert "Asset preparation failed. Your game files were not changed. Details are in the log."; exit 1; }
say "Launching"
exec "$GAME" --data "$DATA" "${FLAGS[@]}"
