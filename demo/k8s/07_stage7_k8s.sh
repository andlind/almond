#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
log_file="${TMPDIR:-/tmp}/almond-demo-stage7-port-forward.log"

docker buildx build --platform linux/amd64 --load \
  -t quickpizza-almond:local "$script_dir/overlays/almond"

kubectl apply -f "$script_dir/07-stage7.yaml"
kubectl -n quick-pizza rollout status deployment/quickpizza

nohup kubectl -n quick-pizza port-forward svc/quickpizza 3333:3333 \
  >"$log_file" 2>&1 </dev/null &
pid=$!
printf '%s\n' "$pid" >> "$pid_file"

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if ! kill -0 "$pid" 2>/dev/null; then
    cat "$log_file" >&2
    echo "Failed to start QuickPizza port-forward; see $log_file" >&2
    exit 1
  fi
  if grep -q "Forwarding from 127.0.0.1" "$log_file"; then
    echo "QuickPizza port-forward ready"
    break
  fi
  sleep 1
done

if ! grep -q "Forwarding from 127.0.0.1" "$log_file"; then
  cat "$log_file" >&2
  echo "Timed out starting QuickPizza port-forward; see $log_file" >&2
  exit 1
fi

echo "Load-balanced quick-pizza app: http://localhost:3333"
