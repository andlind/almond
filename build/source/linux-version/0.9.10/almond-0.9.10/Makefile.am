AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = almond

# Common source files
COMMON_SOURCES = main.c logger.c

# Conditional source and flags
if USE_AVRO
almond_SOURCES = $(COMMON_SOURCES) mod_avro.c
almond_CPPFLAGS = \
  -I/usr/include/json_c \
  -I/usr/include/librdkafka \
  -I/usr/include/avro \
  -I/usr/include/libserdes

almond_CFLAGS = -g -Wall -std=c99 -O0
almond_LDFLAGS = -L/usr/lib/x86_64-linux-gnu -fsanitize=address
almond_LDADD = \
  -lpthread \
  -ljson-c \
  -lserdes \
  -lavro \
  -lrdkafka \
  -lssl \
  -lcrypto \
  -lz
else
almond_SOURCES = $(COMMON_SOURCES) mod_kafka.c
almond_CPPFLAGS = \
  -I/usr/include/json_c \
  -I/usr/include/librdkafka

almond_CFLAGS = -g -Wall -std=c99 -O0
almond_LDFLAGS = -L/usr/lib/x86_64-linux-gnu -fstack-protector
almond_LDADD = \
  -lpthread \
  -ljson-c \
  -lrdkafka \
  -lssl \
  -lcrypto \
  -lz
endif
