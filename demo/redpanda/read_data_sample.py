#!/usr/bin/env python3

from confluent_kafka import Consumer, KafkaError
from confluent_kafka.schema_registry import SchemaRegistryClient
from confluent_kafka.schema_registry.avro import AvroDeserializer
from confluent_kafka.serialization import SerializationError, MessageField, SerializationContext

# ANSI colors
red = "\033[31m"
green = "\033[32m"
black = "\033[0;30m"
yellow = "\033[1;33m"
purple = "\033[0;35m"

# --- Redpanda Schema Registry configuration ---
schema_registry_conf = {
    "url": "http://127.0.0.1:18081"
}
schema_registry_client = SchemaRegistryClient(schema_registry_conf)
avro_deserializer = AvroDeserializer(schema_registry_client)

# --- Redpanda Kafka API configuration ---
consumer_conf = {
    "bootstrap.servers": "127.0.0.1:19092",
    "group.id": "almond-monitoring-consumer",
    "auto.offset.reset": "earliest"
}

consumer = Consumer(consumer_conf)
consumer.subscribe(["almond_monitoring"])

print("Connected to Redpanda… waiting for messages.")

try:
    while True:
        msg = consumer.poll(1.0)
        if msg is None:
            continue

        if msg.error():
            if msg.error().code() != KafkaError._PARTITION_EOF:
                print(f"Consumer error: {msg.error()}")
            continue

        payload = msg.value()
        if not payload:
            continue

        # Try deserializing with Avro Schema Registry
        ctx = SerializationContext(msg.topic(), MessageField.VALUE)
        try:
            value = avro_deserializer(payload, ctx)
        except (SerializationError, Exception) as e:
            # Fallback for plain text or standard JSON payloads
            print(f"{yellow}[SKIP]{black} Non-Avro message at offset {msg.offset()}: {payload[:50]}...")
            continue

        if not isinstance(value, dict):
            continue

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
            print(red + str(status) + black +
                  f"\tServer: {server}\tTag: {tag}\tId: {id}\tOffset: {msg.offset()}\tPlugin: {plugin}")

except KeyboardInterrupt:
    pass
finally:
    consumer.close()
