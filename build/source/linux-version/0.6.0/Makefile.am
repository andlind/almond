AUTOMAKE_OPTIONS = foreign
CC=gcc
JSON_C_HEADER_DIR=/usr/include/json_c
JSON_C_LIB_DIR=/usr/lib/x86_64-linux-gnu
RDKAFKA_HEADER_DIR=/usr/include/librdkafka
RDKAFKA_LIB_DIR=/usr/lib/x86_64-linux-gnu
bin_PROGRAMS= almond
almond_SOURCES = main.c mod_kafka.c
almond_CFLAGS= -std=c99
almond_LDADD= -ljson-c -lrdkafka -lz
almond_LDFLAGS= -L$(JSON_C_LIB_DIR)
almond_CPPFLAGS= -I$(JSON_C_HEADER_DIR) -I$(RDKAFKA_HEADER_DIR)
