#!/bin/bash
echo "Stopping webapp"
docker stop web_demo
echo "Stopping redis"
docker stop redis_demo
echo "Stopping custom app"
docker stop customapp_demo
echo "Stopping howru api"
docker stop howru_demo
echo "Stopping Nagios"
docker stop nagios4
docker rm nagios4
echo "Stopping Prometheus"
docker stop almond_prometheus
echo "Stopping Grafana"
docker stop almond_grafana
echo "Removing docker images"
docker image rm webapp
docker image rm redisdemo
docker image rm custapplication
docker image rm howruapi
docker image rm my-prometheus
docker network rm almond-demo
echo "Clean up metric files"
rm -f data/*.json
rm -f data/metrics/*
echo "Done - demo finished"
