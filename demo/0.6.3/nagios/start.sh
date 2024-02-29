#!/bin/bash
echo "Starting integration"
CUR_DIR=$(pwd)
cd /opt/almond
./start_integration.sh
cd $CUR_DIR
/usr/local/bin/start_nagios
echo "All started"
while true
do
        echo "Script is running"
        echo "Press [CTRL+C] to stop."
        sleep 10
done
