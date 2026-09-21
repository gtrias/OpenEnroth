#!/usr/bin/env bash
#
# Vita development loop for the OpenEnroth PS Vita port.
#
# Vita is a fullscreen-only, SDL3-with-vitaGL target. Installing a VPK is only needed when packaging changes - for
# code changes it's enough to replace the installed eboot.bin over VitaShell's FTP server.
#
# Usage:
#   scripts/vita.sh build     # compile, and produce build-vita-debug/vpk/eboot.bin
#   scripts/vita.sh deploy    # upload eboot.bin to ux0:/app/<TITLE_ID>/eboot.bin and verify the transfer
#   scripts/vita.sh fetch     # download the newest log & crash dump, and symbolize the dump against the debug ELF
#   scripts/vita.sh           # build + deploy
#
# Environment overrides: VITA_HOST (host:port), VITASDK, BUILD_DIR, TITLE_ID, VITA_CORE_DUMP.

set -euo pipefail

VITA_HOST="${VITA_HOST:-192.168.1.31:1337}"
VITASDK="${VITASDK:-$HOME/vitasdk/vitasdk}"
BUILD_DIR="${BUILD_DIR:-build-vita-debug}"
TITLE_ID="${TITLE_ID:-OPEN00001}"
VITA_CORE_DUMP="${VITA_CORE_DUMP:-$(command -v vita-core-dump || echo /tmp/vita-core-dump/build/vita-core-dump)}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_PATH="$REPO_ROOT/$BUILD_DIR"
DEBUG_ELF="$BUILD_PATH/src/Bin/OpenEnroth/OpenEnroth"
EBOOT="$BUILD_PATH/vpk/eboot.bin"

# True when cmake has to run again. Both the build-system files and the resource file list (resources are globbed at
# configure time, so a new shader only shows up after a reconfigure) are tracked by content hash - cmake leaves
# CMakeCache.txt untouched when nothing changes, so comparing timestamps would reconfigure on every single build.
configure_needed() {
    [ -f "$BUILD_PATH/CMakeCache.txt" ] || return 0

    local stamp="$BUILD_PATH/.cmake_inputs.stamp" hash
    hash="$(
        cd "$REPO_ROOT" || exit 1
        {
            find . -path "./$BUILD_DIR" -prune -o \( -name 'CMakeLists.txt' -o -name '*.cmake' \) -print0 | LC_ALL=C sort -z | xargs -0 cat
            find resources -type f -print | LC_ALL=C sort
        } | sha256sum
    )"

    if [ -f "$stamp" ] && [ "$hash" = "$(cat "$stamp")" ]; then
        return 1
    fi

    printf '%s' "$hash" > "$stamp"
    return 0
}

# VitaShell's FTP server is picky about EPSV, so always speak plain passive FTP to it.
curl_vita() {
    curl --disable-epsv --ftp-pasv -fsS "$@"
}

log() {
    printf '\n=== %s\n' "$*"
}

build() {
    # CMake regenerates the embedded-resource blob on every configure, and rebuilding that blob takes minutes - so a
    # reconfigure only runs when something it depends on actually changed.
    if configure_needed; then
        log "Reconfiguring $BUILD_DIR"
        cmake -S "$REPO_ROOT" -B "$BUILD_PATH" >/dev/null
    fi

    # cmake records per-target compile flags in flags.make, and make doesn't treat them as dependencies - so after a
    # compile definition or flag changes, the affected objects would silently stay stale. Drop the objects in that
    # case and let them be rebuilt.
    local stamp="$BUILD_PATH/.compile_flags.stamp" hash
    hash="$(find "$BUILD_PATH" -name flags.make -exec cat {} + | sha256sum)"
    if [ -f "$stamp" ] && [ "$hash" != "$(cat "$stamp")" ]; then
        log "Compile flags changed, discarding stale object files"
        find "$BUILD_PATH" -name '*.obj' -delete
    fi
    printf '%s' "$hash" > "$stamp"

    log "Building $BUILD_DIR"
    make -C "$BUILD_PATH" -j"$(nproc)" OpenEnroth || return 1

    log "Packaging eboot.bin"
    mkdir -p "$(dirname "$EBOOT")"
    "$VITASDK/bin/vita-elf-create" "$DEBUG_ELF" "$BUILD_PATH/eboot.velf" || return 1
    "$VITASDK/bin/vita-make-fself" "$BUILD_PATH/eboot.velf" "$EBOOT" || return 1
    ls -l "$EBOOT"
}

deploy() {
    log "Uploading eboot.bin to ux0:/app/$TITLE_ID/"
    curl_vita -T "$EBOOT" "ftp://$VITA_HOST/ux0:/app/$TITLE_ID/eboot.bin"

    log "Verifying remote eboot.bin"
    local local_size remote_size
    local_size="$(stat -c %s "$EBOOT")"
    remote_size="$(curl_vita -I "ftp://$VITA_HOST/ux0:/app/$TITLE_ID/eboot.bin" | tr -d '\r' | sed -n 's/^Content-Length: //p')"
    if [ "$local_size" != "$remote_size" ]; then
        echo "Size mismatch: local $local_size, remote $remote_size" >&2
        exit 1
    fi
    echo "OK: $remote_size bytes"
    printf '\nLaunch OpenEnroth on the Vita, then run: %s fetch\n' "$0"
}

newest_remote_line() {
    # $1 - remote directory, $2 - sed address pattern matching the LIST lines to consider.
    # Note the sort by file name: both logs and crash dumps carry a timestamp in their name, which is monotonic -
    # unlike LIST dates, which flip to a year format for files older than six months and break date sorting.
    curl_vita "ftp://$VITA_HOST$1" | sed -n "$2" | sort -k9 | tail -1
}

# LIST lines look like "-rw-r--r-- 1 vita vita <size> <Mon DD HH:MM> <name>", so the timestamp is fields 6-8.
remote_timestamp() {
    printf '%s' "$1" | awk '{ print $6, $7, $8 }'
}

remote_name() {
    printf '%s' "$1" | awk '{ print $NF }'
}

fetch() {
    local out_dir log_line log_name log_ts eboot_line eboot_ts dump_line dump_name
    out_dir="$BUILD_PATH/diagnostics"
    mkdir -p "$out_dir"

    log "Fetching newest log"
    log_line="$(newest_remote_line '/ux0:/data/OpenEnroth/logs/' '/openenroth_.*\.log$/p')"
    log_ts="$(remote_timestamp "$log_line")"
    log_name="$(remote_name "$log_line")"

    # Guard against the most common cause of a confusing debug loop: the Vita resumes a running app instead of
    # restarting it, so the log predates the freshly deployed binary and tests a build that isn't actually running.
    eboot_line="$(newest_remote_line "/ux0:/app/$TITLE_ID/" '/eboot\.bin$/p')"
    eboot_ts="$(remote_timestamp "$eboot_line")"
    if [ -n "$log_ts" ] && [ -n "$eboot_ts" ] &&
       [ "$(printf '%s\n%s\n' "$log_ts" "$eboot_ts" | sort -M | head -1)" = "$log_ts" ]; then
        echo "warning: the newest log is from $log_ts, the deployed eboot.bin is from $eboot_ts - the app looks like" >&2
        echo "         it wasn't restarted since the last deploy. Close it from the LiveArea, launch it again, and" >&2
        echo "         only then run fetch." >&2
    fi

    if [ -n "$log_name" ]; then
        curl_vita -o "$out_dir/$log_name" "ftp://$VITA_HOST/ux0:/data/OpenEnroth/logs/$log_name"
        echo "--- $log_name ($log_ts)"
        cat "$out_dir/$log_name"
    else
        echo "No log found." >&2
    fi

    log "Fetching newest crash dump"
    dump_line="$(newest_remote_line '/ux0:/data/' '/psp2core-[^ ]*\.psp2dmp$/p')"
    dump_name="$(remote_name "$dump_line")"
    if [ -z "$dump_name" ]; then
        echo "No crash dump found."
        return
    fi
    curl_vita -o "$out_dir/$dump_name" "ftp://$VITA_HOST/ux0:/data/$dump_name"
    echo "--- $dump_name ($(remote_timestamp "$dump_line"))"

    if [ ! -x "$VITA_CORE_DUMP" ]; then
        echo "vita-core-dump not found at $VITA_CORE_DUMP, skipping symbolization." >&2
        return
    fi
    # vita-core-dump writes a decompressed copy of the dump next to the working directory, so run it in diagnostics/.
    (cd "$out_dir" && "$VITA_CORE_DUMP" "$dump_name" backtrace --add-elf="$DEBUG_ELF" --show-registers) || true
}

case "${1:-all}" in
    build)     build ;;
    deploy)    deploy ;;
    fetch)     fetch ;;
    configure) log "Reconfiguring $BUILD_DIR" && cmake -S "$REPO_ROOT" -B "$BUILD_PATH" ;;
    # Note the plain `;`: bash suppresses errexit for a function called from an `&&` list, which would make a failed
    # build silently package and deploy the previous binary.
    all)       build; deploy ;;
    *)         echo "Usage: $0 [build|deploy|fetch|configure]" >&2 && exit 2 ;;
esac
