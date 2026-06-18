FROM rockylinux:9


ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/lib64/pkgconfig

# Force bash to avoid QEMU /bin/sh issues
SHELL ["/bin/bash", "-c"]

# ------------------------------------------------------------
# Base development tools
# ------------------------------------------------------------
RUN dnf -y update && dnf -y groupinstall "Development Tools"

RUN dnf -y install \
    git \
    cmake \
    autoconf \
    automake \
    libtool \
    pkgconfig \
    which \
    tar \
    gzip \
    wget \
    curl \
    openssl-devel \
    zlib-devel \
    jansson-devel \
    libcurl-devel

# ------------------------------------------------------------
# Build and install json-c (newer than Rocky’s repo)
# ------------------------------------------------------------
RUN git clone https://github.com/json-c/json-c.git /tmp/json-c && \
    cd /tmp/json-c && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/json-c

# ------------------------------------------------------------
# Build and install Snappy
# ------------------------------------------------------------
RUN git clone https://github.com/google/snappy.git /tmp/snappy && \
    cd /tmp/snappy && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/snappy

# ------------------------------------------------------------
# Build and install librdkafka
# ------------------------------------------------------------
RUN git clone https://github.com/edenhill/librdkafka.git /tmp/librdkafka && \
    cd /tmp/librdkafka && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/librdkafka

# Register /usr/local/lib for librdkafka
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/librdkafka.conf && ldconfig

# ------------------------------------------------------------
# Build and install Avro-C
# ------------------------------------------------------------
RUN git clone https://github.com/apache/avro.git /tmp/avro && \
    cd /tmp/avro/lang/c && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/avro

# ------------------------------------------------------------
# Build and install libserdes
# ------------------------------------------------------------
RUN git clone https://github.com/confluentinc/libserdes.git /tmp/libserdes && \
    cd /tmp/libserdes && \
    ./configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/libserdes

# Register /usr/local/lib64 for libserdes + avro
RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/local-lib64.conf && ldconfig

# ------------------------------------------------------------
# Ready for development
# ------------------------------------------------------------
WORKDIR /workspace
CMD ["/bin/bash"]

