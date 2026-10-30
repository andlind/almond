#!/bin/bash
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
echo "Start Grafana"
docker run --rm --name almond_grafana --network almond-demo --network-alias grafana \
  -v "$script_dir/grafana/provisioning/datasources/prometheus.yaml:/etc/grafana/provisioning/datasources/prometheus.yaml:ro" \
  --publish 3000:3000 --detach grafana/grafana-oss:latest
