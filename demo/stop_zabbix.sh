#!/bin/bash
echo "Stop zabbix agent"
docker stop zabbix-agent
docker rm zabbix-agent
echo "Stop Zabbix web"
docker stop zabbix-web
docker rm zabbix-web
echo "Stop MySQL"
docker stop mariadb 
docker rm mariadb
echo "Stop Zabbix server"
docker stop zabbix-server
docker rm zabbix-server
echo "Removing docker images"
docker image rm my-zabbix-web
docker image rm my-zabbix-server
echo "Done - demo finished"
