#define _POSIX_C_SOURCE 200809L
#define SERDES_SCHEMA_AVRO 1
#define SERDES_SCHEMA_PROTOBUF 2
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avro.h>
#include <libserdes/serdes-avro.h>
#include <librdkafka/rdkafka.h>
#include "main.h"
#include "logger.h"
#include "mod_kafka.h"

extern char *schemaRegistryUrl;
extern char schemaName[100];

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

char *load_file(const char *filename, size_t *len_out) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char *buf = malloc(len + 1);
    if (!buf) return NULL;

    fread(buf, 1, len, fp);
    fclose(fp);

    buf[len] = '\0';
    if (len_out) *len_out = len;
    return buf;
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
        char errstr[512];
	char info[812];
        void *buffer = NULL;
	size_t length = 0;
	avro_value_t v, data;

        rd_kafka_t *producer;        /* Producer instance handle */
        rd_kafka_conf_t *conf; /* Temporary configuration object */

	if (schemaRegistryUrl == NULL) {
		writeLog("No url to schema registry found in config.", 2, 0);
		return -1;
	}
        else {
		//printf("DEBUG: schemaRegistryUrl = %s\n", schemaRegistryUrl);
	}
	
	serdes_conf_t *sconf = serdes_conf_new(errstr, sizeof(errstr), "schema.registry.url", schemaRegistryUrl, NULL);
	serdes_t *serdes = serdes_new(sconf, NULL, 0);
        size_t schema_len;
       	char *schema_buf = load_file("plugin_status.avsc", &schema_len);

	// Schema registration
	serdes_schema_t *schema = serdes_schema_add(
    		serdes,                    // serdes_t pointer
    		schemaName,     	   // const char *name
    		-1,                       // int id (auto-register)
    		schema_buf,              // const void *definition
    		schema_len,              // int definition_len
    		errstr,                  // char *errstr
    		sizeof(errstr)           // int errstr_size
		);

	// Get Avro schema and create interface
        if (!schema) {
                fprintf(stderr, "Failed to register schema: %s\n", errstr);
		writeLog("Failed to register schema...", 2, 0);
                return -1; // or handle error appropriately
        }
	avro_schema_t avro_schema = serdes_schema_avro(schema);
	avro_value_iface_t *iface = avro_generic_class_from_schema(avro_schema);
	avro_value_t record;
	avro_generic_value_new(iface, &record);

	// Set record values
	avro_value_get_by_name(&record, "name", &v, NULL);
	avro_value_set_string(&v, name);
	avro_value_get_by_name(&record, "id", &v, NULL);
	avro_value_set_string(&v, id);
	avro_value_get_by_name(&record, "tag", &v, NULL);
	avro_value_set_string(&v, tag);

	// Set nested data values
	avro_value_get_by_name(&record, "data", &data, NULL);
	avro_value_get_by_name(&data, "lastChange", &v, NULL);
	avro_value_set_string(&v, lastChange);
	avro_value_get_by_name(&data, "lastRun", &v, NULL);
	avro_value_set_string(&v, lastRun);
	avro_value_get_by_name(&data, "name", &v, NULL);
	avro_value_set_string(&v, dataName);
	avro_value_get_by_name(&data, "nextRun", &v, NULL);
	avro_value_set_string(&v, nextRun);
	avro_value_get_by_name(&data, "pluginName", &v, NULL);
	avro_value_set_string(&v, pluginName);
	avro_value_get_by_name(&data, "pluginOutput", &v, NULL);
	avro_value_set_string(&v, pluginOutput);
	avro_value_get_by_name(&data, "pluginStatus", &v, NULL);
	avro_value_set_string(&v, pluginStatus);
	avro_value_get_by_name(&data, "pluginStatusChanged", &v, NULL);
	avro_value_set_string(&v, pluginStatusChanged);
	avro_value_get_by_name(&data, "pluginStatusCode", &v, NULL);
	avro_value_set_int(&v, pluginStatusCode);

	// Serialize the record
	serdes_err_t serr = serdes_schema_serialize_avro(
    		schema,           // serdes_schema_t *schema
    		&record,          // avro_value_t *value
    		&buffer,          // void **buffer
    		&length,          // size_t *length
		errstr,
		sizeof(errstr)
	);

	if (serr != SERDES_ERR_OK) {
		fprintf(stderr, "serialize_avro failed: %s\n", serdes_err2str(serr));
	}

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

        retry:
                rd_kafka_resp_err_t err  = rd_kafka_producev(producer,
                                RD_KAFKA_V_TOPIC(topic),
                                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                                RD_KAFKA_V_VALUE(buffer,length),
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
				       length, topic);
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
	/* Destroy serdes */
	free(buffer);
	serdes_destroy(serdes);
        return 0;
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

	char errstr[512];
        char info[812];
        void *buffer = NULL;
        size_t length = 0;
        avro_value_t v, data;

        rd_kafka_t *producer;        /* Producer instance handle */
        rd_kafka_conf_t *conf; /* Temporary configuration object */

        serdes_conf_t *sconf = serdes_conf_new(errstr, sizeof(errstr), "schema.registry.url", schemaRegistryUrl, NULL);
        serdes_t *serdes = serdes_new(sconf, NULL, 0);
        size_t schema_len;
        char *schema_buf = load_file("plugin_status.avsc", &schema_len);

	if (schemaRegistryUrl == NULL) {
                writeLog("No url to schema registry found in config.", 2, 0);
                return -1;
        }
        
	serdes_schema_t *schema = serdes_schema_add(
                serdes,                    // serdes_t pointer
                "PluginStatusMessage",     // const char *name
                -1,                       // int id (auto-register)
                schema_buf,              // const void *definition
                schema_len,              // int definition_len
                errstr,                  // char *errstr
                sizeof(errstr)           // int errstr_size
        );

        avro_schema_t avro_schema = serdes_schema_avro(schema);
        avro_value_iface_t *iface = avro_generic_class_from_schema(avro_schema);
        avro_value_t record;
        avro_generic_value_new(iface, &record);

        avro_value_get_by_name(&record, "name", &v, NULL);
        avro_value_set_string(&v, name);
        avro_value_get_by_name(&record, "id", &v, NULL);
        avro_value_set_string(&v, id);
        avro_value_get_by_name(&record, "tag", &v, NULL);
        avro_value_set_string(&v, tag);
        avro_value_get_by_name(&record, "data", &data, NULL);
        avro_value_get_by_name(&data, "lastChange", &v, NULL);
        avro_value_set_string(&v, lastChange);
        avro_value_get_by_name(&data, "lastRun", &v, NULL);
        avro_value_set_string(&v, lastRun);
        avro_value_get_by_name(&data, "name", &v, NULL);
        avro_value_set_string(&v, dataName);
        avro_value_get_by_name(&data, "nextRun", &v, NULL);
        avro_value_set_string(&v, nextRun);
        avro_value_get_by_name(&data, "pluginName", &v, NULL);
        avro_value_set_string(&v, pluginName);
        avro_value_get_by_name(&data, "pluginOutput", &v, NULL);
        avro_value_set_string(&v, pluginOutput);
        avro_value_get_by_name(&data, "pluginStatus", &v, NULL);
        avro_value_set_string(&v, pluginStatus);
        avro_value_get_by_name(&data, "pluginStatusChanged", &v, NULL);
        avro_value_set_string(&v, pluginStatusChanged);
        avro_value_get_by_name(&data, "pluginStatusCode", &v, NULL);
        avro_value_set_int(&v, pluginStatusCode);

        serdes_err_t serr = serdes_schema_serialize_avro(
                schema,           // serdes_schema_t *schema
                &record,          // avro_value_t *value
                &buffer,          // void **buffer
                &length,          // size_t *length
                errstr,
                sizeof(errstr)
        );

        if (serr != SERDES_ERR_OK) {
                fprintf(stderr, "serialize_avro failed: %s\n", serdes_err2str(serr));
        }

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
        retry:
                rd_kafka_resp_err_t err  = rd_kafka_producev(producer,
                                RD_KAFKA_V_TOPIC(topic),
                                RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                                RD_KAFKA_V_VALUE(buffer, length),
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
                                       length, topic);
                        writeLog(triminfo(info), 0, 0);
                }
                rd_kafka_poll(producer, 0);
        writeLog("Flushing final Kafka messages...", 0, 0);
        rd_kafka_flush(producer, 10 * 1000);
        if (rd_kafka_outq_len(producer) > 0) {
                snprintf(info, 810, "%% %d message(s) were not delivered", rd_kafka_outq_len(producer));
                writeLog(triminfo(info), 1, 0);
        }
        rd_kafka_destroy(producer);
        free(buffer);
        serdes_destroy(serdes);
	return 0;
}
