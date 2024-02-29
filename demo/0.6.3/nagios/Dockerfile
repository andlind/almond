FROM jasonrivers/nagios:latest

RUN apt-get update && apt-get install -y libjsoncpp-dev python3-requests python3-schedule supervisor 

COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
RUN mkdir /opt/almond
COPY restart_nagios_process.sh /opt/almond/restart_nagios_process.sh
COPY integration.py /opt/almond/
COPY almond-collector /opt/almond/
COPY start_integration.sh /opt/almond/
COPY urls.conf /opt/almond/
RUN chmod +x /opt/almond/almond-collector
RUN chmod +x /opt/almond/start_integration.sh
RUN chmod +x /opt/almond/restart_nagios_process.sh

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]
