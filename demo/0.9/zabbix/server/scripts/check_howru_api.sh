#!/bin/bash

# Usage: check_howru_api.sh <api_address> <plugin_name> <server_name>
API_ADDRESS="$1"
PLUGIN_NAME="$2"
SERVER_NAME="$3"

# Validate input
if [[ -z "$API_ADDRESS" || -z "$PLUGIN_NAME" || -z "$SERVER_NAME" ]]; then
  echo "UNKNOWN - Missing API address, plugin name, or server name"
  exit 3
fi

# URL-encode function
urlencode() {
  local raw="$1"
  local encoded=""
  for (( i=0; i<${#raw}; i++ )); do
    c="${raw:$i:1}"
    case "$c" in
      [a-zA-Z0-9.~_-]) encoded+="$c" ;;
      *) encoded+=$(printf '%%%02X' "'$c") ;;
    esac
  done
  echo "$encoded"
}

ENC_PLUGIN_NAME=$(urlencode "$PLUGIN_NAME")
ENC_SERVER_NAME=$(urlencode "$SERVER_NAME")

API_URL="${API_ADDRESS}/api/plugin?index=${ENC_PLUGIN_NAME}&server=${ENC_SERVER_NAME}"

# Call the API
response=$(curl -s "$API_URL")

# Validate JSON
if ! echo "$response" | jq . >/dev/null 2>&1; then
  echo "UNKNOWN - Invalid JSON response from API"
  exit 3
fi

# Extract second object (plugin data)
plugin_data=$(echo "$response" | jq '.[1]')

# Extract fields
status_code=$(echo "$plugin_data" | jq -r '.pluginStatusCode')
message=$(echo "$plugin_data" | jq -r '.pluginOutput')

# Validate and return
if [[ "$status_code" =~ ^[0-3]$ ]]; then
  echo "${status_code}|${message}"
  exit "$status_code"
else
  echo "3|UNKNOWN - Invalid status code: $status_code"
  exit 3
fi

