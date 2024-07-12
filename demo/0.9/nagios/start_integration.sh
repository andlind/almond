#!/bin/bash
echo "Starting almond_collector"
nohup ./almond-collector 5795 > /var/log/almond-collector.log 2>&1 &
echo "Starting integration"
nohup /usr/bin/python3 integration.py -u http://localhost:5795 -f /opt/nagios/etc/conf.d/containers.cfg -s 3 > /var/log/almond-nagios-integration.log 2>&1 &
