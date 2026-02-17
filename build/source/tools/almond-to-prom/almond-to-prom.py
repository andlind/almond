#!/usr/bin/env python3
import argparse
import requests
import re
from datetime import datetime

def clean_metric(line):
    line = line.strip()

    # Replace hostname with host
    line = line.replace('hostname="', 'host="')

    # Remove empty key=""
    line = line.replace(', key=""', '')

    # Remove trailing underscores in metric names
    line = re.sub(r'^([a-zA-Z0-9_]+)_\{', r'\1{', line)

    return line

def main():
    parser = argparse.ArgumentParser(description="Fetch Almond metrics and write Prometheus file.")
    parser.add_argument("--url", required=True, help="URL to POST for metrics, e.g. http://localhost:9165")
    parser.add_argument("--output", default="/var/lib/node_exporter/textfile_collector/almond.prom", help="Output .prom file (default: /var/lib/node_exporter/textfile_collector/almond.prom)")

    args = parser.parse_args()

    payload = {"action": "metrics", "name": "getmetrics"}

    response = requests.post(args.url, json=payload)
    raw_lines = response.text.splitlines()

    cleaned = [clean_metric(line) for line in raw_lines]

    with open(args.output, "w") as f:
        f.write("# Almond metrics generated at {}\n".format(datetime.utcnow().isoformat()))
        for line in cleaned:
            f.write(line + "\n")

    print(f"Wrote {len(cleaned)} metrics to {args.output}")

if __name__ == "__main__":
    main()
