#!/bin/bash
echo "Start Zabbix server"
docker build -t my-zabbix-server zabbix/server
docker run -d --name zabbix-server \
  -e ZBX_TIMEOUT=30 \
  --network almond-demo \
  -e DB_SERVER_HOST=mariadb \
  -e MYSQL_DATABASE=zabbix \
  -e MYSQL_USER=zabbix \
  -e MYSQL_PASSWORD=zabbix \
  -p 10051:10051 \
  my-zabbix-server
echo "Start MySQL server"
docker run -d --name mariadb \
  --network almond-demo \
  -e MARIADB_ROOT_PASSWORD=root \
  -e MARIADB_DATABASE=zabbix \
  -e MARIADB_USER=zabbix \
  -e MARIADB_PASSWORD=zabbix \
  -v zabbix-mariadb-data:/var/lib/mysql \
  mariadb:10.5
echo "Start Zabbix web"
docker build -t my-zabbix-web zabbix/web
docker run -d --name zabbix-web \
  --network almond-demo \
  -e DB_SERVER_HOST=mariadb \
  -e DB_SERVER_TYPE=MYSQL \
  -e MYSQL_DATABASE=zabbix \
  -e MYSQL_USER=zabbix \
  -e MYSQL_PASSWORD=zabbix \
  -e PHP_TZ=Europe/Stockholm \
  -e ZBX_SERVER_HOST=zabbix-server \
  -e ZBX_SERVER_PORT=10051 \
  -p 8888:8888 \
  my-zabbix-web
echo "Start Zabbix agent"
docker run -d --name zabbix-agent \
  --network almond-demo \
  -e ZBX_SERVER_HOST=zabbix-server \
  -e HOSTNAME=agent-host \
  zabbix/zabbix-agent:alpine-latest
echo "Start the integration"
docker exec -u root zabbix-server python3 /opt/almond/integration/integration.py > /dev/null 2>&1 &

