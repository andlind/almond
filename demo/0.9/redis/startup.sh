#!/bin/bash
source ./source.sh
/usr/sbin/apache2 -DBACKGROUND
/usr/bin/supervisord -c /etc/supervisor/supervisord.conf

