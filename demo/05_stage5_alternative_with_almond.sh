#!/bin/bash
echo "Start Grafana"
docker run --rm --name almond_grafana --network almond-demo --network-alias grafana -v ~/almond_demo/data:/opt/almond/data --publish 3000:3000 --detach grafana/grafana-oss:latest
cd grafana/almond
./install.sh
