#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"
log_file="${TMPDIR:-/tmp}/almond-demo-prometheus-port-forward.log"

if ! kubectl get service howru-api -n howru-proxy >/dev/null 2>&1; then
  echo "HowRU API service not found in namespace howru-proxy. Run ./02_stage2_k8s.sh first." >&2
  exit 1
fi
kubectl rollout status -n howru-proxy deployment/howru-api --timeout=180s

ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/prometheus( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done

docker buildx build --platform linux/amd64 --load \
  -t my-prometheus-k8s-base:local "$demo_dir/prometheus"
docker buildx build --platform linux/amd64 --load \
  --build-arg BASE_IMAGE=my-prometheus-k8s-base:local \
  -f "$script_dir/overlays/prometheus/Dockerfile" \
  -t my-prometheus-k8s:local "$script_dir/overlays/prometheus"

kubectl apply -f "$script_dir/04-stage4.yaml"
kubectl create configmap prometheus-config \
  --from-file="$script_dir/overlays/prometheus/prometheus.yml" \
  -n prometheus \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl rollout restart -n prometheus deployment/prometheus
kubectl rollout status -n prometheus deployment/prometheus --timeout=180s

nohup kubectl port-forward -n prometheus service/prometheus 9090:9090 >"$log_file" 2>&1 </dev/null &
port_forward_pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if ! kill -0 "$port_forward_pid" 2>/dev/null; then
    cat "$log_file" >&2
    echo "Failed to start port-forward for service/prometheus; see $log_file" >&2
    exit 1
  fi
  if grep -q "Forwarding from 127.0.0.1" "$log_file"; then
    echo "Prometheus: http://localhost:9090"
    exit 0
  fi
  sleep 1
done

cat "$log_file" >&2
echo "Timed out starting port-forward for service/prometheus; see $log_file" >&2
exit 1
