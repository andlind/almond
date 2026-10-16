#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"
pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
port_forward_log="${TMPDIR:-/tmp}/almond-demo-microservice-port-forward-webapp.log"

if ! kubectl get service redis -n microservice >/dev/null 2>&1; then
  echo "Microservice Redis service not found. Run ./01_stage1_k8s.sh first." >&2
  exit 1
fi
kubectl rollout status -n microservice deployment/redis --timeout=180s
if ! kubectl get service webapp -n microservice >/dev/null 2>&1; then
  echo "Web app service not found. Run ./01_stage1_k8s.sh first." >&2
  exit 1
fi

ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/webapp2( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done

kubectl delete service/webapp2 -n microservice --ignore-not-found=true

docker buildx build --platform linux/amd64 --load \
  -t webapp2-k8s-base:local "$demo_dir/webapp2"
docker buildx build --platform linux/amd64 --load \
  --build-arg BASE_IMAGE=webapp2-k8s-base:local \
  -f "$script_dir/overlays/webapp2/Dockerfile" \
  -t webapp2-k8s:local "$script_dir/overlays/webapp2"

kubectl apply -f "$script_dir/06-stage6.yaml"
kubectl rollout status -n microservice deployment/webapp2 --timeout=180s
kubectl rollout status -n microservice deployment/webapp-proxy --timeout=180s

stop_existing_webapp_port_forward() {
  local pids
  pids="$(ps -axo pid=,command= |
    awk '$0 ~ /kubectl/ && $0 ~ /port-forward/ && $0 ~ /(service|svc)\/webapp-access([[:space:]]|$)/ && $0 ~ /8070:8070([[:space:]]|$)/ {print $1}')"

  if [[ -z "$pids" ]]; then
    return
  fi

  while read -r existing_pid; do
    if [[ "$existing_pid" =~ ^[0-9]+$ ]]; then
      kill "$existing_pid"
    fi
  done <<< "$pids"

  for _ in 1 2 3 4 5 6 7 8 9 10; do
    if ! ps -axo pid=,command= |
      awk '$0 ~ /kubectl/ && $0 ~ /port-forward/ && $0 ~ /(service|svc)\/webapp-access([[:space:]]|$)/ && $0 ~ /8070:8070([[:space:]]|$)/ {found=1} END {exit !found}'; then
      return
    fi
    sleep 1
  done

  echo "Timed out stopping existing webapp port-forward processes." >&2
  return 1
}

stop_existing_webapp_port_forward

nohup kubectl -n microservice port-forward service/webapp-access 8070:8070 \
  >"$port_forward_log" 2>&1 </dev/null &
port_forward_pid=$!
printf '%s\n' "$port_forward_pid" >> "$pid_file"

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if ! kill -0 "$port_forward_pid" 2>/dev/null; then
    cat "$port_forward_log" >&2
    echo "Failed to start port-forward for service/webapp-access; see $port_forward_log" >&2
    exit 1
  fi
  if grep -q "Forwarding from 127.0.0.1" "$port_forward_log"; then
    echo "Port-forward ready for service/webapp-access"
    break
  fi
  sleep 1
done

if ! grep -q "Forwarding from 127.0.0.1" "$port_forward_log"; then
  cat "$port_forward_log" >&2
  echo "Timed out starting port-forward for service/webapp-access; see $port_forward_log" >&2
  exit 1
fi

echo "Second web app pod is now behind service/webapp in namespace microservice."
echo "In-cluster clients to service/webapp:8070 can be load-balanced across both app pods."
echo "Load-balanced web app: http://localhost:8070"
