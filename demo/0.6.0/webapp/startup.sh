#!/bin/bash
source ./source.sh
echo "Starting supervisord"
/usr/bin/supervisord -c /etc/supervisor/supervisord.conf > /var/log/supervisord.log 2>&1 &
echo "Starting flask"
/usr/bin/nohup /usr/bin/flask run --host=0.0.0.0 --port=80 > /var/log/webapp.log 2>&1 &
echo "All started"
while true
do
        echo "Script is running"
        echo "Press [CTRL+C] to stop."
        sleep 10
done
