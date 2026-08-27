#!/bin/sh
# /opt/almond/plugins/check_nginx.sh
# Usage:
#   check_nginx.sh active [WARN] [CRIT]
#   check_nginx.sh access_log [WARN] [CRIT]
#   check_nginx.sh status reading|writing|waiting [WARN] [CRIT]

set -eu

cmd=${1:-}
warn=${2:-}
crit=${3:-}

# defaults
case "$cmd" in
  active) WARN=${warn:-50}; CRIT=${crit:-100} ;;
  access_log) WARN=${warn:-1}; CRIT=${crit:-2} ;;
  status) WARN=${warn:-5}; CRIT=${crit:-10} ;;
  *) echo "UNKNOWN: missing or invalid command"; exit 3 ;;
esac

# helper: numeric compare
ge() {
  [ "$1" -ge "$2" ] 2>/dev/null
}

# active connections (parses /nginx_status body)
if [ "$cmd" = "active" ]; then
  HOST=localhost
  PORT=80
  URL="/nginx_status"
  BODY=$(curl -sS "http://$HOST:$PORT$URL" 2>/dev/null) || {
    echo "CRITICAL: cannot fetch $URL"
    exit 2
  }
  NUM=$(printf '%s\n' "$BODY" | sed -n 's/^[[:space:]]*Active connections:[[:space:]]*\([0-9][0-9]*\).*$/\1/p')
  [ -n "$NUM" ] || { echo "CRITICAL: cannot parse Active connections"; exit 2; }
  if ge "$NUM" "$CRIT"; then
    echo "CRITICAL: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
    exit 2
  elif ge "$NUM" "$WARN"; then
    echo "WARNING: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
    exit 1
  else
    echo "OK: Active connections $NUM | active=${NUM};${WARN};${CRIT};0"
    exit 0
  fi
fi

# access_log: count 5xx responses in access.log
if [ "$cmd" = "access_log" ]; then
  LOG=/var/log/nginx/access.log
  if [ ! -f "$LOG" ]; then
    echo "UNKNOWN: $LOG not found"
    exit 3
  fi
  # Count 5xx status codes (common log format)
  COUNT=$(grep -E '" [5][0-9]{2} ' "$LOG" 2>/dev/null | wc -l | tr -d ' ')
  COUNT=${COUNT:-0}
  if ge "$COUNT" "$CRIT"; then
    echo "CRITICAL: $COUNT 5xx responses in $LOG | errors=${COUNT};${WARN};${CRIT};0"
    exit 2
  elif ge "$COUNT" "$WARN"; then
    echo "WARNING: $COUNT 5xx responses in $LOG | errors=${COUNT};${WARN};${CRIT};0"
    exit 1
  else
    echo "OK: $COUNT 5xx responses in $LOG | errors=${COUNT};${WARN};${CRIT};0"
    exit 0
  fi
fi

# status reading|writing|waiting
if [ "$cmd" = "status" ]; then
  field=${2:-}
  # allow calling: check_nginx.sh status reading WARN CRIT
  if [ -z "$field" ]; then
    echo "UNKNOWN: missing field (reading|writing|waiting)"
    exit 3
  fi
  WARN=${3:-$WARN}
  CRIT=${4:-$CRIT}
  HOST=localhost
  PORT=80
  URL="/nginx_status"
  BODY=$(curl -sS "http://$HOST:$PORT$URL" 2>/dev/null) || {
    echo "CRITICAL: cannot fetch $URL"
    exit 2
  }
  case "$field" in
    reading)
      NUM=$(printf '%s\n' "$BODY" | awk '/Reading:/{print $2; exit}')
      label="Reading"
      perfkey="reading"
      ;;
    writing)
      NUM=$(printf '%s\n' "$BODY" | awk '/Writing:/{print $2; exit}')
      label="Writing"
      perfkey="writing"
      ;;
    waiting)
      NUM=$(printf '%s\n' "$BODY" | awk '/Waiting:/{print $2; exit}')
      label="Waiting"
      perfkey="waiting"
      ;;
    *)
      echo "UNKNOWN: invalid field $field"
      exit 3
      ;;
  esac
  [ -n "$NUM" ] || { echo "CRITICAL: cannot parse $label value"; exit 2; }
  if ge "$NUM" "$CRIT"; then
    echo "CRITICAL: $label $NUM | ${perfkey}=${NUM};${WARN};${CRIT};0"
    exit 2
  elif ge "$NUM" "$WARN"; then
    echo "WARNING: $label $NUM | ${perfkey}=${NUM};${WARN};${CRIT};0"
    exit 1
  else
    echo "OK: $label $NUM | ${perfkey}=${NUM};${WARN};${CRIT};0"
    exit 0
  fi
fi

echo "UNKNOWN: unsupported command"
exit 3
