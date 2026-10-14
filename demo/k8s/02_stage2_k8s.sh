#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"
pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
log_prefix="${TMPDIR:-/tmp}/almond-demo-stage2-port-forward"

ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/(webapp|customapp|redis|howru-api)( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done

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

docker buildx build --platform linux/amd64 --load -t howruapi:local "$demo_dir/api"

# Apply the shared hostPath to the microservice workloads before starting HowRU.
kubectl apply -f "$script_dir/01-stage1.yaml"
kubectl rollout status -n microservice deployment/redis --timeout=180s
kubectl rollout status -n microservice deployment/customapp --timeout=180s
kubectl rollout status -n microservice deployment/webapp --timeout=180s

kubectl apply -f "$script_dir/02-stage2.yaml"
kubectl rollout status -n howru-proxy deployment/howru-api --timeout=180s

start_port_forward webapp microservice service/webapp 8075:80
start_port_forward customapp microservice service/customapp 8076:8099 9909:9909
start_port_forward redis microservice service/redis 6379:6379
start_port_forward howru-api howru-proxy service/howru-api 8085:80

echo "Web app:      http://localhost:8075"
echo "Custom app:   http://localhost:8076"
echo "Almond API:   localhost:9909"
echo "Redis:        localhost:6379"
echo "HowRU API:    http://localhost:8085"
