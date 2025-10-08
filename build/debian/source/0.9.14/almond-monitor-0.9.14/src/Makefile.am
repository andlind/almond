AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = almond

# Common source files
COMMON_SOURCES = main.c plugins.c logger.c

AM_CPPFLAGS = -I/usr/include -I/usr/local/include -I/opt/remi/librdkafka/include
AM_LDFLAGS = -L/usr/lib64 -L/usr/local/lib -L/usr/lib -L/opt/remi/librdkafka/lib64

# Conditional source and flags
if USE_AVRO
almond_SOURCES = $(COMMON_SOURCES) mod_avro.c
almond_CPPFLAGS = \
  -I/usr/include/json_c \
  -I/opt/remi/librdkafka/include \
  -I/usr/local/include \
  -I/usr/include/avro \
  -I/usr/local/include/libserdes

almond_CFLAGS = -g -Wall -std=c99 -O0 -fPIC

almond_LDFLAGS = \
  -L/usr/lib/x86_64-linux-gnu \
  -L/usr/local/lib \
  -fsanitize=address \
  -L/opt/remi/librdkafka/lib64 

almond_LDADD = \
  -lpthread \
  -ljson-c \
  -lserdes \
  -lavro \
  -lrdkafka \
  -lssl \
  -lcrypto \
  -lz \
  -llzma
else
almond_SOURCES = $(COMMON_SOURCES) mod_kafka.c
almond_CPPFLAGS = \
  -I/usr/include/json_c \
  -I/usr/include/librdkafka

almond_CFLAGS = -g -Wall -std=c99 -O0 -fPIC
almond_LDFLAGS = \
  -L/usr/lib/x86_64-linux-gnu \
  -fstack-protector \
  -L/opt/remi/librdkafka/lib64 

almond_LDADD = \
  -lpthread \
  -ljson-c \
  -lrdkafka \
  -lssl \
  -lcrypto \
  -lz
endif
