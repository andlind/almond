#!/bin/bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/.." && pwd)"

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

echo "Second web app pod is now behind service/webapp in namespace microservice."
echo "In-cluster clients to service/webapp:8070 can be load-balanced across both app pods."
echo "Load-balanced web app: http://localhost:8070"
