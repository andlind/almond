FROM grafana/grafana-oss:latest

USER root

RUN apk add --no-cache \
    bash \
    busybox \
    iputils \
    jansson \
    json-c \
    libc6-compat \
    librdkafka \
    openssl \
    perl \
    procps \
    py3-psutil \
    python3 \
    su-exec \
    sysstat

COPY alpine/libjwt/libjwt.so.2.11.2 /usr/lib/libjwt.so.2.11.2
COPY alpine/almond/almond-26.2.1.alpine.aarch64.tar.gz /tmp/almond.tar.gz
RUN tar -xzf /tmp/almond.tar.gz -C /tmp \
    && cd /tmp/almond-26.2.1.alpine.aarch64 \
    && ./install.sh \
    && rm -rf /tmp/almond-26.2.1.alpine.aarch64 /tmp/almond.tar.gz

COPY grafana/almond/almond.conf /etc/almond/almond.conf
COPY grafana/almond/plugins.conf /etc/almond/plugins.conf
COPY k8s/overlays/grafana/start-with-almond.sh /start-with-almond.sh

RUN chmod +x /start-with-almond.sh \
    && chown -R grafana:root /etc/almond /opt/almond /var/log/almond

ENTRYPOINT ["/start-with-almond.sh"]
