#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"
pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
log_prefix="${TMPDIR:-/tmp}/almond-demo-microservice-port-forward"

stop_existing_port_forwards() {
  ps -axo pid=,command= |
    awk '$0 ~ /port-forward .*service\/(webapp-access|webapp|customapp|redis)( |$)/ {print $1}' |
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

docker build -t redisdemo:local "$demo_dir/redis"
docker buildx build --platform linux/amd64 --load \
  -t custapplication-k8s-base:local "$demo_dir/custapp"
docker buildx build --platform linux/amd64 --load \
  --build-arg BASE_IMAGE=custapplication-k8s-base:local \
  -f "$script_dir/overlays/custapp/Dockerfile" \
  -t custapplication:local "$script_dir/overlays/custapp"
docker buildx build --platform linux/amd64 --load \
  -t webapp-k8s-base:local "$demo_dir/webapp"
docker buildx build --platform linux/amd64 --load \
  --build-arg BASE_IMAGE=webapp-k8s-base:local \
  -f "$script_dir/overlays/webapp/Dockerfile" \
  -t webapp:local "$script_dir/overlays/webapp"
docker buildx build --platform linux/arm64 --load \
  -t webapp-proxy-k8s:local "$script_dir/overlays/webapp-proxy"

kubectl rollout status -n redpanda statefulset/redpanda --timeout=180s
kubectl apply -f "$script_dir/01-stage1.yaml"
kubectl rollout restart -n microservice deployment/customapp
kubectl rollout status -n microservice deployment/redis --timeout=180s
kubectl rollout status -n microservice deployment/customapp --timeout=180s
kubectl rollout status -n microservice deployment/webapp --timeout=180s
kubectl rollout status -n microservice deployment/webapp-proxy --timeout=180s

start_port_forward webapp microservice service/webapp-access 8070:8070
start_port_forward customapp microservice service/customapp 8076:8099 9909:9909
start_port_forward redis microservice service/redis 6379:6379

echo "Load-balanced web app: http://localhost:8070"
echo "Custom app:   http://localhost:8076"
echo "Almond API:   localhost:9909"
echo "Redis:        localhost:6379"
