FROM andreaslindell72/almond-monitor:latest

RUN yum update -y && yum install redis perl-Redis perl-fields -y

RUN chmod +x /opt/almond/howru 

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY supervisord.conf /etc/supervisord.conf
COPY apps.conf /etc/supervisord.d/apps.conf
COPY redis.conf /etc/redis/redis.conf
COPY plugins/* /opt/almond/plugins
RUN useradd almond && usermod -a -G almond almond
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 6379
EXPOSE 87

ENTRYPOINT ["/usr/bin/supervisord"]
