FROM andreaslindell72/almond-monitor:0.9.1

RUN yum update -y && yum install python3-redis -y

RUN chmod +x /opt/almond/howru
RUN mkdir -p /custapp

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY plugins/* /opt/almond/plugins
COPY supervisord.conf /etc/supervisord.conf
COPY apps.conf /etc/supervisord.d/apps.conf
COPY app.py /custapp/app.py
RUN chmod +x /custapp/app.py && chmod +x /opt/almond/plugins/check_app.py && chown -R almond:almond /opt/almond/plugins

EXPOSE 80
EXPOSE 9909

ENTRYPOINT ["/usr/bin/supervisord"]


