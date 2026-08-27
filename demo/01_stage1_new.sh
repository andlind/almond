#!/bin/bash

# Detect the current user's home directory
USER_HOME=$(eval echo "~$USER")
echo "Using home directory: $USER_HOME"

echo "Building Docker images"
docker build -t redisdemo redis
docker build --platform linux/amd64 -t custapplication custapp
docker build --platform linux/amd64 -t webapp webapp

echo "Applying Redis Deployment and Service"
sed "s|YOURUSER_HOME|$USER_HOME|g" redis.yaml.template | kubectl apply -f -
kubectl rollout status deployment/redis-demo

echo "Applying Custom Backend Deployment and Service"
sed "s|YOURUSER_HOME|$USER_HOME|g" backend.yaml.template | kubectl apply -f -
kubectl rollout status deployment/customapp-demo

echo "Applying Webapp Deployment and Service"
sed "s|YOURUSER_HOME|$USER_HOME|g" webapp.yaml.template | kubectl apply -f -
kubectl rollout status deployment/webapp-demo

echo "Stage 1 deployed (microservice)"
