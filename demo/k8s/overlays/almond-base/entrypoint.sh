#!/bin/sh
set -eu

: "${ALMOND_HOST_NAME:?ALMOND_HOST_NAME must be set}"
: "${ALMOND_DATA_PREFIX:?ALMOND_DATA_PREFIX must be set}"

case "$ALMOND_HOST_NAME" in
  *[!A-Za-z0-9_.:-]*)
    echo "ALMOND_HOST_NAME contains unsupported characters" >&2
    exit 1
    ;;
esac

case "$ALMOND_DATA_PREFIX" in
  *[!A-Za-z0-9_-]*)
    echo "ALMOND_DATA_PREFIX contains unsupported characters" >&2
    exit 1
    ;;
esac

sed \
  -e "s|@HOST_NAME@|$ALMOND_HOST_NAME|g" \
  -e "s|@DATA_PREFIX@|$ALMOND_DATA_PREFIX|g" \
  /etc/almond/almond.conf.template > /etc/almond/almond.conf

exec "$@"
