#!/bin/bash
echo "Starting Nagios"
docker build -t nagios nagios
nohup docker run --rm --platform linux/amd64 --name nagios4 -d -v ~/almond_demo/nagios/etc:/opt/nagios/etc -v ~/almond_demo/nagios/plugins:/opt/Custom-Nagios-Plugins --entrypoint "./start.sh" -p 8180:80 nagios
