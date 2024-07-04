#!/bin/bash
echo "Installing Almond on containter redpanda-10"
echo "Installing dependencies"
cd /redpanda/almond
docker cp almond-0.9.0 redpanda-10:/root
docker exec -u 0 redpanda-10 apt update 
docker exec -u 0 redpanda-10 apt install gcc make automake -y
docker exec -u 0 redpanda-10 apt install libjson-c-dev librdkafka-dev autoconf zlib1g-dev -y
docker exec -u 0 redpanda-10 apt install sysstat ksh python3-psutil iputils-ping procps -y
echo "Installing Almond from source"
docker exec -u 0 redpanda-10 /bin/sh -c "cd /root/almond-0.9.0 && ./install_almond.sh"
echo "Installation finished"
echo "Start Almond"
docker exec -u 0 redpanda-10 /bin/sh -c "/opt/almond/start_almond.sh &"
echo "Installing Almond on container redpanda-console-10"
echo "Installing dependencies"
docker cp almond-0.9.0.2.alpine.aarch64.tar.gz redpanda-console-10:/tmp
docker exec -u 0 redpanda-console-10 apk update
docker exec -u 0 redpanda-console-10 apk add --no-cache perl sysstat bash python3 py3-psutil procps busybox iputils json-c librdkafka
docker exec -u 0 redpanda-console-10 tar xfvz /tmp/almond-0.9.0.2.alpine.aarch64.tar.gz 
echo "Installing precompiled Almond"
docker exec -u 0 redpanda-console-10 /bin/sh -c "cd almond-0.9.0.2.alpine.aarch64 && ./install.sh"
echo "Installation finished"
echo "Start Almond"
docker exec -u 0 redpanda-console-10 /bin/sh -c "/opt/almond/start_almond.sh &"
echo "Done"
