FROM andreaslindell72/almond-monitor:latest
LABEL org.opencontainers.image.authors="andreas.lindell@almondmonitor.com"
LABEL version="0.9.23"

RUN yum update -y && yum upgrade -y

RUN chmod +x /opt/almond/howru

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
COPY supervisor/almond.conf /etc/supervisord.d/almond.conf
#RUN useradd almond && usermod -a -G almond almond
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]


