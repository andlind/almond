CC = gcc
CFLAGS = -g -Wall -std=c99 -O0 -fstack-protector \
         -I/usr/include/json_c -I/usr/include/librdkafka
LDFLAGS = -pthread \
          -L/usr/lib/x86_64-linux-gnu \
          -ljson-c -lrdkafka -lssl -lcrypto -lz

SRCS = src/main.c src/logger.c src/mod_kafka.c
TARGET = almond

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

