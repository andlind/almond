#!/bin/sh
set -eu

/opt/almond/start_almond.sh &
almond_pid=$!
sleep 1
if ! kill -0 "$almond_pid" 2>/dev/null; then
    if wait "$almond_pid"; then
        echo "Almond exited during startup." >&2
        exit 1
    else
        status=$?
        echo "Almond failed during startup with status ${status}." >&2
        exit "$status"
    fi
fi

exec su-exec grafana /run.sh
