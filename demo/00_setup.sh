#!/bin/bash
echo "Create docker network"
docker network create almond-demo
exec docker compose -f redpanda/docker-compose.yml up -d
#echo "Creating schema registry"
#curl -X POST http://localhost:18081/subjects/almond-monitor-topic-value/versions \
#     -H "Content-Type: application/vnd.schemaregistry.v1+json" \
#     -d @<(echo '{"schema": '"$(jq -Rs . < plugin_status.avsc)"'}')
