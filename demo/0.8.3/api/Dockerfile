FROM andreaslindell72/almond-monitor:0.8
MAINTAINER Andreas Lindell version: 0.8

RUN apt-get update && apt-get install -y python3 libwww-perl

RUN chmod +x /opt/almond/howru

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]


