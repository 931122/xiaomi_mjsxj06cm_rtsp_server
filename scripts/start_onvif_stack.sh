#!/bin/sh

BASE_DIR=$(cd "$(dirname "$0")/.." && pwd)
IFACE=${ONVIF_IFACE:-wlan0}
RTSP_PORT=${RTSP_PORT:-554}
ONVIF_PORT=${ONVIF_PORT:-1000}
RTSP_BIN=${RTSP_BIN:-$BASE_DIR/bin/mi_rtsp_server}
ONVIF_BIN=${ONVIF_BIN:-$BASE_DIR/onvif_srvd/onvif_srvd}
WSDD_BIN=${WSDD_BIN:-$BASE_DIR/onvif_wsdd/onvif_wsdd}
PTZ_BIN=${PTZ_BIN:-$BASE_DIR/onvif_srvd/mi_ptz_control}
MORTOX_LIB=${MORTOX_LIB:-$BASE_DIR/lib/libmortox.so.0.0.0}
MORTOX_SOCKET=${MORTOX_SOCKET:-/tmp/mortox.socket}
RUN_DIR=${RUN_DIR:-/tmp/mjsxj06cm_onvif}
START_RTSP=${START_RTSP:-1}

export LD_LIBRARY_PATH="/mnt/data/lib:$BASE_DIR/lib:${LD_LIBRARY_PATH:-}"

mkdir -p "$RUN_DIR"

start_daemon()
{
	name=$1
	pid_file=$2
	shift 2

	if [ -f "$pid_file" ] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
		echo "$name already running: $(cat "$pid_file")"
		return 0
	fi

	echo "starting $name"
	nohup "$@" > "$RUN_DIR/$name.log" 2>&1 &
	echo $! > "$pid_file"
}

if [ "$START_RTSP" = "1" ]; then
	start_daemon rtsp "$RUN_DIR/rtsp.pid" "$RTSP_BIN" "$RTSP_PORT"
fi

start_daemon onvif "$RUN_DIR/onvif.pid" "$ONVIF_BIN" \
	--no_fork \
	--no_chdir \
	--pid_file "$RUN_DIR/onvif_srvd.pid" \
	--log_file "$RUN_DIR/onvif_srvd.log" \
	--port "$ONVIF_PORT" \
	--ifs "$IFACE" \
	--manufacturer Xiaomi \
	--model MJSXJ06CM \
	--firmware_ver unknown \
	--serial_num MJSXJ06CM \
	--hardware_id MJSXJ06CM \
	--scope onvif://www.onvif.org/type/video_encoder \
	--scope onvif://www.onvif.org/type/audio_encoder \
	--scope onvif://www.onvif.org/type/ptz \
	--scope onvif://www.onvif.org/hardware/MJSXJ06CM \
	--scope onvif://www.onvif.org/name/XiaomiMJSXJ06CM \
	--scope onvif://www.onvif.org/location/China \
	--scope onvif://www.onvif.org/Profile/Streaming \
	--scope onvif://www.onvif.org/Profile/S \
	--scope onvif://www.onvif.org/Profile/T \
	--name main --width 2304 --height 1296 --url "rtsp://%s:$RTSP_PORT/main" --type H264 \
	--name sub --width 640 --height 360 --url "rtsp://%s:$RTSP_PORT/sub" --type H264 \
	--ptz \
	--move_left "$PTZ_BIN left $MORTOX_LIB $MORTOX_SOCKET" \
	--move_right "$PTZ_BIN right $MORTOX_LIB $MORTOX_SOCKET" \
	--move_up "$PTZ_BIN up $MORTOX_LIB $MORTOX_SOCKET" \
	--move_down "$PTZ_BIN down $MORTOX_LIB $MORTOX_SOCKET" \
	--move_stop "$PTZ_BIN stop $MORTOX_LIB $MORTOX_SOCKET"

start_daemon wsdd "$RUN_DIR/wsdd.pid" "$WSDD_BIN" \
	--ifs "$IFACE" \
	--onvif-port "$ONVIF_PORT" \
	--scopes "onvif://www.onvif.org/type/video_encoder onvif://www.onvif.org/type/audio_encoder onvif://www.onvif.org/type/ptz onvif://www.onvif.org/hardware/MJSXJ06CM onvif://www.onvif.org/name/XiaomiMJSXJ06CM onvif://www.onvif.org/location/China onvif://www.onvif.org/Profile/Streaming onvif://www.onvif.org/Profile/S onvif://www.onvif.org/Profile/T"

echo "logs: $RUN_DIR"
