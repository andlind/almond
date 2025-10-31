# Use official Zabbix server image
FROM zabbix/zabbix-server-mysql:alpine-latest

# Optional: install curl, jq, or other tools for API integration
USER root
RUN apk update && apk add --no-cache curl jq python3 py3-pip libffi-dev openssl-dev

# Copy your Nagios plugin scripts or API client scripts
COPY ./scripts /usr/lib/zabbix/externalscripts/

# Set permissions
RUN chown -R zabbix:zabbix /usr/lib/zabbix/externalscripts/
RUN chmod +x /usr/lib/zabbix/externalscripts/*

# Set up Almond sync
RUN mkdir -p /opt/almond/integration
RUN python3 -m venv /opt/almond/integration

# Activate venv and install Python packages
RUN /opt/almond/integration/bin/pip install --upgrade pip && \
    /opt/almond/integration/bin/pip install requests py-zabbix

# Set environment variables so Python uses the venv by default
ENV VIRTUAL_ENV=/opt/almond/integration
ENV PATH="$VIRTUAL_ENV/bin:$PATH"
ENV ZBX_TIMEOUT=30

COPY integration/* /opt/almond/integration

# Optional: expose Zabbix server port
