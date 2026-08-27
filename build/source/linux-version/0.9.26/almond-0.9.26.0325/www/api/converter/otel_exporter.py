import requests

OTLP_ENDPOINT = "http://localhost:4318"   # or your custom endpoint

class CollectorConnectionError(Exception):
    pass

def set_otlp_endpoint(url):
    global OTLP_ENDPOINT
    OTLP_ENDPOINT = url.rstrip("/")  # normalize

def export_metrics_via_http(otel_objects):
    payload = {"resourceMetrics": []}

    for obj in otel_objects:
        payload["resourceMetrics"].append({
            "resource": {
                "attributes": [
                    {"key": "plugin.name", "value": {"stringValue": obj["resource"]["plugin.name"]}},
                    {"key": "service.name", "value": {"stringValue": obj["resource"]["service.name"]}}
                ]
            },
            "scopeMetrics": [
                {"metrics": obj["metrics"]}
            ]
        })

    try:
        requests.post(
            f"{OTLP_ENDPOINT}/v1/metrics",
            json=payload,
            timeout=3
        )
    except requests.exceptions.ConnectionError:
        raise CollectorConnectionError(
            f"No OpenTelemetry Collector reachable at {OTLP_ENDPOINT}"
        )


def export_logs_via_http(otel_objects):
    payload = {"resourceLogs": []}

    for obj in otel_objects:
        payload["resourceLogs"].append({
            "resource": {
                "attributes": [
                    {"key": "plugin.name", "value": {"stringValue": obj["resource"]["plugin.name"]}},
                    {"key": "service.name", "value": {"stringValue": obj["resource"]["service.name"]}}
                ]
            },
            "scopeLogs": [
                {"logRecords": obj["logs"]}
            ]
        })

    try:
        requests.post(
            f"{OTLP_ENDPOINT}/v1/logs",
            json=payload,
            timeout=3
        )
    except requests.exceptions.ConnectionError:
        raise CollectorConnectionError(
            f"No OpenTelemetry Collector reachable at {OTLP_ENDPOINT}"
        )


def export_otel_data(otel_objects):
    export_metrics_via_http(otel_objects)
    export_logs_via_http(otel_objects)
