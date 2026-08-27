AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = almond

COMMON_SOURCES = plugins.c logger.c api.c config.c

AM_CPPFLAGS = -I/usr/include -I/usr/local/include
AM_LDFLAGS  = -L/usr/lib64 -L/usr/local/lib -L/usr/lib

# Default flags for all variants
almond_CPPFLAGS = -I/usr/include/json_c
almond_CFLAGS   = -g -Wall -std=c99 -O0
almond_LDFLAGS  = -L/usr/lib/x86_64-linux-gnu -fstack-protector
almond_LDADD    = -lpthread -ljson-c -lssl -lcrypto -lz

# ---------------------------------------------------------
# Minimal version (no Kafka, no Avro)
# ---------------------------------------------------------
if USE_MINIMAL
almond_SOURCES = main_s.c $(COMMON_SOURCES)
almond_CPPFLAGS += -DUSE_MINIMAL
endif

# ---------------------------------------------------------
# Kafka-only version
# ---------------------------------------------------------
if USE_KAFKA_ONLY
almond_SOURCES = main.c $(COMMON_SOURCES) mod_kafka.c
almond_CPPFLAGS += -I/usr/include/librdkafka
almond_LDADD += -lrdkafka
endif

# ---------------------------------------------------------
# Avro version
# ---------------------------------------------------------
if USE_AVRO
almond_SOURCES = main.c $(COMMON_SOURCES) mod_avro.c
almond_CPPFLAGS += -I/usr/include/librdkafka -I/usr/include/avro -I/usr/include/libserdes
almond_LDADD += -lrdkafka -lserdes -lavro
endif
