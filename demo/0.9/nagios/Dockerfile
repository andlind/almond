FROM jasonrivers/nagios:latest

RUN apt-get update && apt-get install -y libjsoncpp-dev python3-requests python3-schedule supervisor 

COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisor/conf.d/apps.conf
RUN mkdir -p /opt/almond/scripts
COPY restart_nagios_process.sh /opt/almond/scripts/restart_nagios_process.sh
COPY almond-nagios-integration /opt/almond/
COPY start_integration.sh /opt/almond/
RUN mkdir -p /etc/almond/integration
COPY integration.conf /etc/almond/integration/integration.conf 
COPY nagiosurls.conf /etc/almond/integration/nagiosurls.conf
RUN mkdir -p /var/log/almond
RUN chmod +x /opt/almond/almond-nagios-integration
RUN chmod +x /opt/almond/start_integration.sh
RUN chmod +x /opt/almond/scripts/restart_nagios_process.sh
RUN chown nagios:nagios /opt/nagios/var/rw/nagios.cmd

EXPOSE 80 

ENTRYPOINT ["/usr/bin/supervisord"]
