from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource
from opentelemetry.metrics import get_meter_provider, set_meter_provider

#OTLP_ENDPOINT = "http://localhost:4318"   # or your custom endpoint
OTLP_ENDPOINT = "http://host.docker.internal:4318/v1/metrics"

resource = Resource.create({
    "service.name": "almond-monitoring",
})

exporter = OTLPMetricExporter(
    endpoint=OTLP_ENDPOINT,
    timeout=5,
)

#reader = PeriodicExportingMetricReader(
#    exporter,
#    export_interval_millis=1000 )

reader = PeriodicExportingMetricReader(exporter)

provider = MeterProvider(
    resource=resource,
    metric_readers=[reader]
)

set_meter_provider(provider)
meter = get_meter_provider().get_meter("almond")

class CollectorConnectionError(Exception):
    pass

def set_otlp_endpoint(url):
    global OTLP_ENDPOINT
    OTLP_ENDPOINT = url.rstrip("/")  # normalize

def clean_attributes(attrs):
    return {k: v for k, v in attrs.items() if v is not None}

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
    #global logger
    for obj in otel_objects:
        host = obj["resource"]["host.name"]

        for m in obj["metrics"]:
            metric_name = m["name"]
            unit = m.get("unit", "")

            # GAUGE
            if "gauge" in m:
                datapoint = m["gauge"]["dataPoints"][0]
                value = datapoint["asDouble"]

                counter = meter.create_up_down_counter(
                    name=metric_name,
                    unit=unit,
                    description="Nagios gauge metric"
                )

                attributes = clean_attributes(datapoint["attributes"])
                counter.add(
                    value,
                    attributes={"host.name": host, **attributes}
                )

            # HISTOGRAM
            elif "histogram" in m:
                datapoint = m["histogram"]["dataPoints"][0]
                if "sum" not in datapoint:
                    print (datapoint)
                    print (f"Histogram missing 'sum': {m}")
                    continue
                value = datapoint["sum"]  # correct for histogram

                hist = meter.create_histogram(
                    name=metric_name,
                    unit=unit,
                    description="Nagios histogram metric"
                )

                attributes = clean_attributes(datapoint["attributes"])
                hist.record(
                    value,
                    attributes={"host.name": host, **attributes}
                )

            else:
                # Unknown metric type → log and skip
                logger.warning(f"Unknown metric type for {metric_name}: {m}")
                continue

#def export_otel_data(otel_objects):
#    for obj in otel_objects:
#        host = obj["resource"]["host.name"]
#        plugin = obj["resource"]["plugin.name"]
#
#        for m in obj["metrics"]:
#            metric_name = m["name"]  # already plugin.metricname
#            unit = m.get("unit", "")
#
#            # Extract the value from your JSON structure
#            datapoint = None
#            if "gauge" in m:
#                datapoint = m["gauge"]["dataPoints"][0]
#            elif "sum" in m:
#                datapoint = m["sum"]["dataPoints"][0]
#            else:
#                continue  # skip unknown metric types
#
#            value = datapoint["asDouble"]
#
#            # Create histogram once per metric name
#            hist = meter.create_histogram(
#                name=metric_name,
#                unit=unit,
#                description="Nagios perfdata metric"
#            )
#
#            # Record the value immediately
#            #print("EXPORTING", metric_name, "value=", value, "raw metric=", m)
#            raw_attrs = datapoint["attributes"]
#            attributes = clean_attributes(raw_attrs)
#            value = max(0, value)
#            hist.record(
#                value,
#                attributes={
#                    "host.name": host,
#                    **attributes
#                }
#            )
#
#def export_otel_data(otel_objects):
    #export_metrics_via_http(otel_objects)
    #export_logs_via_http(otel_objects)
    #for obj in otel_objects:
    #    metric_name = obj["metrics"][0]["name"]
    #    value = obj["metrics"][0]["gauge"]["dataPoints"][0]["asDouble"]
    #    gauge = meter.create_observable_gauge(
    #        name=metric_name,
    #        callbacks=[lambda options: [value]],
    #        unit="ms",
    #        description="Nagios check metric"
    #    )
    #for obj in otel_objects:
    #    host = obj["resource"]["host.name"]
    #    plugin = obj["resource"]["plugin.name"]
    #
    #    for m in obj["metrics"]:
    #        metric_name = f"{plugin}.{m['name']}"    
    #        value = m["gauge"]["dataPoints"][0]["asDouble"]
    #
    #        # Create an observable gauge for this metric
    #        def callback(options, value=value, host=host):
    #            return [value]  

    #        meter.create_observable_gauge(
    #            name=metric_name,
    #            callbacks=[callback],
    #            unit=m.get("unit", ""),
    #            description="Nagios perfdata metric"
    #        )
