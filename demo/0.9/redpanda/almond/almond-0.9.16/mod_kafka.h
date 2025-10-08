#ifndef MODKAFKA_H
#define MODKAFKA_H

void setKafkaConfigFile(const char*);
int loadKafkaConfig();
int init_kafka_producer(); 
int send_message_to_gkafka(const char*);
int send_avro_message_to_gkafka(const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, int);
int send_message_to_kafka(char*, char*, char*);
int send_ssl_message_to_kafka(char*, char*, char*, char*, char*, char*);
int send_avro_message_to_kafka(char*, char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, int);
int send_ssl_avro_message_to_kafka(char*, char*, char*, char*, char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, int);
void free_kafka_memalloc();

#endif // MODKAFKA_H 
