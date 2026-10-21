#!/bin/bash
set -euo pipefail

pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
ps -axo pid=,command= |
  awk '$0 ~ /port-forward service\/(redpanda|redpanda-console|webapp|customapp|redis)( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done
rm -f "$pid_file"

kubectl delete --ignore-not-found=true -f 01-stage1.yaml
kubectl delete --ignore-not-found=true -f redpanda/kubernetes.yaml
docker image rm redisdemo:local custapplication:local webapp:local
