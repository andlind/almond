#!/bin/bash
set -euo pipefail

redpanda_pod=redpanda-0
namespace=redpanda
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
demo_dir="$(cd -- "$script_dir/../../.." && pwd)"
console_pod="$(kubectl get pod -n "$namespace" -l app=redpanda-console -o jsonpath='{.items[0].metadata.name}')"
if [[ -z "$console_pod" ]]; then
  echo "No Redpanda Console pod found. Run 00_setup.sh first." >&2
  exit 1
fi

echo "Installing Almond on pod ${redpanda_pod}"
echo "Installing dependencies"
kubectl cp -n "$namespace" "$script_dir/almond-26.2.0" "${redpanda_pod}:/root"
kubectl exec -n "$namespace" "$redpanda_pod" -- apt update -y
kubectl exec -n "$namespace" "$redpanda_pod" -- apt install gcc make automake -y
kubectl exec -n "$namespace" "$redpanda_pod" -- apt install libjson-c-dev libcurl4-openssl-dev libjwt-dev librdkafka-dev autoconf zlib1g-dev -y
kubectl exec -n "$namespace" "$redpanda_pod" -- apt install libssl-dev sysstat ksh python3-psutil iputils-ping procps sudo -y
kubectl exec -n "$namespace" "$redpanda_pod" -- apt upgrade -y
echo "Installing Almond from source"
kubectl exec -n "$namespace" "$redpanda_pod" -- /bin/sh -c "cd /root/almond-26.2.0 && autoreconf -fi && ./install_almond.sh"
echo "Installation finished"
echo "Start Almond"
kubectl exec -n "$namespace" "$redpanda_pod" -- /bin/sh -c "/opt/almond/start_almond.sh &"
echo "Installing Almond on pod ${console_pod}"
echo "Installing dependencies"
kubectl cp -n "$namespace" "$demo_dir/alpine/libjwt/libjwt.so.2.10.2" "${console_pod}:/usr/lib/libjwt.so.2.10.2"
kubectl exec -n "$namespace" "$console_pod" -- ln -sf /usr/lib/libjwt.so.2.10.2 /usr/lib/libjwt.so.2
kubectl exec -n "$namespace" "$console_pod" -- ln -sf /usr/lib/libjwt.so.2.10.2 /usr/lib/libjwt.so.14
kubectl cp -n "$namespace" "$demo_dir/alpine/almond/almond-26.2.0.alpine.aarch64.tar.gz" "${console_pod}:/tmp/almond-26.2.0.alpine.aarch64.tar.gz"
kubectl exec -n "$namespace" "$console_pod" -- apk update
kubectl exec -n "$namespace" "$console_pod" -- apk add --no-cache perl sysstat bash openssl musl libc6-compat python3 py3-psutil procps busybox iputils json-c librdkafka jansson
kubectl exec -n "$namespace" "$console_pod" -- tar xfvz /tmp/almond-26.2.0.alpine.aarch64.tar.gz
echo "Installing precompiled Almond"
kubectl exec -n "$namespace" "$console_pod" -- /bin/sh -c "cd almond-26.2.0.alpine.aarch64 && ./install.sh"
echo "Installation finished"
echo "Start Almond"
kubectl exec -n "$namespace" "$console_pod" -- /bin/sh -c "/opt/almond/start_almond.sh &"
echo "Done"
