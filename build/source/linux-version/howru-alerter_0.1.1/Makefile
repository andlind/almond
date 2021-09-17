default: build

build: clean
	gcc -Wall -o howru-alerter -I/usr/include/json-c  main.c util.c -lcurl -ljson-c

clean:
	rm -rf howru-alerter
