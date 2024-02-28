#!/bin/bash
echo "Starting integration"
CUR_DIR=$(pwd)
cd /opt/almond
./start_integration.sh
cd $CUR_DIR
/usr/local/bin/start_nagios
