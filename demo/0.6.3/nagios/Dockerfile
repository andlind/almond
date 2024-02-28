FROM jasonrivers/nagios:latest

RUN apt-get update && apt-get install -y libjsoncpp-dev python3-requests python3-schedule 

RUN mkdir /opt/almond
COPY integration.py /opt/almond/
COPY almond-collector /opt/almond/
COPY start_integration.sh /opt/almond/
COPY urls.conf /opt/almond/
RUN chmod +x /opt/almond/almond-collector
RUN chmod +x /opt/almond/start_integration.sh
COPY ./start.sh /
RUN chmod +x /start.sh

EXPOSE 80 

ENTRYPOINT ["./start.sh"]
