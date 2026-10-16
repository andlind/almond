#!/bin/bash
set -euo pipefail

pid_file="${TMPDIR:-/tmp}/almond-demo-redpanda-port-forwards.pids"
ps -axo pid=,command= |
  awk '$0 ~ /port-forward .*service\/(redpanda|redpanda-console|webapp-access|webapp|webapp2|customapp|redis|howru-api|nagios|prometheus|grafana|quickpizza)( |$)/ {print $1}' |
  while read -r pid; do
    if [[ "$pid" =~ ^[0-9]+$ ]]; then
      kill "$pid"
    fi
  done
rm -f "$pid_file"

if kubectl get deployment/webapp2 -n microservice >/dev/null 2>&1; then
  kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s \
    deployment/webapp2 -n microservice
  if kubectl get deployment/redis -n microservice >/dev/null 2>&1; then
    kubectl exec -n microservice deployment/redis -- rm -f /opt/almond/data/webapp02.json
    echo "Removed stale Almond snapshot for webapp2."
  fi
fi

kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n microservice \
  deployment/redis deployment/customapp deployment/webapp deployment/webapp2 deployment/webapp-proxy \
  service/redis service/customapp service/webapp service/webapp-access service/webapp2 service/redpanda-10
kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n redpanda \
  service/redpanda-headless service/redpanda statefulset/redpanda \
  service/redpanda-console deployment/redpanda-console
kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n howru-proxy \
  service/howru-api deployment/howru-api
kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n nagios \
  service/nagios deployment/nagios configmap/nagios-plugins configmap/nagios-integration
kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n prometheus \
  service/prometheus deployment/prometheus configmap/prometheus-config
kubectl delete --ignore-not-found=true --wait=true --cascade=foreground --timeout=180s -n grafana \
  service/grafana deployment/grafana configmap/grafana-datasource

kubectl delete namespace --ignore-not-found=true --wait=true --timeout=180s \
  grafana howru-proxy microservice nagios prometheus quick-pizza redpanda

cleanup_pod="almond-demo-data-cleanup"
kubectl delete pod "$cleanup_pod" --ignore-not-found=true --wait=true
kubectl apply -f - <<'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: almond-demo-data-cleanup
  namespace: default
spec:
  restartPolicy: Never
  containers:
    - name: cleanup
      image: busybox:1.36
      imagePullPolicy: IfNotPresent
      command: ["/bin/sh", "-c"]
      args:
        - |
          set -eu
          find /almond-data -mindepth 1 -maxdepth 1 -exec rm -rf {} \;
          find /prometheus-data -mindepth 1 -maxdepth 1 -exec rm -rf {} \;
      securityContext:
        runAsUser: 0
      volumeMounts:
        - name: almond-data
          mountPath: /almond-data
        - name: prometheus-data
          mountPath: /prometheus-data
  volumes:
    - name: almond-data
      hostPath:
        path: /var/lib/almond-demo/data
        type: DirectoryOrCreate
    - name: prometheus-data
      hostPath:
        path: /var/lib/almond-demo/prometheus
        type: DirectoryOrCreate
EOF

if ! kubectl wait --for=jsonpath='{.status.phase}'=Succeeded "pod/$cleanup_pod" --timeout=180s; then
  kubectl logs "$cleanup_pod" >&2 || true
  kubectl describe pod "$cleanup_pod" >&2 || true
  kubectl delete pod "$cleanup_pod" --ignore-not-found=true --wait=true
  echo "Failed to remove the persisted Almond and Prometheus data." >&2
  exit 1
fi
kubectl delete pod "$cleanup_pod" --wait=true
echo "Removed persisted Almond and Prometheus data."

remove_image() {
  local image="$1"
  local output

  if ! docker image inspect "$image" >/dev/null 2>&1; then
    return
  fi

  for attempt in 1 2 3 4 5 6; do
    if output="$(docker image rm "$image" 2>&1)"; then
      printf '%s\n' "$output"
      return
    fi
    if (( attempt < 6 )); then
      sleep 5
    fi
  done

  printf '%s\n' "$output" >&2
  echo "Failed to remove image ${image} after waiting for its containers to stop." >&2
  return 1
}

remove_image redisdemo:local
remove_image custapplication:local
remove_image webapp:local
remove_image webapp-proxy-k8s:local
remove_image almond-sidecar:local
remove_image webapp-proxy-almond:local
remove_image custapplication-k8s-base:local
remove_image webapp-k8s-base:local
remove_image howruapi:local
remove_image nagios:local
remove_image my-prometheus-k8s:local
remove_image my-prometheus-k8s-base:local
remove_image grafana-almond-k8s:local
remove_image webapp2-k8s:local
remove_image webapp2-k8s-base:local
remove_image quickpizza-almond:local
