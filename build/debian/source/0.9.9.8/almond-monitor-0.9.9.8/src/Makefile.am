AUTOMAKE_OPTIONS = foreign
CC=gcc
JSON_C_HEADER_DIR=/usr/include/json_c
JSON_C_LIB_DIR=/usr/lib/x86_64-linux-gnu
RDKAFKA_HEADER_DIR=/usr/include/librdkafka
RDKAFKA_LIB_DIR=/usr/lib/x86_64-linux-gnu
bin_PROGRAMS= almond
almond_SOURCES = main.c mod_kafka.c logger.c
CFLAGS = -L$(JSON_C_LIB_DIR)
almond_LDADD = -lpthread -ljson-c -lrdkafka -lssl -lcrypto -lz
almond_LDFLAGS= -L$(JSON_C_LIB_DIR)
almond_LDFLAGS = -I$(JSON_C_HEADER_DIR) -I$(RDKAFKA_HEADER_DIR) -std=c99
almond_CPPFLAGS= -I$(JSON_C_HEADER_DIR) -I$(RDKAFKA_HEADER_DIR)
