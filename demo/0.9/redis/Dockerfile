FROM redis
COPY redis.conf /usr/local/etc/redis/redis.conf
RUN mkdir -p /etc/almond
RUN mkdir -p /var/log/almond
RUN mkdir -p /opt/almond/plugins
RUN apt update -y && apt install gcc make automake -y
RUN apt install libjson-c-dev librdkafka-dev autoconf zlib1g-dev -y
RUN apt install libssl-dev libredis-perl sysstat ksh python3-psutil iputils-ping procps -y
RUN apt install supervisor wget -y && apt upgrade -y
COPY supervisord.conf /etc/supervisor/supervisord.conf
COPY apps.conf /etc/supervisord/conf.d/apps.conf
COPY plugins/* /opt/almond/plugins
RUN wget https://github.com/andlind/almond/raw/refs/heads/main/build/source/linux-version/0.9.9/almond-0.9.9.8.tar.gz
RUN tar xfvz almond-0.9.9.8.tar.gz 
RUN chmod +x almond-0.9.9.8/install_almond.sh
RUN cd almond-0.9.9.8 && ./install_almond.sh
RUN rm -rf almond-0.9.9.8* 
RUN mkdir -p /var/log/redis
COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
RUN useradd almond && usermod -a -G almond almond
RUN chmod +x /opt/almond/plugins/* && chown -R almond:almond /opt/almond/plugins

EXPOSE 6379
EXPOSE 87

ENTRYPOINT ["/usr/bin/supervisord"]
