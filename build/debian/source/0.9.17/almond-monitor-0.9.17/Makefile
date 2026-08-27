CC = gcc
COMMON_FLAGS = -g -Wall -std=c99 -O0 -fstack-protector
COMMON_LDFLAGS = -pthread -ljson-c -lssl -lcrypto -lz

# Include paths
INCLUDE_JSON = -I/usr/include/json_c
INCLUDE_KAFKA = -I/usr/include/librdkafka
INCLUDE_SERDES = -I/usr/local/include/libserdes
INCLUDE_AVRO = -I/usr/include/avro

# Library paths
LIB_KAFKA = -lrdkafka
LIB_SERDES = -lserdes
LIB_AVRO = -lavro

# Targets
all: almond almond-avro almond-minimal

almond: src/main.c src/plugins.c src/logger.c src/mod_kafka.c
	$(CC) $(COMMON_FLAGS) $(INCLUDE_JSON) $(INCLUDE_KAFKA) \
	$^ $(COMMON_LDFLAGS) $(LIB_KAFKA) -o almond

almond-avro: src/main.c src/plugins.c src/logger.c src/mod_avro.c
	$(CC) $(COMMON_FLAGS) $(INCLUDE_JSON) $(INCLUDE_KAFKA) $(INCLUDE_SERDES) $(INCLUDE_AVRO) \
	$^ $(COMMON_LDFLAGS) $(LIB_KAFKA) $(LIB_SERDES) $(LIB_AVRO) -o almond-avro

almond-minimal: src/main_s.c src/plugins.c src/logger.c
	$(CC) $(COMMON_FLAGS) $(INCLUDE_JSON) \
	$^ $(COMMON_LDFLAGS) -o almond-minimal

clean:
	rm -f almond almond-avro almond-minimal
