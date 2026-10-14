#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

kubectl rollout status -n redpanda statefulset/redpanda --timeout=180s
kubectl rollout status -n redpanda deployment/redpanda-console --timeout=180s

cd "$script_dir/redpanda/almond"
./install_almond.sh
