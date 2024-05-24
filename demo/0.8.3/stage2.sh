#!/bin/bash
echo "Starting howru-api"
docker build -t howruapi api 
nohup docker run --rm --platform linux/amd64 --name howru_demo -d -v ~/almond_demo/data:/opt/almond/data -p 8080:80 howruapi
echo "Stage 2 done"
