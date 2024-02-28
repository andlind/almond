FROM andreaslindell72/almond-monitor:latest

RUN apt-get update && apt-get install -y python3 libwww-perl python3-redis

RUN chmod +x /opt/almond/howru-api 

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
COPY app.py /custapp/app.py
RUN chmod +x /custapp/app.py && chmod +x /opt/almond/plugins/check_app.py && chown -R almond:almond /opt/almond/plugins

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]


