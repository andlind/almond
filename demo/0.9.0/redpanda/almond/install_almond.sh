#!/bin/bash
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
echo "Done"
