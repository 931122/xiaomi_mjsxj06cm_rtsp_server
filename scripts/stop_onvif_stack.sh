#!/bin/sh

RUN_DIR=${RUN_DIR:-/tmp/mjsxj06cm_onvif}

stop_one()
{
	name=$1
	pid_file=$2

	if [ ! -f "$pid_file" ]; then
		return 0
	fi

	pid=$(cat "$pid_file")
	if kill -0 "$pid" 2>/dev/null; then
		echo "stopping $name: $pid"
		kill "$pid"
	fi
	rm -f "$pid_file"
}

stop_one wsdd "$RUN_DIR/wsdd.pid"
stop_one onvif "$RUN_DIR/onvif.pid"
stop_one rtsp "$RUN_DIR/rtsp.pid"
