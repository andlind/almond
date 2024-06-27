#!/bin/bash
echo "Create docker network"
docker network create almond-demo
echo "Starting webbapplication"
docker build -t webapp webapp
nohup docker run --rm --platform linux/amd64 --net almond-demo --name web_demo -d -v ~/almond_demo/data:/opt/almond/data -p 8075:80 webapp
echo "Starting redis database"
docker build -t redisdemo redis
nohup docker run --rm --platform linux/amd64 --net almond-demo --name redis_demo -d -v ~/almond_demo/data:/opt/almond/data -p 6379:6379 redisdemo
echo "Starting custom application"
docker build -t custapplication custapp
nohup docker run --rm --platform linux/amd64 --net almond-demo --name customapp_demo -d -v ~/almond_demo/data:/opt/almond/data -p 8076:80 custapplication
echo "Stage 1 done"
