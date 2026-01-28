#!/bin/bash
echo "Installing Almond on container almond_grafana"
echo "Installing dependencies"
docker cp ../../alpine/almond/almond-0.9.23.alpine.aarch64.tar.gz almond_grafana:/tmp
docker exec -u 0 almond_grafana apk update
docker exec -u 0 almond_grafana apk add --no-cache perl sysstat bash python3 py3-psutil procps busybox iputils json-c librdkafka
docker exec -u 0 almond_grafana apk add --no-cache openssl musl libc6-compat
docker exec -u 0 almond_grafana tar xfvz /tmp/almond-0.9.23.alpine.aarch64.tar.gz
echo "Installing precompiled Almond"
docker exec -u 0 almond_grafana /bin/bash -c "pwd && cd almond-0.9.23.alpine.aarch64 && ./install.sh"
echo "Installation finished"
echo "Copy configuration files"
docker cp almond.conf almond_grafana:/etc/almond/almond.conf
docker cp plugins.conf almond_grafana:/etc/almond/plugins.conf
echo "Start Almond"
docker exec -u 0 almond_grafana /bin/bash -c "/opt/almond/start_almond.sh &"
echo "Done"
