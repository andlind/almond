#!/bin/bash
set -e

# Start Almond in background
/opt/almond/almond &

# Start Prometheus in foreground
exec /bin/prometheus \
  --config.file=/etc/prometheus/prometheus.yml \
  --storage.tsdb.path=/prometheus
