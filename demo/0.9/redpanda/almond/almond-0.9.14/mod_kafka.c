#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <librdkafka/rdkafka.h>
#include "main.h"
#include "logger.h"
#include "mod_kafka.h"

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
