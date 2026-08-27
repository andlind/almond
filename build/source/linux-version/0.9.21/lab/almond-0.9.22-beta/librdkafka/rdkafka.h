#ifndef _FAKE_RDKAFKA_H
#define _FAKE_RDKAFKA_H

#include <stdint.h>
#include <stddef.h>

typedef struct rd_kafka_s rd_kafka_t;
typedef struct rd_kafka_conf_s rd_kafka_conf_t;
typedef int rd_kafka_resp_err_t;
typedef struct rd_kafka_message_s {
    rd_kafka_resp_err_t err; /* delivery/report error */
    void *payload;           /* pointer to payload (if any) */
    size_t len;             /* payload length */
    int32_t partition;      /* partition message was produced to */
} rd_kafka_message_t;

#define RD_KAFKA_CONF_OK 0
#define RD_KAFKA_PRODUCER 1
#define RD_KAFKA_MSG_F_COPY 0x1
#define RD_KAFKA_RESP_ERR__QUEUE_FULL 1
#define RD_KAFKA_RESP_ERR_NO_ERROR 0
#define RD_KAFKA_RESP_ERR__UNKNOWN -1

/* Minimal macros used in code: these are no-ops for compile-time check */
#define RD_KAFKA_V_TOPIC(x) 0
#define RD_KAFKA_V_MSGFLAGS(x) 0
#define RD_KAFKA_V_VALUE(ptr, len) 0
#define RD_KAFKA_V_OPAQUE(x) 0
#define RD_KAFKA_V_END 0

/* Functions used (prototypes only) */
rd_kafka_conf_t *rd_kafka_conf_new(void);
int rd_kafka_conf_set(rd_kafka_conf_t *conf, const char *name, const char *value, char *errstr, size_t errstr_size);
void rd_kafka_conf_set_dr_msg_cb(rd_kafka_conf_t *conf, void (*dr_cb)(rd_kafka_t *, const rd_kafka_message_t *, void *));
rd_kafka_t *rd_kafka_new(int type, rd_kafka_conf_t *conf, char *errstr, size_t errstr_size);
int rd_kafka_init_transactions(rd_kafka_t *rk, int timeout_ms);
void rd_kafka_begin_transaction(rd_kafka_t *rk);
void rd_kafka_abort_transaction(rd_kafka_t *rk, int timeout_ms);
void rd_kafka_commit_transaction(rd_kafka_t *rk, int timeout_ms);
int rd_kafka_producev(rd_kafka_t *rk, ...);
void rd_kafka_poll(rd_kafka_t *rk, int timeout_ms);
int rd_kafka_brokers_add(rd_kafka_t *rk, const char *broker);
int rd_kafka_outq_len(rd_kafka_t *rk);
int rd_kafka_flush(rd_kafka_t *rk, int timeout_ms);
void rd_kafka_destroy(rd_kafka_t *rk);
const char *rd_kafka_err2str(rd_kafka_resp_err_t err);
rd_kafka_resp_err_t rd_kafka_last_error(void);

#endif /* _FAKE_RDKAFKA_H */
