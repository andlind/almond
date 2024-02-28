FROM andreaslindell72/almond-monitor:latest

RUN apt-get update && apt-get install -y redis python3 libwww-perl libredis-perl

RUN chmod +x /opt/almond/howru-api 

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
COPY redis.conf /etc/redis/redis.conf
COPY plugins/* /opt/almond/plugins
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 6379
EXPOSE 87

#CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/supervisord.conf"]
ENTRYPOINT ["/usr/bin/supervisord"]
