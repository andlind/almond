FROM ubuntu:latest
MAINTAINER andreas_li@hotmail.com

RUN apt update && apt install -y python3-flask

RUN apt install -y supervisor
RUN apt install -y nagios-plugins
RUN apt install -y wget

RUN wget https://github.com/andlind/howru/raw/main/packages/debian/howru-monitor_0.3.1.deb

RUN dpkg -i howru-monitor_0.3.1.deb && apt install -f

RUN mkdir -p /var/log/supervisor

COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY howruc.conf /etc/howru/howruc.conf
COPY plugins.conf /etc/howru/plugins.conf

CMD ["/usr/bin/supervisord"]
