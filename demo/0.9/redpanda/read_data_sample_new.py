#!/usr/bin/env python3

from confluent_kafka import DeserializingConsumer
from confluent_kafka.schema_registry import SchemaRegistryClient
from confluent_kafka.schema_registry.avro import AvroDeserializer
from confluent_kafka.serialization import StringDeserializer

# ANSI colors
red = "\033[31m"
green = "\033[32m"
black = "\033[0;30m"
yellow = "\033[1;33m"
purple = "\033[0;35m"

# --- Redpanda Schema Registry configuration ---
schema_registry_conf = {
    "url": "http://localhost:8081"   # Redpanda Schema Registry
}
schema_registry_client = SchemaRegistryClient(schema_registry_conf)

value_deserializer = AvroDeserializer(schema_registry_client)

# --- Redpanda Kafka API configuration ---
consumer_conf = {
    "bootstrap.servers": "localhost:9092",  # Redpanda broker
    "key.deserializer": StringDeserializer(),
    "value.deserializer": value_deserializer,
    "group.id": "almond-monitoring-consumer",
    "auto.offset.reset": "earliest"
}

consumer = DeserializingConsumer(consumer_conf)
consumer.subscribe(["almond_monitoring"])

print("Connected to Redpanda… waiting for messages.")

while True:
    msg = consumer.poll(1.0)
    if msg is None:
        continue

    value = msg.value()  # Already a Python dict from Avro

    server = value.get("name", "Unknown")
    tag = value.get("tag", "None")
    id = value.get("id", "-1")

    data = value.get("data", {})
    plugin = data.get("pluginName")
    status = data.get("pluginStatus")

    # Pretty output
    if server == "app01.demo.com":
        server = "app01.demo.com\t"

    if status == "OK":
        print(green + status + "     " + black +
              f"\t\tServer: {server}\tTag: {tag}\tId: {id}\tOffset: {msg.offset()}\tPlugin: {plugin}")
    elif status == "WARNING":
        print(yellow + status + " " + black +
              f"\tServer: {server}\tTag: {tag}\tId: {id}\tOffset: {msg.offset()}\tPlugin: {plugin}")
    elif status == "UNKNOWN":
        print(purple + status + "\t" + black +
              f"\tServer: {server}\tTag: {tag}\tId: {id}\tOffset: {msg.offset()}\tPlugin: {plugin}")
    else:
        print(red + status + black +
              f"\tServer: {server}\tTag: {tag}\tId: {id}\tOffset: {msg.offset()}\tPlugin: {plugin}")

