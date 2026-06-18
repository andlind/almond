FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ------------------------------------------------------------
# Install build dependencies
# ------------------------------------------------------------
RUN apt update && apt install -y \
    build-essential \
    git \
    cmake \
    autoconf \
    automake \
    debhelper \
    libradcli-dev \
    libdbi-dev \
    liburiparser-dev \
    libmysqlclient-dev \
    libpq-dev \
    libjson-c-dev \
    librdkafka-dev \
    libavro-dev \
    libjwt-dev \
    libtool \
    pkg-config \
    zlib1g-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libjansson-dev \
    ca-certificates

# ------------------------------------------------------------
# Build and install librdkafka into /usr
# ------------------------------------------------------------
#RUN git clone https://github.com/edenhill/librdkafka.git /tmp/librdkafka && \
#    cd /tmp/librdkafka && \
#    ./configure --prefix=/usr && \
#    make -j$(nproc) && \
#    make install && \
#    rm -rf /tmp/librdkafka

# ------------------------------------------------------------
# Build and install Avro-C into /usr
# ------------------------------------------------------------
#RUN git clone https://github.com/apache/avro.git /tmp/avro && \
#    cd /tmp/avro/lang/c && \
#    mkdir build && cd build && \
#    cmake .. -DCMAKE_INSTALL_PREFIX=/usr && \
#    make -j$(nproc) && \
#    make install && \
#    rm -rf /tmp/avro

# ------------------------------------------------------------
# Build and install libserdes into /usr
# ------------------------------------------------------------
RUN git clone https://github.com/confluentinc/libserdes.git /tmp/libserdes && \
    cd /tmp/libserdes && \
    ./configure --prefix=/usr && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/libserdes

# ------------------------------------------------------------
# Ready for development
# ------------------------------------------------------------
WORKDIR /workspace
CMD ["/bin/bash"]

