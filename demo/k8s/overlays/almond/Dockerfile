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
    python3-prometheus-client \
    && rm -rf /var/lib/apt/lists/*

COPY src/almond-26.2.0.tar.gz /tmp/almond.tar.gz

RUN mkdir -p /usr/src/almond \
    && tar -xzf /tmp/almond.tar.gz \
       -C /usr/src/almond --strip-components=1

WORKDIR /usr/src/almond

RUN autoreconf -fi \
    && ./configure --prefix=/opt/almond --disable-kafka \
    && make -j"$(nproc)" \
    && make install

RUN mkdir -p /etc/almond/check_exporter
COPY config.yaml /etc/almond/check_exporter/config.yaml
COPY plugins.conf /etc/almond/plugins.conf
COPY almond.conf /etc/almond/almond.conf
RUN mkdir -p /opt/almond/plugins
COPY plugins/check_exporter.py /usr/lib/nagios/plugins/check_exporter.py
RUN chmod +x /opt/almond/plugins/check_procs \
    /opt/almond/plugins/check_exporter.py
RUN mkdir -p /var/log/almond
RUN mkdir -p /opt/almond/api_cmd

CMD ["/opt/almond/bin/almond"]
