#!/bin/sh
# /opt/plugins/check_nginx_active.sh
HOST=localhost
PORT=80
URL="/nginx_status"
WARN=50
CRIT=100

# fetch raw body only
BODY=$(curl -sS "http://$HOST:$PORT$URL")
if [ $? -ne 0 ] || [ -z "$BODY" ]; then
  echo "CRITICAL: cannot fetch $URL"
  exit 2
fi

# extract the Active connections number
NUM=$(printf '%s\n' "$BODY" | sed -n 's/^[[:space:]]*Active connections:[[:space:]]*\([0-9][0-9]*\).*$/\1/p')

if [ -z "$NUM" ]; then
  echo "CRITICAL: cannot parse Active connections"
  exit 2
fi

# numeric compare and output with perfdata
if [ "$NUM" -ge "$CRIT" ]; then
  echo "CRITICAL: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
  exit 2
elif [ "$NUM" -ge "$WARN" ]; then
  echo "WARNING: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
  exit 1
else
  echo "OK: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
  exit 0
fi
