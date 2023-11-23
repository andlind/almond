FROM andreaslindell72/almond-monitor:latest

RUN apt-get update && apt-get install -y apache2 python3 libwww-perl

RUN chmod +x /opt/almond/howru-api 

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY ./source.sh / 
COPY ./startup.sh /
COPY plugins/* /opt/almond/plugins
RUN chown -R almond:almond /opt/almond/plugins

RUN /usr/sbin/useradd apache -d /usr/share/apache2 -s /sbin/nologin

RUN chmod +x /source.sh && chmod +x /startup.sh

EXPOSE 80
EXPOSE 88

ENTRYPOINT ["/startup.sh"]


