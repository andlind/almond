#!/bin/bash
echo "Create docker network"
docker network create almond-demo
exec docker compose -f redpanda/docker-compose.yml up -d
