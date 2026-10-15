check_exporter

A generic Python monitoring plugin that turns Prometheus-compatible exporter endpoints into monitoring checks and performance data.

The script fetches metrics from an HTTP endpoint, evaluates configured thresholds, and returns a monitoring state and exit code suitable for systems such as Nagios and Icinga.

Features
HTTP endpoint validation
Prometheus text-format metric parsing
YAML-based configuration
Metric allowlisting
Optional label matching
Multiple threshold comparison types
OK, WARNING, CRITICAL, and UNKNOWN states
Monitoring-compatible exit codes
Performance data output
Metric discovery with --list-metrics
Requirements
Python 3.9 or later
PyYAML

Install the dependency in a virtual environment:

python3 -m venv .venv
source .venv/bin/activate
python -m pip install PyYAML
python -m pip install prometheus-client

Usage
./check_exporter.py -H <URL> -c <CONFIG>


Example:

./check_exporter.py \
  -H http://localhost:9121/metrics \
  -c config.yaml

Command-line options
Option	Description
-H URL	Exporter endpoint URL
-c FILE	Path to the YAML configuration file
-l, --list-metrics	List available metric names and exit
-h, --help	Display command-line help

List metrics without supplying a configuration file:

./check_exporter.py \
  -H http://localhost:9121/metrics \
  --list-metrics


The listing displays unique metric names. Metrics with multiple label combinations are listed only once.

Configuration file

The configuration file defines which metrics are monitored and how their values are evaluated.

Only metrics explicitly listed under metrics are considered. Metrics not listed are ignored.

Each metric can have one rule or multiple rules. Multiple rules are useful when different label combinations require different thresholds.

Basic configuration
metrics:
  process_cpu_seconds_total:
    type: above
    thresholds:
      warning: 0.5
      critical: 10

  process_open_fds:
    type: above
    thresholds:
      warning: 100
      critical: 200

  exporter_healthy:
    type: not_equal
    thresholds:
      critical: 1

  process_max_fds:

Configuration properties
Property	Required	Description
enabled	No	Enables or disables the rule. Defaults to true.
type	No	Comparison type. Defaults to above.
thresholds	No	Warning and critical thresholds. Without thresholds, the metric is processed but cannot trigger an alert.
labels	No	Label filters that a sample must match for the rule to apply.

A rule with enabled: false is ignored.

Example:

metrics:
  process_open_fds:
    enabled: false

Comparison types

A comparison triggers the configured severity when its condition is true.

Type	Trigger condition
above	Value is greater than the threshold
below	Value is less than the threshold
equal	Value equals the threshold
not_equal	Value differs from the threshold
outside	Value is below min or above max
inside	Value is between min and max, inclusive
Numeric thresholds

The above, below, equal, and not_equal types use numeric thresholds.

metrics:
  queue_length:
    type: above
    thresholds:
      warning: 100
      critical: 500

Range thresholds

The outside and inside types use min and max for each severity.

metrics:
  service_temperature:
    type: outside
    thresholds:
      warning:
        min: 10
        max: 30
      critical:
        min: 5
        max: 40


For outside, a value below min or above max triggers the configured severity. Values exactly equal to either boundary do not trigger.

For inside, a value between min and max, including the boundaries, triggers the configured severity.

Important: Thresholds describe the condition that triggers an alert, not necessarily an acceptable range. For example, inside triggers when the value enters the configured range.

Warning and critical precedence

Critical has higher priority than warning.

If any check is critical, the overall state is CRITICAL.
Otherwise, if any check is warning, the overall state is WARNING.
Otherwise, the overall state is OK.

Threshold comparisons are strict for above and below. A value exactly equal to a threshold does not trigger those comparisons.

Label matching

Prometheus metrics can have multiple samples with the same name but different labels.

Example:

redis_latency_percentiles_usec{cmd="slowlog|len",quantile="50"} 1.003
redis_latency_percentiles_usec{cmd="slowlog|len",quantile="99"} 2.007
redis_latency_percentiles_usec{cmd="slowlog|len",quantile="99.9"} 2.007


Use multiple rules to configure different thresholds for each quantile:

metrics:
  redis_latency_percentiles_usec:
    - labels:
        cmd: "slowlog|len"
        quantile: "50"
      type: above
      thresholds:
        warning: 100
        critical: 200

    - labels:
        cmd: "slowlog|len"
        quantile: "99"
      type: above
      thresholds:
        warning: 300
        critical: 500

    - labels:
        cmd: "slowlog|len"
        quantile: "99.9"
      type: above
      thresholds:
        warning: 500
        critical: 1000


All specified labels must match. Other labels that are not specified in the rule do not affect matching.

A sample that does not match any enabled rule is ignored.

If multiple rules match a sample, the highest severity should determine its result. Avoid overlapping rules unless that behavior is intentional.

Synthetic health metric

A synthetic metric such as exporter_healthy can be added after a successful HTTP request and successful metric parsing:

metrics.append({
    "name": "exporter_healthy",
    "labels": "",
    "value": "1",
})


This metric is generated by the script; it is not provided by the exporter.

A successful value of 1 can be monitored with:

metrics:
  exporter_healthy:
    type: not_equal
    thresholds:
      critical: 1


If the HTTP request or metric validation fails, the script should return UNKNOWN instead. The synthetic metric only represents endpoint reachability and parseability; it does not mean every exported metric is healthy.

Output format

The script produces a single line of output.

OK
OK: 3 metrics processed | process_cpu_seconds_total=0.2; process_open_fds=9; exporter_healthy=1

WARNING
WARNING: 3 metrics processed process_cpu_seconds_total above warning threshold 0.5 | process_cpu_seconds_total=0.6; process_open_fds=9; exporter_healthy=1

CRITICAL
CRITICAL: 3 metrics processed process_open_fds above critical threshold 200 | process_cpu_seconds_total=0.6; process_open_fds=250; exporter_healthy=1

UNKNOWN
UNKNOWN: script_failure Could not reach URL: Connection refused


The text after | contains performance data in the form:

metric_name=value


Samples with labels include their label set in the performance-data name.

Exit codes
Exit code	State	Meaning
0	OK	No warning or critical condition detected
1	WARNING	At least one warning, no critical condition
2	CRITICAL	At least one critical condition
3	UNKNOWN	Script, configuration, HTTP, or metrics-validation error

Check the exit code in a shell:

./check_exporter.py -H http://localhost:9121/metrics -c config.yaml
echo $?

Implementation notes and limitations
The script uses a lightweight Prometheus text-format parser. It should be tested against escaped label values and other edge cases before being relied on in production.
Label matching is based on the metric name and configured label filters.
Multiple rules for the same metric can overlap. Configuration validation and tests should be used to catch unintended overlaps.
The script evaluates the current sample values only. It does not store historical values between executions.
Cumulative counters, such as process_cpu_seconds_total, may require rate calculations to provide meaningful utilization alerts.
A metric without thresholds is included in the processed metrics and performance data but does not trigger warning or critical states.
Suggested tests before production use
HTTP success, connection failure, timeout, and HTTP error responses
Invalid or empty metrics responses
Every comparison type and threshold boundary
Label matching with escaped characters
Disabled and unconfigured metrics
Overlapping rules and severity precedence
Correct exit codes for all monitoring states
