#!/bin/bash

set -e

echo "Creating ConfigMaps..."
kubectl create configmap almond-plugins --from-file=plugins.conf
kubectl create configmap almond-config --from-file=almond.conf
kubectl create configmap almond-supervisor --from-file=supervisord-override.conf
kubectl create configmap nginx-config --from-file=nginx.conf
kubectl create configmap almond-nginx-scripts \
  --from-file=plugins/check_nginx_active.sh \
  --from-file=plugins/check_nginx.sh \
  --dry-run=client -o yaml | kubectl apply -f -

echo "Applying manifests..."
kubectl apply -f manifest.yaml
kubectl apply -f services.yaml

echo "Waiting for pod to be ready..."

# Wait until the pod is in Ready state
while [[ $(kubectl get pod nginx-almond -o jsonpath='{.status.containerStatuses[*].ready}') != "true true" ]]; do
    echo "Pod not ready yet..."
    sleep 2
done

echo "Pod is ready. Starting port-forward in background..."
nohup kubectl port-forward pod/nginx-almond 8880:80 8885:8085 9890:9090 > portforward.log 2>&1 &
echo "Port-forward running in background. Logs in portforward.log"

echo "Init API..."
for i in {5..1}; do echo -ne "$i... \r"; sleep 1; done; echo "Lift off!"
kubectl exec -it nginx-almond -c almond-monitor -- curl localhost:8085/api/json
echo "Nginx running on localhost:8880"
echo "HowRU API is running on localhost:8885"
echo "Almond API is running on localhost:9890"
echo "Ready to go..."
