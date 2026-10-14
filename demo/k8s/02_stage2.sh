#!/bin/bash
echo "Starting howru-api"
docker buildx build --platform linux/amd64 -t howruapi api 
nohup docker run --rm --platform linux/amd64 --name howru_demo --net almond-redpanda-quickstart_almond-demo -d -v ~/almond_demo/data:/opt/almond/data -p 8085:80 howruapi
echo "Stage 2 done"
