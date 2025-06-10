#!/bin/bash
echo "Starting Nagios"
docker buildx build --platform linux/amd64 -t nagios nagios
nohup docker run --rm --platform linux/amd64 --name nagios4 --net almond-redpanda-quickstart_almond-demo -d -v ~/almond_demo/nagios/etc:/opt/nagios/etc -v ~/almond_demo/nagios/plugins:/opt/Custom-Nagios-Plugins --entrypoint "/usr/bin/supervisord" -p 8180:80 nagios
