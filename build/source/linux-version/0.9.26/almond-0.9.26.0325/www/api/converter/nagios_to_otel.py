import re
import time
from datetime import datetime

# ---------------------------------------------------------
# Perfdata Parsing
# ---------------------------------------------------------

PERFDATA_RE = re.compile(
    r"(?P<label>[a-zA-Z0-9_\-\/]+)="
    r"(?P<value>[-+]?[0-9]*\.?[0-9]+)"
    r"(?P<unit>[a-zA-Z%]*)"
    r"(?:;(?P<warn>[-+]?[0-9]*\.?[0-9]*))?"
    r"(?:;(?P<crit>[-+]?[0-9]*\.?[0-9]*))?"
    r"(?:;(?P<min>[-+]?[0-9]*\.?[0-9]*))?"
    r"(?:;(?P<max>[-+]?[0-9]*\.?[0-9]*))?"
)

def parse_perfdata(perf_string):
    metrics = []
    for match in PERFDATA_RE.finditer(perf_string):
        d = match.groupdict()
        metrics.append({
            "name": d["label"],
            "value": float(d["value"]),
            "unit": d["unit"] or None,
            "warn": float(d["warn"]) if d["warn"] else None,
            "crit": float(d["crit"]) if d["crit"] else None,
            "min": float(d["min"]) if d["min"] else None,
            "max": float(d["max"]) if d["max"] else None,
        })
    return metrics

# ---------------------------------------------------------
# Metric Type Inference
# ---------------------------------------------------------

def infer_metric_type(name, unit):
    # Heuristics
    if unit in ["%", "ms", "s", "us"]:
        return "gauge"
    if name in ["load1", "load5", "load15"]:
        return "gauge"
    # Default fallback
    return "gauge"

# ---------------------------------------------------------
# OTEL Metric Builder
# ---------------------------------------------------------

def build_otel_metric(plugin_name, metric, host_name):
    metric_type = infer_metric_type(metric["name"], metric["unit"])

    return {
        "name": f"{plugin_name}.{metric['name']}",
        "unit": metric["unit"] or "",
        metric_type: {
            "dataPoints": [
                {
                    "asDouble": metric["value"],
                    "timeUnixNano": int(time.time() * 1e9),
                    "attributes": {
                        "host.name": host_name,
                        "warn": metric["warn"],
                        "crit": metric["crit"],
                        "min": metric["min"],
                        "max": metric["max"],
                    }
                }
            ]
        }
    }

# ---------------------------------------------------------
# OTEL Log Builder
# ---------------------------------------------------------

def build_otel_log(plugin_json, host_name):
    ts = int(datetime.strptime(
        plugin_json["lastRun"], "%Y-%m-%d %H:%M:%S"
    ).timestamp() * 1e9)

    return {
        "timeUnixNano": ts,
        "severityText": plugin_json["pluginStatus"],
        "severityNumber": int(plugin_json["pluginStatusCode"]),
        "body": plugin_json["pluginOutput"].split("|")[0].strip(),
        "attributes": {
            "host.name": host_name,
            "plugin.name": plugin_json["name"],
            "plugin.status_code": int(plugin_json["pluginStatusCode"]),
            "maintenance": plugin_json["maintenance"] == "true",
        }
    }

# ---------------------------------------------------------
# Main Converter
# ---------------------------------------------------------

def nagios_to_otel(plugin_json, host_name):
    output = plugin_json["pluginOutput"]
    parts = output.split("|", 1)
    perfdata = parts[1] if len(parts) > 1 else ""

    parsed_metrics = parse_perfdata(perfdata)

    otel_metrics = [
        build_otel_metric(plugin_json["name"], m, host_name)
        for m in parsed_metrics
    ]

    otel_log = build_otel_log(plugin_json, host_name)

    return {
        "resource": {
            "service.name": "nagios-converter",
            "host.name": host_name,
            "plugin.name": plugin_json["name"]
        },
        "metrics": otel_metrics,
        "logs": [otel_log]
    }
