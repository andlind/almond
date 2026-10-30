#!/bin/bash
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
echo "Start Grafana"
docker run --rm --name almond_grafana --network almond-demo --network-alias grafana \
  -v ~/almond_demo/data:/opt/almond/data \
  -v "$script_dir/grafana/provisioning/datasources/prometheus.yaml:/etc/grafana/provisioning/datasources/prometheus.yaml:ro" \
  --publish 3000:3000 --detach grafana/grafana-oss:latest
cd "$script_dir/grafana/almond"
./install.sh
