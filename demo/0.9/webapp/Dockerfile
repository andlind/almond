FROM andreaslindell72/almond-monitor:0.9.3

RUN yum update -y && yum install python3-redis -y

COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
COPY supervisord.conf /etc/supervisord.conf
COPY apps.conf /etc/supervisord.d/apps.conf
COPY plugins/* /opt/almond/plugins
RUN useradd almond && usermod -a -G almond almond
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins
RUN mkdir /webapp
COPY webapp/ /webapp
RUN chmod +x /webapp/start_app.sh
COPY ./source.sh /
COPY ./startup.sh /
RUN chmod +x /source.sh && chmod +x /startup.sh

EXPOSE 80
EXPOSE 88

ENTRYPOINT ["./startup.sh"]
