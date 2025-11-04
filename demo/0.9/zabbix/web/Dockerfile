# zabbix-web/Dockerfile
FROM zabbix/zabbix-web-nginx-mysql:alpine-latest

COPY php-zabbix.conf /etc/php84/php-fpm.d/zabbix.conf
COPY zabbix.conf /etc/nginx/http.d/zabbix.conf
COPY --chmod=755 start.sh /start.sh

#ENTRYPOINT ["/start.sh"]
