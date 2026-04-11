#!/bin/sh

set -eu

OUT_ROOT="${1:-/mnt/sdcard/hacks}"
STAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${OUT_ROOT}/talk_trace_${STAMP}"

mkdir -p "${OUT_DIR}"

log() {
    echo "[collect_talk] $*"
}

save_cmdline() {
    pid="$1"
    name="$2"
    if [ -n "$pid" ] && [ -r "/proc/$pid/cmdline" ]; then
        tr '\000' ' ' < "/proc/$pid/cmdline" > "${OUT_DIR}/${name}.cmdline.txt" 2>/dev/null || true
        echo >> "${OUT_DIR}/${name}.cmdline.txt"
    fi
}

FETCH_PID="$(pidof fetch_av 2>/dev/null || true)"
MIIO_PID="$(pidof miio_miss 2>/dev/null || true)"

log "output dir: ${OUT_DIR}"
log "fetch_av pid: ${FETCH_PID:-<none>}"
log "miio_miss pid: ${MIIO_PID:-<none>}"

date > "${OUT_DIR}/started_at.txt"
uname -a > "${OUT_DIR}/uname.txt" 2>/dev/null || true
ps > "${OUT_DIR}/ps.txt" 2>/dev/null || true
cat /proc/net/unix > "${OUT_DIR}/proc_net_unix.before.txt" 2>/dev/null || true
ls -l /proc/644/fd > "${OUT_DIR}/fetch_av.fd.before.txt" 2>/dev/null || true

save_cmdline "${FETCH_PID}" "fetch_av"
save_cmdline "${MIIO_PID}" "miio_miss"

if [ -x /mnt/data/bin/fetch_av ]; then
    strings /mnt/data/bin/fetch_av > "${OUT_DIR}/fetch_av.strings.txt" 2>/dev/null || true
fi

if [ -x /mnt/data/bin/miio_miss ]; then
    strings /mnt/data/bin/miio_miss > "${OUT_DIR}/miio_miss.strings.txt" 2>/dev/null || true
fi

PIDS=""

stop_all() {
    log "stopping traces..."
    for p in $PIDS; do
        kill -INT "$p" 2>/dev/null || true
    done
    sleep 1
    for p in $PIDS; do
        kill -TERM "$p" 2>/dev/null || true
    done
    cat /proc/net/unix > "${OUT_DIR}/proc_net_unix.after.txt" 2>/dev/null || true
    ls -l /proc/644/fd > "${OUT_DIR}/fetch_av.fd.after.txt" 2>/dev/null || true
    date > "${OUT_DIR}/stopped_at.txt"
    log "done: ${OUT_DIR}"
}

trap stop_all INT TERM EXIT

if [ -n "${FETCH_PID}" ]; then
    strace -ff -yy -s 256 -xx -tt -o "${OUT_DIR}/fetch_av" \
        -p "${FETCH_PID}" -e trace=recvmsg,sendmsg >/dev/null 2>&1 &
    PIDS="$PIDS $!"

    strace -ff -yy -s 256 -xx -tt -o "${OUT_DIR}/fetch_av_io" \
        -p "${FETCH_PID}" -e trace=connect,accept,read,write >/dev/null 2>&1 &
    PIDS="$PIDS $!"
fi

if [ -n "${MIIO_PID}" ]; then
    strace -ff -yy -s 256 -xx -tt -o "${OUT_DIR}/miio_miss" \
        -p "${MIIO_PID}" -e trace=recvmsg,sendmsg,connect,read,write >/dev/null 2>&1 &
    PIDS="$PIDS $!"
fi

log "tracing started"
log "now open the app, press and hold talk for 3-5 seconds, then press Ctrl+C here"

while :; do
    sleep 1
done
