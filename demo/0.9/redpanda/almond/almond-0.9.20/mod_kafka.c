#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <librdkafka/rdkafka.h>
#include "main.h"
#include "config.h"
#include "logger.h"
#include "mod_kafka.h"

#define MAX_STRING_SIZE 50
#define INFO_STR_SIZE 810

char* configFile = NULL;
char* brokers = NULL;
char* topic = NULL;
char* kafka_client_id = NULL;
char* kafka_partitioner = NULL;
char* transactional_id = NULL;
char* kafka_compression = NULL;
char* kafkaCALocation = NULL;
char* kafkaSSLCertificate = NULL;
char* kafka_SSLKey = NULL;
char* kafka_sasl_mechanisms = NULL;
char* kafka_sasl_username = NULL;
char* kafka_sasl_password = NULL;
char* kafka_security_protocol = NULL;
bool use_transactions = false;
bool enable_idempotence = false;
bool kafka_dr_cb = true;
bool kafka_log_connection_close = false;
int metadata_request_timeout = 60000;
int acks = -2;
int retries = 5;
int max_in_flight_requests = 5;
int linger_ms = 10;
int batch_num_messages = 10000;
int queue_buffering_max_messages = 100000;
int queue_buffering_max_kbytes = 1048576;
int message_max = 1000000;
int message_copy_max = 65535;
int request_timeout = 30000;
int message_timeout_ms = 300000;
int retry_backoff = 100;
int statistics_interval = 60000;
int kafka_socket_timeout = 60000;
int kafka_socket_blocking = 1000;
//int transaction_timeout = 30000;
int timeout_ms = 30000;
rd_kafka_t *global_producer = NULL;

char info[INFO_STR_SIZE];
int config_k_memalloc_fails = 0;
int requirements_met = 0;

static void process_kafka_brokers(ConfVal);
static void process_kafka_topic(ConfVal);
void process_kafka_client_id(ConfVal);
void process_metadata_request_timeout(ConfVal);
void process_kafka_acks(ConfVal);
void process_kafka_idempotence(ConfVal);
void process_kafka_transactional_id(ConfVal);
void process_kafka_retries(ConfVal);
void process_kafka_in_flight_requests(ConfVal);
void process_kafka_linger(ConfVal);
void process_kafka_batch_num_messages(ConfVal);
void process_queue_buffering_messages(ConfVal);
void process_queue_buffering_kbytes(ConfVal);
void process_kafka_compression(ConfVal);
void process_kafka_partitioner(ConfVal);
void process_kafka_message_max(ConfVal);
void process_message_copy_max(ConfVal);
void process_kafka_request_timeout(ConfVal);
void process_kafka_message_timeout(ConfVal);
void process_kafka_retry_backoff(ConfVal);
void process_kafka_dr_cb(ConfVal);
void process_kafka_statistics_interval(ConfVal);
void process_kafka_log_connection_close(ConfVal);
void process_kafka_ssl_ca_location(ConfVal);
void process_kafka_certificate(ConfVal);
void process_kafka_key(ConfVal);
void process_kafka_security_protocol(ConfVal);
void process_kafka_sasl_mechanisms(ConfVal);
void process_kafka_sasl_password(ConfVal);
void process_kafka_sasl_username(ConfVal);
void process_kafka_socket_timeout(ConfVal);
void process_kafka_socket_blocking(ConfVal);
void process_kafka_transaction_timeout(ConfVal);

ConfigEntry kafka_entries[] = {
    {"kafka.bootstrap.servers", process_kafka_brokers},
    {"kafka.topic", process_kafka_topic},
    {"kafka.client.id", process_kafka_client_id},
    {"kafka.metadata.max.age.ms", process_metadata_request_timeout},
    {"kafka.acks", process_kafka_acks},
    {"kafka.enable.idempotence", process_kafka_idempotence},
    {"kafka.transactional.id", process_kafka_transactional_id},
    {"kafka.retries", process_kafka_retries},
    {"kafka.max.in.flight.requests.per.connection", process_kafka_in_flight_requests},
    {"kafka.linger.ms", process_kafka_linger},
    {"kafka.batch.num.messages", process_kafka_batch_num_messages},
    {"kafka.queue.buffering.max.messages", process_queue_buffering_messages},
    {"kafka.queue.buffering.max.kbytes", process_queue_buffering_kbytes},
    {"kafka.compression.codec", process_kafka_compression},
    {"kafka.message.max.bytes", process_kafka_message_max},
    {"kafka.message.copy.max.bytes", process_message_copy_max},
    {"kafka.request.timeout.ms", process_kafka_request_timeout},
    {"kafka.delivery.timeout.ms", process_kafka_message_timeout}, 
    {"kafka.retry.backoff.ms", process_kafka_retry_backoff},
    {"kafka.dr_cb", process_kafka_dr_cb},
    {"kafka.statistics.interval.ms", process_kafka_statistics_interval},
    {"kafka.log.connection.close", process_kafka_log_connection_close},
    {"kafka.ssl.ca.location", process_kafka_ssl_ca_location},
    {"kafka.ssl.certificate.location", process_kafka_certificate},
    {"kafka.ssl.key.location", process_kafka_key},
    {"kafka.security.protocol", process_kafka_security_protocol},
    {"kafka.sasl.mechanisms", process_kafka_sasl_mechanisms},
    {"kafka.sasl.username", process_kafka_sasl_username},
    {"kafka.sasl.password", process_kafka_sasl_password},
    {"kafka.socket.timeout.ms", process_kafka_socket_timeout},
    {"kafka.socket.blocking.max.ms", process_kafka_socket_blocking},
    {"kafka.transaction.timeout.ms", process_kafka_transaction_timeout},
    {"kafka.partitioner", process_kafka_partitioner}
};

char *triminfo(char *s) {
	char *ptr;
    	if (!s)
        	return NULL;   // NULL string
    	if (!*s)
        	return s;      // empty string
    	for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
    	ptr[1] = '\0';
    	return s;
}

bool is_numeric_string(const char *s) {
	if (!s || !*s) return false;
    	for (; *s; s++) {
        	if (!isdigit(*s)) return false;
    	}
    	return true;
}

void to_uppercase(char *str) {
	for (int i = 0; str[i]; i++) {
        	str[i] = toupper((unsigned char)str[i]);
    	}
}

void processIntConfigInfo(char* name, int value) {
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] %s is set to %d.", name, value);
	writeLog(triminfo(info), 0, 1);
}

int mem_k_alloc(void* ptr, char* name) {
	if(ptr == NULL) {
		fprintf(stderr, "[mod_kafka] Failed to allocate memory for %s.\n", name);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] Failed to allocate memory for '%s'.", name);
		writeLog(triminfo(info), 2, 1);
		config_k_memalloc_fails++;
		return 1;
	}
	return 0;
}

static void process_kafka_brokers(ConfVal value) {
        size_t kf_len = strlen(value.strval) + 1;
	if (kf_len > 5) {
        	brokers = malloc(kf_len);
		mem_k_alloc(brokers, "kafka.bootstrap.servers");
        	if (brokers == NULL) return;
        	snprintf(brokers, kf_len, "%s", value.strval);
        	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka export brokers is set to '%s'.", brokers);
        	writeLog(triminfo(info), 0, 1);
		requirements_met++;
	}
	else
		writeLog("[mod_kafka] Almond brokers is set to NULL.", 2, 1);
}

static void process_kafka_topic(ConfVal value) {
	size_t t_len = strlen(value.strval) + 1;
	if (t_len > 2) {
		topic = malloc(t_len);
		mem_k_alloc(topic, "kafka.topic");
		if (topic == NULL) return;
		snprintf(topic, t_len, "%s", value.strval);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka topic is set to '%s'.", topic);
		writeLog(triminfo(info), 0, 1);
		requirements_met++;
	}
	else
		writeLog("[mod_kafka] Almond topic is set to NULL.", 2, 1);
}

void process_kafka_client_id(ConfVal value) {
	size_t c_len = strlen(value.strval) + 1;
	if (c_len > 2) {
		kafka_client_id = malloc(c_len);
		mem_k_alloc(kafka_client_id, "kafka.client.id");
		if (kafka_client_id == NULL) return;
		snprintf(kafka_client_id, c_len, "%s", value.strval);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka client id is set to '%s'.", kafka_client_id);
		writeLog(triminfo(info), 0, 1);
	}
	else
		writeLog("[mod_kafka] kafka.client.id is set to NULL.", 0, 1);
}

void process_metadata_request_timeout(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i == 0) {
		writeLog("[mod_kafka] Could not interpret 'kafka.metadata.max.age.ms' value from config file.", 1, 1);
		writeLog("[mod_kafka] 'kafka.metadata.max.age.ms' is set to 60000.", 0, 1);
		return;
	}
	metadata_request_timeout = i;
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.metadata.max.age.ms is set to %i.", metadata_request_timeout);
	writeLog(triminfo(info), 0, 1);
}

void process_kafka_acks(ConfVal value) {
	char *str = triminfo(value.strval);

    	if (str && strlen(str) > 0 && !is_numeric_string(str)) {
        	if (strcmp(str, "all") == 0) {
           		acks = -1; // sentinel for 'all'
            		writeLog("[mod_kafka] Kafka.acks is set to 'all'.", 0, 1);
        	} 
		else {
            		snprintf(info, INFO_STR_SIZE, "[mod_kafka] Invalid string value for Kafka.acks: %s.", str);
            		writeLog(triminfo(info), 1, 1);
            		writeLog("[mod_kafka] Kafka.acks will be set to 1.", 0, 1);
            		acks = 1;
        	}
    	} 
	else if (value.intval == 0 || value.intval == 1) {
        	acks = value.intval;
        	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka.acks is set to %d.", value.intval);
        	writeLog(triminfo(info), 0, 1);
    	} 
	else {
        	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Invalid integer value for Kafka.acks: %d", value.intval);
        	writeLog(triminfo(info), 1, 1);
        	writeLog("[mod_kafka] Kafka.acks will be set to 1.", 0, 1);
        	acks = 1;
    	}
}

/*void process_kafka_acks(ConfVal value) {
	if (value.strval && strcmp(triminfo(value.strval), "all") == 0) {
        	writeLog("[mod_kafka] Kafka.acks is set to 'all'.", 0, 1);
        	acks = -1;
        	return;
    	}
    	if (value.intval == 0 || value.intval == 1) {
        	acks = value.intval;
        	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka.acks is set to %d.", value.intval);
        	writeLog(triminfo(info), 0, 1);
    	} else {
        	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Invalid value for Kafka.acks: %d", value.intval);
        	writeLog(triminfo(info), 1, 1);
        	writeLog("[mod_kafka] Kafka.acks will be set to 1.", 0, 1);
        	acks = 1;
    	}*/
	/*
    	char acks_str[8];
    	if (acks_setting == -1) {
        	strcpy(acks_str, "all");
    	} else {
        	snprintf(acks_str, sizeof(acks_str), "%d", acks_setting);
    	}

    	if (rd_kafka_conf_set(conf, "acks", acks_str, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        	fprintf(stderr, "Failed to set acks: %s\n", errstr);
    	}*/       
//}

void process_kafka_idempotence(ConfVal value) {
	if (strcmp(triminfo(value.strval), "true") == 0) {
		enable_idempotence = true;
	}
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] Kafka.enable.idempotence is set to %s.", enable_idempotence ? "true" : "false");
	writeLog(triminfo(info), 0, 1);
}

void process_kafka_transactional_id(ConfVal value) {
	size_t t_len = strlen(value.strval) + 1;
	if (t_len <= 1) {
		writeLog("[mod_kafka] kafka.transactional.id can not be an empty string.", 1, 1);
		return;
	}
	if (value.intval) {
		if (value.intval != 0) {
			writeLog("[mod_kafka] kafka.transactional.id should not be an integer value, but a string.", 1, 1);
			return;
		}
	}
	transactional_id = malloc(t_len);
	mem_k_alloc(transactional_id, "kafka.transactional.id");
	if (transactional_id == NULL) return;
	snprintf(transactional_id, t_len, "%s", value.strval);
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.transactional.id is set to '%s'.", transactional_id);
	writeLog(triminfo(info), 0, 1);
	/*
	rd_kafka_conf_set(conf, "transactional.id", "my-transactional-producer", errstr, sizeof(errstr));
	rd_kafka_conf_set(conf, "enable.idempotence", "true", errstr, sizeof(errstr));
	You must also:
    		Initialize transactions with rd_kafka_init_transactions()

    		Begin a transaction with rd_kafka_begin_transaction()

    		Send messages as usual

    		Commit with rd_kafka_commit_transaction() or abort with rd_kafka_abort_transaction()
	*/
}

void process_kafka_retries(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i < 0) {
		writeLog("[mod_kafka] Can not evaluate negative numbers in kafka.retries.", 1, 1);
		writeLog("[mod_kafka] Kafka.retries is set to 5.", 0, 1);
		return;
	}
	retries = i;
	processIntConfigInfo("kafka.retries", retries);
}

void process_kafka_in_flight_requests(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i > 5) {
		writeLog("[mod_kafka] kafka.max.in.flight.requests.per.connection should not be above 5 if enabling kafka idempotence.", 0, 1);
	}
	max_in_flight_requests = i;
	processIntConfigInfo("kafka.max.in.flight.requests.per.connection", max_in_flight_requests);
}

void process_kafka_linger(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i > 900000) {
		writeLog("[mod_kafka] kafka.linger.ms value is above maximum value, will reduce it to 900000.", 1, 1);
		writeLog("[mod_kafka] Note that kafka.linger.ms is at its hard limit (15 minutes).", 1, 1);
		i = 90000;
		return; 
	}
	if (i < 0) i = 0;
	linger_ms = i;
	processIntConfigInfo("kafka.linger.ms", linger_ms);
}

void process_kafka_batch_num_messages (ConfVal value) {
	int i = value.intval;
	if (i < 0) i = 10000;
	batch_num_messages = i;
	processIntConfigInfo("kafka.batch_num_messages", batch_num_messages);
	writeLog("[mod_kafka] Use queue.buffering.max.messages instead of batch.num.messages.", 1, 1);
}

void process_queue_buffering_messages(ConfVal value) {
	int i = value.intval;
	if (i < 0) i = 100000;
	queue_buffering_max_messages = i;
	processIntConfigInfo("kafka.queue.buffering.max.messages", queue_buffering_max_messages);
}

void process_queue_buffering_kbytes(ConfVal value) {
	int i = value.intval;
	if (i < 0) i = 1048576;
	queue_buffering_max_kbytes = i;
	processIntConfigInfo("kafka.queue.buffering.max.kbytes", queue_buffering_max_kbytes);
}

void process_kafka_compression(ConfVal value) {
	size_t c_pen = strlen(value.strval) + 1;
	kafka_compression = malloc(c_pen);
	mem_k_alloc(kafka_compression, "kafka.compression.codec");
	if (kafka_compression == NULL) return;
	snprintf(kafka_compression, c_pen, "%s", value.strval);
	if (strcmp(triminfo(kafka_compression), "gzip") == 0) {
		writeLog("[mod_kafka] kafka.compression.codec is set to 'gzip'.", 0, 1);
	}
	else if (strcmp(triminfo(kafka_compression), "snappy") == 0) {
		writeLog("[mod_kafka] kafka.compression.codec is set to 'snappy'.", 0, 1);
	}
	else if (strcmp(triminfo(kafka_compression), "lz4") == 0) {
                writeLog("[mod_kafka] kafka.compression.codec is set to 'lz4'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_compression), "zstd") == 0) {
                writeLog("[mod_kafka] kafka.compression.codec is set to 'zstd'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_compression), "none") == 0) {
                writeLog("[mod_kafka] kafka.compression.codec is set to 'none'.", 0, 1);
        }
	else {
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] %s is not a valid kafka.compression.codec value", kafka_compression);
		writeLog(triminfo(info), 1, 1);
		writeLog("[mod_kafka] kafka.compression.codec will be set to 'lz4'.", 0, 1);
		snprintf(kafka_compression, 4, "%s", "lz4");
	}
}

void process_kafka_message_max(ConfVal value) {
        int i = value.intval;
        if (i <= 0) i = 1000000;
        message_max = i;
        processIntConfigInfo("kafka.message.max.bytes", message_max);
}

void process_message_copy_max(ConfVal value) {
	int i = value.intval;
        if (i <= 0) i = 65535;
        message_copy_max = i;
        processIntConfigInfo("kafka.message.copy.max.bytes", message_copy_max);
}

void process_kafka_request_timeout(ConfVal value) {
	int i = value.intval;
        if (i <= 0) i = 30000;
	request_timeout = i;
	processIntConfigInfo("kafka.request.timeout.ms", request_timeout);
}

void process_kafka_message_timeout(ConfVal value) {
	int i = value.intval;
	if (i <= 0) i = 300000;
	message_timeout_ms = i;
	processIntConfigInfo("kafka.delivery.timeout.ms", message_timeout_ms);
}

void process_kafka_retry_backoff(ConfVal value) {
	int i = value.intval;
        if (i <= 0) i = 100;
        retry_backoff = i;
        processIntConfigInfo("kafka.retry.backoff.ms", retry_backoff);
}

void process_kafka_dr_cb(ConfVal value) {
	if (strcmp(triminfo(value.strval), "false") == 0) {
                kafka_dr_cb = false;
        }
        snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.dr_cb is set to %s.", kafka_dr_cb ? "true" : "false");
        writeLog(triminfo(info), 0, 1);
}

void process_kafka_statistics_interval(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i <= 100) i = 60000;
	statistics_interval = i;
	processIntConfigInfo("kafka.statistics.interval.ms", statistics_interval);
}

void process_kafka_log_connection_close(ConfVal value) {
	if (strcmp(triminfo(value.strval), "true") == 0) {
		kafka_log_connection_close = true;
	}
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.log.connection.close is set to  %s.", kafka_log_connection_close ? "true" : "false");
	writeLog(triminfo(info), 0, 1);
}

void process_kafka_ssl_ca_location(ConfVal value) {
	size_t ca_len = strlen(value.strval) + 1;
	if (ca_len > 5) {
        	kafkaCALocation = malloc(ca_len);
		mem_k_alloc(kafkaCALocation, "kafka.ssl.ca.location");
        	if (kafkaCALocation== NULL) return;
		snprintf(kafkaCALocation, ca_len, "%s", value.strval);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.ssl.ca.location is set to '%s'.", kafkaCALocation);
		writeLog(triminfo(info), 0, 1);
	}
	else
		writeLog("[mod_kafka] kafka.ssl.ca.location is set to NULL.", 0, 1);
}

void process_kafka_certificate(ConfVal value) {
	size_t this_len = strlen(value.strval) + 1;
	if (this_len > 5) {
		kafkaSSLCertificate = malloc(this_len);
		mem_k_alloc(kafkaSSLCertificate, "kafka.ssl.certificate.location");
		if (kafkaSSLCertificate == NULL) return;
		snprintf(kafkaSSLCertificate, this_len, "%s", value.strval);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.ssl.certificate.location is set to '%s'.", kafkaSSLCertificate);
		writeLog(triminfo(info), 0, 1);
	}
	else
		writeLog("[mod_kafka] kafka.ssl.certificate.location is set to NULL.", 0, 1);
}

void process_kafka_key(ConfVal value) {
	size_t k_len = strlen(value.strval) + 1;
	if (k_len > 5) {
		kafka_SSLKey = malloc(k_len);
		mem_k_alloc(kafka_SSLKey, "kafka.ssl.key.location");
		if (kafka_SSLKey == NULL) return;
		snprintf(kafka_SSLKey, k_len, "%s", value.strval);
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.ssl.key.location is set to '%s'.", kafka_SSLKey);
		writeLog(triminfo(info), 0, 1);
	}
	else
		writeLog("[mod_kafka] kafka.ssl.key.location is set to NULL.", 0, 1);
} 

void process_kafka_security_protocol(ConfVal value) {
	if (strlen(value.strval) > 2) {
		size_t s_len = strlen(value.strval) + 1;
		kafka_security_protocol = malloc(s_len);
		mem_k_alloc(kafka_security_protocol, "kafka.security.protocol");
		if (kafka_security_protocol == NULL) return;
		//snprintf(kafka_security_protocol, s_len, "%s", to_uppercase(value.strval));
		strncpy(kafka_security_protocol, value.strval, s_len);
		kafka_security_protocol[s_len -1] = '\0';
		to_uppercase(kafka_security_protocol);
		char *trimmed = triminfo(kafka_security_protocol);
		if ((strcmp(trimmed, "PLAINTEXT") == 0) ||
			(strcmp(trimmed, "SSL") == 0) ||
			(strcmp(trimmed, "SASL_PLAINTEXT") == 0) ||
			(strcmp(trimmed, "SASL_SSL") == 0) ||
			(strcmp(trimmed, "SASL") == 0)) {
			snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.security.protocol is set to '%s'.", kafka_security_protocol);
			writeLog(triminfo(info), 0, 1);

		}
		else {
			snprintf(info, INFO_STR_SIZE, "[mod_kafka] %s is not a valid entry for kafka.security.protocol.", value.strval);
			writeLog(triminfo(info), 1, 1);
			free(kafka_security_protocol);
			kafka_security_protocol = NULL;
			writeLog("[mod_kafka] kafka.security.protocol is set to NULL.", 0, 1);
		}
	}
	else {
		writeLog("[mod_kafka] kafka.security.protocol is set to NULL.", 0, 1);
	}
}

void process_kafka_sasl_mechanisms(ConfVal value) {
	size_t m_len = strlen(value.strval) + 1;
	kafka_sasl_mechanisms = malloc(m_len);
	mem_k_alloc(kafka_sasl_mechanisms, "kafka.sasl.mechanisms");
	if (kafka_sasl_mechanisms == NULL) return;
	//snprintf(kafka_sasl_mechanisms, m_len, "%s", value.strval);
	strncpy(kafka_sasl_mechanisms, value.strval, m_len);
	kafka_sasl_mechanisms[m_len -1] = '\0';
	to_uppercase(kafka_sasl_mechanisms);
	char *trimmed = triminfo(kafka_sasl_mechanisms);
	if (strcmp(trimmed, "PLAIN") == 0) {
		writeLog("[mod_kafka] Kafka security mechanism is set to 'PLAIN'.", 0, 1);
	}
	else if (strcmp(trimmed, "SCRAM-SHA-256") == 0) {
		writeLog("[mod_kafka] Kafka security mechanism is set to SCRAM-SHA-256.", 0, 1);
	}
	else if (strcmp(trimmed, "SCRAM-SHA-512") == 0) {
                writeLog("[mod_kafka] Kafka security mechanism is set to SCRAM-SHA-512.", 0, 1);
	}
	else if (strcmp(trimmed, "GSSAPI") == 0) {
                writeLog("[mod_kafka] Kafka security mechanism is set to GSSAPI.", 0, 1);
	}
	else if (strcmp(trimmed, "OAUTHBEARER") == 0) {
                writeLog("[mod_kafka] Kafka security mechanism is set to OAUTHBEARER.", 0, 1);
	}
	else if (strcmp(trimmed, "AWS_MSK_IAM") == 0) {
                writeLog("[mod_kafka] Kafka security mechanism is set to AWS_MSK_IAM.", 0, 1);
	}
	else {
		if (m_len > 1) {
			snprintf(info, INFO_STR_SIZE, "[mod_kafka] '%s' is not a valid entry for kafka.sasl.mechanims.", kafka_sasl_mechanisms);
			writeLog(triminfo(info), 1, 1);
		}
		else {
			writeLog("[mod_kafka] kafka.sasl.mechanisms is set to NULL.", 0, 1);
		}
		free(kafka_sasl_mechanisms);
		kafka_sasl_mechanisms = NULL;
		writeLog("[mod_kafka] kafka.sasl.mechanism is set to NULL.", 0, 1);
	}
}

void process_kafka_sasl_username(ConfVal value) {
        size_t u_len = strlen(value.strval) + 1;
        kafka_sasl_username = malloc(u_len);
        mem_k_alloc(kafka_sasl_username, "kafka.sasl.username");
        if (kafka_sasl_username == NULL) return;
	if (u_len <= 1) {
		free(kafka_sasl_username);
		kafka_sasl_username = NULL;
		writeLog("[mod_kafka] kafka.sasl.username is empty. Value is set to NULL.", 0, 1);
		return;
	}
        snprintf(kafka_sasl_username, u_len, "%s", value.strval);
	snprintf(info, INFO_STR_SIZE, "[mod_kafka] kafka.sasl.username is set to '%s'.", kafka_sasl_username);
	writeLog(triminfo(info), 0, 1);
}

void process_kafka_sasl_password(ConfVal value) {
        size_t p_len = strlen(value.strval) + 1;
	if (p_len > 2) {
        	kafka_sasl_password = malloc(p_len);
        	mem_k_alloc(kafka_sasl_password, "kafka.sasl.password");
        	if (kafka_sasl_mechanisms == NULL) return;
        	snprintf(kafka_sasl_password, p_len, "%s", value.strval);
		writeLog("[mod_kafka] Kafka.sasl.password is set.", 0, 1);
	}
	else
		writeLog("[mod_kafka] kafka.sasl.password is set to NULL.", 0, 1);
}

void process_kafka_socket_timeout(ConfVal value) {
	int i = value.intval;
	if (i <= 0) i = 30000;
	kafka_socket_timeout = i;
	processIntConfigInfo("kafka.socket.timeout.ms", kafka_socket_timeout);
}

void process_kafka_socket_blocking(ConfVal value) {
	int i = value.intval;
        if (i <= 0) i = 1000;
        kafka_socket_blocking = i;
        processIntConfigInfo("kafka.socket.blocking.max.ms", kafka_socket_blocking);
}

void process_kafka_transaction_timeout(ConfVal value)  {
	int i = value.intval;
	if (i <= 0) i = 30000;
	timeout_ms = i;
	processIntConfigInfo("Kafka transaction timeout", timeout_ms);
}

void process_kafka_partitioner(ConfVal value) {
	size_t p_len = strlen(value.strval) + 1;
	kafka_partitioner = malloc(p_len);
	mem_k_alloc(kafka_partitioner, "kafka.partitioner");
	if (kafka_partitioner == NULL) return;
	snprintf(kafka_partitioner, p_len, "%s", value.strval);
	if (strcmp(triminfo(kafka_partitioner), "consistent_random") == 0) {
		writeLog("[mod_kafka] Kafka partitioner is set to 'consistent_random'.", 0, 1);
	}
	else if (strcmp(triminfo(kafka_partitioner), "random") == 0) {
		writeLog("[mod_kafka] Kafka partitioner is set to 'random'.", 0, 1);
	}
	else if (strcmp(triminfo(kafka_partitioner), "consistent") == 0) {
                writeLog("[mod_kafka] Kafka partitioner is set to 'consistent'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_partitioner), "murmur2") == 0) {
                writeLog("[mod_kafka] Kafka partitioner is set to 'murmur2'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_partitioner), "murmur2_random") == 0) {
                writeLog("[mod_kafka] Kafka partitioner is set to 'murmur2_random'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_partitioner), "fnvla") == 0) {
                writeLog("[mod_kafka] Kafka partitioner is set to 'fnvla'.", 0, 1);
        }
	else if (strcmp(triminfo(kafka_partitioner), "fnvla2_random") == 0) {
                writeLog("[mod_kafka] Kafka partitioner is set to 'fnvla2_random'.", 0, 1);
        }
	else {
		snprintf(info, INFO_STR_SIZE, "[mod_kafka] '%s' is not a valid value for kafka.partioner.", kafka_partitioner);
		writeLog(triminfo(info), 1, 1);
		snprintf(kafka_partitioner, p_len, "%s", "consistent_random");
		writeLog("[mod_kafka] Kafka partitioner will be set to 'consistent_random'.", 0, 1);
	}
	/*
	rd_kafka_conf_t *conf = rd_kafka_conf_new();
	char errstr[512];

	rd_kafka_conf_set(conf, "partitioner", "murmur2_random", errstr, sizeof(errstr));
	*/
}

static int getKafkaConfiguration() {
        char* line = NULL;
        size_t len = 0;
        ssize_t read;
        FILE *fp = NULL;
        int index = 0;
        fp = fopen(configFile, "r");
        char confName[MAX_STRING_SIZE] = "";
        char confValue[MAX_STRING_SIZE] = "";

        if (fp == NULL)
        {
                perror("Error while opening the Kafka configuration file.\n");
                writeLog("Error opening Kafka configuration file.", 2, 1);
                exit(EXIT_FAILURE);
        }

        while ((read = getline(&line, &len, fp)) != -1) {
                char * token = strtok(line, "=");
                while (token != NULL) {
                        if (index == 0) {
                                snprintf(confName, sizeof(confName), "%s", token);
                        }
                        else {
                                snprintf(confValue, sizeof(confValue), "%s", token);
                        }
                        token = strtok(NULL, "=");
                        index++;
                        if (index == 2) index = 0;
                }
                ConfVal cvu;
                cvu.intval = strtol(triminfo(confValue), NULL, 0);
                cvu.strval = triminfo(confValue);
                for (int i = 0; i < sizeof(kafka_entries)/sizeof(ConfigEntry);i++) {
                        if (strcmp(confName, kafka_entries[i].name) == 0) {
                                kafka_entries[i].process(cvu);
                                break;
                        }
                }
        }
        fclose(fp);
        fp = NULL;
        if (line){
                free(line);
                line = NULL;
        }
        if (config_k_memalloc_fails > 0) {
                config_k_memalloc_fails = 0;
		return 2;
        }
        return 0;
}

static void initConfigFile() {
	if (configFile == NULL) {
		configFile = malloc(strlen("/etc/almond/kafka.conf") + 1);
		if (configFile != NULL) {
			snprintf(configFile, strlen("/etc/almond/kafka.conf") + 1, "%s", "/etc/almond/kafka.conf");
		}
		else {
			fprintf(stderr, "[mod_kafka] Memory allocation failed for Kafka config file.");
			writeLog("[mod_kafka] Memory allocation failed for Kafka config file.", 2, 0);
		}
	}
}

void setKafkaConfigFile(const char* configPath) {
	if (configPath == NULL) return;
	size_t len = strlen(configPath) + 1;
	char* tmp = malloc(len);
	if (tmp == NULL) {
		fprintf(stderr, "[mod_kafka] Failed to allocate memory for config file\n");
		writeLog("[mod_kafka] Memory allocation failed in 'setConfigFile'.", 2, 0);
		writeLog("[mod_kafka] Could not change config file value.", 1, 0);
		return;
	}
	snprintf(tmp, len, "%s", configPath);
	if (configFile != NULL) {
		free(configFile);
	}
	configFile = tmp;
}

void setKafkaTopic(const char* topicName) {
	if (topicName == NULL) return;
	size_t len = strlen(topicName) + 1;
	char* tmp = malloc(len);
	if (tmp == NULL) {
                fprintf(stderr, "[mod_kafka] Failed to allocate memory for topic name.\n");
                writeLog("[mod_kafka] Memory allocation failed in 'setKafkaTopic'.", 2, 0);
                writeLog("[mod_kafka] Could not change topic name.", 1, 0);
                return;
        }
	snprintf(tmp, len, "%s", topicName);
	if (topic != NULL) {
		free(topic);
	}
	topic = tmp;
}

char* getKafkaTopic(void) {
	return topic;
}

int loadKafkaConfig() {
	initConfigFile();
	if (configFile == NULL) return 2;
	if (getKafkaConfiguration() == 0) {
		if (requirements_met == 2) {
			return 0;
		}
	}
	return 1;
}

static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
	char info[810];
	if (rkmessage->err) {
                fprintf(stderr, "%% Message delivery failed: %s\n",
                        rd_kafka_err2str(rkmessage->err));
		snprintf(info, 810, "%% Message delivery failed: %s", rd_kafka_err2str(rkmessage->err));
		writeLog(triminfo(info), 1, 0);
	}
        else {
		snprintf(info, 810, "%% Message delivered (%zd bytes, "
                        "partition %" PRId32 ")",
			rkmessage->len, rkmessage->partition);
		writeLog(triminfo(info), 0, 0);
	}
        /* The rkmessage is destroyed automatically by librdkafka */
}

static int set_kafka_conf(rd_kafka_conf_t *conf, const char *key, const char *value) {
	char errstr[512];
	if (rd_kafka_conf_set(conf, key, value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		printf("[mod_kafka] %s\n", errstr);
        	writeLog(errstr, 1, 0);
        	return 1;
    	}
	return 0;
}

int init_kafka_producer() {
	char acks_str[8];
	char value_str[32];
        char errstr[512];

	if (acks == -1) {
                strcpy(acks_str, "all");
        } 
	else {
		snprintf(acks_str, sizeof(acks_str), "%d", acks);
        }

	rd_kafka_conf_t *conf = rd_kafka_conf_new();

        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                writeLog(errstr, 1, 0);
                return 1;
        }
	if (set_kafka_conf(conf, "bootstrap.servers", brokers)) return 1;
	if (set_kafka_conf(conf, "client.id", kafka_client_id)) return 1;
	snprintf(value_str, sizeof(value_str), "%d", metadata_request_timeout);
	if (set_kafka_conf(conf, "metadata.max.age.ms", value_str)) return 1;
	snprintf(value_str, sizeof(value_str), "%d", linger_ms);
	if (set_kafka_conf(conf, "linger.ms", value_str)) return 1;
	snprintf(value_str, sizeof(value_str), "%d", queue_buffering_max_messages);
        if (set_kafka_conf(conf, "queue.buffering.max.messages", value_str)) return 1;
        if (set_kafka_conf(conf, "compression.codec", kafka_compression)) return 1; 
	snprintf(value_str, sizeof(value_str), "%d", message_max);
	if (set_kafka_conf(conf, "message.max.bytes", value_str)) return 1;
	snprintf(value_str, sizeof(value_str), "%d", message_copy_max);
        if (set_kafka_conf(conf, "message.copy.max.bytes", value_str)) return 1;
        snprintf(value_str, sizeof(value_str), "%d", request_timeout);
        if (set_kafka_conf(conf, "request.timeout.ms", value_str)) return 1;
        snprintf(value_str, sizeof(value_str), "%d", message_timeout_ms);
        if (set_kafka_conf(conf, "delivery.timeout.ms", value_str)) return 1;
        snprintf(value_str, sizeof(value_str), "%d", retry_backoff);
        if (set_kafka_conf(conf, "retry.backoff.ms", value_str)) return 1;
        snprintf(value_str, sizeof(value_str), "%d", statistics_interval);
	if (set_kafka_conf(conf, "statistics.interval.ms", value_str)) return 1;
	const char *log_close_str = kafka_log_connection_close ? "true" : "false";
	if (set_kafka_conf(conf, "log.connection.close", log_close_str)) return 1;
        if (set_kafka_conf(conf, "partitioner", kafka_partitioner)) return 1;
	if (enable_idempotence) { // and transactions!?
		const char *enable_idempotence_str = enable_idempotence ? "true" : "false";
		if (set_kafka_conf(conf, "enable.idempotence", enable_idempotence_str)) return 1;
		if (acks != -1) {
			writeLog("[mod_kafka] Setting kafka.acks to 'all' since you enabled idempotency.", 1, 0);
			snprintf(acks_str, sizeof(acks_str), "%s", "all");
		}
		if (set_kafka_conf(conf, "acks", acks_str)) return 1;
		if (retries > 0) {
			snprintf(value_str, sizeof(value_str), "%d", retries);
			if (set_kafka_conf(conf, "retries", value_str)) return 1;
		}
		else {
			writeLog("[mod_kafka] Setting kafka.retries to 5 since you enabled idempotency.", 1, 0);
			if (set_kafka_conf(conf, "retries", "5")) return 1;
		}
		if (max_in_flight_requests <= 5) {
			snprintf(value_str, sizeof(value_str), "%d", max_in_flight_requests);
			if (set_kafka_conf(conf, "max.in.flight.requests.per.connection", value_str)) return 1;
		}
		else {
			writeLog("[mod_kafka] Setting kafka.max.in.flight.requests.per.connection to 5 since idempotency is enabled.", 1, 0);
			if (set_kafka_conf(conf, "max.in.flight.requests.per.connection", "5")) return 1;
		}
		if (transactional_id != NULL) {
			if (set_kafka_conf(conf, "transactional.id", transactional_id)) return 1;
			snprintf(value_str, sizeof(value_str), "%d", timeout_ms);
			if (set_kafka_conf(conf, "transaction.timeout.ms", value_str)) return 1;
			use_transactions = true;
		}
	}
	else {
		if (set_kafka_conf(conf, "acks", acks_str)) return 1;
		snprintf(value_str, sizeof(value_str), "%d", retries);
		if (set_kafka_conf(conf, "retries", value_str)) return 1;
		snprintf(value_str, sizeof(value_str), "%d", max_in_flight_requests);
		if (set_kafka_conf(conf, "max.in.flight.requests.per.connection", value_str)) return 1;
	}
	if (kafka_security_protocol != NULL) {
		if (set_kafka_conf(conf, "security.protocol", kafka_security_protocol)) return 1;
		if (kafka_sasl_mechanisms != NULL) {
			if ((strcmp(kafka_security_protocol, "SASL_PLAINTEXT") == 0) || (strcmp(kafka_security_protocol,  "SASL_SSL") == 0)) {
				writeLog("[mod_kafka] kafka.sasl.mechanisms requiere SASL_SSL or SASL_PLAINTEXT which is not set.", 1, 1);
				snprintf(info, INFO_STR_SIZE, "[mod_kafka] You need to change kafka.security.protocol currently set as '%s'.", kafka_security_protocol);
				writeLog(triminfo(info), 1, 0);
				writeLog("[mod_kafka] Almond will not initiate kafka.sasl.mechanisms.", 0, 0);
			}
			else {
				if (set_kafka_conf(conf, "sasl.mechanisms", kafka_sasl_mechanisms)) return 1;
				if (set_kafka_conf(conf, "sasl.username", kafka_sasl_username)) return 1;
				if (set_kafka_conf(conf, "sasl.password", kafka_sasl_password)) return 1;
			}
		}
		if (kafkaCALocation != NULL) {
			if (set_kafka_conf(conf, "ssl.ca.location", kafkaCALocation)) return 1;
		}
	}
	if (kafkaSSLCertificate != NULL) {
		if (set_kafka_conf(conf, "ssl.certificate.location", kafkaSSLCertificate)) return 1;
		if (kafka_SSLKey != NULL) {
			if (set_kafka_conf(conf, "ssl.key.location", kafka_SSLKey)) return 1;
		}
	}
	snprintf(value_str, sizeof(value_str), "%d", kafka_socket_timeout);
	if (set_kafka_conf(conf, "socket.timeout.ms", value_str)) return 1;
	snprintf(value_str, sizeof(value_str), "%d", kafka_socket_blocking);
	if (set_kafka_conf(conf, "socket.blocking.max.ms", value_str)) return 1; 

        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

        global_producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!global_producer) {
                writeLog(errstr, 2, 0);
                return 1;
        }
	if (use_transactions) {
    		if (rd_kafka_init_transactions(global_producer, timeout_ms) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        		writeLog("[mod_kafka] Failed to initialize transactions", 2, 0);
        		return 1;
    		}
	}
        return 0;
}

int send_message_to_gkafka(const char *payload) {
	size_t plen = strlen(payload);
    	rd_kafka_resp_err_t err;

	if (use_transactions) {
		rd_kafka_begin_transaction(global_producer);
	}

	retry:
    		err = rd_kafka_producev(global_producer,
            		RD_KAFKA_V_TOPIC(topic),
            		RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            		RD_KAFKA_V_VALUE((void *)payload, plen),
            		RD_KAFKA_V_OPAQUE(NULL),
            		RD_KAFKA_V_END);

    	if (err) {
        	writeLog(rd_kafka_err2str(err), 1, 0);
        	if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
			if (use_transactions) {
				rd_kafka_abort_transaction(global_producer, timeout_ms);
			}
            		rd_kafka_poll(global_producer, 1000);
            		goto retry;
        	}
    	} 
	else {
        	writeLog("Message enqueued", 0, 0);
    	}
	if (use_transactions)
		rd_kafka_commit_transaction(global_producer, timeout_ms);
	else
    		rd_kafka_poll(global_producer, 0);
   	return 0;
}

int send_avro_message_to_gkafka(const char *name,
                                const char *id,
                                const char *tag,
                                const char *lastChange,
                                const char *lastRun,
                                const char *dataName,
                                const char *nextRun,
                                const char *pluginName,
                                const char *pluginOutput,
                                const char *pluginStatus,
                                const char *pluginStatusChanged,
                                int pluginStatusCode) {
	writeLog("Writing with schema registry and avro not enabled in this Almond version.", 2, 0);
	writeLog("To enable Avro recompile Almond with mod_avro.", 0, 0);
        return -1;
}

int send_message_to_kafka(char *brokers, char *topic, char *payload) {
        char errstr[512];
	char info[812];
        rd_kafka_t *producer;        /* Producer instance handle */
        rd_kafka_conf_t *conf; /* Temporary configuration object */
	
        conf = rd_kafka_conf_new();
        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        	fprintf(stderr,"%s\n", errstr);
		writeLog(errstr, 1, 0);
        }

        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
        producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!producer) {
                fprintf(stderr, "Failed to create producer: %s\n", errstr);
		snprintf(info, 810, "Failed to create producer: %s", errstr);
		writeLog(triminfo(info), 2, 0);
                return 1;
        }

        if (rd_kafka_brokers_add(producer, brokers) == 0) {
                fprintf(stderr, "Failed to add brokers: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
		snprintf(info, 810, "Failed to add brokers: %s", rd_kafka_err2str(rd_kafka_last_error()));
		writeLog(triminfo(info), 2, 0);
                rd_kafka_destroy(producer);
                return 1;
        }

        size_t plen = strlen(payload);
        retry:
                rd_kafka_resp_err_t err  = rd_kafka_producev(producer,
                                RD_KAFKA_V_TOPIC(topic),
                                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                                RD_KAFKA_V_VALUE(payload, plen),
                                RD_KAFKA_V_OPAQUE(NULL),
                                RD_KAFKA_V_END);

                if (err) {
			snprintf(info, 810, "%% Failed to produce topic %s: %s", topic, rd_kafka_err2str(err));
			writeLog(triminfo(info), 1, 0);
                        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                                rd_kafka_poll(producer, 1000);
                                goto retry;
                        }
                }
                else {
			snprintf(info, 810, "%% Enqueued message (%zd bytes) "
                                       "for topic %s", 
				       plen, topic);
			writeLog(triminfo(info), 0, 0);
                }
                rd_kafka_poll(producer, 0);
	writeLog("Flushing final Kafka messages...", 0, 0);
        rd_kafka_flush(producer, 10 * 1000 /* wait for max 10 seconds */);
        if (rd_kafka_outq_len(producer) > 0) {
		snprintf(info, 810, "%% %d message(s) were not delivered", rd_kafka_outq_len(producer));
		writeLog(triminfo(info), 1, 0);
	}
	/* Destroy the producer instance */
        rd_kafka_destroy(producer);
        return 0;
}

int send_ssl_message_to_kafka(char *brokers, char *cacertificate, char *certificate, char *key, char *topic,char *payload) {
        char errstr[512];
        char info[812];
        rd_kafka_t *producer;        /* Producer instance handle */
        rd_kafka_conf_t *conf; /* Temporary configuration object */

        conf = rd_kafka_conf_new();
        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1, 0);
        }
	rd_kafka_conf_set(conf, "enable.ssl.certificate.verification", "false", errstr, sizeof(errstr));
	rd_kafka_conf_set(conf, "security.protocol", "SSL", errstr, sizeof(errstr));
	if (rd_kafka_conf_set(conf, "ssl.ca.location", cacertificate, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1, 0);
        }
	if (rd_kafka_conf_set(conf, "ssl.certificate.location", certificate, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1, 0);
        }
	if (rd_kafka_conf_set(conf, "ssl.key.location", key, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1, 0);
        }

        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
        producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!producer) {
                fprintf(stderr, "Failed to create producer: %s\n", errstr);
                snprintf(info, 810, "Failed to create producer: %s", errstr);
                writeLog(triminfo(info), 2, 0);
                return 1;
        }

        if (rd_kafka_brokers_add(producer, brokers) == 0) {
                fprintf(stderr, "Failed to add brokers: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
                snprintf(info, 810, "Failed to add brokers: %s", rd_kafka_err2str(rd_kafka_last_error()));
                writeLog(triminfo(info), 2, 0);
                rd_kafka_destroy(producer);
                return 1;
        }
        size_t plen = strlen(payload);
        retry:
                rd_kafka_resp_err_t err  = rd_kafka_producev(producer,
                                RD_KAFKA_V_TOPIC(topic),
                                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                                RD_KAFKA_V_VALUE(payload, plen),
                                RD_KAFKA_V_OPAQUE(NULL),
                                RD_KAFKA_V_END);

                if (err) {
                        snprintf(info, 810, "%% Failed to produce topic %s: %s", topic, rd_kafka_err2str(err));
                        writeLog(triminfo(info), 1, 0);
                        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                                rd_kafka_poll(producer, 1000);
                                goto retry;
                        }
                }
                else {
                        snprintf(info, 810, "%% Enqueued message (%zd bytes) "
                                       "for topic %s",
                                       plen, topic);
                        writeLog(triminfo(info), 0, 0);
                }
                rd_kafka_poll(producer, 0);
        writeLog("Flushing final Kafka messages...", 0, 0);
        rd_kafka_flush(producer, 10 * 1000 /* wait for max 10 seconds */);
        if (rd_kafka_outq_len(producer) > 0) {
                snprintf(info, 810, "%% %d message(s) were not delivered", rd_kafka_outq_len(producer));
                writeLog(triminfo(info), 1, 0);
        }
        /* Destroy the producer instance */
        rd_kafka_destroy(producer);
	return 0;
}

int send_avro_message_to_kafka(char *brokers, char *topic,
                              const char *name,
                              const char *id,
                              const char *tag,
                              const char *lastChange,
                              const char *lastRun,
                              const char *dataName,
                              const char *nextRun,
                              const char *pluginName,
                              const char *pluginOutput,
                              const char *pluginStatus,
                              const char *pluginStatusChanged,
                              int pluginStatusCode) {
	writeLog("Writing with schema registry and avro not enabled in this Almond version.", 2, 0);
	return -1;
}

int send_ssl_avro_message_to_kafka(char *brokers, char *cacertificate, char *certificate, char *key, char *topic,
                              const char *name,
                              const char *id,
                              const char *tag,
                              const char *lastChange,
                              const char *lastRun,
                              const char *dataName,
                              const char *nextRun,
                              const char *pluginName,
                              const char *pluginOutput,
                              const char *pluginStatus,
                              const char *pluginStatusChanged,
                              int pluginStatusCode) {
	writeLog("Writing with schema registry and avro not enabled in this Almond version.", 2, 0);
	return -1;
}

static void shutdown_kafka_producer() {
    	rd_kafka_flush(global_producer, 10000);
    	rd_kafka_destroy(global_producer);
}

void free_kafka_memalloc() {
	shutdown_kafka_producer();
	if (brokers != NULL) {
		free(brokers);
		brokers = NULL;
	}
	if (topic != NULL) {
		free(topic);
		topic = NULL;
	}
	if (kafka_client_id != NULL) {
		free(kafka_client_id);
		kafka_client_id = NULL;
	}
	if (kafka_partitioner != NULL) {
		free(kafka_partitioner);
		kafka_partitioner = NULL;
	}
	if (transactional_id != NULL) {
		free(transactional_id);
		transactional_id = NULL;
	}
	if (kafka_compression != NULL) {
		free(kafka_compression);
		kafka_compression = NULL;
	}
	if (kafkaCALocation != NULL) {
		free(kafkaCALocation);
		kafkaCALocation = NULL;
	}
	if (kafkaSSLCertificate != NULL) {
		free(kafkaSSLCertificate);
		kafkaSSLCertificate = NULL;
	}
	if (kafka_SSLKey != NULL) {
		free(kafka_SSLKey);
		kafka_SSLKey = NULL;
	}
	if (kafka_sasl_mechanisms != NULL) {
		free(kafka_sasl_mechanisms);
		kafka_sasl_mechanisms = NULL;
	}
	if (kafka_sasl_username != NULL) {
		free(kafka_sasl_username);
		kafka_sasl_username = NULL;
	}
	if (kafka_sasl_password != NULL) {
		free(kafka_sasl_password);
		kafka_sasl_password = NULL;
	}
	if (kafka_security_protocol != NULL) {
		free(kafka_security_protocol);
		kafka_security_protocol = NULL;
	}
}
