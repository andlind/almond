#ifndef MODKAFKA_H
#define MODKAFKA_H

int send_message_to_kafka(char*, char*, char*);
int send_ssl_message_to_kafka(char*, char*, char*, char*, char*, char*);
int send_avro_message_to_kafka(char*, char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, int);
int send_ssl_avro_message_to_kafka(char*, char*, char*, char*, char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, int);

#endif // MODKAFKA_H 
