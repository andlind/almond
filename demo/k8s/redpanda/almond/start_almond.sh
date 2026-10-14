#!/bin/bash
set -euo pipefail
kubectl exec -n redpanda redpanda-0 -- /opt/almond/start_almond.sh
