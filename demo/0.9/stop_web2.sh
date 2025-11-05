#!/bin/bash
echo "Stopping web_demo2"
docker stop web_demo2
rm -f ~/almond_demo/data/webapp02.json
