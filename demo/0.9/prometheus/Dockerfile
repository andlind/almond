# Stage 1 — extract Prometheus
FROM --platform=linux/amd64 prom/prometheus:latest AS prometheus

# Stage 2 — final runtime image (Ubuntu, amd64)
FROM --platform=linux/amd64 ubuntu:24.04

# Install tools Almond plugins need
RUN apt-get update && apt-get install -y \
    bash \
    perl \
    python3 \
    python3-psutil \
    ca-certificates \
    tini \
    && rm -rf /var/lib/apt/lists/*

COPY almond/almond-minimal_0.9.23-1_amd64.deb /tmp
# Create Almond directories
RUN mkdir -p /etc/almond /var/log/almond \ 
   && chown -R nobody:nogroup /var/log/almond && chmod -R 755 /var/log/almond

# Install Almond
RUN apt-get update && apt-get install -y /tmp/almond-minimal_0.9.23-1_amd64.deb && rm /tmp/almond-minimal_0.9.23-1_amd64.deb

# Create Almond extra directories
RUN mkdir -p /opt/almond/api_cmd \
   && chown -R nobody:nogroup /opt/almond/api_cmd

# Create Prometheus directories
RUN mkdir -p /etc/prometheus /prometheus \ 
   && chown -R nobody:nogroup /prometheus \ 
   && chmod -R 775 /prometheus

# Copy Prometheus binary + assets
COPY --from=prometheus /bin/prometheus /bin/prometheus
#COPY --from=prometheus /usr/share/prometheus/consoles /usr/share/prometheus/consoles
#COPY --from=prometheus /usr/share/prometheus/console_libraries /usr/share/prometheus/console_libraries

# Copy Prometheus config
COPY prometheus.yml /etc/prometheus/prometheus.yml

# Copy Almond extra plugins
COPY almond/plugins/check_memory.py /usr/lib/nagios/plugins
COPY almond/plugins/check_uptime.sh /usr/lib/nagios/plugins
COPY almond/plugins/check_cpu_stats.sh /usr/lib/nagios/plugins
COPY almond/plugins/check_almond_process.sh /usr/lib/nagios/plugins

# Copy Almond configuration files
COPY almond/almond.conf /etc/almond
COPY almond/plugins.conf /etc/almond

COPY start.sh /start.sh
RUN chmod +x /start.sh

RUN chown -R nobody:nogroup /etc/prometheus
RUN chown -R nobody:nogroup /opt/almond/

USER nobody
#WORKDIR /prometheus

EXPOSE 9090

#ENTRYPOINT ["/bin/prometheus"]
#CMD ["--config.file=/etc/prometheus/prometheus.yml", "--storage.tsdb.path=/prometheus"]
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/start.sh"]
