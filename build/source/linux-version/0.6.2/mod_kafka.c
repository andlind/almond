#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <librdkafka/rdkafka.h>
#include "main.h"
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
		writeLog(triminfo(info), 1);
	}
        else {
                /*fprintf(stderr,
                        "%% Message delivered (%zd bytes, "
                        "partition %" PRId32 ")\n",
                        rkmessage->len, rkmessage->partition);*/
		snprintf(info, 810, "%% Message delivered (%zd bytes, "
                        "partition %" PRId32 ")",
			rkmessage->len, rkmessage->partition);
		writeLog(triminfo(info), 0);
	}
        /* The rkmessage is destroyed automatically by librdkafka */
}

int send_message_to_kafka(char *brokers, char *topic, char *payload) {
	//char *brokers = "172.21.0.3:9092";
	//char *topic = "almond_monitoring";
        char errstr[512];
	char info[812];
        rd_kafka_t *producer;        /* Producer instance handle */
        rd_kafka_conf_t *conf; /* Temporary configuration object */

        conf = rd_kafka_conf_new();
        if (rd_kafka_conf_set(conf, "bootstrap.servers", brokers, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        	fprintf(stderr,"%s\n", errstr);
		writeLog(errstr, 1);
        }

        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
        producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!producer) {
                fprintf(stderr, "Failed to create producer: %s\n", errstr);
		snprintf(info, 810, "Failed to create producer: %s", errstr);
		writeLog(triminfo(info), 2);
                return 1;
        }

        if (rd_kafka_brokers_add(producer, brokers) == 0) {
                fprintf(stderr, "Failed to add brokers: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
		snprintf(info, 810, "Failed to add brokers: %s", rd_kafka_err2str(rd_kafka_last_error()));
		writeLog(triminfo(info), 2);
                rd_kafka_destroy(producer);
                return 1;
        }

        //char *payload = "{\"name\":\"tmauv13.test.almond.com\"}, {\"lastChange\": \"2023-04-14 10:12:14\", \"lastRun\": \"2023-06-21 09:22:28\", \"name\": \"check_ping\", \"nextRun\": \"2023-06-21 09:23:28\", \"pluginName\": \"Its alive\", \"pluginOutput\": \"PING OK - Packet loss = 0%, RTA = 0.03 ms|rta=0.028000ms;100.000000;500.000000;0.000000 pl=0%;20;60;0\", \"pluginStatus\": \"OK\", \"pluginStatusChanged\": \"0\", \"pluginStatusCode\": \"0\"}";
        size_t plen = strlen(payload);
        retry:
                rd_kafka_resp_err_t err  = rd_kafka_producev(producer,
                                RD_KAFKA_V_TOPIC(topic),
                                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                                RD_KAFKA_V_VALUE(payload, plen),
                                RD_KAFKA_V_OPAQUE(NULL),
                                RD_KAFKA_V_END);

                if (err) {
                        //fprintf(stderr, "%% Failed to produce topic %s: %s\n", topic, rd_kafka_err2str(err));
			snprintf(info, 810, "%% Failed to produce topic %s: %s", topic, rd_kafka_err2str(err));
			writeLog(triminfo(info), 1);
                        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                                rd_kafka_poll(producer, 1000);
                                goto retry;
                        }
                }
                else {
                        /*fprintf(stderr, "%% Enqueued message (%zd bytes) "
                                       "for topic %s\n",
                                        plen, topic);*/
			snprintf(info, 810, "%% Enqueued message (%zd bytes) "
                                       "for topic %s", 
				       plen, topic);
			writeLog(triminfo(info), 0);
                }
                rd_kafka_poll(producer, 0);
        //fprintf(stderr, "%% Flushing final messages..\n");
	writeLog("Flushing final Kafka messages...", 0);
        rd_kafka_flush(producer, 10 * 1000 /* wait for max 10 seconds */);
        if (rd_kafka_outq_len(producer) > 0) {
                /*fprintf(stderr, "%% %d message(s) were not delivered\n",
                        rd_kafka_outq_len(producer));*/
		snprintf(info, 810, "%% %d message(s) were not delivered", rd_kafka_outq_len(producer));
		writeLog(triminfo(info), 1);
	}
	/* Destroy the producer instance */
        rd_kafka_destroy(producer);

        //rd_kafka_topic_t *kafka_topic = rd_kafka_topic_new(producer, topic, NULL);

        /*rd_kafka_message_t *rkmessage = rd_kafka_message_new();
        rd_kafka_message_set_topic(rkmessage, topic);
        char *payload = "{\"name\":\"tmauv13.test.almond.com\"}, {\"lastChange\": \"2023-04-14 10:12:14\", \"lastRun\": \"2023-06-21 09:22:28\", \"name\": \"check_ping\", \"nextRun\": \"2023-06-21 09:23:28\", \"pluginName\": \"Its alive\", \"pluginOutput\": \"PING OK - Packet loss = 0%, RTA = 0.03 ms|rta=0.028000ms;100.000000;500.000000;0.000000 pl=0%;20;60;0\", \"pluginStatus\": \"OK\", \"pluginStatusChanged\": \"0\", \"pluginStatusCode\": \"0\"}"
        rd_kafka_message_set_payload(rkmessage, payload,strlen(payload));
        if (rd_kafka_produce(kafka_topic, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, rkmessage->payload, rkmessage->len, NULL, 0, NULL) == -1) {
                fprintf(stderr, "Failed to produce message: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
                rd_kafka_message_destroy(rkmessage);
                rd_kafka_topic_destroy(kafka_topic);
                rd_kafka_destroy(producer);
                return 1;
        }
        rd_kafka_poll(producer,0);
        rd_kafka_message_destroy(rkmessage);
        rd_kafka_flush(producer, 5000);
        rd_kafka_topic_destroy(kafka_topic);
        rd_kafka_destroy(producer);*/

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
                writeLog(errstr, 1);
        }
	rd_kafka_conf_set(conf, "enable.ssl.certificate.verification", "false", errstr, sizeof(errstr));
	rd_kafka_conf_set(conf, "security.protocol", "SSL", errstr, sizeof(errstr));
	if (rd_kafka_conf_set(conf, "ssl.ca.location", cacertificate, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1);
        }
	if (rd_kafka_conf_set(conf, "ssl.certificate.location", certificate, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1);
        }
	if (rd_kafka_conf_set(conf, "ssl.key.location", key, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                fprintf(stderr,"%s\n", errstr);
                writeLog(errstr, 1);
        }

        rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
        producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!producer) {
                fprintf(stderr, "Failed to create producer: %s\n", errstr);
                snprintf(info, 810, "Failed to create producer: %s", errstr);
                writeLog(triminfo(info), 2);
                return 1;
        }

        if (rd_kafka_brokers_add(producer, brokers) == 0) {
                fprintf(stderr, "Failed to add brokers: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
                snprintf(info, 810, "Failed to add brokers: %s", rd_kafka_err2str(rd_kafka_last_error()));
                writeLog(triminfo(info), 2);
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
                        writeLog(triminfo(info), 1);
                        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                                rd_kafka_poll(producer, 1000);
                                goto retry;
                        }
                }
                else {
                        snprintf(info, 810, "%% Enqueued message (%zd bytes) "
                                       "for topic %s",
                                       plen, topic);
                        writeLog(triminfo(info), 0);
                }
                rd_kafka_poll(producer, 0);
        writeLog("Flushing final Kafka messages...", 0);
        rd_kafka_flush(producer, 10 * 1000 /* wait for max 10 seconds */);
        if (rd_kafka_outq_len(producer) > 0) {
                snprintf(info, 810, "%% %d message(s) were not delivered", rd_kafka_outq_len(producer));
                writeLog(triminfo(info), 1);
        }
        /* Destroy the producer instance */
        rd_kafka_destroy(producer);
	return 0;
}
