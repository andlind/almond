#!/bin/bash
echo "Starting Nagios"
docker pull jasonrivers/nagios:latest
docker run --name nagios4 --platform linux/amd64 -p 0.0.0.0:8180:80 -d -v ~/almond_demo/nagios/etc:/opt/nagios/etc -v ~/almond_demo/nagios/plugins:/opt/Custom-Nagios-Plugins jasonrivers/nagios:latest
