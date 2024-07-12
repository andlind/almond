#!/bin/bash
echo "Start Grafana"
docker run --rm --name almond_grafana --network almond-demo --network-alias grafana --publish 3000:3000 --detach grafana/grafana-oss:latest
