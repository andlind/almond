#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"
log_file="${TMPDIR:-/tmp}/almond-demo-nagios-port-forward.log"

if ! kubectl get service howru-api -n howru-proxy >/dev/null 2>&1; then
  echo "HowRU API service not found in namespace howru-proxy. Run ./02_stage2_k8s.sh first." >&2
  exit 1
fi

ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/nagios( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done

docker buildx build --platform linux/amd64 --load -t nagios:local "$demo_dir/nagios"

kubectl apply -f "$script_dir/03-stage3.yaml"
kubectl create configmap nagios-plugins \
  --from-file="$demo_dir/nagios/plugins" \
  -n nagios \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl create configmap nagios-integration \
  --from-literal=nagiosurls.conf=$'howru-api.howru-proxy.svc.cluster.local:80\n' \
  -n nagios \
  --dry-run=client -o yaml | kubectl apply -f -

kubectl rollout restart -n nagios deployment/nagios
kubectl rollout status -n nagios deployment/nagios --timeout=180s

nohup kubectl port-forward -n nagios service/nagios 8180:80 >"$log_file" 2>&1 </dev/null &
port_forward_pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if ! kill -0 "$port_forward_pid" 2>/dev/null; then
    cat "$log_file" >&2
    echo "Failed to start port-forward for service/nagios; see $log_file" >&2
    exit 1
  fi
  if grep -q "Forwarding from 127.0.0.1" "$log_file"; then
    echo "Nagios: http://localhost:8180"
    exit 0
  fi
  sleep 1
done

cat "$log_file" >&2
echo "Timed out starting port-forward for service/nagios; see $log_file" >&2
exit 1
