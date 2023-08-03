AUTOMAKE_OPTIONS = foreign
JSON_C_HEADER_DIR=/usr/include/json_c
JSON_C_LIB_DIR=/usr/lib/x86_64-linux-gnu
bin_PROGRAMS = almond
almond_SOURCES = main.c
CFLAGS= -pthread -L$(JSON_C_LIB_DIR) -I$(JSON_C_HEADER_DIR) -std=c99 -ljson-c


