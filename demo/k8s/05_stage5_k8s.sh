#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
log_file="${TMPDIR:-/tmp}/almond-demo-grafana-port-forward.log"

if ! kubectl get service prometheus -n prometheus >/dev/null 2>&1; then
  echo "Prometheus service not found in namespace prometheus. Run ./04_stage4_k8s.sh first." >&2
  exit 1
fi
kubectl rollout status -n prometheus deployment/prometheus --timeout=180s

ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/grafana( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done

kubectl apply -f "$script_dir/05-stage5.yaml"
kubectl create configmap grafana-datasource \
  --from-file=prometheus.yaml="$script_dir/overlays/grafana/prometheus-datasource.yaml" \
  -n grafana \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl rollout restart -n grafana deployment/grafana
kubectl rollout status -n grafana deployment/grafana --timeout=180s

nohup kubectl port-forward -n grafana service/grafana 3000:3000 >"$log_file" 2>&1 </dev/null &
port_forward_pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if ! kill -0 "$port_forward_pid" 2>/dev/null; then
    cat "$log_file" >&2
    echo "Failed to start port-forward for service/grafana; see $log_file" >&2
    exit 1
  fi
  if grep -q "Forwarding from 127.0.0.1" "$log_file"; then
    echo "Grafana: http://localhost:3000"
    exit 0
  fi
  sleep 1
done

cat "$log_file" >&2
echo "Timed out starting port-forward for service/grafana; see $log_file" >&2
exit 1
