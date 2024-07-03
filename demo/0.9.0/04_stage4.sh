#!/bin/bash
echo "Start prometheus"
docker build -t my-prometheus prometheus
docker run --rm --name almond_prometheus --network almond-demo --network-alias prometheus -d -p 9090:9090 my-prometheus
