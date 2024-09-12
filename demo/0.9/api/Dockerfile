FROM andreaslindell72/almond-monitor:0.9.3
MAINTAINER Andreas Lindell version: 0.9.3

RUN yum update -y && yum upgrade -y

RUN chmod +x /opt/almond/howru

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]


