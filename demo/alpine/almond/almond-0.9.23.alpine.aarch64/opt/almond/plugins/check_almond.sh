#!/bin/bash
SERVICE="almond"
if pgrep -x "$SERVICE" >/dev/null
then
    echo "OK: $SERVICE is running"
    exit 0
else
    echo "CRITICAL: $SERVICE stopped"
    exit 2
fi
