#!/bin/bash
echo "Starting otel collector"
docker run --rm -it \
  -p 4318:4318 \
  -v $(pwd)/otel-collector.yaml:/etc/otel-collector.yaml \
  otel/opentelemetry-collector:latest \
  --config=/etc/otel-collector.yaml
