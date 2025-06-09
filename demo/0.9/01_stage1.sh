#!/bin/bash
echo "Starting redis database"
docker buildx build --platform linux/amd64 -t redisdemo redis
nohup docker run --privileged --rm --platform linux/amd64 --net almond-redpanda-quickstart_almond-demo --name redis_demo -d -v ~/almond_demo/data:/opt/almond/data -p 6379:6379 redisdemo
echo "Starting custom (backend) app"
docker buildx build --platform linux/amd64 -t custapplication custapp
nohup docker run --rm --platform linux/amd64 --net almond-redpanda-quickstart_almond-demo --name customapp_demo -d -v ~/almond_demo/data:/opt/almond/data -p 8076:8099 custapplication
echo "Starting webbapplication"
docker buildx build --platform linux/amd64 -t webapp webapp
nohup docker run --rm --platform linux/amd64 --net almond-redpanda-quickstart_almond-demo --name web_demo -d -v ~/almond_demo/data:/opt/almond/data -p 8075:80 webapp
echo "Stage 1 done"
