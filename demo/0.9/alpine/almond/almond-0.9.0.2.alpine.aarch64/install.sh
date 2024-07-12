#!/bin/bash
mkdir -p /etc/almond
cp etc/almond/* /etc/almond/
mkdir -p /var/log/almond
mkdir -p /opt/almond
cp -r opt/almond/* /opt/almond
chmod -R +x /opt/almond/plugins/*
chmod +x /opt/almond/start_almond.sh
