FROM andreaslindell72/almond-monitor:0.8

RUN apt-get update && apt-get install -y python3 libwww-perl python3-redis

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
COPY plugins/* /opt/almond/plugins
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins
RUN mkdir /webapp
COPY webapp/ /webapp
RUN chmod +x /webapp/start_app.sh
COPY ./source.sh /
COPY ./startup.sh /
RUN chmod +x /source.sh && chmod +x /startup.sh

EXPOSE 80
EXPOSE 88

#ENTRYPOINT ["/usr/bin/supervisord"]
ENTRYPOINT ["./startup.sh"]
