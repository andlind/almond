#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
log_prefix="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forward"

stop_existing_port_forwards() {
  ps -axo pid=,command= |
    awk '$0 ~ /port-forward .*service\/(redpanda|redpanda-console)( |$)/ {print $1}' |
    while read -r pid; do
      if [[ "$pid" =~ ^[0-9]+$ ]]; then
        kill "$pid"
      fi
    done
  rm -f "$pid_file"
}

start_port_forward() {
  local name="$1"
  local namespace="$2"
  local service="$3"
  shift 3
  local log_file="${log_prefix}-${name}.log"
  local pid

  nohup kubectl port-forward -n "$namespace" "$service" "$@" >"$log_file" 2>&1 </dev/null &
  pid=$!
  printf '%s\n' "$pid" >> "$pid_file"

  for _ in 1 2 3 4 5 6 7 8 9 10; do
    if ! kill -0 "$pid" 2>/dev/null; then
      cat "$log_file" >&2
      echo "Failed to start port-forward for ${service}; see ${log_file}" >&2
      return 1
    fi
    if grep -q "Forwarding from 127.0.0.1" "$log_file"; then
      echo "Port-forward ready for ${service}"
      return
    fi
    sleep 1
  done

  cat "$log_file" >&2
  echo "Timed out starting port-forward for ${service}; see ${log_file}" >&2
  return 1
}

stop_existing_port_forwards
kubectl apply -f "$script_dir/redpanda/kubernetes.yaml"
kubectl rollout status -n redpanda statefulset/redpanda --timeout=180s
kubectl rollout status -n redpanda deployment/redpanda-console --timeout=180s

start_port_forward redpanda redpanda service/redpanda \
  18081:18081 18082:18082 19092:19092 19644:9644
start_port_forward console redpanda service/redpanda-console 8080:8080

echo "Redpanda Console:  http://localhost:8080"
echo "Kafka:             localhost:19092"
echo "Schema Registry:   http://localhost:18081"
echo "HTTP Proxy:        http://localhost:18082"
echo "Admin API:         http://localhost:19644"
