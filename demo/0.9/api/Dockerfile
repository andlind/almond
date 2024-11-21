FROM andreaslindell72/almond-monitor:latest
MAINTAINER Andreas Lindell version: 0.9.5

RUN yum update -y && yum upgrade -y

RUN chmod +x /opt/almond/howru

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
RUN useradd almond && usermod -a -G almond almond
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]


