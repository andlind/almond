#!/bin/bash
mkdir -p /etc/almond
cp etc/almond/* /etc/almond/
mkdir -p /var/log/almond
mkdir -p /opt/almond/api_cmd
cp -r opt/almond/* /opt/almond
chmod -R +x /opt/almond/plugins/*
chmod 777 /var/log/almond
chmod +x /opt/almond/start_almond.sh
