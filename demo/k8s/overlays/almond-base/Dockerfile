FROM debian:bookworm AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    make \
    autoconf \
    automake \
    libtool \
    pkg-config \
    ca-certificates \
    curl \
    tar \
    gzip \
    libjson-c-dev \
    libssl-dev \
    libjwt-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    nagios-plugins \
    python3 \
    python3-yaml \
    python3-psutil \
    python3-prometheus-client \
    && rm -rf /var/lib/apt/lists/*

COPY almond/src/almond-26.2.1.tar.gz /tmp/almond.tar.gz

RUN mkdir -p /usr/src/almond \
    && tar -xzf /tmp/almond.tar.gz \
       -C /usr/src/almond --strip-components=1

WORKDIR /usr/src/almond

RUN autoreconf -fi \
    && ./configure --prefix=/opt/almond --disable-kafka \
    && make -j"$(nproc)" \
    && make install

RUN mkdir -p /etc/almond/check_exporter /var/log/almond /opt/almond/api_cmd
COPY almond/plugins/check_exporter.py /usr/lib/nagios/plugins/check_exporter.py
COPY webapp-proxy-almond/plugins/check_memory.py /usr/lib/nagios/plugins/check_memory.py
COPY webapp-proxy-almond/plugins/check_uptime.sh /usr/lib/nagios/plugins/check_uptime.sh
COPY webapp-proxy-almond/plugins/check_nginx_status.sh \
    /usr/lib/nagios/plugins/check_nginx_status.sh
RUN chmod +x \
    /usr/lib/nagios/plugins/check_exporter.py \
    /usr/lib/nagios/plugins/check_memory.py \
    /usr/lib/nagios/plugins/check_uptime.sh \
    /usr/lib/nagios/plugins/check_nginx_status.sh

COPY almond-base/almond.conf.template /etc/almond/almond.conf.template
COPY almond-base/entrypoint.sh /usr/local/bin/almond-entrypoint
RUN chmod +x /usr/local/bin/almond-entrypoint

ENTRYPOINT ["/usr/local/bin/almond-entrypoint"]
CMD ["/opt/almond/bin/almond"]
