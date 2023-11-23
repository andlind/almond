#!/bin/bash
echo "Start Grafana"
docker run --rm --name almond_grafana --network almond-demo --network-alias grafana --publish 3000:3000 --detach grafana/grafana-oss:latest
docker cp grafana/Almond_demo-1696941093195.json almond_grafana:/tmp
