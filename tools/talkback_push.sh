#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./talkback_push.sh <camera_ip> tone [freq_hz]
  ./talkback_push.sh <camera_ip> file <audio_file>
  ./talkback_push.sh <camera_ip> mic [device]

Examples:
  ./talkback_push.sh 10.0.0.71 tone
  ./talkback_push.sh 10.0.0.71 tone 800
  ./talkback_push.sh 10.0.0.71 file test.wav
  ./talkback_push.sh 10.0.0.71 mic
  ./talkback_push.sh 10.0.0.71 mic ":0"
  ./talkback_push.sh 10.0.0.71 mic "default"

Notes:
  - transport: RTSP over UDP
  - codec: PCMA / G.711 A-law
  - sample rate: 16000 Hz mono
  - target URL: rtsp://<camera_ip>:554/main/streamid=0
  - mic mode:
      macOS uses avfoundation, common device is ":0"
      Linux uses pulse, common device is "default"
EOF
}

if [[ $# -lt 2 ]]; then
  usage
  exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg not found in PATH" >&2
  exit 1
fi

CAMERA_IP="$1"
MODE="$2"
URL="rtsp://${CAMERA_IP}:554/main/streamid=0"
UNAME_S="$(uname -s)"

run_ffmpeg() {
  exec ffmpeg "$@"
}

case "$MODE" in
  tone)
    FREQ="${3:-1000}"
    run_ffmpeg -re \
      -f lavfi -i "sine=frequency=${FREQ}:sample_rate=16000" \
      -af "volume=8" \
      -ac 1 -ar 16000 \
      -c:a pcm_alaw \
      -f rtsp -rtsp_transport udp \
      "$URL"
    ;;
  file)
    if [[ $# -lt 3 ]]; then
      usage
      exit 1
    fi
    INPUT_FILE="$3"
    run_ffmpeg -re -stream_loop -1 \
      -i "$INPUT_FILE" \
      -af "volume=8" \
      -ac 1 -ar 16000 \
      -c:a pcm_alaw \
      -f rtsp -rtsp_transport udp \
      "$URL"
    ;;
  mic)
    if [[ "$UNAME_S" == "Darwin" ]]; then
      DEVICE="${3:-:0}"
      run_ffmpeg \
        -f avfoundation -i "$DEVICE" \
        -af "volume=8" \
        -ac 1 -ar 16000 \
        -c:a pcm_alaw \
        -f rtsp -rtsp_transport udp \
        "$URL"
    else
      DEVICE="${3:-default}"
      run_ffmpeg \
        -f pulse -i "$DEVICE" \
        -af "volume=8" \
        -ac 1 -ar 16000 \
        -c:a pcm_alaw \
        -f rtsp -rtsp_transport udp \
        "$URL"
    fi
    ;;
  *)
    usage
    exit 1
    ;;
esac
