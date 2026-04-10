#!/bin/bash

NETWORK_NAME="almond-demo"

if [ -z "$(docker network ls --filter name=^${NETWORK_NAME}$ --format="{{.Name}}")" ]; then
  echo "Creating docker network: $NETWORK_NAME"
  docker network create $NETWORK_NAME
else
  echo "Network $NETWORK_NAME already exists, skipping..."
fi

exec docker compose -f docker-compose.yaml up -d
