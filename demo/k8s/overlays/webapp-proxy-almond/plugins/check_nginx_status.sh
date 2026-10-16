#!/bin/sh
set -eu

metric=${1:-}
warning=${2:-}
critical=${3:-}

case "$metric" in
  active|reading|writing|waiting)
    if [ -z "$warning" ] || [ -z "$critical" ]; then
      echo "UNKNOWN: warning and critical thresholds are required for $metric"
      exit 3
    fi
    ;;
  requests)
    if [ -n "$warning" ] || [ -n "$critical" ]; then
      echo "UNKNOWN: requests does not accept thresholds"
      exit 3
    fi
    ;;
  *)
    echo "UNKNOWN: expected active, reading, writing, waiting, or requests"
    exit 3
    ;;
esac

if ! status="$(curl -fsS --max-time 2 http://127.0.0.1:8070/nginx_status)"; then
  echo "CRITICAL: cannot fetch NGINX stub status"
  exit 2
fi

case "$metric" in
  active)
    value=$(printf '%s\n' "$status" | awk '/^Active connections:/ { print $3; exit }')
    ;;
  reading|writing|waiting)
    value=$(printf '%s\n' "$status" | awk -v metric="$metric" '
      $1 == "Reading:" { field = metric == "reading" ? 2 : metric == "writing" ? 4 : 6; print $field; exit }
    ')
    ;;
  requests)
    value=$(printf '%s\n' "$status" | awk 'NR == 3 { print $3 }')
    ;;
esac

case "$value" in
  ''|*[!0-9]*)
    echo "CRITICAL: cannot parse NGINX $metric metric"
    exit 2
    ;;
esac

perfdata="nginx_${metric}=${value}"
if [ "$metric" = "requests" ]; then
  echo "OK: NGINX total requests $value | $perfdata"
  exit 0
fi

perfdata="${perfdata};${warning};${critical};0"
if [ "$value" -ge "$critical" ]; then
  echo "CRITICAL: NGINX $metric $value | $perfdata"
  exit 2
elif [ "$value" -ge "$warning" ]; then
  echo "WARNING: NGINX $metric $value | $perfdata"
  exit 1
fi

echo "OK: NGINX $metric $value | $perfdata"
