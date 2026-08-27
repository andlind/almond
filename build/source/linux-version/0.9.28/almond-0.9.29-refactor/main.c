#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#ifndef VERSION
#define VERSION "0.9.28"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <zlib.h>
#include <stddef.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include "uthash.h"
#include "data.h"
#include "constants.h"
#include "configuration.h"
#include "logger.h"
#include "plugins.h"
#ifdef USE_AVRO 
#include "mod_avro.h"
#else
#include "mod_kafka.h"
#endif
#include "api.h"
#include "kafkaapi.h"
#include "main.h"
#include "jwt_validate.h"

#define MAX_COLUMNS 2
#define MAX_STRING_SIZE 50
#define MAX_CONSTANTS 50
#define JSON_OUTPUT 0
#define METRICS_OUTPUT 1
#define JSON_AND_METRICS_OUTPUT 2
#define PROMETHEUS_OUTPUT 3
#define JSON_AND_PROMETHEUS_OUTPUT 4
#define HOWRU_API 10 
#define KAFKA_EXPORT_TAG 10
#define KAFKA_EXPORT_ID 20
#define KAFKA_EXPORT_IDTAG 30
#define MAX_PLUGINS 256
#define TIME_BUF_LEN 80
#define MAX_THREAD_COUNT 4294967290
/*#define CMD_BUF_SIZE      1024
#define LINE_BUF_SIZE     1024
#define TIMESTAMP_SIZE     64*/
/*#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
#define HAS_BIRTHTIME 1
#else
#define HAS_BIRTHTIME 0
#endif*/

enum shutdown_reason {
    SR_NORMAL,
    SR_SIGINT,
    SR_SIGKILL,
    SR_SIGTERM,
    SR_SIGSTOP,
    SR_ERROR
};

GlobalDirectories g_dirs;
GlobalFiles g_files;
GlobalStrings g_strings;
GlobalBooleans g_bools = {.allowAllHosts = true};
GlobalIntegers g_ints = {.push_interval = 120, .schedulerSleep = 5000, .timeTunerMaster = 1, .timeTunerCycle = 15, .local_port = 9909, .max_try = 60};
GlobalSizes g_sizes = {.infostr_size = 400, .gardenermessage_size = 1035, .pluginmessage_size = 2300, .storename_size = 100, .apimessage_size = 2000, .socketservermessage_size = 2000, .socketclientmessage_size = 8192, .logmessage_size = 1545, .confdir_size = 50, .datadir_size = 50, .plugindeclarationfile_size = 75, .metricsoutputprefix_size = 30, .datafilename_size = 100, .jsonfilename_size = 50, .metricsfilename_size = 50, .gardenerscript_size = 75, .logdir_size = 50, .hostname_size = 255, .plugindir_size = 50, .pluginitemname_size = 50, .pluginitemdesc_size = 100, .pluginitemcmd_size = 255, .pluginoutput_size = 1500, .plugincommand_size = 100, .newfilename_size = 250, .storedir_size = 50, .backupdirectory_size = 100, .filename_size = 100, .logfile_size = 100, .max_timestamp_size = 64, .truncateLogInterval = 604800, .gardenerInterval = 43200, .clearDataCacheInterval = 300, .dataCacheTimeFrame = 330, .total_threads_run = 1};
GlobalArrays g_arrays;
GlobalTime g_time;
GlobalNetwork g_network = {.server_fd = -1};
GlobalPointers g_pointers;
static int g_current_scheduler_cnt = 0;
static pid_t plugin_pid_set[MAX_PLUGINS];
static pthread_mutex_t plugin_set_mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t already_exiting = 0;
GlobalThreading g_threading = {.shutdown_reason = SR_NORMAL};

void safe_free_str(char **ptr);
char constantsFile[26] = "/opt/almond/memalloc.alm";
char schemaName[100] = "almond-monitor-topic-value";
void flushLog();
int isConstantsEnabled();
int getConstants();
void initNewPlugin(int index);
void initScheduler(int, int, bool);
void runPluginCommand(int, char*);
//void runPlugin(int, int);
void runPluginArgs(int, int, int);
void executeGardener();
int initTimeScheduler(bool);
void sig_handler(int);
void process_almond_api(ConfVal);
void process_almond_port(ConfVal);
void process_almond_standalone(ConfVal);
void process_iam_issuer(ConfVal);
void process_iam_public_key_file(ConfVal);
void process_iam_aud(ConfVal);
void process_json_file(ConfVal);
void process_metrics_file(ConfVal);
void process_metrics_output_prefix(ConfVal);
void process_save_on_exit(ConfVal);
void process_plugin_declaration(ConfVal);
void process_plugin_directory(ConfVal);
void process_almond_certificate( ConfVal);
void process_clear_data_cache_interval(ConfVal);
void process_conf_dir(ConfVal);
void process_data_cache_time_frame(ConfVal);
void process_enable_clear_data_cache( ConfVal);
void process_enable_gardener(ConfVal);
void process_enable_kafka_export(ConfVal);
void process_enable_kafka_id(ConfVal);
void process_enable_kafka_ssl(ConfVal);
void process_enable_kafka_tags(ConfVal);
void process_almond_format(ConfVal);
void process_gardener_run_interval(ConfVal);
void process_gardener_script(ConfVal);
void process_host_name(ConfVal);
void process_init_sleep(ConfVal);
void process_kafka_brokers(ConfVal);
void process_kafka_ca_certificate(ConfVal);
void process_kafka_config_file(ConfVal);
void process_kafka_producer_certificate(ConfVal);
void process_kafka_start_id(ConfVal);
void process_kafka_tag(ConfVal);
void process_kafka_topic(ConfVal);
void process_almond_key(ConfVal);
void process_data_dir(ConfVal);
void process_log_dir(ConfVal);
void process_log_plugin_output(ConfVal);
void process_log_to_stdout(ConfVal);
void process_almond_quickstart(ConfVal);
void process_run_gardener_at_start(ConfVal);
void process_store_results(ConfVal);
void process_almond_sleep( ConfVal);
void process_store_dir(ConfVal);
void process_truncate_log(ConfVal);
void process_truncate_log_interval(ConfVal);
void process_tune_master(ConfVal);
void process_tune_cycle(ConfVal);
void process_tune_timer(ConfVal);
void process_almond_scheduler_type(ConfVal);
void process_almond_api_tls(ConfVal);
void process_external_scheduler(ConfVal);
void process_schema_registry_url(ConfVal);
void process_schema_name(ConfVal);
void process_use_kafka_config(ConfVal);
void writePluginResultToFile(int, int);
void writeToKafkaTopic(int, int);
void run_plugin(PluginItem *item);

ConfigEntry config_entries[] = {
    {"almond.api", process_almond_api},
    {"almond.certificate", process_almond_certificate},
    {"almond.g_bools.enableIamAud", process_enable_iam_aud},
    {"almond.enforceIAMRoles", process_enable_iam_roles},
    {"almond.iamAud", process_iam_aud},
    {"almond.iamIssuer", process_iam_issuer},
    {"almond.iamRolesAccepted", process_iam_roles_accepted},
    {"almond.iamPublicKeyFile", process_iam_public_key_file},
    {"almond.key", process_almond_key},
    {"almond.port", process_almond_port},
    {"almond.pushInterval", process_push_interval},
    {"almond.pushPort", process_push_port},
    {"almond.pushUrl", process_push_url},
    {"almond.g_bools.standalone", process_almond_standalone},
    {"almond.useMetricsPush", process_metrics_push},
    {"almond.usePush", process_almond_push},
    {"almond.useSSL", process_almond_api_tls},
    {"data.jsonFile", process_json_file},
    {"data.metricsFile", process_metrics_file},
    {"data.g_strings.metricsOutputPrefix", process_metrics_output_prefix},
    {"data.g_bools.saveOnExit", process_save_on_exit},
    {"plugins.declaration", process_plugin_declaration},
    {"plugins.directory", process_plugin_directory},
    {"g_pointers.scheduler.g_bools.allowAllHosts", process_allow_all_hosts},
    {"g_pointers.scheduler.certificate", process_almond_certificate},
    {"g_pointers.scheduler.g_sizes.clearDataCacheInterval", process_clear_data_cache_interval},
    {"g_pointers.scheduler.g_dirs.confDir", process_conf_dir},
    {"g_pointers.scheduler.g_sizes.dataCacheTimeFrame", process_data_cache_time_frame},
    {"g_pointers.scheduler.g_dirs.dataDir", process_data_dir},
    {"g_pointers.scheduler.g_bools.enableClearDataCache", process_enable_clear_data_cache},
    {"g_pointers.scheduler.g_bools.enableGardener", process_enable_gardener},
    {"g_pointers.scheduler.g_bools.enableKafkaExport", process_enable_kafka_export},
    {"g_pointers.scheduler.g_bools.enableKafkaId", process_enable_kafka_id},
    {"g_pointers.scheduler.g_bools.enableKafkaSSL", process_enable_kafka_ssl},
    {"g_pointers.scheduler.g_bools.enableKafkaTag", process_enable_kafka_tags},
    {"g_pointers.scheduler.format", process_almond_format},
    {"g_pointers.scheduler.gardenerRunInterval", process_gardener_run_interval},
    {"g_pointers.scheduler.g_files.gardenerScript", process_gardener_script},
    {"g_pointers.scheduler.g_strings.hostName", process_host_name},
    {"g_pointers.scheduler.initSleepMs", process_init_sleep},
    #ifdef USE_AVRO 
    {"g_pointers.scheduler.g_bools.kafkaAvro", process_kafka_avro},
    #endif
    {"g_pointers.scheduler.kafkaBrokers", process_kafka_brokers},
    {"g_pointers.scheduler.g_strings.kafkaCACertificate", process_kafka_ca_certificate},
    {"g_pointers.scheduler.g_strings.kafkaConfigFile", process_kafka_config_file},
    {"g_pointers.scheduler.g_strings.kafkaProducerCertificate", process_kafka_producer_certificate},
    {"g_pointers.scheduler.kafkaStartId", process_kafka_start_id},
    {"g_pointers.scheduler.kafkaTag", process_kafka_tag},
    {"g_pointers.scheduler.key", process_almond_key},
    {"g_pointers.scheduler.g_dirs.logDir", process_log_dir},
    {"g_pointers.scheduler.g_bools.logPluginOutput", process_log_plugin_output},
    {"g_pointers.scheduler.logToStdout", process_log_to_stdout},
    {"g_pointers.scheduler.quickStart", process_almond_quickstart},
    {"g_pointers.scheduler.g_bools.runGardenerAtStart", process_run_gardener_at_start},
    {"g_pointers.scheduler.schemaName", process_schema_name},
    {"g_pointers.scheduler.g_strings.schemaRegistryUrl", process_schema_registry_url},
    {"g_pointers.scheduler.storeResults", process_store_results},
    {"g_pointers.scheduler.sleepMs", process_almond_sleep},
    {"g_pointers.scheduler.g_dirs.storeDir", process_store_dir},
    {"g_pointers.scheduler.g_bools.truncateLog", process_truncate_log},
    {"g_pointers.scheduler.g_sizes.truncateLogInterval", process_truncate_log_interval},
    {"g_pointers.scheduler.tuneMaster", process_tune_master},
    {"g_pointers.scheduler.tuneCycle", process_tune_cycle},
    {"g_pointers.scheduler.tuneTimer", process_tune_timer},
    {"g_pointers.scheduler.type", process_almond_scheduler_type},
    {"g_pointers.scheduler.useExternal", process_external_scheduler},
    {"g_pointers.scheduler.g_bools.useKafkaConfigFile", process_use_kafka_config},
    {"g_pointers.scheduler.useTLS", process_almond_api_tls}
};

struct resp_buf { char *data; size_t len; };

static size_t file_read_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fread(ptr, size, nmemb, (FILE*)stream);
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t real = size * nmemb;
    struct resp_buf *rb = userdata;
    char *tmp = realloc(rb->data, rb->len + real + 1);
    if (!tmp) return 0;
    rb->data = tmp;
    memcpy(rb->data + rb->len, ptr, real);
    rb->len += real;
    rb->data[rb->len] = '\0';
    return real;
}

char *trim(char *s) {
    char *ptr;
    if (!s)
        return NULL;   // NULL string
    if (!*s)
        return s;      // empty string
    for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
    ptr[1] = '\0';
    return s;
}

void removeChar(char *str, char garbage) {
        char *src, *dest;
        for (src = dest = str; *src != '\0'; src++){
                *dest = *src;
                if (*dest != garbage) dest++;
        }
        *dest ='\0';
}

char *replaceWord(char *sentence, char *find, char *replace) {
	char *dest = malloc((size_t)strlen(sentence)-strlen(find)+strlen(replace)+1);
	if (dest != NULL)
		dest[0] = '\0';
	strcpy(dest,sentence);
	char buffer[1024] = { 0 };
	char *insert_point = &buffer[0];
	const char *tmp = dest;
	size_t needle_len = strlen(find);
    	size_t repl_len = strlen(replace);
	while (1) {
        	const char *p = strstr(tmp, find);

        	if (p == NULL) {
            		strcpy(insert_point, tmp);
            		break;
        	}
        	memcpy(insert_point, tmp, (size_t)(p - tmp));
        	insert_point += p - tmp;
		memcpy(insert_point, replace, repl_len);
        	insert_point += repl_len;
        	tmp = p + needle_len;
	}
    	strcpy(dest, buffer);
    	return dest;
}

char *load_file_to_string(const char *path) {
	FILE *f = fopen(path, "r");
    	if (!f) return NULL;

    	fseek(f, 0, SEEK_END);
    	long size = ftell(f);
    	rewind(f);

    	char *buf = malloc(size + 1);
    	if (!buf) { fclose(f); return NULL; }

    	fread(buf, 1, size, f);
    	buf[size] = '\0';

    	fclose(f);
    	return buf;
}

static int contains_scheme(const char *s) {
        return (strstr(s, "http://") == s) || (strstr(s, "https://") == s);
}

static int has_port_in_host(const char *s) {
        // crude check: if there's a ':' after the host part but before any '/' then assume port present
         const char *p = strstr(s, "://");
        if (p) s = p + 3;
        const char *slash = strchr(s, '/');
        const char *colon = strchr(s, ':');
        return (colon && (!slash || colon < slash));
}

static int is_ipv6_literal_no_brackets(const char *s) {
        // IPv6 literal contains ':' and does not start with '[' and does not contain scheme
        return (strchr(s, ':') != NULL) && (s[0] != '[') && !contains_scheme(s);
}

char *extract_authorization_header(const char *request) {
	const char *p = strstr(request, "Authorization:");
    	if (!p) return NULL;

    	p += strlen("Authorization:");
    	while (*p == ' ') p++; // skip spaces

    	const char *end = strstr(p, "\r\n");
    	if (!end) return NULL;

    	size_t len = end - p;
    	char *header = malloc(len + 1);
    	if (!header) return NULL;

    	strncpy(header, p, len);
    	header[len] = '\0';
    	return header;
}

char *extract_bearer_token(const char *auth_header) {
    	if (!auth_header) return NULL;

    	const char *prefix = "Bearer ";
    	if (strncmp(auth_header, prefix, strlen(prefix)) != 0)
        	return NULL;

    	return strdup(auth_header + strlen(prefix));
}

void build_push_url(char *out, size_t outlen, const char *push_url, int port, const char *path) {
        const char *p = push_url ? push_url : "";
        const char *final_path = path ? path : "";

        if (contains_scheme(p)) {
                // g_strings.push_url already has scheme
                if (has_port_in_host(p)) {
                        // already has port, just append path if provided
                        snprintf(out, outlen, "%s%s", p, final_path);
                } else {
                        // add port after host portion
                        // find end of host (first '/' after scheme)
                        const char *host_end = strchr(p + (strstr(p, "://") - p) + 3, '/');
                        if (!host_end) {
                                snprintf(out, outlen, "%s:%d%s", p, port, final_path);
                        } else {
                                // insert :port before host_end
                                size_t prefix_len = host_end - p;
                                if (prefix_len + 32 + strlen(final_path) + 1 > outlen) {
                                        out[0] = '\0';
                                        return;
                                }
                                strncpy(out, p, prefix_len);
                                out[prefix_len] = '\0';
                                snprintf(out + prefix_len, outlen - prefix_len, ":%d%s", port, final_path);
                        }
                }
        }
        else {
                // no scheme: decide if IPv6 literal
                if (is_ipv6_literal_no_brackets(p)) {
                        // wrap in brackets
                        snprintf(out, outlen, "http://[%s]:%d%s", p, port, final_path);
                } else {
                        // hostname or IPv4
                        snprintf(out, outlen, "http://%s:%d%s", p, port, final_path);
                }
        }
}

int load_allowed_hosts(const char *filename) {
        FILE *fp = fopen(filename, "r");
        if (!fp) {
                perror("Failed to open allow_hosts file");
                return -1;
        }

        char line[256];
        while (fgets(line, sizeof(line), fp)) {
                // Trim newline
                line[strcspn(line, "\r\n")] = 0;
                if (strlen(line) == 0) continue; // skip empty lines

                if (g_ints.hosts_allowed_count < MAX_HOSTS) {
                        g_arrays.hosts_allowed[g_ints.hosts_allowed_count] = strdup(line);
                        g_ints.hosts_allowed_count++;
                }
        }
        fclose(fp);
        return 0;
}

int is_host_allowed(const char *client_ip) {
        for (int i = 0; i < g_ints.hosts_allowed_count; i++) {
                if (strcmp(client_ip, g_arrays.hosts_allowed[i]) == 0) {
                        return 1; // exact match
                }
                if (strstr(g_arrays.hosts_allowed[i], "/24")) {
                        char prefix[INET_ADDRSTRLEN];
                        size_t len = strlen(g_arrays.hosts_allowed[i]);
                        if (len > 3) {
                                memcpy(prefix, g_arrays.hosts_allowed[i], len -3);
                                prefix[len -3] = '\0';
                        }
                        if (strncmp(client_ip, prefix, strlen(prefix)) == 0) {
                                return 1;
                        }
                }
        }
        return 0;
}

void add_plugin_pid(pid_t pid) {
    	pthread_mutex_lock(&plugin_set_mtx);
    	for (int i = 0; i < MAX_PLUGINS; i++) {
        	if (plugin_pid_set[i] == 0) { plugin_pid_set[i] = pid; break; }
    	}
    	pthread_mutex_unlock(&plugin_set_mtx);
}

void remove_plugin_pid(pid_t pid) {
    	pthread_mutex_lock(&plugin_set_mtx);
    	for (int i = 0; i < MAX_PLUGINS; i++) {
        	if (plugin_pid_set[i] == pid) { plugin_pid_set[i] = 0; break; }
    	}
    	pthread_mutex_unlock(&plugin_set_mtx);
}

int is_plugin_pid(pid_t pid) {
    	int found = 0;
    	pthread_mutex_lock(&plugin_set_mtx);
    	for (int i = 0; i < MAX_PLUGINS; i++) {
        	if (plugin_pid_set[i] == pid) { found = 1; break; }
    	}
    	pthread_mutex_unlock(&plugin_set_mtx);
    	return found;
}

TrackedPopen tracked_popen(const char *cmd) {
	int pfd[2];
    	if (pipe(pfd) == -1) { perror("pipe"); return (TrackedPopen){NULL, -1}; }

    	pid_t pid = fork();
    	if (pid < 0) {
        	perror("fork");
        	close(pfd[0]); close(pfd[1]);
        	return (TrackedPopen){NULL, -1};
    	}
    	if (pid == 0) {
        	// child
        	close(pfd[0]);
        	dup2(pfd[1], STDOUT_FILENO);
        	close(pfd[1]);
        	execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        	_exit(127);
    	}

    	close(pfd[1]);
    	FILE *fp = fdopen(pfd[0], "r");
    	return (TrackedPopen){fp, pid};
}

int tracked_pclose(TrackedPopen *tp) {
	if (!tp || !tp->fp) return -1;
	fclose(tp->fp);
	int status, rc;
	do {
        	rc = waitpid(tp->pid, &status, 0);
    	} while (rc == -1 && errno == EINTR);
	if (rc == -1) {
		return -1;
	}
	if (WIFEXITED(status))       
		return WEXITSTATUS(status);
    	else if (WIFSIGNALED(status)) 
		return 128 + WTERMSIG(status);
    	else         
                return -1;
}

int getNextMessage() {
	int count = 0;
	for (int i = 0; i < 5; ++i) {
		if (g_arrays.logmessage_id[i] == 0) {
			g_arrays.logmessage_id[i] = 1;
			//printf("DEBUG count is %d\n", count);
			return count;
		}
		count++;
	}
	return 0;
}

void logError(const char* message, int severity, int mode) {
        writeLog(message, severity, mode);
        fprintf(stderr, "%s\n", message);
}

void logInfo(const char*message, int severity,int mode) {
        writeLog(message, severity, mode);
        printf("%s\n", message);
}

void checkCtMemoryAlloc() {
	if (g_dirs.confDir == NULL) {
                fprintf(stderr, "Failed to allocate memory.\n");
        }
	if (g_dirs.dataDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_dirs.dataDir].\n");
        }
	if (g_dirs.pluginDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_dirs.pluginDir].\n");
        }
	if (g_files.pluginDeclarationFile == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_files.pluginDeclarationFile].\n");
        }
	if (g_dirs.storeDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_dirs.storeDir].\n");
        }
        if (g_strings.infostr == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_strings.infostr].\n");
        }
        if (g_dirs.logDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_dirs.logDir].\n");
        }
        if (g_files.fileName == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_files.fileName].\n");
        }
	if (g_files.logfile == NULL ) {
		fprintf(stderr, "Failed to allocate memory [g_files.logfile].\n");
	}
        if (g_files.dataFileName == NULL ) {
                fprintf(stderr, "Failed to allocate memory [g_files.dataFileName].\n");
        }
        if (g_dirs.backupDirectory == NULL ) {
                fprintf(stderr, "Failed to allocate memory [g_dirs.backupDirectory].\n");
        }
        if (g_files.newFileName == NULL ) {
                fprintf(stderr, "Failed to allocate memory [g_files.newFileName].\n");
        }
	if (g_strings.gardenerRetString == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.gardenerRetString].\n");
	}
	if (g_strings.pluginCommand == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.pluginCommand].\n");
	}
	if (g_strings.pluginReturnString == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.pluginReturnString].\n");
	}
	if (g_files.storeName == NULL) {
		fprintf(stderr, "Failed to allocare memory [g_files.storeName].\n");
	}
	/*if (apiMessage == NULL) {
		fprintf(stderr, "Failed to allocate memory [apiMessage].\n");
	}*/
}

void updateHostName(char * str) {
	for (int i = 0; i < 255; i++) {
                g_strings.hostName[i] = str[i];
                if (str[i] == '\0')
                        break;
        }
}

int parse__conf_line(char *buf) {
        int i;
        int x;
        int y;
        int s_count = 0;
        int p_count = 0;
        for (i = 0; i < 1000; i++) {
                if (buf[i] == '\n')
                        break;
                if (buf[i] == ';')
                        s_count++;
                if (buf[i] == '[' || buf[i] == ']')
                        p_count++;
        }
        i = 0;
		    char *saveptr = NULL;
		    char *p = strtok_r(buf, ";", &saveptr);
		    char *array[4];
		    i = 0;
		    while (p != NULL && i < 4) {
			    array[i++] = p;
			    p = strtok_r(NULL, ";", &saveptr);
		    }
	if (i < 4) {
		printf("Not enough tokens...\n");
		writeLog("[parse__conf_line] Not enough tokens. Faulty config file.", 1, 0);
		return 2;
	}
        sscanf(array[2], "%d", &x);
        if (x == 0) {
                if (strcmp(array[2], "0") != 0)
                        x = -1;
        }
        if (x == 0 || x == 1) {
                 y = atoi(array[3]);
                 if (!(y > 0)) {
                         return 2;
                 }
        }
        else
                return 2;
        if (s_count == 3 && p_count == 2)
                return 0;
        else
                return 2;
}

static char* getCurrentTimestamp() {
	static char timestamp[20];
	time_t now = time(NULL);
	/* Use proper format specifier for year */
	strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));
	return timestamp;
}

static int compress_log(const char* src_filename, const char* dest_filename) {
	gzFile dest = NULL;
	FILE* source = NULL;
	char buffer[8192];
	size_t bytes_read;

	source = fopen(src_filename, "rb");
	if (!source) {
		fprintf(stderr, "Error opening %s: %s\n", src_filename, strerror(errno));
		writeLog("Failed to open the log source file for compression.", 1, 1);
		return -1;
	}
	dest = gzopen(dest_filename, "wb");
	if (!dest) {
		fprintf(stderr, "Error opening %s: %s\n", dest_filename, strerror(errno));
		writeLog("Failed to create the compressed log file.", 1, 1);
		if (source) fclose(source);
		return -1;
	}
	while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
		if (ferror(source)) {
            		perror("fread");
            		fclose(source);
            		gzclose(dest);
            		return -1;
        	}
		if (gzwrite(dest, buffer, bytes_read) != bytes_read) {
			fprintf(stderr, "Compression failed: %s\n", strerror(errno));
			writeLog("Compression of log file failed.", 1, 1);
			fclose(source);
			gzclose(dest);
			return -1;
		}
	}
	fclose(source);
	gzclose(dest);
	return 0;
}

void run_plugin(PluginItem *item) {
	if (!item) return;

    	/* 1) Save old return code and start timers */
    	int    prevRet = item->output.retCode;
    	clock_t start  = clock();
    	time_t  now    = time(NULL);

    	/* 2) Build full plugin command */
    	char cmd[g_sizes.plugincommand_size];
   	snprintf(cmd, g_sizes.plugincommand_size, "%s/%s", g_dirs.pluginDir, item->command);
    	//printf("Running: %s\n", cmd);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Running command '%s'.", cmd);
	writeLog(trim(g_strings.infostr), 0, 0);

    	/* 3) Spawn process and capture last non-empty line */
    	TrackedPopen tp = tracked_popen(cmd);
    	if (!tp.fp) {
        	perror("tracked_popen");
        	item->output.retCode = -1;
    	}
    	else {
        	add_plugin_pid(tp.pid);

        	char *last_line = NULL;
        	char  buf[g_sizes.pluginoutput_size];

        	while (fgets(buf, sizeof buf, tp.fp)) {
            		char *t = trim(buf);
            		if (*t) {
                		free(last_line);
                		last_line = strdup(t);
            		}
        	}
        	int rc = tracked_pclose(&tp);
        	remove_plugin_pid(tp.pid);

        	/* 4) Map shell exit codes to our retCode */
        	if (rc == 126)           
			item->output.retCode = 0;
        	else if (rc == 256)           
			item->output.retCode = 1;
        	else if (rc == 512)           
			item->output.retCode = 2;
        	else                          
			item->output.retCode = rc;

        	/* 5) Safely replace retString, capped at g_sizes.pluginoutput_size */
        	free(item->output.retString);
        	item->output.retString = NULL;

        	if (last_line) {
            		size_t len = strlen(last_line);
            		if (len >= (size_t)g_sizes.pluginoutput_size) {
                		len = g_sizes.pluginoutput_size - 1;
            		}
            		item->output.retString = malloc(len + 1);
            		if (item->output.retString) {
                		memcpy(item->output.retString, last_line, len);
                		item->output.retString[len] = '\0';
            		}
            		free(last_line);
        	}
    	}
    	/* 6) Format current timestamp */
    	char ts_now[TIMESTAMP_SIZE];
    	struct tm tm_now;
    	localtime_r(&now, &tm_now);
    	strftime(ts_now, sizeof ts_now, "%Y-%m-%d %H:%M:%S", &tm_now);

    	/* 7) Update statusChanged and lastChangeTimestamp */
    	if (prevRet != item->output.retCode) {
        	/* statusChanged is a char[2] array */
        	memcpy(item->statusChanged, "1", 2);
        	strncpy(item->lastChangeTimestamp,
                ts_now,
                sizeof item->lastChangeTimestamp - 1);
        	item->lastChangeTimestamp[sizeof item->lastChangeTimestamp - 1] = '\0';
    	}
	else {
        	memcpy(item->statusChanged, "0", 2);
    	}

    	/* 8) Update lastRunTimestamp */
    	strncpy(item->lastRunTimestamp,
            ts_now,
            sizeof item->lastRunTimestamp - 1);
    	item->lastRunTimestamp[sizeof item->lastRunTimestamp - 1] = '\0';

    	/* 9) Compute and store nextRunTimestamp */
    	time_t next = now + (item->interval * 60);
    	struct tm tm_next;
    	localtime_r(&next, &tm_next);
    	strftime(item->nextRunTimestamp,
             sizeof item->nextRunTimestamp,
             "%Y-%m-%d %H:%M:%S",
             &tm_next);
    	item->nextRun = next;

    	/* 10) Save prevRetCode for next iteration */
    	item->output.prevRetCode = prevRet;

    	/* 11) Print elapsed time */
    	double ms = (double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC;
    	/*printf("%s executed in %.0f ms (ret=%d)\n\n",
           item->name,
           ms,
           item->output.retCode);*/
	snprintf(g_strings.infostr, g_sizes.infostr_size, "%s executed in %.0f ms (ret=%d)", item->name, ms, item->output.retCode);
	writeLog(trim(g_strings.infostr), 0, 0);
     	//ct = clock() -ct;
        //snprintf(g_strings.infostr, g_sizes.infostr_size, "%s executed. Execution took %.0f milliseconds.\n", g_pointers.g_plugins[storeIndex]->name, (double)ct);
        //writeLog(trim(g_strings.infostr), 0, 0);
        if (g_bools.logPluginOutput) {
                char* o_info;
                int o_info_size = g_sizes.pluginmessage_size + 195;
                o_info = malloc((size_t)o_info_size * sizeof(char));
                if (o_info == NULL) {
                        writeLog("Could not allocate memory for variable 'o_info'.", 2, 0);
                }
		else {
                	snprintf(o_info, (size_t)o_info_size, "%s : %s", item->name, item->output.retString);
                	writeLog(trim(o_info), 0, 0);
                	free(o_info);
               	 	o_info = NULL;
		}
        }
        if (g_bools.pluginResultToFile) {
                writePluginResultToFile(item->id, 0);
        }
        if (g_bools.enableKafkaExport) {
                writeToKafkaTopic(item->id, 0);
        }
}


void execute_all_plugins(void) {
    for (int i = 0; i < g_ints.g_plugin_count; ++i) {
        PluginItem *item = g_pointers.g_plugins[i];
        if (item && item->active) {
            run_plugin(item);
        }
    }
}

int post_json_file_stream(const char *url, const char *filepath) {
	CURL *c = NULL;
   	struct curl_slist *hdrs = NULL;
	FILE *f = NULL;
    	struct stat st;
    	struct resp_buf rb = { .data = NULL, .len = 0 };
    	int rc = -1;

    	f = fopen(filepath, "rb");
    	if (!f) {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_json] Can not open file '%s'.", filepath);
		writeLog(g_strings.infostr, 1, 0); 
		fprintf(stderr, "ERROR: cannot open file '%s'\n", filepath); 
		return -1; 
	}
    	if (fstat(fileno(f), &st) != 0) { 
		snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_json] fstat failed for '%s'.", filepath);
                writeLog(g_strings.infostr, 1, 0);     
		fprintf(stderr, "ERROR: fstat failed for '%s'\n", filepath); 
		fclose(f); 
		return -1; 
	}

    	curl_global_init(CURL_GLOBAL_DEFAULT);
    	c = curl_easy_init();
    	if (!c) { 
		fclose(f); 
		curl_global_cleanup(); 
		return -1; 
	}

    	hdrs = curl_slist_append(NULL, "Content-Type: application/json");
    	curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    	curl_easy_setopt(c, CURLOPT_URL, url);
    	curl_easy_setopt(c, CURLOPT_POST, 1L);
    	curl_easy_setopt(c, CURLOPT_READFUNCTION, file_read_cb);
    	curl_easy_setopt(c, CURLOPT_READDATA, f);
    	curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)st.st_size);
    	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    	curl_easy_setopt(c, CURLOPT_WRITEDATA, &rb);

    	/* DEBUG: verbose output to stderr */
    	curl_easy_setopt(c, CURLOPT_VERBOSE, 1L);
    	curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);

    	CURLcode res = curl_easy_perform(c);
    	if (res != CURLE_OK) {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_json] curl_easy_perform failed: %s.", curl_easy_strerror(res));
                writeLog(g_strings.infostr, 1, 0);     
        	fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
    	} else {
        	long http_code = 0;
        	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
		snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_json] HTTP status: '%ld'.", http_code);
                writeLog(g_strings.infostr, 0, 0);     
        	fprintf(stderr, "HTTP status: %ld\n", http_code);
        	if (rb.len) 
			fprintf(stderr, "Response body: %s\n", rb.data);
        	rc = (http_code >= 200 && http_code < 300) ? 0 : -1;
    	}

    	free(rb.data);
    	curl_slist_free_all(hdrs);
    	curl_easy_cleanup(c);
    	curl_global_cleanup();
    	fclose(f);
    	return rc;
}

int post_metrics_file_stream(const char *url, const char *filepath) {
        CURL *c = NULL;
        struct curl_slist *hdrs = NULL;
        FILE *f = NULL;
        struct stat st;
        struct resp_buf rb = { .data = NULL, .len = 0 };
        int rc = -1;

        f = fopen(filepath, "rb");
        if (!f) {
                snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_metrics] Can not open file '%s'.", filepath);
                writeLog(g_strings.infostr, 1, 0);
                fprintf(stderr, "ERROR: cannot open file '%s'\n", filepath);
                return -1;
        }
        if (fstat(fileno(f), &st) != 0) {
                snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_metrics] fstat failed for '%s'.", filepath);
                writeLog(g_strings.infostr, 1, 0);
                fprintf(stderr, "ERROR: fstat failed for '%s'\n", filepath);
                fclose(f);
                return -1;
        }

        curl_global_init(CURL_GLOBAL_DEFAULT);
        c = curl_easy_init();
        if (!c) {
                fclose(f);
                curl_global_cleanup();
                return -1;
        }
	hdrs = curl_slist_append(NULL, "Content-Type: text/plain; version=0.0.4; charset=utf-8");
	// If you want to use the newer OpenMetrics format
	// hdrs = curl_slist_append(NULL, "Content-Type: application/openmetrics-text; version=1.0.0; charset=utf-8");
        curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
        curl_easy_setopt(c, CURLOPT_URL, url);
        curl_easy_setopt(c, CURLOPT_POST, 1L);
        curl_easy_setopt(c, CURLOPT_READFUNCTION, file_read_cb);
        curl_easy_setopt(c, CURLOPT_READDATA, f);
        curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)st.st_size);
        curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(c, CURLOPT_WRITEDATA, &rb);

        /* DEBUG: verbose output to stderr */
        curl_easy_setopt(c, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(c);
        if (res != CURLE_OK) {
                snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_metrics] curl_easy_perform failed: %s.", curl_easy_strerror(res));
                writeLog(g_strings.infostr, 1, 0);
                fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        } else {
                long http_code = 0;
                curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
                snprintf(g_strings.infostr, g_sizes.infostr_size, "[push_metrics] HTTP status: '%ld'.", http_code);
                writeLog(g_strings.infostr, 1, 0);
                fprintf(stderr, "HTTP status: %ld\n", http_code);
                if (rb.len)
                        fprintf(stderr, "Response body: %s\n", rb.data);
                rc = (http_code >= 200 && http_code < 300) ? 0 : -1;
        }

        free(rb.data);
        curl_slist_free_all(hdrs);
        curl_easy_cleanup(c);
        curl_global_cleanup();
        fclose(f);
        return rc;
}

int toggleHostName(char *name) {
        FILE * fPtr = NULL;
        FILE * fTemp = NULL;
        char * filename = NULL;
        char * tempfile = NULL;

        char buffer[1000];
        char fhost[50] = "g_pointers.scheduler.g_strings.hostName=";
        char newline[300];
        filename = "/etc/almond/almond.conf";
        tempfile = "/etc/almond/almond.temp";

        int i = 0, j = 0;
        while(fhost[i] != '\0') {
                newline[j] = fhost[i];
                i++;
                j++;
        }
        i = 0;
        while (name[i] != '\0') {
                newline[j] = name[i];
                i++;
                j++;
        }
        newline[j] = '\0';

        fPtr = fopen(filename, "r");
        fTemp = fopen(tempfile, "w");

        if (fPtr == NULL || fTemp == NULL) {
                writeLog("Could not update hostname value in configuration file. Read error.", 1, 0);
                exit(EXIT_SUCCESS);
        }

        int changed = 0;
        while ((fgets(buffer, 1000, fPtr)) != NULL){
                char *pch = strstr(buffer, fhost);
                if (pch) {
                        fputs(newline, fTemp);
                        fputs("\n", fTemp);
                        changed = 1;
                }
                else
                        fputs(buffer, fTemp);
        }
        if (changed == 0) {
                // append to file
                fclose(fTemp);
                fTemp = NULL;
                fTemp = fopen("/etc/almond/almond.temp", "a");
                fprintf(fTemp, "%s\n", newline);
        }
        fclose(fPtr);
        fclose(fTemp);
        fPtr = fTemp = NULL;
        remove(filename);
        rename(tempfile, filename);
        writeLog("Updated almond.conf file", 0, 0);
        return 0;
}

int toggleExportFileName(char *name, int mode) {
        FILE * fPtr = NULL;
        FILE * fTemp = NULL;
        char * filename = NULL;
        char * tempfile = NULL;

        char buffer[1000];
        char dFile[15] = "data.jsonFile=";
	char mFile[18] = "data.metricsFile=";
        char newline[300];
        filename = "/etc/almond/almond.conf";
        tempfile = "/etc/almond/almond.temp";

        int i = 0, j = 0;
	if (mode == 0) {
        	while(dFile[i] != '\0') {
                	newline[j] = dFile[i];
               		i++;
                	j++;
        	}
	}
	if (mode == 1) {
                while(mFile[i] != '\0') {
                        newline[j] = mFile[i];
                        i++;
                        j++;
                }
        }
        i = 0;
        while (name[i] != '\0') {
                newline[j] = name[i];
                i++;
                j++;
        }
        newline[j] = '\0';

        fPtr = fopen(filename, "r");
        fTemp = fopen(tempfile, "w");

        if (fPtr == NULL || fTemp == NULL) {
                writeLog("Could not update filename value in configuration file. Read error.", 1, 0);
                exit(EXIT_SUCCESS);
        }

        int changed = 0;
        while ((fgets(buffer, 1000, fPtr)) != NULL){
                char *pch = NULL;
	        if (mode == 0)
			pch = strstr(buffer, dFile);
		else if (mode == 1)
			pch = strstr(buffer, mFile);
                if (pch) {
                        fputs(newline, fTemp);
                        fputs("\n", fTemp);
                        changed = 1;
                }
                else
                        fputs(buffer, fTemp);
        }
        if (changed == 0) {
                // append to file
                fclose(fTemp);
                fTemp = NULL;
                fTemp = fopen("/etc/almond/almond.temp", "a");
                fprintf(fTemp, "%s\n", newline);
        }
        fclose(fPtr);
        fclose(fTemp);
        fPtr = fTemp = NULL;
        remove(filename);
        rename(tempfile, filename);
        writeLog("Updated almond.conf file", 0, 0);
        return 0;
}

void updateFileName(char value[100], int mode) {
	int count = 1;
        char oldName[50];

	if (mode == 0) {
        	for (int c = 0; c < sizeof(oldName); c++) {
        		oldName[c] = g_files.jsonFileName[c];
        		g_files.jsonFileName[c] = '\0';
        	}
	}
	else if (mode == 1) {
		for (int c = 0; c < sizeof(oldName); c++) {
                        oldName[c] = g_files.metricsFileName[c];
                        g_files.metricsFileName[c] = '\0';
                }
	}
        for (int i = 0; i < strlen(value); i++) {
        	if (value[i] == '\n')
                	break;
        	else {
			if (mode == 0)
               			g_files.jsonFileName[i] = value[i];
			if (mode == 1)
				g_files.metricsFileName[i] = value[i];
                }
                if (value[i] == '\0')
                	break;
               	count++;
                if (count == 45) {
			if (mode == 0)
                		writeLog("New jsonfile name possibly writing over buffer size and will be truncated.", 1, 0);
			if (mode == 1)
				writeLog("New metrics filename possible writing over buffer size and will be truncated.", 1, 0);
                       	break;
                }
        }
        if (count == 45) {
		if (mode == 0) {
        		strcat(g_files.jsonFileName, ".json");
               		g_files.jsonFileName[50] = '\0';
		}
		else if (mode == 1) {
			strcat(g_files.metricsFileName, ".metrics");
			g_files.metricsFileName[50] = '\0';
		}
        }
        else {
		if (mode == 0) {
        		char *ext = strrchr(g_files.jsonFileName, '.');
                	if (ext) {
                		if (*(ext+1) == '\0') {
                        		writeLog("New jsonfile name is ending with a dot.", 1, 0);
                        	}
                        	g_files.jsonFileName[strlen(value)+1] = '\0';
                	}
                	else {
                		strcat(g_files.jsonFileName, ".json");
                        	g_files.jsonFileName[strlen(value)+6] = '\0';
                	}
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Json export file name changed to '%s'.", g_files.jsonFileName);
		}
		else if (mode == 1) {
			char *ext = strrchr(g_files.metricsFileName, '.');
                        if (ext) {
                                if (*(ext+1) == '\0') {
                                        writeLog("New metrics filname is ending with a dot.", 1, 0);
                                }
                                g_files.metricsFileName[strlen(value)+1] = '\0';
                        }
                        else {
                                strcat(g_files.metricsFileName, ".metrics");
                                g_files.metricsFileName[strlen(value)+9] = '\0';
                        }
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics filename changed to '%s'.", g_files.metricsFileName);
		}

	}
       	writeLog(g_strings.infostr, 1, 0);
       	writeLog("If using howru api together with Almond, you need to restart Howru service.", 2, 0);
	if (mode == 0)
       		toggleExportFileName(g_files.jsonFileName, 0);
	else if (mode == 1)
		toggleExportFileName(g_files.metricsFileName, 1);
       	char * removeFileName = NULL;
        if (mode == 0)
		removeFileName = malloc(g_sizes.datafilename_size);
	else if (mode == 1)
		removeFileName = malloc(100);
	if (removeFileName == NULL) {
       		writeLog("Could not allocate memory for removing old file.", 1, 0);
        }
        else {
		if (mode == 0) {
        		memset(removeFileName, '\0', g_sizes.datafilename_size);
                	snprintf(removeFileName, g_sizes.datafilename_size, "%s%c%s", g_dirs.dataDir, '/', oldName);
			if (remove(removeFileName) == 0) {
                        	snprintf(g_strings.infostr, g_sizes.infostr_size, "Json export file '%s' is removed.", oldName);
                        	writeLog(g_strings.infostr, 1, 0);
                	}
                	else {
                        	snprintf(g_strings.infostr, g_sizes.infostr_size, "Could not remove old export file '%s'.", oldName);
                        	writeLog(g_strings.infostr, 1, 0);
                	}
		}
		else if (mode == 1) {
			memset(removeFileName, '\0',100);
                        snprintf(removeFileName, 100, "%s%c%s", g_dirs.storeDir, '/', oldName);
			if (remove(removeFileName) == 0) {
                                snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics export file '%s' is removed.", oldName);
                                writeLog(g_strings.infostr, 1, 0);
                        }
                        else {
                                snprintf(g_strings.infostr, g_sizes.infostr_size, "Could not remove old metrics file '%s'.", oldName);
                                writeLog(g_strings.infostr, 1, 0);
                        }
		}
        }
        if (removeFileName != NULL) {
        	free(removeFileName);
                removeFileName = NULL;
        }
}

int compare_timestamps(const void* a, const void* b) {
    const struct Scheduler* sa = (const struct Scheduler*)a;
    const struct Scheduler* sb = (const struct Scheduler*)b;

    if (sa->timestamp < sb->timestamp) return -1;
    if (sa->timestamp > sb->timestamp) return 1;

    // Tie-breaker: sort by ID ascending
    if (sa->id < sb->id) return -1;
    if (sa->id > sb->id) return 1;

    return 0;
}


int get_thread_count() {
    int count = 0;
    DIR *dir = opendir("/proc/self/task");
    if (dir) {
        while (readdir(dir)) count++;
        closedir(dir);
    }
    return count - 2; // subtract '.' and '..'
}

void print_io_stats() {
    FILE *fp = fopen("/proc/self/io", "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);  // e.g., "read_bytes: 1024"
    }
    fclose(fp);
}

int get_fd_count() {
    int count = 0;
    DIR *dir = opendir("/proc/self/fd");
    if (dir) {
        while (readdir(dir)) count++;
        closedir(dir);
    }
    return count - 2; // subtract '.' and '..'
}

int check_plugin_conf_file(char *declarationFile) {
        FILE * fPtr = NULL;
        int i;
        char buffer[1000];
        int retval = 0;

        fPtr = fopen(declarationFile, "r");
        if (fPtr == NULL)
        {
                writeLog("Error opening the plugin g_pointers.g_plugins file.", 2, 0);
		perror("Error while opening the file [check_plugin_conf_file].\n");
                exit(EXIT_FAILURE);
        }
        while ((fgets(buffer, 1000, fPtr)) != NULL){
                for(i = 0; i < 1000; i++) {
                        if (buffer[i] == '#')
                                break;
                        else {
                                if (parse__conf_line(buffer) > 0) {
                                        retval = 2;
                                }
                                break;
                        }
                }
        }
        fclose(fPtr);
        fPtr = NULL;
        return retval;
}

void checkSchedulerCount() {
	if (g_current_scheduler_cnt != g_ints.decCount) {
		writeLog("Reinitate g_pointers.scheduler since number of plugins changed.", 0, 0);
		free(g_pointers.scheduler);
		g_pointers.scheduler = NULL;
		g_current_scheduler_cnt = g_ints.decCount;
		initTimeScheduler(true);
	}
}

void rescheduleChecks() {
	size_t n = (size_t)g_ints.decCount;
        writeLog("Schedule new exectution times.", 0, 0);
	checkSchedulerCount();
        qsort(g_pointers.scheduler, n, sizeof(struct Scheduler), compare_timestamps);
        flushLog();
}

int updateValuesFromUdfFile(char id[3]) {
	FILE *fp = NULL;
	char* token;
	char filename[30];
	size_t buffer_size = g_sizes.pluginoutput_size + 100;
	char buffer[buffer_size];
	char columns[2][g_sizes.pluginoutput_size];
	int columnCount = 0;
	int pId = -1;

	strcpy(filename, "/opt/almond/api_cmd/");
	strncat(filename, trim(id), 3);
	strncat(filename, ".udf", 5);

	fp = fopen(filename, "r");
	if (fp == NULL) {
		writeLog("Could not open update file in api_cmd directory.", 1, 0);
		return 2;
	}
	while (fgets(buffer, buffer_size, fp)) {
		char *saveptr = NULL;
		token = strtok_r(buffer, "\t", &saveptr);
		while (token != NULL) {
			strncpy(columns[columnCount], token, g_sizes.pluginoutput_size - 1);
			columns[columnCount][g_sizes.pluginoutput_size - 1] = '\0';
			columnCount++;
			token = strtok_r(NULL, "\t", &saveptr);
		}
		if (strcmp(columns[0], "item_id") == 0) {
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Updating pluginitem with id '%s' from update file.", trim(columns[1]));
			writeLog(g_strings.infostr, 0, 0);
			pId = atoi(columns[1]);
		}
		if (pId != -1) {
			if (strcmp(columns[0], "item_lastruntimestamp") == 0) {
				//strncpy(g_pointers.g_plugins[pId].lastRunTimestamp, trim(columns[1]), 20);
				snprintf(g_pointers.g_plugins[pId]->lastRunTimestamp, 20, "%s", trim(columns[1])); 
			}
			else if (strcmp(columns[0], "item_lastchangetimestamp") == 0) {
				//strncpy(g_pointers.g_plugins[pId].lastChangeTimestamp, trim(columns[1]), 20);
				snprintf(g_pointers.g_plugins[pId]->lastChangeTimestamp, 20, "%s", trim(columns[1]));
			}
			else if (strcmp(columns[0], "item_nextruntimestamp") == 0) {
				//strncpy(g_pointers.g_plugins[pId].nextRunTimestamp, trim(columns[1]), 20);
				snprintf(g_pointers.g_plugins[pId]->nextRunTimestamp, 20, "%s", trim(columns[1]));
			}
			else if (strcmp(columns[0], "item_statuschanged") == 0) {
				//strncpy(g_pointers.g_plugins[pId].statusChanged, trim(columns[1]), 1);
				snprintf(g_pointers.g_plugins[pId]->statusChanged, 2, "%s", trim(columns[1]));
			}
			else if (strcmp(columns[0], "output_retcode") == 0) {
				//strcpy(outputs[pId].retCode, trim(columns[1]));
				g_pointers.g_plugins[pId]->output.retCode = atoi(trim(columns[1]));
			}
			else if (strcmp(columns[0], "output_retstring") == 0) {
				//strcpy(outputs[pId].retString, trim(columns[1]));
				snprintf(g_pointers.g_plugins[pId]->output.retString, g_sizes.pluginoutput_size, "%s", trim(columns[1])); 
			}
		}
		columnCount = 0;
	}
	fclose(fp);
	fp = NULL;
	remove(filename);
	if (pId >= 0) {
		struct tm tm_struct;
		time_t time_var;
		char *timestamp = g_pointers.g_plugins[pId]->nextRunTimestamp;
		if (strptime(timestamp,"%Y-%m-%d %H:%M:%S", &tm_struct)) {
			time_var = mktime(&tm_struct);
			if (time_var != -1) {
				g_pointers.g_plugins[pId]->nextRun = time_var;
				if (g_bools.timeScheduler) {
					g_pointers.scheduler[pId].timestamp = time_var;
					rescheduleChecks();
				}
				writeLog("A nextRun timestamp was updated from udf-file.", 0, 0);
			}
			else {
				writeLog("Could not update next run time stamp from udf-file.", 1, 0);
			}
		}
		else {
			writeLog("Error parsing nextRunTimestamp to t_time object in udf-file.", 1, 0);
		}
	}
	return 0;
}

void parseExArgsCmd(char command[100]) {
	const char* sNum;
	char* cmdRun;
	int num;

	sNum = strtok(command, ";");
	cmdRun = strtok(NULL, ";");

	num = atoi(sNum);
	runPluginCommand(num, cmdRun);
}

void setApiCmdFile(char * name, char * value) {
        FILE * fp;
        char filename[100] = "/opt/almond/api_cmd/";
        char content[100];
	snprintf(filename, sizeof(filename), "/opt/almond/api_cmd/%s.cmd", name);
	int written = snprintf(content, sizeof(content), "%s\t%s", name, value);
    	if (written < 0 || written >= sizeof(content)) {
        	writeLog("Content too long or formatting error.", 2, 0);
        	return;
    	}
        fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("Failed to open command file.");
		writeLog("Failed to open command file.", 2, 0);
		return;
	}
        /*strncpy(content, name, sizeof(content)-1);
        strcat(content, "\t");
        strcat(content, value);*/
        fprintf(fp, "%s\n",content);
        fclose(fp);
	fp = NULL;
	writeLog("Command file written from API call.", 0, 0);
}

int runApiCmds(char * cmd) {
        FILE * cmdfile;
        char* token;
        int columnCount = 0;
        char line[100];
        char columns[2][100];
	char filename[PATH_MAX];

	int written = snprintf(filename, sizeof(filename), "/opt/almond/api_cmd/%s", cmd);
	if (written < 0 || (size_t)written >= sizeof(filename)) {
		writeLog("Command filename too long", 1, 0);
		return 2;
	}
        cmdfile = fopen(filename, "r");
        if (cmdfile == NULL) {
                perror("Failed to open file");
		writeLog("Could not open command file.", 1, 0);
                return 1;
        }
	while (fgets(line, sizeof(line), cmdfile)) {
		char *saveptr = NULL;
		token = strtok_r(line, "\t", &saveptr);
		while (token != NULL) {
			strncpy(columns[columnCount], token, sizeof(columns[columnCount]) - 1);
			columns[columnCount][sizeof(columns[columnCount]) - 1] = '\0';
			columnCount++;
			token = strtok_r(NULL, "\t", &saveptr);
		}
		columnCount = 0;
	}
        fclose(cmdfile);
	cmdfile = NULL;
        if (strcmp(columns[0], "hostname") == 0) {
		writeLog("Hostname will be updated in memory and config by API call.", 0, 0);
                updateHostName(trim(columns[1]));
		toggleHostName(trim(columns[1]));
        }
	else if (strcmp(columns[0], "kafkatag") == 0) {
		if (g_strings.kafka_tag == NULL) {
			g_strings.kafka_tag = malloc((strlen(columns[1]) + 1) * sizeof(char));
                 	if (g_strings.kafka_tag != NULL)
				memset(g_strings.kafka_tag, 0, strlen(columns[1]) + 1);
		 	else {
				 writeLog("Failed to allocate memory for variable 'g_strings.kafka_tag'.", 1, 0);
				 return 2;
		 	}
		}
		int i = 0;
		while (columns[1][i] != '\n' && columns[1][i] != '\0') {
        		g_strings.kafka_tag[i] = columns[1][i];
        		i++;
    		}
    		g_strings.kafka_tag[i] = '\0'; 
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka tag is set to '%s'", g_strings.kafka_tag);
		writeLog(g_strings.infostr, 0, 0);
	}
	else if (strcmp(columns[0], "kafkatopic") == 0) {
		if (g_strings.kafka_topic == NULL) {
			g_strings.kafka_topic = malloc((strlen(columns[1]) + 1) * sizeof(char));
			if (g_strings.kafka_topic != NULL)
				memset(g_strings.kafka_topic, 0, strlen(columns[1]) + 1);
			else {
				writeLog("Failed to allocate memory for variable 'g_strings.kafka_topic'.", 1, 0);
				return 2;
			}
		}
		int i = 0;
		while (columns[1][i] != '\n' && columns[1][i] != '\0') {
        		g_strings.kafka_topic[i] = columns[1][i];
        		i++;
    		}
    		g_strings.kafka_topic[i] = '\0'; 
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka topic is set to '%s'.", g_strings.kafka_topic);
		if (g_bools.useKafkaConfigFile) {
			setKafkaTopic(g_strings.kafka_topic);
		}
		writeLog(g_strings.infostr, 0, 0);
	}
	else if (strcmp(columns[0], "jsonfilename") == 0) {
		updateFileName(columns[1], 0);
	}
	else if (strcmp(columns[0], "metricsfilename") == 0) {
		updateFileName(columns[1], 1);
        }
	else if (strcmp(columns[0], "execute") == 0) {
		int id = atoi(columns[1]);
		writeLog("Execute plugin from command file.", 0, 0);
		//runPlugin(id, 0);
                PluginItem *item = g_pointers.g_plugins[id];
        	if (item) {
            		run_plugin(item);
        	}
		else {
			printf("DEBUG: Failed to execute item id %d.\n", id);
		}
		if (g_bools.timeScheduler) {
			rescheduleChecks();
		}
	}
	else if (strcmp(columns[0], "executeargs") == 0) {
		writeLog("Execute plugin with added arguments from command file.", 0, 0);
		parseExArgsCmd(columns[1]);
		if (g_bools.timeScheduler) {
			rescheduleChecks();
		}
	}
	else if (strcmp(columns[0], "metricsprefix") == 0) {
                memset(g_strings.metricsOutputPrefix, '\0', g_sizes.metricsoutputprefix_size);
		size_t len = strlen(columns[1]);
		for (int i = 0; i < (int)len; i++) {
			if (columns[1][i] == '\n')
				break;
			else
				g_strings.metricsOutputPrefix[i] = columns[1][i];
			if (columns[1][i] == '\0')
				break;
		}
	        snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics prefix is set to '%s'", g_strings.metricsOutputPrefix);
		writeLog(g_strings.infostr, 0, 0);

	}
	else if (strcmp(columns[0], "update") == 0) {
		writeLog("Ready to run updates from udf-file.", 0, 0);
		updateValuesFromUdfFile(columns[1]);
	}
	else if (strcmp(columns[0], "g_pointers.scheduler") == 0) {
		char* scheduler_type;
		scheduler_type = malloc((size_t)strlen(columns[1])+1);
		for (int i = 0; i < strlen(columns[1]); i++) {
                        if (columns[1][i] == '\n')
                                break;
                        else
                                scheduler_type[i] = columns[1][i];
                        if (columns[1][i] == '\0')
                                break;
                }
		if (strcmp(trim(scheduler_type), "external") == 0) {
			g_bools.external_scheduler = true;
			writeLog("Almond g_pointers.scheduler type is set to external through command file.", 0, 0);
		}
		else {
			g_bools.external_scheduler = false;
			writeLog("Almond g_pointers.scheduler type is set to internal after running command file.", 0, 0);
		}
		free(scheduler_type);
	}
	else if (strcmp(columns[0], "pushurl") == 0) {
		if (g_strings.push_url == NULL) {
			g_strings.push_url = malloc((strlen(columns[1]) + 1) * sizeof(char));
                        if (g_strings.push_url != NULL)
                                memset(g_strings.push_url, 0, strlen(columns[1]) + 1);
                        else {
                                writeLog("Failed to allocate memory for variable 'g_strings.push_url'.", 1, 0);
                                return 2;
                        }
                }
		else {
			size_t olen = sizeof(g_strings.push_url);
			memset(g_strings.push_url, '\0', olen);
		}
		size_t len = strlen(columns[1]);
                for (int i = 0; i < (int)len; i++) {
                        if (columns[1][i] == '\n')
                                break;
                        else
                                g_strings.push_url[i] = columns[1][i];
                        if (columns[1][i] == '\0')
                                break;
                }
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Push url is set to '%s'", g_strings.push_url);
                writeLog(g_strings.infostr, 0, 0);
	}
	if (remove(filename) == 0) {
        	writeLog("Command file was deleted.", 0, 0);
        }
        else {
                writeLog("Unable to delete command file. The command will run again!", 1, 0);
        }
        return 0;
}

int checkApiCmds() {
    DIR *d;
    struct dirent *entry;
    const char *dirPath = "/opt/almond/api_cmd";  // Define your directory path

    if (!(d = opendir(dirPath))) {
        perror("Failed to open directory");
        writeLog("Failed to open command file directory.", 1, 0);
        return 1;
    }

    while ((entry = readdir(d)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len < 4)
            continue;

        if (strcmp(entry->d_name + len - 4, ".cmd") != 0)
            continue;

        if (entry->d_type == DT_REG) {
            runApiCmds(entry->d_name);
        }
        else if (entry->d_type == DT_UNKNOWN) {
            char fullPath[PATH_MAX];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);
            struct stat st;
            if (stat(fullPath, &st) == 0 && S_ISREG(st.st_mode)) {
                runApiCmds(entry->d_name);
            }
        }
    }
    closedir(d);
    return 0;
}

void initConstants() {
	g_strings.logmessage = calloc(g_sizes.logmessage_size+1, sizeof(char));
	if (g_strings.logmessage == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_strings.logmessage].\n");
        }
        else {
                strncpy(g_strings.logmessage, "", g_sizes.logmessage_size+1);
		g_strings.logmessage[g_sizes.logmessage_size] = '\0';
	}
        g_files.logfile = malloc(g_sizes.logfile_size);
        if (g_files.logfile == NULL) {
                 fprintf(stderr, "Failed to allocate memory [logFile].\n");
        }
        else
                memset(g_files.logfile, '\0', g_sizes.logfile_size);
	if (isConstantsEnabled() > 0) {
		getConstants();
        }
	g_dirs.confDir = malloc(g_sizes.confdir_size);
	if (g_dirs.confDir != NULL) {
		memset(g_dirs.confDir, 0, g_sizes.confdir_size);
	}
	g_dirs.dataDir = malloc(g_sizes.datadir_size);
	if (g_dirs.dataDir != NULL)
		memset(g_dirs.dataDir, '\0', g_sizes.datadir_size);
	g_dirs.pluginDir = malloc(g_sizes.plugindir_size);
	if (g_dirs.pluginDir != NULL)
		memset(g_dirs.pluginDir, '\0', g_sizes.plugindir_size);
	g_files.pluginDeclarationFile = malloc(g_sizes.plugindeclarationfile_size);
	if (g_files.pluginDeclarationFile != NULL)
		memset(g_files.pluginDeclarationFile, '\0', g_sizes.plugindeclarationfile_size);
	g_files.jsonFileName = calloc(g_sizes.jsonfilename_size+1, sizeof(char));
	if (g_files.jsonFileName == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_files.jsonFileName].\n");
        }
	else
		strncpy(g_files.jsonFileName, "monitor_data.json", 18);
	g_files.metricsFileName = calloc(g_sizes.metricsfilename_size+1, sizeof(char));
	if (g_files.metricsFileName == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_files.metricsFileName].\n");
	}
	else
		strncpy(g_files.metricsFileName, "monitor.metrics", 16);
	g_files.gardenerScript = calloc(g_sizes.gardenerscript_size+1, sizeof(char));
	if (g_files.gardenerScript == NULL) {
                fprintf(stderr, "Failed to allocate memory [g_files.gardenerScript].\n");
        }
        else
		strncpy(g_files.gardenerScript, "/opt/almond/gardener.py", 24);
	g_dirs.storeDir = malloc(g_sizes.storedir_size);
	if (g_dirs.storeDir == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_dirs.storeDir].\n");
	}
	else
		memset(g_dirs.storeDir, '\0', g_sizes.storedir_size);
	g_dirs.logDir = malloc(g_sizes.logdir_size);
	if (g_dirs.logDir != NULL)
		memset(g_dirs.logDir, '\0', g_sizes.logdir_size);
	g_strings.infostr = malloc((size_t)g_sizes.infostr_size * sizeof(char));
	if (g_strings.infostr == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.infostr].\n");
	}
	else
		memset(g_strings.infostr, '\0', (size_t)g_sizes.infostr_size * sizeof(char));
	g_strings.hostName = calloc(g_sizes.hostname_size+1, sizeof(char));
	if (g_strings.hostName == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.hostName].\n");
	}
	else
		strncpy(g_strings.hostName, "None", 5);
	g_files.fileName = malloc((size_t)g_sizes.filename_size * sizeof(char));
	if (g_files.fileName == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_files.fileName].\n");
	}
	else
		memset(g_files.fileName, '\0', (size_t)g_sizes.filename_size * sizeof(char));
	g_strings.metricsOutputPrefix = calloc(g_sizes.metricsoutputprefix_size+1, sizeof(char));
	if (g_strings.metricsOutputPrefix == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.metricsOutputPrefix].\n");
	}
	else
		strncpy(g_strings.metricsOutputPrefix, "almond", 7);
	g_files.dataFileName = malloc(g_sizes.datafilename_size);
	memset(g_files.dataFileName, '\0', g_sizes.datafilename_size);
	g_dirs.backupDirectory = malloc(g_sizes.backupdirectory_size);
	memset(g_dirs.backupDirectory, '\0', g_sizes.backupdirectory_size);
	g_files.newFileName = malloc(g_sizes.newfilename_size);
	memset(g_files.newFileName, '\0', (size_t)(150 * sizeof(char)));
	g_strings.gardenerRetString = malloc((size_t)g_sizes.gardenermessage_size * sizeof(char));
	memset(g_strings.gardenerRetString, '\0', (size_t)(sizeof(char) * g_sizes.gardenermessage_size));
	g_strings.pluginCommand = malloc((size_t)100 * sizeof(char));
	memset(g_strings.pluginCommand, '\0', sizeof(char) * 100);
	g_strings.pluginReturnString = malloc((size_t)g_sizes.pluginmessage_size * sizeof(char));
	memset(g_strings.pluginReturnString, '\0', (size_t)(g_sizes.pluginmessage_size * sizeof(char)));
	g_files.storeName = malloc((size_t)g_sizes.storename_size * sizeof(char));
	memset(g_files.storeName, '\0', (size_t)(sizeof(char) * g_sizes.storename_size));
	//apiMessage = malloc(g_sizes.apimessage_size * sizeof(char));
	checkCtMemoryAlloc();
}

int getConstants() {
	int count = 0;

	if (g_strings.logmessage == NULL) {
		g_strings.logmessage = malloc(g_sizes.logmessage_size);
		if (g_strings.logmessage == NULL) {
			printf("Could not allocate memory for g_strings.logmessage!\n");
			return -1;
		}
		else {
			memset(g_strings.logmessage, 0, g_sizes.logmessage_size);
                	g_strings.logmessage[0] = '\0';
		}
	}

	writeLog("Reading memory variable g_arrays.constants.", 0, 1);

        FILE *file = fopen(constantsFile, "r");
        if (file == NULL) {
		printf("Could not read g_arrays.constants file. Not found.");
                return 1;
        }

        while (fscanf(file, "%s %d", g_arrays.constants[count], &g_arrays.values[count]) == 2) {
                count++;
		if (count == MAX_CONSTANTS) break;
        }
        for (int i = 0; i < count; i++) {
                if (strcmp(g_arrays.constants[i], "CONFDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'g_dirs.confDir' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.confdir_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "DATADIR_SIZE") == 0) {
			writeLog("Memory for variable 'g_dirs.dataDir' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.datadir_size = (size_t)(g_arrays.values[i] * sizeof(char) + 1);
		}
		else if (strcmp(g_arrays.constants[i], "PLUGINDECLARATIONFILE_SIZE") == 0) {
			writeLog("Memory for variable 'pluginDeclarationSize' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.plugindeclarationfile_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "JSONFILENAME_SIZE") == 0) {
			writeLog("Memory for variable 'g_files.jsonFileName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.jsonfilename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "METRICSFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.metricsFileName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.metricsfilename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "GARDENERSCRIPT_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.gardenerScript' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.gardenerscript_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		} 
		else if (strcmp(g_arrays.constants[i], "HOSTNAME_SIZE") == 0) {
			writeLog("Memory for variable 'g_strings.hostName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.hostname_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "METRICSOUTPUTPREFIX_SIZE") == 0) {
                        writeLog("Memory for variable 'g_strings.metricsOutputPrefix' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.metricsoutputprefix_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "STOREDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'g_dirs.storeDir' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.storedir_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "LOGDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'g_dirs.logDir' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.logdir_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "INFOSTR_SIZE") == 0) {
			writeLog("Memory for 'info_str' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.infostr_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "PLUGINDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'g_dirs.pluginDir' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.plugindir_size =  (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "FILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.fileName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.filename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "LOGMESSAGE_SIZE") == 0) {
			writeLog("Memory for variable 'g_strings.logmessage' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.logmessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "LOGFILE_SIZE") == 0) {
			writeLog("Memory for variable 'g_files.logfile' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.logfile_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "DATAFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.dataFileName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.datafilename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "BACKUPDIRECTORY_SIZE") == 0) {
                        writeLog("Memory for variable 'g_dirs.backupDirectory' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.backupdirectory_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "NEWFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.newFileName' will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.newfilename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "GARDENERMESSAGE_SIZE") == 0) {
			writeLog("Memory for gardener return message will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.gardenermessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "PLUGINCOMMAND_SIZE") == 0) {
			writeLog("Memory for plugin command size will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.plugincommand_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "PLUGINMESSAGE_SIZE") == 0) {
                        writeLog("Memory for plugin message size will be allocated by g_arrays.constants file.", 0, 1);
			g_sizes.pluginmessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "STORENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'g_files.storeName' will be allocated by g_arrays.constants file.", 0, 1);
                        g_sizes.storename_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "APIMESSAGE_SIZE") == 0) {
                        writeLog("Memory for API messages will dynamically be allocated by size inconstants file.", 0, 1);
                        g_sizes.apimessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "SOCKETSERVERMESSAGE_SIZE") == 0) {
			writeLog("Memory for socket server messages will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
			g_sizes.socketservermessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "SOCKETCLIENTMESSAGE_SIZE") == 0) {
			writeLog("Memory for socket client messages will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
			g_sizes.socketclientmessage_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
		}
		else if (strcmp(g_arrays.constants[i], "PLUGINITEMNAME_SIZE") == 0) {
			writeLog("Memory for pluginitem name size will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
                        g_sizes.pluginitemname_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
 		else if (strcmp(g_arrays.constants[i], "PLUGINITEMDESC_SIZE") == 0) {
                        writeLog("Memory for pluginitem description size will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
                        g_sizes.pluginitemdesc_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
 		else if (strcmp(g_arrays.constants[i], "PLUGINITEMCMD_SIZE") == 0) {
                        writeLog("Memory for pluginitem command size will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
                        g_sizes.pluginitemcmd_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else if (strcmp(g_arrays.constants[i], "PLUGINOUTPUT_SIZE") == 0) {
                        writeLog("Memory for plugin output will dynamically be allocated by size in g_arrays.constants file.", 0, 1);
                        g_sizes.pluginoutput_size = (size_t)(g_arrays.values[i] * sizeof(char)+1);
                }
		else {
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Constant '%s' not implemented by Almond %s", g_arrays.constants[i], VERSION);
			writeLog(trim(g_strings.infostr), 1, 1);
		}
	}
        return 0;
}

void constructSocketMessage(const char* action, const char* message) {
	/*int size = strlen(action) + strlen(message);
	size += 11;*/
	int needed = snprintf(NULL, 0, "{ \"%s\":\"%s\" }\n", action, message);
	if (needed <  0) {
		perror("[constructSocketMessage] snprintf");
		writeLog("Could not compute size of socket message.", 2, 0);
		return;
	} 
	g_strings.socket_message = malloc((size_t)needed + 1);
    	if (g_strings.socket_message == NULL) {
        	printf("Memory allocation failed.\n");
		writeLog("Memory allocation failed [constructSocketMessage:g_strings.socket_message]", 2, 0);
        	return;
    	}
	//else
	//	memset(g_strings.socket_message, '\0', (size_t)size * sizeof(char));
    	int written = snprintf(g_strings.socket_message, (size_t)needed + 1, "{ \"%s\":\"%s\" }\n", action, message);
	if (written != needed) {
		writeLog("[constructSocketMessage] snprintf mismatch. This should not really ever happen.", 2, 0);
		free(g_strings.socket_message);
		g_strings.socket_message = NULL;
	}
}

int directoryExists(const char *checkDir, size_t length) {
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Checking directory %s", checkDir);
        writeLog(trim(g_strings.infostr), 0, 1);

        DIR* dir = opendir(checkDir);
        if (dir) {
                closedir(dir);
                return 0;
        }
        else if (ENOENT == errno) {
                return 1;
        }
        else { return 2; }
}

int getIdFromName(char *plugin_name) {
	char* pluginName = NULL;
	int retVal = -1;

	for (int i = 0; i < g_ints.decCount; i++) {
		pluginName = malloc((size_t)g_sizes.pluginitemname_size * sizeof(char)+1);
		if (pluginName == NULL) {
			fprintf(stderr, "Failed to allocate memory.\n");
			writeLog("Failed to allocate memory [getIdFromName:pluginName]", 2, 0);
			return -1;
		}
		else
			memset(pluginName, '\0', (size_t)g_sizes.pluginitemname_size+1 * sizeof(char));
                strncpy(pluginName, g_pointers.g_plugins[i]->name, g_sizes.pluginitemname_size);
		pluginName[g_sizes.pluginitemname_size] = '\0';
		removeChar(pluginName, '[');
		removeChar(pluginName, ']');
		if (strcmp(trim(plugin_name), pluginName) == 0) {
			retVal = g_pointers.g_plugins[i]->id;
			break;
		}
		free(pluginName);
		pluginName = NULL;
	}
	if (pluginName != NULL) {
		free(pluginName);
		pluginName = NULL;
	}
	return retVal +1;
}

void* apiThread(void* data) {
	int retrys = 3;
	int retry_count = 0;
	int createSocketRetVal = 0;
        pthread_detach(pthread_self());
	createSocketRetVal = createSocket(g_network.server_fd);
        while ((createSocketRetVal != 0)  && (retry_count > retrys)) {
		perror("Create socket.");
		printf("Could not create socket!\n");
		writeLog("Could not create socket for API thread.", 1, 0);
		sleep(1);
		createSocketRetVal = createSocket(g_network.server_fd);
		retry_count++;
	}
	g_sizes.total_threads_run++;
	pthread_mutex_lock(&g_threading.mtx);
	g_sizes.thread_counter--;
	pthread_mutex_unlock(&g_threading.mtx);
        pthread_exit(NULL);
	g_sizes.total_threads_run++;
}

void startApiSocket() {
        pthread_t thread_id;
        int rc;

        rc = pthread_create(&thread_id, NULL, apiThread, "almondapi");
        if(rc != 0) {
		printf("Error creating phtread\n");
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Error: return code from phtread_create is %d\n", rc);
                writeLog(trim(g_strings.infostr), 2, 0);
		return;
        }
	pthread_detach(thread_id);
	pthread_setspecific(thread_id, "API Connection Listener");
	printf("New thread accepting socket created.\n");
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Created new thread (%lu) listening for connections on port %d \n", thread_id, g_ints.local_port);
        writeLog(trim(g_strings.infostr), 0, 0);
	g_sizes.total_threads_run++;
	pthread_mutex_lock(&g_threading.mtx);
	g_sizes.thread_counter++;
	pthread_mutex_unlock(&g_threading.mtx);
}

void changeSetValue(int id, int newval) {
	if (id > 10) {
		if (newval > 0)
			newval = 1;
		else
			newval = 0;
	}
	switch (id) {
		case 1:
			g_bools.logPluginOutput = (newval > 0);
			break;
		case 2:
			g_bools.saveOnExit = (newval > 0);
			break;
		case 10:
                        if ((newval < 1000) || (newval > 60000)) {
				writeLog("API call is trying to set sleep to unsupported value.", 1, 0);
				writeLog("Scheduler sleep value is unchanged.", 0, 0);
			}
			else
				g_ints.schedulerSleep = newval;
			break;
		case 11:
			g_sizes.kafka_start_id = newval;
			break;
		case 12:
			g_ints.push_port = newval;
			break;
		case 13:
			if (newval < 15) {
				writeLog("API call is trying to set push interval to unsupported value.", 1, 0);
				writeLog("Push interval value is unchanged.", 0, 0);
			}
			else
				g_ints.push_interval = newval;
			break;
		default:
			writeLog("changeSetValue called with wrong index", 1, 0);
	}
}

void setMaintenanceStatus(int id, char* value) {
	int maintenance_status_value = 1;
	if (strcmp(value, "true") == 0) {
		maintenance_status_value = 0;
	}
	if (g_pointers.g_plugins[id]->active != maintenance_status_value)
		g_pointers.g_plugins[id]->active = maintenance_status_value;
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Updating maintenance status to %d for plugin '%s'.", maintenance_status_value, g_pointers.g_plugins[id]->name);
	writeLog(g_strings.infostr, 1, 0);
}

void setPluginOutput(int newval) {
	if (newval > 0)
	       	newval = 1 ;
	else newval = 0;
	g_bools.logPluginOutput = (newval > 0);
}

int toggleQuickStart(int on) {
	FILE * fPtr = NULL;
	FILE * fTemp = NULL;
	char * filename = NULL;
	char * tempfile = NULL;

	char buffer[1000];
	char enable[50] = "g_pointers.scheduler.quickStart=1";
	char disable[50] = "g_pointers.scheduler.quickStart=0";
	filename = "/etc/almond/almond.conf";
	tempfile = "/etc/almond/almond.temp";

	fPtr = fopen(filename, "r");
	fTemp = fopen(tempfile, "w");

	if (fPtr == NULL || fTemp == NULL) {
		writeLog("Could not update quick start value in configuration file. Read error.", 1, 0);
		exit(EXIT_SUCCESS);
	}

	while ((fgets(buffer, 1000, fPtr)) != NULL){
                char *pch = strstr(buffer, "quickStart");
	       	if (pch) {
			if (on > 0)	
				fputs(enable, fTemp);
			else
				fputs(disable, fTemp);
			fputs("\n", fTemp); 
		}
		else
			fputs(buffer, fTemp);
	}
	fclose(fPtr);
	fPtr = NULL;
	fclose(fTemp);
	fTemp = NULL;
	remove(filename);
	rename(tempfile, filename);
	writeLog("Updated almond.conf file", 0, 0);
	return 0;
}

void send_socket_message(int socket, SSL* ssl,  int id, int aflags) {
        //char header[100] = "HTTP/1.1 200 OK\nContent-Type:application/txt\nContent-Length: ";
	const char *fmt = 
		"HTTP/1.1 200 OK\n"
  		"Content-Type:application/txt\n"
  		"Content-Length: %zu\n\n";
	char * send_message = NULL;
	size_t content_length = 0;
	size_t total = 0;
	//char lenbuf[21];

	if (g_ints.args_set == 0) {
		switch (g_ints.api_action) {
        		case API_READ:
				apiReadData(id, aflags);
                        	break;
			case API_MONITOR:
                        	apiMonitorItem(id, aflags);
                        	break;
			case API_RUN:
				apiRunPlugin(id, aflags);
				break;
                	case API_DRY_RUN:
				apiDryRun(id);	
                       	 	break;
                	case API_EXECUTE_AND_READ:
				apiRunAndRead(id, aflags);
                        	break;
			case API_GET_METRICS:
				apiGetMetrics();
				break;
			case API_READ_ALL:
				apiReadAll();
				break;
			case API_EXECUTE_GARDENER:
                                executeGardener();
				constructSocketMessage("execute", "Almond gardener script executed.");
                                break;
                        case API_ENABLE_TIMETUNER:
                                g_bools.enableTimeTuner = true;
                                writeLog("Time tuner enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Time tuner is now enabled.");
                                break;
                        case API_DISABLE_TIMETUNER:
                                g_bools.enableTimeTuner = false;
                                writeLog("Time tuner disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Time tuner is now disabled.");
                                break;
                        case API_ENABLE_GARDENER:
                                g_bools.enableGardener = true;
                                writeLog("Gardener enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Gardener is now enabled.");
                                break;
                        case API_DISABLE_GARDENER:
                                g_bools.enableGardener = false;
                                writeLog("Gardener disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Gardener is now disabled.");
                                break;
			case API_ENABLE_CLEARCACHE:
                                g_bools.enableClearDataCache = true;
                                writeLog("ClearDataCache enabled through API call.", 0, 0);
				constructSocketMessage("enable", "ClearDataCache is now enabled.");
                                break;
                        case API_DISABLE_CLEARCACHE:
                                g_bools.enableClearDataCache = false;
                                writeLog("ClearDataCache disabled through API call.", 0, 0);
				constructSocketMessage("disable", "ClearDataCache is now disabled.");
                                break;
                        case API_ENABLE_QUICKSTART:
                                g_bools.quick_start = true;
				toggleQuickStart(1);
                                writeLog("Quick start enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Quick start is now enabled.");
                                break;
                        case API_DISABLE_QUICKSTART:
                                g_bools.quick_start = false;
				toggleQuickStart(0);
                                writeLog("Quick start disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Quick start is now disabled");
                                break;
                        case API_ENABLE_STANDALONE:
                                g_bools.standalone = true;
                                writeLog("Standalone mode enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Standalone mode is now enabled");
                                break;
                        case API_DISABLE_STANDALONE:
                                g_bools.standalone = false;
                                writeLog("Standalone mode disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Standalone mode is now disabled.");
                                break;
			case API_ENABLE_PUSH:
                                g_bools.use_push = true;
                                writeLog("Almond push enabled through API call.", 0, 0);
                                constructSocketMessage("enable", "Almond push is now enabled.");
                                break;
			case API_DISABLE_PUSH:
                                g_bools.use_push = false;
                                writeLog("Almond push disabled through API call.", 0, 0);
                                constructSocketMessage("disable", "Almond push is now disabled.");
                                break;
                        case API_ENABLE_METRICS_PUSH:
                                g_bools.use_metrics_push = true;
                                writeLog("Almond metrics push enabled through API call.", 0, 0);
                                constructSocketMessage("enable", "Almond metrics push is now enabled.");
                                break;
                        case API_DISABLE_METRICS_PUSH:
                                g_bools.use_metrics_push = false;
                                writeLog("Almond metricd push disabled through API call.", 0, 0);
                                constructSocketMessage("disable", "Almond metrics push is now disabled.");
				break;
			case API_SET_PLUGINOUTPUT:
                                writeLog("Log plugin output toggled through API call.", 0, 0);
				constructSocketMessage("set", "Log plugin output toggled.");
                                break;
			case API_SET_SAVEONEXIT:
                                writeLog("Save on exit is toggled through API call.", 0, 0);
				constructSocketMessage("set", "Save on exit output toggled.");
                                break;
			case API_SET_SLEEP:
				writeLog("Scheduler sleep toggled through API call.", 1, 0);
				constructSocketMessage("set", "Scheduler sleep toggled");
                                break;
			case API_SET_KAFKATAG:
                                writeLog("Kafka tag toggled through API call.", 0, 0);
				constructSocketMessage("set", "Kafka tag toggled");
                                break;
			case API_SET_KAFKA_START_ID:
                                writeLog("Kafka start id toggled through API call.", 0, 0);
				constructSocketMessage("set","Kafka start id toggled.");
				break;
			case API_SET_HOSTNAME:
				writeLog("The virtual hostname of the unit has been changed through API call.", 1, 0);
				constructSocketMessage("set", "Virtual hostname has been toggled.");
				break;
			case API_SET_METRICSPREFIX:
				writeLog("Metrics prefix is toggled through API call.", 0, 0);
				constructSocketMessage("set", "Metrics prefix will be changed.");
				break;
			case API_SET_KAFKATOPIC:
				writeLog("Kafka topic name toggled through API call.", 1, 0);
				constructSocketMessage("set", "Kafka topic toggled.");
				break;
			case API_SET_JSONFILENAME:
				writeLog("Json file name is toggled through API call.", 1, 0);
				constructSocketMessage("set", "Json export file name toggled.");
				break;
			case API_SET_METRICSFILENAME:
				writeLog("Metrics file name is toggled through API call.", 1, 0);
				constructSocketMessage("set", "Metrics file name toggled.");
				break;
                        case API_SET_MAINTENANCE_STATUS:
                                writeLog("Maintenance has been toggled through API call.", 1, 0);
                                constructSocketMessage("maintenance", "Maintenance status has been updated.");
                                break;
			case API_SET_SCHEDULER_TYPE:
				writeLog("Scheduler type changed through API call.", 1, 0);
				constructSocketMessage("g_pointers.scheduler", "Scheduler type changed");
				break;
			case API_SET_PUSH_URL:
				writeLog("Push url changed through API call.", 1, 0);
				constructSocketMessage("pushurl", "Push url changed");
				break;
			case API_SET_PUSH_PORT:
				writeLog("Push port changed through API call.", 1, 0);
				constructSocketMessage("pushport", "Push port changed");
				break;
			case API_SET_PUSH_INTERVAL:
				writeLog("Push interval changed through API call.", 1, 0);
				constructSocketMessage("pushinterval", "Push interval changed");
				break;
			case API_GET_HOSTNAME:
				apiGetHostName();
				break;
			case API_GET_KAFKATAG:
				apiGetVars(1);
				break;
			case API_GET_METRICSPREFIX:
				apiGetVars(2);
				break;
			case API_GET_JSONFILENAME:
				apiGetVars(3);
				break;
			case API_GET_METRICSFILENAME:
				apiGetVars(4);
				break;
			case API_GET_KAFKATOPIC:
				apiGetVars(5);
				break;
			case API_GET_SLEEP:
				apiGetVars(6);
				break;
			case API_GET_SAVEONEXIT:
				apiGetVars(7);
				break;
			case API_GET_PLUGINOUTPUT:
				apiGetVars(8);
				break;
			case API_GET_KAFKA_START_ID:
				apiGetVars(9);
				break;
		        case API_GET_PLUGIN_RELOAD_TS:
				apiGetVars(10);
				break;
			case API_GET_SCHEDULER:
				apiGetVars(11);
				break;
			case API_GET_PUSH_URL:
				apiGetVars(12);
				break;
			case API_GET_PUSH_PORT:
				apiGetVars(13);
				break;
			case API_GET_PUSH_INTERVAL:
				apiGetVars(14);
				break;
			case API_CHECK_PLUGIN_CONFIG:
				apiCheckPluginConf();
				break;
			case API_RELOAD_CONFIG_HARD:
				apiReloadConfigHard();
				break;
			case API_RELOAD_CONFIG_SOFT:
				apiReloadConfigSoft();
				break;
			case API_RELOAD_ALMOND:
				apiReload();
				break;
			case API_ALMOND_VERSION:
				apiShowVersion();
				break;
			case API_ALMOND_STATUS:
				apiShowStatus();
				break;
			case API_ALMOND_PLUGINSTATUS:
				apiShowPluginStatus();
				break;
			case API_DENIED:
				constructSocketMessage("return", "Access denied: You need a valid token.");
                                break;
                        case API_ERROR:
				constructSocketMessage("return", "Error: Could not parse API call parameters.");
				break;
                	default:
                        	//printf("The request did not trigger any action.\n");
				constructSocketMessage("return", "The request id did not trigger any action.");
		}
        }
	else {
		if (g_ints.api_action == API_MONITOR) {
                       	apiMonitorItem(id, aflags);
		}
		g_ints.args_set = 0;
	}
	content_length = (size_t)strlen(g_strings.socket_message); 
	int hdr_len = snprintf(NULL, 0, fmt, content_length);
	if (hdr_len < 0) {
		writeLog("[send_socket_message] snprintf size calculation failed.", 2, 0);
		return;
	}
	//sprintf(len, "%li", content_length);
	/*int written = snprintf(lenbuf, sizeof(lenbuf), "%zu", content_length);
	if (written < 0) {
		writeLog("[send_socket_message] snprintf error.", 2, 0);
		return;
	}
	if (written >= sizeof(lenbuf)) {
		writeLog("[send_socket_message] snprintf truncated output", 1, 0);
	}
        strcat(header, trim(lenbuf));
        strcat(header, "\n\n");*/
	char *header = malloc((size_t)hdr_len +1);
	if (!header) {
		writeLog("[send_socket_message] Out of memory allocating header.", 2, 0);
		return;
	}
	snprintf(header, (size_t)hdr_len + 1, fmt, content_length);
	//content_length += (size_t)strlen(header);
	total = (size_t)hdr_len + content_length;
	send_message = malloc(total +1);
	if (send_message == NULL) {
		perror("Failed to allocate memory for send_message");
		writeLog("Could not allocate memory [send_socket_message:send_message]", 2, 0);
		free(header);
		return;
	}
	//else
	//	memset(send_message, '\0', (content_length+1) * sizeof(char));
	memcpy(send_message, header, hdr_len);
        //strncpy(send_message, header, (size_t)(sizeof(header)));
	//strcat(send_message, g_strings.socket_message);
	memcpy(send_message + hdr_len, g_strings.socket_message, content_length);
	send_message[total] = '\0';
	if (g_bools.use_ssl) {
		if (SSL_write(g_network.ssl, send_message, strlen(send_message)) <= 0) {
			writeLog("Could not send g_network.ssl message to client", 1, 0);
		}
	}
	else {
        	if (send(socket, send_message, strlen(send_message), 0) < 0) {
                	writeLog("Could not send message to client.", 1, 0);
        	}
	}
	writeLog("Message sent on socket. Closing connection.", 0, 0);
        close(socket);
	free(send_message);
	free(header);
	send_message = NULL;
	if (g_strings.socket_message != NULL) {
		free(g_strings.socket_message);
		g_strings.socket_message = NULL;
	}
}

struct json_object* getJsonValue(struct json_object *jobj, const char* key) {
        struct json_object *tmp;
        if (json_object_object_get_ex(jobj, key, &tmp)) {
                return tmp;
        }
        return NULL;
}

void parseClientMessage(char str[], int arr[], bool jwt_valid) {
        struct json_object *jobj, *jaction, *jid, *jname,  *jflags;
        struct json_object *jargs, *jvalue, *jmode, *joption;
	struct json_object *jtoken;
        char *value = NULL;
        char action[13] = {0};
        char sid[10] = {0};
	char flags[10] = {0};
	char args[100] = {0};
	char sval[100] = {0};
	char name[50] = {0};
	char mode[10] = {0};
	char * fname = NULL;
        char * lname = NULL;
        char username[40] = {0};
        char* token = NULL;
        char line[100] = {0};
        int id = -1;
	int aflags = 0;
	int bExecute = 0;
        enum json_tokener_error jerr;

	g_ints.args_set = 0;
	//printf("DEBUG: [parseClientMessage] str = %s\n", str);
        json_tokener *tok = json_tokener_new();
	if (str != NULL)
        	jobj = json_tokener_parse_ex(tok, str, (size_t)(strlen(str)));
	else {
		fprintf(stderr, "parseClientMessage: str is NULL.");
		writeLog("[parseClientMessage] Recieved NULL instead of string.", 1, 0);
           	json_tokener_free(tok);
		return;
	}
        jerr = json_tokener_get_error(tok);
        if (jerr != 0) {
                printf("jerr = %s\n", json_tokener_error_desc(jerr));
                printf("j = %p\n", jobj);
                printf("jerr_raw = %d\n", jerr);
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Json error: %s", json_tokener_error_desc(jerr));
		writeLog(trim(g_strings.infostr), 1, 0);
		writeLog("Could not parse API call. Wrong syntax.", 1, 0);
		json_object_put(jobj);
           	json_tokener_free(tok);
                return;
        }
        json_object_object_foreach(jobj, key, val) {
                value = (char *) json_object_get_string(val);
		(void)key;
        }
        jaction = getJsonValue(jobj, "action");
        jid = getJsonValue(jobj, "id");
	jname = getJsonValue(jobj, "name");
	jflags = getJsonValue(jobj, "flags");
	jargs = getJsonValue(jobj, "args");
	jtoken = getJsonValue(jobj, "token");
	jvalue = getJsonValue(jobj, "value");
	jmode = getJsonValue(jobj, "mode");
	joption = getJsonValue(jobj, "option");
	if (jid != NULL) {
        	//strncpy(sid, json_object_to_json_string_ext(jid, JSON_C_TO_STRING_PLAIN), 5);
		snprintf(sid, sizeof(sid), "%s", json_object_to_json_string_ext(jid, JSON_C_TO_STRING_PLAIN));
        	removeChar(sid, '"');
	}
	if (jaction != NULL) {
        	//strncpy(action, json_object_to_json_string_ext(jaction, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 12);
		snprintf(action, sizeof(action), "%s", json_object_to_json_string_ext(jaction, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
        	removeChar(action, '"');
	}
	if (jname != NULL) {
		//strncpy(name, json_object_to_json_string_ext(jname, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 50);
		snprintf(name, sizeof(name), "%s", json_object_to_json_string_ext(jname, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
		removeChar(name, '"');
	}
	if (jmode != NULL) {
		//strncpy(mode, json_object_to_json_string_ext(jmode, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 5);
		snprintf(mode, sizeof(mode), "%s", json_object_to_json_string_ext(jmode, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
		removeChar(mode, '"');
	}
        if (jflags != NULL) {
		//strncpy(flags, json_object_to_json_string_ext(jflags, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 10);
		snprintf(flags, sizeof(flags), "%s", json_object_to_json_string_ext(jflags, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY));
		removeChar(flags, '"');
		if (strcmp(trim(flags), "verbose") == 0) {
			aflags = 1;
		}
		else if (strcmp(trim(flags), "dry") == 0) {
			aflags = API_DRY_RUN;
			g_ints.api_action = API_DRY_RUN;
		}
		else if (strcmp(trim(flags), "all") == 0) {
			aflags = 10;
		}
		else if (strcmp(trim(flags), "soft") == 0) {
                        aflags = 200;
        	}
		else if (strcmp(trim(flags), "hard") == 0) {
			aflags = 205;
		}
		else aflags = 0;
	}
	if (jargs != NULL) {
		//strncpy(args, json_object_to_json_string_ext(jargs, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 100);
		snprintf(args, sizeof(args), "%s", json_object_to_json_string_ext(jargs, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY));
		removeChar(args, '"');
		if (aflags > 199) {
			if (joption != NULL) {
				// Make g_strings.customMonitorVals atomic
				char option[25] = {0};
				snprintf(option, sizeof(option), "%s", json_object_to_json_string_ext(joption, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY));
				removeChar(option, '"');
				if (g_strings.customMonitorVals != NULL) {
					free(g_strings.customMonitorVals);
					g_strings.customMonitorVals = NULL;
				}
				size_t cmv_size = sizeof(args) + sizeof(option);
				g_strings.customMonitorVals = malloc(cmv_size);
				snprintf(g_strings.customMonitorVals, cmv_size, "%s;%s", args, option);
				aflags++;
			} 
			else {
				printf("DEBUG: [parseClientMessage] joption == NULL\n");
			}
		}
		g_ints.args_set++;
	}
	else g_ints.args_set = 0;
	if (jvalue != NULL) {
		//strncpy(sval, json_object_to_json_string_ext(jvalue, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 100);
		snprintf(sval, sizeof(sval), "%s", json_object_to_json_string_ext(jvalue, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY));
		removeChar(sval, '"');
	}
	bExecute = jwt_valid ? 1 : 0;
	if (jtoken != NULL && !bExecute) {
		token = malloc(30);
		if (token == NULL) {
			writeLog("Could not allocate memory for execute token", 1, 0);
		}
		else
			memset(token, '\0', 30 * sizeof(char));
                //strncpy(token, json_object_to_json_string_ext(jtoken, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 30);
		snprintf(token, 30, "%s", json_object_to_json_string_ext(jtoken, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
                removeChar(token, '"');
		trim(token);
                FILE *in_file = fopen("/etc/almond/tokens", "r");
                if (in_file == NULL)
                {
                        writeLog("Could not find token file.", 1, 0);
                }
                else {
                        int i = 1;
                        while (fscanf(in_file, "%s", line) == 1) {
                                if (i == 1){
					/*fname = malloc((size_t)sizeof(line)+1);
					if (fname == NULL) {
						writeLog("Could not allocate message [parseClientMessage:fname]", 2, 0);
						json_object_put(jobj);
   						json_tokener_free(tok);
						return;
					}
					else
						memset(fname, '\0', (size_t)sizeof(line)+1 * sizeof(char));
					strncpy(fname, trim(line), sizeof(line));*/
					char *trimmed_line = trim(line);
					size_t len = strlen(trimmed_line);
					fname = malloc(len +1);
					if (fname == NULL) {
						writeLog("Could not allocate message [parseClientMessage:fname]", 2, 0);
						json_object_put(jobj);
   						json_tokener_free(tok);
       						return;
    					}
					strcpy(fname, trimmed_line); 
                                }
                                if (i == 2){
                                        lname = malloc((size_t)sizeof(line)+1);
					if (lname == NULL) {
						writeLog("Could not allocate memory [parseClientMessage:lname]", 2, 0);
						json_object_put(jobj);
   						json_tokener_free(tok);
						return;
					}
					else
						memset(lname, '\0', (size_t)sizeof(line)+1 * sizeof(char));
					strncpy(lname, trim(line), sizeof(line));
                                }
                                i++;
                                if (strstr(line, token) != 0) {
                                        bExecute = 1;
                                        // Get username from file to log
					/*strncpy(username, "", 2);
					strcat(username, fname);
                                        strcat(username, " ");
                                        strcat(username, lname);*/
					snprintf(username, sizeof(username), "%s %s", fname, lname);
                                        snprintf(g_strings.infostr, g_sizes.infostr_size, "User '%s' granted API execution rights from token.", username);
                                        writeLog(trim(g_strings.infostr), 0, 0);
                                        flushLog();
					free(fname);
					free(lname);
					fname = lname = NULL;
                                        break;
                                }
                                if (i == 4){
                                        i = 1;
                                        free(fname);
                                        free(lname);
					fname = NULL;
					lname = NULL;
                                }
                        }
			fclose(in_file);
			in_file = NULL;
                }
		free(token);
		token = NULL;
        }
        if ((strcmp(trim(action), "read") == 0) || (strcmp(trim(action), "get") == 0)) {
		if (aflags == 10) {
			g_ints.api_action = API_READ_ALL;
		}
		else {
                	g_ints.api_action = API_READ;
		}
        }
	else if (strcmp(trim(action), "monitor") == 0) {
		g_ints.api_action = API_MONITOR;
	}
        else if ((strcmp(trim(action), "execute") == 0)|| (strcmp(trim(action), "run") == 0)) {
		if (bExecute > 0) {
                        if (strcmp(trim(name), "gardener") == 0) {
                                g_ints.api_action = API_EXECUTE_GARDENER;
                        }
                        else if (g_ints.api_action != API_DRY_RUN)
                                g_ints.api_action = API_RUN;
                }
                else g_ints.api_action = API_DENIED;
        }
	else if ((strcmp(trim(action), "runread") == 0) || (strcmp(trim(action), "exread") == 0)) {
		if (bExecute != 0)
			g_ints.api_action = API_EXECUTE_AND_READ;
		else 
			g_ints.api_action = API_DENIED;
	}
	else if ((strcmp(trim(action), "metrics") == 0) || (strcmp(trim(action), "getm") == 0)) { 
		g_ints.api_action = API_GET_METRICS;
	}
        else if (strcmp(trim(action), "maintenance") == 0) {
                if (jid == NULL) {
                	id = getIdFromName(trim(name));
                }
                else {
			id = atoi(sid);
		}
		if (id < 0) {
			g_ints.api_action = API_ERROR;
		}
		else {
			if ((strcmp(trim(value), "true") == 0) || (strcmp(trim(value), "false") == 0)) {
				setMaintenanceStatus(id, trim(value));
        			g_ints.api_action = API_SET_MAINTENANCE_STATUS;
			}
			else {
				g_ints.api_action = API_ERROR;
			}
		}
        }	
	else if ((strcmp(trim(action), "enable") == 0) || (strcmp(trim(action), "disable") == 0)) {
 		if (bExecute != 0) {
 			if (strcmp(trim(name), "timetuner") == 0) {
 				if (strcmp(trim(action), "enable") == 0)
 					g_ints.api_action = API_ENABLE_TIMETUNER;
 				else if (strcmp(trim(action), "disable") == 0)
 					g_ints.api_action = API_DISABLE_TIMETUNER;
 			}
 			if (strcmp(trim(name), "gardener") == 0) {
 				if (strcmp(trim(action), "enable") == 0)
 					g_ints.api_action = API_ENABLE_GARDENER;
 				else if (strcmp(trim(action), "disable") == 0)
 					g_ints.api_action = API_DISABLE_GARDENER;
 			}
                        if (strcmp(trim(name), "cleancache") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        g_ints.api_action = API_ENABLE_CLEARCACHE;
                                else if (strcmp(trim(action), "disable") == 0)
                                        g_ints.api_action = API_DISABLE_CLEARCACHE;
                        }
			if (strcmp(trim(name), "quickstart") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        g_ints.api_action = API_ENABLE_QUICKSTART;
                                else if (strcmp(trim(action), "disable") == 0)
                                        g_ints.api_action = API_DISABLE_QUICKSTART;
                        }
			if (strcmp(trim(name), "g_bools.standalone") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        g_ints.api_action = API_ENABLE_STANDALONE;
                                else if (strcmp(trim(action), "disable") == 0)
                                        g_ints.api_action = API_DISABLE_STANDALONE;
                        }
			if (strcmp(trim(name), "push") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        g_ints.api_action = API_ENABLE_PUSH;
                                else if (strcmp(trim(action), "disable") == 0)
                                        g_ints.api_action = API_DISABLE_PUSH;
                        }
			if (strcmp(trim(name), "pushmetrics") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        g_ints.api_action = API_ENABLE_METRICS_PUSH;
                                else if (strcmp(trim(action), "disable") == 0)
                                        g_ints.api_action = API_DISABLE_METRICS_PUSH;
                        }
 		}
		else
			g_ints.api_action = API_DENIED;
	}
	else if ((strcmp(trim(action), "set") == 0) || (strcmp(trim(action), "setvar") == 0)) {
		if (bExecute != 0) {
			pthread_mutex_lock(&g_threading.update_mtx);
			//printf("DEBUG: sval = %s\n", trim(sval));
			if (strcmp(trim(name), "pluginoutput") == 0) {
				int val = atoi(trim(sval));
				setPluginOutput(val);
				changeSetValue(1, val);
				g_ints.api_action = API_SET_PLUGINOUTPUT;
			}
			else if (strcmp(trim(name), "saveonexit") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(2, val);
				g_ints.api_action = API_SET_SAVEONEXIT;
			}
			else if (strcmp(trim(name), "sleep") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(10, val);
				g_ints.api_action = API_SET_SLEEP;
			}
			else if (strcmp(trim(name), "kafkatag") == 0) {
				setApiCmdFile("kafkatag", trim(sval));
				writeLog("A command file for changing kafkatag has been created.", 0, 0);
				g_ints.api_action = API_SET_KAFKATAG;
			}
			else if (strcmp(trim(name), "kafkatopic") == 0) {
				setApiCmdFile("kafkatopic", trim(sval));
				writeLog("A command file for changing Kafka topic name has been created.", 0, 0);
				g_ints.api_action = API_SET_KAFKATOPIC;
			}
			else if (strcmp(trim(name), "jsonfilename") == 0) {
				setApiCmdFile("jsonfilename", trim(sval));
				writeLog("A command file for changing json export file name has been created.", 0, 0);
				g_ints.api_action = API_SET_JSONFILENAME;
			}
			else if (strcmp(trim(name), "metricsfilename") == 0) {
				setApiCmdFile("metricsfilename", trim(sval));
				writeLog("A command file for changing metrics file name has been created.", 0, 0);
				g_ints.api_action = API_SET_METRICSFILENAME;
			}
			else if (strcmp(trim(name), "pushurl") == 0) {
				setApiCmdFile("pushurl", trim(sval));
				writeLog("A command file for changing push url has been created.", 0, 0);
				g_ints.api_action = API_SET_PUSH_URL;
			}
			else if (strcmp(trim(name), "pushport") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(12, val);
				g_ints.api_action = API_SET_PUSH_PORT;
			}
			else if (strcmp(trim(name), "pushinterval") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(13, val);
				g_ints.api_action = API_SET_PUSH_INTERVAL;
			}
			else if (strcmp(trim(name), "kafkastartid") == 0) {
				int val = atoi(trim(sval));
				if (val > 0) {
					changeSetValue(11, val);
                                	snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka start id is set to '%d'", val);
                                	writeLog("Kafka start id is toggled through API call.", 0, 0);
                                	writeLog(trim(g_strings.infostr), 0, 0);
				}
				else {
					snprintf(g_strings.infostr, g_sizes.infostr_size, "Could not set Kafka start id to '%s'", sval);
					writeLog("Kafka start id was toggled through API call.", 0, 0);
					writeLog(trim(g_strings.infostr), 1, 0);
				}
				g_ints.api_action = API_SET_KAFKA_START_ID;
                        }
			else if (strcmp(trim(name), "hostname") == 0) {
				char* newname = malloc(256);
				if (!newname) {
					perror("Failed to allocate memory");
					exit(EXIT_FAILURE);
				}
				else
					memset(newname, '\0', 256);
				strncpy(newname, trim(sval), strlen(sval));
				snprintf(g_strings.infostr,  g_sizes.infostr_size, "Virtal hostname set to '%s'", newname);
				writeLog("Hostname (virtual) is toggled through API call.", 1, 0);
				writeLog(trim(g_strings.infostr), 1, 0);
				free(newname);
				setApiCmdFile("hostname", trim(sval));
				g_ints.api_action = API_SET_HOSTNAME;
			}
			else if (strcmp(trim(name), "metricsprefix") == 0) {
				char* newname = malloc(31);
				if (!newname) {
					writeLog("Could not allocate memory [parseClientMessage:metricsprefix->newname]\n", 1, 0);
					exit(EXIT_FAILURE);
				}
				else
					memset(newname, '\0', 31);
				strncpy(newname, trim(sval), strlen(sval));
				free(newname);
				setApiCmdFile("metricsprefix", trim(sval));
				g_ints.api_action = API_SET_METRICSPREFIX;
			}
			else if (strcmp(trim(name), "g_pointers.scheduler") == 0) {
				char* s_type = malloc(9);
				if (!s_type) {
					writeLog("Could not allocate memory[parseClientMessage: scheduler_type]\n", 1, 0);
					exit(EXIT_FAILURE);
				}
				else
					memset(s_type, '\0', 9);
				strncpy(s_type, trim(sval), strlen(sval));
				if (strcmp(s_type, "external") == 0) {
					writeLog("External g_pointers.scheduler activated. Almond g_pointers.scheduler is now inactive.", 1, 0);
					setApiCmdFile("g_pointers.scheduler", "external");
					g_ints.api_action = API_SET_SCHEDULER_TYPE;
				}
				else if (strcmp(s_type, "internal") == 0) {
					writeLog("Almond g_pointers.scheduler now activated through API call.", 0, 0);
					setApiCmdFile("g_pointers.scheduler", "internal");
					g_ints.api_action = API_SET_SCHEDULER_TYPE;
				}
				else {
					writeLog("Failed to change g_pointers.scheduler type. Unrecognized value supplied.", 1, 0);
					g_ints.api_action = -1;
				}
				free(s_type);
			}
			else {
				g_ints.api_action = -1;
			}
			pthread_mutex_unlock(&g_threading.update_mtx);
		}
		else {
			writeLog("API action was denied. Wrong or no token supplied.", 1, 0);
			g_ints.api_action = API_DENIED;
		}
	}
	else if (strcmp(trim(action), "getvar") == 0) {
                if (strcmp(trim(name), "hostname") == 0) {
                        g_ints.api_action = API_GET_HOSTNAME;
                }
		else if (strcmp(trim(name), "kafkatag") == 0) {
			g_ints.api_action = API_GET_KAFKATAG;
		}
		else if (strcmp(trim(name), "metricsprefix") == 0) {
			g_ints.api_action = API_GET_METRICSPREFIX;
		}
		else if (strcmp(trim(name), "jsonfilename") == 0) {
			g_ints.api_action = API_GET_JSONFILENAME;
		}
		else if (strcmp(trim(name), "metricsfilename") == 0) {
			g_ints.api_action = API_GET_METRICSFILENAME;
		}
		else if (strcmp(trim(name), "kafkatopic") == 0) {
			g_ints.api_action = API_GET_KAFKATOPIC;
		}
		else if (strcmp(trim(name), "sleep") == 0) {
                        g_ints.api_action = API_GET_SLEEP;
                }
		else if (strcmp(trim(name), "saveonexit") == 0) {
			g_ints.api_action = API_GET_SAVEONEXIT;
		}
		else if (strcmp(trim(name), "pluginoutput") == 0) {
			g_ints.api_action = API_GET_PLUGINOUTPUT;
		}
		else if (strcmp(trim(name), "kafkastartid") == 0) {
			g_ints.api_action = API_GET_KAFKA_START_ID;
		}
		else if (strcmp(trim(name), "g_pointers.scheduler") == 0) {
			g_ints.api_action = API_GET_SCHEDULER;
		}
		else if (strcmp(trim(name), "pushurl") == 0) {
			g_ints.api_action = API_GET_PUSH_URL;
		}
		else if (strcmp(trim(name), "pushport") == 0) {
			g_ints.api_action = API_GET_PUSH_PORT;
		}
		else if (strcmp(trim(name), "pushinterval") == 0) {
			g_ints.api_action = API_GET_PUSH_INTERVAL;
		}
		else {
			g_ints.api_action = -1;
		}
        }
	else if (strcmp(trim(action), "check") == 0) {
		if (strcmp(trim(name), "pluginconfig") == 0) {
			g_ints.api_action = API_CHECK_PLUGIN_CONFIG;
		}
		else if (strcmp(trim(name), "pluginconfigts") == 0) {
			g_ints.api_action = API_GET_PLUGIN_RELOAD_TS;
		}
		else {
			g_ints.api_action = -1;
		}
	}
	else if (strcmp(trim(action), "reload") == 0) {
		if (bExecute != 0) {
			if (strcmp(trim(name), "almond") == 0) {
				// Reload Almond
				g_ints.api_action = API_RELOAD_ALMOND;
			}
			else if (strcmp(trim(name), "plugins") == 0) {
				if (strcmp(trim(mode), "hard") == 0) {
					// Hard reload
					g_ints.api_action = API_RELOAD_CONFIG_HARD;
				}
				else if (strcmp(trim(mode), "soft") == 0) {
					// Soft reload
					g_ints.api_action = API_RELOAD_CONFIG_SOFT;
				}
				else {
					g_ints.api_action = -1;
				}
			}
			else {
				g_ints.api_action = -1;
			}
		}
		else {
			g_ints.api_action = API_DENIED;
		}
	}
	else if(strcmp(trim(action), "almond") == 0) {
		if (strcmp(trim(name), "version") == 0) {
			g_ints.api_action = API_ALMOND_VERSION;
		}
		else if (strcmp(trim(name), "status") == 0) {
			g_ints.api_action = API_ALMOND_STATUS;
		}
		else if (strcmp(trim(name), "plugins") == 0) {
			g_ints.api_action = API_ALMOND_PLUGINSTATUS;
		}
		else {
			g_ints.api_action = -1;
		}
	}
        else {
                g_ints.api_action = 0;
        }
        if (g_ints.api_action > 0) {
                id = atoi(sid);
                if (id == 0) {
			if (jname != NULL) {
				id = getIdFromName(name);
				if (id == -1) {
					// Some api action does not need name
					if (g_ints.api_action > API_NAME_END && g_ints.api_action < API_NAME_START) {
						snprintf(g_strings.infostr, g_sizes.infostr_size, "Try to run API command with name '%s', which does not exist.", name);
                                        	writeLog(trim(g_strings.infostr), 1, 0);
						g_ints.api_action = 0;
						json_object_put(jobj);
   						json_tokener_free(tok);
						return;
					}
					else {
						json_object_put(jobj);
   						json_tokener_free(tok);
						return;
					}
				}
			}	
			else {
				writeLog("Received a bad json-request. API call is aborted.", 1, 0);
				g_ints.api_action = 0;
				json_object_put(jobj);
   				json_tokener_free(tok);
                        	return;
			}
			if (id < 0) {
				writeLog("Could not get id from name. This might cause strange things to happen. Aborting API call.", 1, 0);
				g_ints.api_action = 0;
				json_object_put(jobj);
   				json_tokener_free(tok);
				return;
			}
                }
                id--;
		if (g_ints.args_set > 0 && (g_ints.api_action == API_RUN || g_ints.api_action == API_DRY_RUN || g_ints.api_action == API_EXECUTE_AND_READ || g_ints.api_action == API_MONITOR)) {
			size_t arg_len = strlen(args) + 1;
			g_strings.api_args = malloc(arg_len);
			if (g_strings.api_args == NULL) {
				fprintf(stderr, "Could not allocate memory.\n");
				writeLog("Could not allocate memory [parseClientMessage:g_strings.api_args]", 2, 0);
				json_object_put(jobj);
   				json_tokener_free(tok);
				return;
			}
			else
				memset(g_strings.api_args, '\0', (size_t)strlen(args)+1 * sizeof(char));
			//size_t len = strlen(args)+ 1;
			/*strncpy(g_strings.api_args, args, len-1);
			g_strings.api_args[len-1] = '\0';*/
			//snprintf(g_strings.api_args, len, "%s", args);
			snprintf(g_strings.api_args, arg_len, "%s", args);
			if (g_ints.api_action != API_MONITOR) {
				runPluginArgs(id, aflags, g_ints.api_action);
				if (g_bools.timeScheduler) {
					rescheduleChecks();
				}
				free(g_strings.api_args);
				g_strings.api_args = NULL;
			}
		}
        }
	json_tokener_free(tok);
	if (jobj) json_object_put(jobj);
	arr[0] = id;
	arr[1] = aflags;
}

SSL_CTX *create_context() {
    	const SSL_METHOD *method = TLS_server_method();
     SSL_CTX *ctx;

     ctx = SSL_CTX_new(method);
	if (!ctx) {
         perror("Unable to create SSL context");
         ERR_print_errors_fp(stderr);
         exit(EXIT_FAILURE);
     }
     g_network.ctx = ctx;
     return g_network.ctx;
}

void configure_context(SSL_CTX *ctx) {
	/* Set the key and cert */
    	if (SSL_CTX_use_certificate_file(ctx, g_strings.almondCertificate, SSL_FILETYPE_PEM) <= 0) {
         ERR_print_errors_fp(stderr);
         exit(EXIT_FAILURE);
     }
     if (SSL_CTX_use_PrivateKey_file(ctx, g_strings.almondKey, SSL_FILETYPE_PEM) <= 0 ) {
         ERR_print_errors_fp(stderr);
         exit(EXIT_FAILURE);
     }
	/*if (!SSL_CTX_use_certificate_chain_file(ctx, "almonds.crt"))
         ERR_print_errors_fp(stderr);*/
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    	if(!SSL_CTX_check_private_key(ctx)) {
        	exit(EXIT_FAILURE);
    	}
}

int initSocket () {
        int opt = 1;
        if ((g_network.server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                perror("Socket failed");
                writeLog("Could not initiate socket.", 2, 0);
                return -1;
        }
	/*int flags = fcntl(g_network.server_fd, F_GETFL, 0);
	if (flags == -1) {
    		perror("fcntl F_GETFL failed");
    		writeLog("Failed to get socket flags.", 2, 0);
    		return -1;
	}
	if (fcntl(g_network.server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    		perror("fcntl F_SETFL failed");
    		writeLog("Failed to set socket to non-blocking.", 2, 0);
    		return -1;
	}*/
        if (setsockopt(g_network.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt))) {
                perror("setsockopt");
                writeLog("Setsockopt failed.", 2, 0);
                return -1;
        }
	bzero((char *)&g_network.address, sizeof(g_network.address));
	//memset(&g_network.address, 0, sizeof(g_network.address);
        g_network.address.sin_family = AF_INET;
        g_network.address.sin_addr.s_addr = INADDR_ANY;
        if (g_ints.local_port == ALMOND_API_PORT)
                g_network.address.sin_port = htons((uint16_t)ALMOND_API_PORT);
        else
                g_network.address.sin_port = htons((uint16_t)g_ints.local_port);
        if (bind(g_network.server_fd, (struct sockaddr*)&g_network.address,sizeof(g_network.address))< 0) {
                perror("bind failed");
                writeLog("Failed to bind port.", 2, 0);
                return -1;
        }
	if (g_bools.use_ssl) {
    		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
        	g_network.ctx = create_context();
                configure_context(g_network.ctx);
        }
        writeLog("Almond socket initialized.", 0, 0);
        g_sizes.socket_is_ready = 1;
        return g_sizes.socket_is_ready;
}

int createSocket(int server_fd) {
    char local_msg[g_sizes.infostr_size];
    int client_socket;
    socklen_t client_size;
    struct sockaddr_in client_addr;
    int params[2];

    memset(local_msg, 0, g_sizes.infostr_size);

    g_strings.server_message = malloc((size_t)g_sizes.socketservermessage_size + 1);
    if (g_strings.server_message == NULL) {
        fprintf(stderr, "Failed to allocate memory for servermessage.\n");
        writeLog("Failed to allocate memory [createSocket:servermessage].", 1, 0);
        return -1;
    }
    memset(g_strings.server_message, '\0', (size_t)g_sizes.socketservermessage_size + 1);

    if (g_strings.iam_public_key_file == NULL)
        g_strings.iam_public_key = NULL;
    else
        g_strings.iam_public_key = load_file_to_string(g_strings.iam_public_key_file);

    if (!g_strings.iam_public_key) {
        writeLog("IAM public key not found — JWT authentication disabled", 0, 0);
    }

    if (listen(g_network.server_fd, 5) < 0) {
        perror("listen");
        writeLog("Failed listening...", 2, 0);
        g_sizes.socket_is_ready = 0;
        free(g_strings.server_message);
        return -1;
    }

    snprintf(local_msg, g_sizes.infostr_size, "Ready listening on port %d", g_ints.local_port);
    writeLog(trim(local_msg), 0, 0);

    client_size = sizeof(client_addr);

    while (!g_threading.is_stopping) {

        client_socket = accept(g_network.server_fd, (struct sockaddr*)&client_addr, &client_size);
        if (client_socket < 0) {
            int e = errno;
            if (e == EINTR || e == EBADF || e == EINVAL || g_threading.is_stopping)
                break;
            writeLog("Could not accept client socket.", 1, 0);
            continue;
        }

        // SSL handshake
        if (g_bools.use_ssl) {
            g_network.ssl = SSL_new(g_network.ctx);
            SSL_set_fd(g_network.ssl, client_socket);
            if (SSL_accept(g_network.ssl) <= 0) {
                writeLog("SSL handshake failed.", 1, 0);
                SSL_free(g_network.ssl);
                close(client_socket);
                continue;
            }
        }

        // Allocate per-request buffer
        char *client_message = malloc(g_sizes.socketclientmessage_size + 1);
        if (!client_message) {
            writeLog("Failed to allocate memory for client_message.", 1, 0);
            if (g_bools.use_ssl) {
                SSL_shutdown(g_network.ssl);
                SSL_free(g_network.ssl);
            }
            close(client_socket);
            continue;
        }
        memset(client_message, 0, g_sizes.socketclientmessage_size + 1);

        bool jwt_valid = false;

        // Read request
        int n = g_bools.use_ssl
            ? SSL_read(g_network.ssl, client_message, g_sizes.socketclientmessage_size)
            : recv(client_socket, client_message, g_sizes.socketclientmessage_size, 0);

        if (n <= 0) {
            writeLog("Could not receive client message.", 1, 0);
            free(client_message);
            if (g_bools.use_ssl) {
                SSL_shutdown(g_network.ssl);
                SSL_free(g_network.ssl);
            }
            close(client_socket);
            continue;
        }

        // JWT
        char username[128] = {0};
	char fullname[128] = {0};
        char *auth_header = extract_authorization_header(client_message);
        char *auth_token  = auth_header ? extract_bearer_token(auth_header) : NULL;
        if (auth_token && g_strings.iam_public_key && 
            validate_jwt(auth_token, g_strings.iam_public_key, username, sizeof(username), fullname, sizeof(fullname))) {
            jwt_valid = true;
	    snprintf(g_strings.infostr, g_sizes.infostr_size,"User '%s' (%s) granted API execution rights from IAM provider.",fullname, username);
    	    writeLog(trim(g_strings.infostr), 0, 0);
        } else if (auth_token) {
            writeLog("JWT decode or validation failed", 1, 0);
        }

        free(auth_header);
        free(auth_token);

        // Extract JSON payload
        char *json_start = strchr(client_message, '{');
        char message[250] = {0};

        if (json_start) {
        	size_t len = strlen(json_start);
            	if (len >= sizeof(message)) {
			len = sizeof(message) - 3;
			memcpy(message, json_start, len);
			message[len] = '"';
			message[len+1] = '}';
			message[len+2] = '\0';
		}
		else {
            		memcpy(message, json_start, len);
            		message[len] = '\0';
		}
        } else {
        	writeLog("JSON payload not found [clientMessage]", 1, 0);
            	message[0] = '\0';
        }

        parseClientMessage(message, params, jwt_valid);

        // Send response
        if (g_bools.use_ssl)
            send_socket_message(NO_SOCKET, g_network.ssl, params[0], params[1]);
        else
            send_socket_message(client_socket, NULL, params[0], params[1]);

        // Cleanup
        free(client_message);
        if (g_bools.use_ssl) {
            SSL_shutdown(g_network.ssl);
            SSL_free(g_network.ssl);
        }
        close(client_socket);
    }

    close(g_network.server_fd);
    free(g_strings.server_message);
    return 0;
}

void closeSocket() {
        writeLog("Closing socket.", 0, 0);
        shutdown(g_network.server_fd, SHUT_RDWR);
}

void closejsonfile() {
	const char bFolderName[7] = "backup";
	char ch = '/';
	char dot = '.';
        
	snprintf(g_files.dataFileName, g_sizes.datafilename_size, "%s%c%s", g_dirs.dataDir, ch, g_files.jsonFileName);


	if (g_bools.saveOnExit == false) {
		//printf("\nDEBUG: Save on exit. Remove %s\n", g_files.dataFileName);
		remove(g_files.dataFileName);
	}
	else {
		char date[13];
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
                strftime(date, sizeof(date), "%Y%m%d%H%M", t);
		snprintf(g_dirs.backupDirectory, g_sizes.backupdirectory_size, "%s%c%s", g_dirs.dataDir, ch, bFolderName);
		if (directoryExists(g_dirs.backupDirectory, 100) != 0) {
			int status = mkdir(trim(g_dirs.backupDirectory), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (status != 0 && errno != EEXIST) {
				printf("Failed to create backup directory. Errno: %d\n", errno);
				return;
			}
		}
		char bd[g_sizes.backupdirectory_size];
		char jfn[g_sizes.filename_size];
		memset(bd, 0, sizeof(bd));
		memset(jfn, 0, sizeof(jfn));
		strncpy(bd, g_dirs.backupDirectory, g_sizes.backupdirectory_size);
		strncpy(jfn, g_files.jsonFileName, g_sizes.filename_size);
		snprintf(g_files.newFileName, g_sizes.newfilename_size, "%s%c%s%c%s", bd, ch, jfn, dot, date);
		rename(g_files.dataFileName, g_files.newFileName);
	}	
}

void safe_free(void** ptr) {
	if (*ptr != NULL) {
		free(*ptr);
		*ptr = NULL;
	}
}

void safe_free_str(char **ptr) {
	if (ptr && *ptr) {
        	free(*ptr);
        	*ptr = NULL;
    	}
}

void free_kafka_vars() {
	if (g_ints.kafkaexportreqs > 0) {
		free(g_strings.kafka_brokers);
		if (g_strings.kafka_topic != NULL) {
			free(g_strings.kafka_topic);
			g_strings.kafka_topic = NULL;
		}
		if (g_strings.kafka_tag != NULL) { 
			free(g_strings.kafka_tag);
			g_strings.kafka_tag = NULL;
		}
		free(g_strings.kafkaCACertificate);
		free(g_strings.kafkaProducerCertificate);
		free(g_strings.kafkaSSLKey);
		g_strings.kafka_brokers = NULL;
		g_strings.kafka_topic = NULL;
		g_strings.kafka_tag = NULL;
		g_strings.kafkaCACertificate = NULL;
		g_strings.kafkaProducerCertificate = NULL;
		g_strings.kafkaSSLKey = NULL;
	}
}

void free_iam_roles(void) {
	if (g_arrays.iam_roles_accepted) {
		for (int i = 0; i < g_ints.iam_roles_count; i++) {
            		safe_free_str(&g_arrays.iam_roles_accepted[i]);  
        	}
        	free(g_arrays.iam_roles_accepted);          
		g_arrays.iam_roles_accepted = NULL;
    	}
    	g_ints.iam_roles_count = 0;
}

void free_constants() {
	safe_free_str(&g_dirs.confDir);
	safe_free_str(&g_dirs.dataDir);
	safe_free_str(&g_dirs.storeDir);
	safe_free_str(&g_dirs.logDir);
	safe_free_str(&g_files.pluginDeclarationFile);
	safe_free_str(&g_files.jsonFileName);
	safe_free_str(&g_files.metricsFileName);
	safe_free_str(&g_files.gardenerScript);
	safe_free_str(&g_strings.infostr);
	safe_free_str(&g_dirs.pluginDir);
	safe_free_str(&g_strings.hostName);
	safe_free_str(&g_files.fileName);
	safe_free_str(&g_strings.metricsOutputPrefix);
	safe_free_str(&g_files.logfile);
	safe_free_str(&g_files.dataFileName);
	safe_free_str(&g_dirs.backupDirectory);
	safe_free_str(&g_files.newFileName);
	safe_free_str(&g_strings.gardenerRetString);
	safe_free_str(&g_strings.pluginCommand);
	safe_free_str(&g_strings.pluginReturnString);
	safe_free_str(&g_files.storeName);
	safe_free_str(&g_strings.schemaRegistryUrl);
	safe_free_str(&g_strings.socket_message);
	//safe_free_str(&client_message);
	safe_free_str(&g_strings.kafkaConfigFile);
	safe_free_str(&g_strings.push_url);
        safe_free_str(&g_strings.iam_issuer);
	safe_free_str(&g_strings.iam_public_key_file);
	safe_free_str(&g_strings.iam_aud);
	
	//safe_free_str(&g_strings.logmessage);
	writeLog("All g_arrays.constants freed from memory.", 0, 0);
}

static void free_plugin_item(PluginItem *item) {
    if (!item) return;

    HASH_DEL(g_pointers.g_plugin_map, item);

    free(item->name);
    free(item->description);
    free(item->command);

    free(item->output.retString);

    free(item);
}

void free_all_plugins(void) {
    PluginItem *item, *tmp;

    HASH_ITER(hh, g_pointers.g_plugin_map, item, tmp) {
        free_plugin_item(item);
    }
    g_pointers.g_plugin_map = NULL;

    free(g_pointers.g_plugins);
    g_pointers.g_plugins       = NULL;
    g_ints.g_plugin_count  = 0;
}

void free_structures(int numOfS) {
	free_all_plugins();
	/*if (g_pointers.scheduler != NULL) {
		free(g_pointers.scheduler);
	}*/
}

void freemem() {
	g_dirs.confDir = NULL;
	g_dirs.dataDir = NULL;
	g_dirs.storeDir = NULL;
	g_dirs.pluginDir = NULL;
	g_dirs.logDir = NULL;
	g_files.pluginDeclarationFile = NULL;
	g_strings.hostName = NULL;
	g_files.fileName = NULL;
	g_files.jsonFileName = NULL;
	g_files.metricsFileName = NULL;
	g_files.gardenerScript = NULL;
	g_strings.infostr = NULL;
	if (g_strings.socket_message != NULL) {
		free(g_strings.socket_message);
		g_strings.socket_message = NULL;
	}
	g_files.dataFileName = NULL;
	g_dirs.backupDirectory = NULL;
	g_files.newFileName = NULL;
	g_strings.gardenerRetString = NULL;
	g_strings.pluginCommand = NULL;
	g_strings.pluginReturnString = NULL;
	g_files.storeName = NULL;
        if (g_pointers.update_g_plugins != NULL) {
		for (int i = 0; i < g_sizes.update_declaration_size; i++) {
                	free(g_pointers.update_g_plugins[i].name);
                        free(g_pointers.update_g_plugins[i].description);
                        free(g_pointers.update_g_plugins[i].command);
			g_pointers.update_g_plugins[i].name = NULL;
			g_pointers.update_g_plugins[i].description = NULL;
			g_pointers.update_g_plugins[i].command = NULL;
                }
                free(g_pointers.update_g_plugins);
		g_pointers.update_g_plugins = NULL;
	}
	/*if (update_outputs != NULL) {
		for (int i=0; i < g_sizes.update_output_size; i++) {
			free(update_outputs[i].retString);
			update_outputs[i].retString = NULL;
		}
		free(update_outputs);
	}*/
	if (g_strings.api_args != NULL) {
		free(g_strings.api_args);
		g_strings.api_args = NULL;
	}
}

void destroy_mutexes() {
	pthread_mutex_destroy(&g_threading.mtx);
	pthread_mutex_destroy(&g_threading.update_mtx);
	destroy_log_mutex();
}

void sig_exit_app() {
	g_ints.is_file_open = 1;
	pthread_cond_broadcast(&g_threading.file_opened);
	closeSocket();
	g_ints.shutdown_phase = 1;
        flushLog();
	printf("\nClosing ");
	for (int i = 0; i < 6; i++) {
		printf("%i ", i+1);
		fflush(stdout);
		sleep(1);
	}
	writeLog("Almond says goodbye.", 0, 0);
	g_ints.shutdown_phase = 2;
	closeLog();
        closejsonfile();
        //int try_count = 0;
        /*while (g_sizes.thread_counter > 0) {
                writeLog("Waiting for threads to finish...", 0, 0);
                fflush(g_pointers.fptr);
                sleep(2);
                printf("There are %i threads waiting to finish.\n", g_sizes.thread_counter);
                try_count++;
                if (try_count >= g_ints.max_try) break;
        }*/
	for (int i = 0; i < g_sizes.thread_counter; ++i) {
        	pthread_join(g_arrays.threadIds[i], NULL);
    	}
        free_structures(g_ints.decCount);
	if (g_pointers.scheduler) {
		free(g_pointers.scheduler);
		g_pointers.scheduler = NULL;
	}
        free(g_pointers.g_plugins);
	if (g_bools.useKafkaConfigFile) {
  		free_kafka_memalloc();
	}	
        free_kafka_vars();
	free_iam_roles();
        free_constants();
        //free(g_arrays.threadIds);
        freemem();

	destroy_mutexes();
	if (g_pointers.fptr != NULL) {
		fclose(g_pointers.fptr);
        	g_pointers.fptr = NULL;
	}
        fflush(stdout);
        fflush(stderr);
	if (g_strings.logmessage) {
        	memset(g_strings.logmessage, 0, strlen(g_strings.logmessage));  // Optional: zero out content
                free(g_strings.logmessage);
                g_strings.logmessage = NULL;
        }
        printf("\nExiting application.\n");
}

void install_signals(void) {
	struct sigaction sa;
    	memset(&sa, 0, sizeof(sa));
    	sa.sa_handler = sig_handler;
    	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_ONSTACK;;
	/*sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGINT);
    	sigaddset(&signal_set, SIGTERM);
    	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);*/
    	sigaction(SIGINT,  &sa, NULL);
    	sigaction(SIGTERM, &sa, NULL);
}

void sig_handler(int sig){
	if (already_exiting) return;
	g_threading.is_stopping = 1;
	already_exiting = 1;
	g_threading.shutdown_reason = (sig == SIGINT ? SR_SIGINT
        	: sig == SIGTERM ? SR_SIGTERM
                : SR_NORMAL);
    	/*switch (sigl) {
        	case SIGINT:
			g_threading.shutdown_reason = SR_SIGINT;
			break;
		case SIGKILL:
			g_threading.shutdown_reason = SR_SIGKILL;
			break;
		case SIGTERM:
			g_threading.shutdown_reason = SR_SIGTERM;
			break;
		case SIGSTOP:
			g_threading.shutdown_reason = SR_SIGSTOP;
			break;
    	}*/
	if (g_network.server_fd >= 0) {
		shutdown(g_network.server_fd, SHUT_RDWR);
       		close(g_network.server_fd);
        	g_network.server_fd = -1;
	}
}

int fileExists(const char *checkFile) {
	if (access(checkFile, F_OK) == 0) 
		return 0;
	else
		return 1;
}

int checkPluginFileStat(const char *path, time_t oldMTime, int set) {
	struct stat file_stat;
	int err = stat(path, &file_stat);
	if (err != 0) {
		perror(" [file_is_modified] stat");
		exit(errno);
	}
	g_time.tPluginFile = file_stat.st_mtime;
	if (set > 0) 
		return 0;
	else
		return file_stat.st_mtime > oldMTime;
}

char *getHostName() {
	struct addrinfo hints, *info, *p;
	int gai_result;
        char host_name[1024];
	char *ret = malloc(255);

	host_name[1023] = '\0';
	gethostname(host_name, 1023);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if ((gai_result = getaddrinfo(host_name, "http", &hints, &info)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));        }
	for (p = info; p != NULL; p = p->ai_next) {
		size_t dest_size = 255;
                snprintf(ret, dest_size, "%s", p->ai_canonname);
	}
	freeaddrinfo(info);
	info = NULL;
	return ret;
}

#if 0
/* process_almond_api moved to config.c */
void process_almond_api(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		g_bools.local_api = true;
	}
}
#endif

/* process_almond_certificate moved to config.c */
#if 0
void process_almond_certificate(ConfVal value) {
	g_strings.almondCertificate = malloc((size_t)strlen(value.strval)+1);
	if (g_strings.almondCertificate == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.almondCertificate].\n");
		writeLog("Failed to allocate memory [g_strings.almondCertificate]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	strncpy(g_strings.almondCertificate, value.strval, strlen(value.strval));
	g_strings.almondCertificate[strlen(value.strval)] = '\0';
	writeLog("Almond certificate provided if TLS for API is enabled.", 0, 1);
}
#endif

/* process_almond_key moved to config.c */
#if 0
void process_almond_key(ConfVal value) {
	g_strings.almondKey = malloc((size_t)strlen(value.strval)+1);
	if (g_strings.almondKey == NULL) {
		fprintf(stderr, "Failed to allocate memory [almondSSLKey].\n");
		writeLog("Failed to allocate memory [almondSSLKey]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	strncpy(g_strings.almondKey, value.strval, strlen(value.strval));
	g_strings.almondKey[strlen(value.strval)] = '\0';
	writeLog("Almond certificate key provided to be used by API to run with  SSL encryption.", 0, 1);
}
#endif

/* process_almond_port moved to config.c */
#if 0
void process_almond_port(ConfVal value) {
	if (value.intval >= 1) {
        	g_ints.local_port = value.intval;
	}
	else g_ints.local_port = ALMOND_API_PORT;
	if (g_bools.local_api) {
        	writeLog("Almond will enable local api.", 0, 1);
        }
}
#endif

/* process_almond_standalone moved to config.c */
#if 0
void process_almond_standalone(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		writeLog("Almond will run g_bools.standalone. No monitor data will be sent to HowRU.", 0, 1);
		g_bools.standalone = true;
	}
}
#endif

/* process_almond_api_tls moved to config.c */
#if 0
#if 0
void process_almond_api_tls(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		writeLog("Almond g_pointers.scheduler use TLS encryption.", 0, 1);
		g_bools.use_ssl = true;
	}
}
#endif
#endif

void process_almond_format(ConfVal value) {
	if (strcmp(value.strval, "json") == 0){
		printf ("Export to json\n");
		writeLog("Export to format 'json'.", 0, 1);
		g_sizes.output_type= JSON_OUTPUT;
	}
	else if (strcmp(value.strval, "metrics") == 0) {
		printf ("Export to metrics file\n");
		writeLog("Export to standard metrics.", 0, 1);
		g_sizes.output_type = METRICS_OUTPUT;
	}
	else if (strcmp(value.strval, "jsonmetrics") == 0) {
		printf ("Export both to json and metrics file.\n");
		writeLog("Exporting both to json and to metrics file.", 0, 1);
		g_sizes.output_type = JSON_AND_METRICS_OUTPUT;
	}
	else if (strcmp(value.strval, "prometheus") == 0) {
		printf("Export to prometheus.\n");
		writeLog("Export to prometheus style metrics.", 0, 1);
		g_sizes.output_type = PROMETHEUS_OUTPUT;
	}
	else if (strcmp(value.strval, "jsonprometheus") == 0) {
		printf("Export to both json and Prometheus style metrics.\n");
		writeLog("Exporting to both json and prometheus style metrics.", 0, 1);
		g_sizes.output_type = JSON_AND_PROMETHEUS_OUTPUT;
	}
	else {
		printf("%s is not a valid value.  supported at this moment.\n", value.strval);
		writeLog("Unsupported value in configuration g_pointers.scheduler.format.", 1, 1);
		writeLog("Using standard output (JSON_OUTPUT).", 0, 1);
		g_sizes.output_type = JSON_OUTPUT;
	}
}

void process_conf_dir(ConfVal value) {
	if (g_dirs.confDir == NULL) {
		g_dirs.confDir = malloc((size_t)50 * sizeof(char));
		if (!g_dirs.confDir) {
			writeLog("Failed to allocate memory.", 1, 1);
			return;
		}
	}
	if (g_dirs.confDir != NULL)
        	memset(g_dirs.confDir, '\0', 50 * sizeof(char));
	if (directoryExists(value.strval, 255) == 0) {
        	//strncpy(g_dirs.confDir, value.strval, strlen(value.strval));
		//g_dirs.confDir[strlen(value.strval)] = '\0';
		snprintf(g_dirs.confDir, 50, "%s", value.strval);
        	g_bools.confDirSet = true;
	}
        else {
        	int status = mkdir(value.strval, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        	if(status != 0 && errno != EEXIST){
        		printf("Failed to create directory. Errno: %d\n", errno);
        		writeLog("Error creating configuration directory.", 2, 1);
        	}
        	else {
        		//strncpy(g_dirs.confDir, value.strval, strlen(value.strval));
			//g_dirs.confDir[strlen(value.strval)] = '\0';
			snprintf(g_dirs.confDir, 50, "%s", value.strval);
        		g_bools.confDirSet = true;
        	}
        }
        writeLog("Configuration directory is set.", 0, 1);
}

void process_almond_quickstart(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		writeLog("Almond g_pointers.scheduler have quick start activated.", 0, 1);
		g_bools.quick_start = true;
	}
}

void process_init_sleep(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i < 2000)
		i = 6000;
	g_ints.initSleep = i;
	writeLog("Init sleep for g_pointers.scheduler read.", 0, 1);
}

void process_almond_scheduler_type(ConfVal value) {
	if (strcmp(value.strval, "time") == 0){
		g_bools.timeScheduler = true;
		writeLog("Almond will use a time g_pointers.scheduler.", 0, 1);
	}
	else {
		writeLog("Almond will use classic g_pointers.scheduler.", 0, 1);
	}
}

void process_almond_sleep(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i < 1000)
		i = 1000;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Scheduler sleep time is %d ms.", i);
	writeLog(trim(g_strings.infostr), 0, 1);
	g_ints.schedulerSleep = i;
}

void process_data_dir(ConfVal value) {
	if (directoryExists(value.strval, 255) == 0) {
		//strncpy(g_dirs.dataDir, value.strval, strlen(value.strval));
		//g_dirs.dataDir[strlen(value.strval)] = '\0';
		snprintf(g_dirs.dataDir, g_sizes.datadir_size, "%s", value.strval);
		g_bools.dataDirSet = true;
	}
	else {
		int status = mkdir(value.strval, 0755);
		if (status != 0 && errno != EEXIST) {
			printf("Failed to create directory. Errno: %d\n", errno);
			writeLog("Error creating Almond data directory.", 2, 1);
			return;
		}
		else {
			//strncpy(g_dirs.dataDir, value.strval, strlen(value.strval));
			//g_dirs.dataDir[strlen(value.strval)] = '\0';
			snprintf(g_dirs.dataDir, g_sizes.datadir_size, "%s", value.strval);
			g_bools.dataDirSet = true;
		}
	}
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Almond data dir is set to %s.", g_dirs.dataDir);
	writeLog(g_strings.infostr, 0, 1);
}

void process_store_dir(ConfVal value) {
	if (directoryExists(value.strval, 255) == 0) {
		strncpy(g_dirs.storeDir, value.strval, g_sizes.storedir_size);
                g_bools.storeDirSet = true;
        }
        else {
        	int status = mkdir(value.strval, 0755);
		if (status != 0 && errno != EEXIST) {
                	printf("Failed to create directory. Errno: %d\n", errno);
                        writeLog("Error creating Almond store directory.", 2, 1);
			return;
                }
                else {
                	//strncpy(g_dirs.storeDir, value.strval, strlen(value.strval));
			//g_dirs.storeDir[strlen(value.strval)] = '\0';
			snprintf(g_dirs.storeDir, g_sizes.storedir_size, "%s", value.strval);
                        g_bools.storeDirSet = true;
                }
	}
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Almond store dir is set to %s.", g_dirs.storeDir);
        writeLog(g_strings.infostr, 0, 1);
}

void process_truncate_log(ConfVal value) {
        if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
                writeLog("Almond will truncate it logs..", 0, 1);
                g_bools.truncateLog = true;
        }
}

void process_external_scheduler(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		writeLog("Almond is set to use external g_pointers.scheduler.", 0, 1);
		writeLog("Almond will after initialization only respond to api calls to execute commands.", 1, 1);
		g_bools.external_scheduler = true;
		writeLog("Almond g_pointers.scheduler is inactivated for running command checks.", 0, 1);
	}
}

void process_use_kafka_config(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval >= 1)) {
		writeLog("Almond will use '/etc/almond/kafka.conf' for Kafka configurations.", 0, 1);
		g_bools.useKafkaConfigFile = true;
	}
}

void process_truncate_log_interval(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i < 3600) {
		writeLog("Truncate log interval configuration value too low. Minumum value is 3600.", 1, 1);
		writeLog("Truncate log interval value will not be changed.", 0, 1);
	}
	else if (i > 2147483647) {
		writeLog("Truncate log interval configuration value too high. Maximum value is 2147483647.", 1, 1);
		writeLog("Truncate log interval value will not be changed.", 0, 1);
	}
	else {
		g_sizes.truncateLogInterval = i;
		writeLog("Truncate log interval value updated from configuration.", 0, 1);
	}
}

void process_log_to_stdout(ConfVal val) {
	if ((strcmp(val.strval, "true") == 0) || (val.intval > 0)) {
		g_bools.dockerLog = true;
		writeLog("Log to stdout is set. Mostly useful for containers this option.", 0, 1);
		writeLog("DEBUG: docker log should be enabled, writing to stdout. TODO: enabled in code.", 1, 1);
	}
}

void process_log_dir(ConfVal val) {
	if (directoryExists(val.strval, 255) == 0) {
		//strncpy(g_dirs.logDir, val.strval, strlen(val.strval));
		//g_dirs.logDir[strlen(val.strval)] = '\0';
		snprintf(g_dirs.logDir, g_sizes.logdir_size, "%s", val.strval);
                g_bools.logDirSet = true;
        }
        else {
        	int status = mkdir(val.strval, 0755);
		if (status != 0 && errno != EEXIST) {
                	printf("Failed to create directory. Errno: %d\n", errno);
                        writeLog("Error creating log directory.", 2, 1);
                }
                else {
                	//strncpy(g_dirs.logDir, val.strval, strlen(val.strval));
			//g_dirs.logDir[strlen(val.strval)] = '\0';
			snprintf(g_dirs.logDir, g_sizes.logdir_size, "%s", val.strval);
                        g_bools.logDirSet = true;
                }
	}
	if (strcmp(val.strval, "/var/log/almond") != 0) {
		char ch =  '/';
                FILE *logFile;
                /*strcpy(g_files.fileName, g_dirs.logDir);
                strncat(g_files.fileName, &ch, 1);
                strcat(g_files.fileName, "almond.log");*/
		snprintf(g_files.fileName, g_sizes.filename_size, "%s/%s", g_dirs.logDir, "almond.log");
                writeLog("Closing g_files.logfile...", 0, 1);
                fclose(g_pointers.fptr);
                g_pointers.fptr = NULL;
                sleep(0.2);
                logFile = fopen("/var/log/almond/almond.log", "r");
                g_pointers.fptr = fopen(g_files.fileName, "a");
                if (g_pointers.fptr == NULL) {
                	fclose(logFile);
                        logFile = NULL;
                        g_pointers.fptr = fopen("/var/log/almond/almond.log", "a");
                        writeLog("Could not create new g_files.logfile.", 1, 1);
                        writeLog("Reopened g_files.logfile '/var/log/almond/almond.log'.", 0, 1);
                        strcpy(g_files.logfile, "/var/log/almond/almond.log");
                }
                else {
			while ( (ch = fgetc(logFile)) != EOF)
                        	fputc(ch, g_pointers.fptr);
                        fclose(logFile);
                        logFile = NULL;
                        writeLog("Created new g_files.logfile.", 0, 1);
                        strcpy(g_files.logfile, g_files.fileName);
		
		}
	}
       	else {
       		strcpy(g_files.logfile, "/var/log/almond/almond.log");
       }
}

void process_log_plugin_output(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Plugin outputs will be written to the log file", 0, 1);
		g_bools.logPluginOutput = true;
	}
        else {
        	writeLog("Plugin outputs will not be written to the log file", 0, 1);
        }
}

void process_store_results(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
	        writeLog("Plugin results will be stored in csv file.", 0, 1);
                g_bools.pluginResultToFile = true;
        }
        else {
                writeLog("Plugin results is not stored in specific csv file.", 0, 1);
        }
}

void process_host_name(ConfVal value) {
	/*strncpy(g_strings.hostName, value.strval, strlen(value.strval));
	g_strings.hostName[strlen(value.strval)] = '\0';*/
	snprintf(g_strings.hostName, g_sizes.hostname_size, "%s", value.strval);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Scheduler will give this host the virtual name: %s", g_strings.hostName);
	writeLog(trim(g_strings.infostr), 0, 1);
}

void process_plugin_directory(ConfVal value) {
	if (directoryExists(value.strval, 255) == 0) {
       		//strncpy(g_dirs.pluginDir, value.strval, strlen(value.strval));
		//g_dirs.pluginDir[strlen(value.strval)-1] = '\0';
		snprintf(g_dirs.pluginDir, g_sizes.plugindir_size, "%s", value.strval);
                g_bools.pluginDirSet = true;
        }
        else {
        	int status = mkdir(value.strval, 0755);
                if (status != 0 && errno != EEXIST) {
                	printf("Failed to create directory. Errno: %d\n", errno);
                        writeLog("Error creating plugins directory.", 2, 1);
                }
                else {
			//strncpy(g_dirs.pluginDir, value.strval, strlen(value.strval));
			//g_dirs.pluginDir[strlen(value.strval)-1] = '\0';
			snprintf(g_dirs.pluginDir, g_sizes.plugindir_size, "%s", value.strval);
			g_bools.pluginDirSet = true;
			writeLog("Created new plugin directory. It most likely is empty!", 1, 1);
                }
        }
}

void process_plugin_declaration(ConfVal v) {
	if (access(v.strval, F_OK) == 0){
		/*strncpy(g_files.pluginDeclarationFile, v.strval, strlen(v.strval));
		g_files.pluginDeclarationFile[strlen(v.strval)] = '\0';*/
		//strlcpy(g_files.pluginDeclarationFile, v.strval, sizeof(g_files.pluginDeclarationFile);
		snprintf(g_files.pluginDeclarationFile, g_sizes.plugindeclarationfile_size, "%s", v.strval);
        }
        else {
        	printf("ERROR: Plugin declaration file does not exist.");
        	writeLog("Plugin declaration file does not exist.", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Plugin g_pointers.g_plugins file is set to '%s'.", g_files.pluginDeclarationFile);
	writeLog(trim(g_strings.infostr), 0, 1);
}

void process_enable_gardener(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Gardener script is enabled.", 0, 1);
                g_bools.enableGardener = true;
	}
	else {
		writeLog("Gardener script is not enabled.", 0, 1);
	}
}

void process_enable_kafka_export(ConfVal v) {
	if ((strcmp(v.strval, "true") == 0) || (v.intval > 0)) {
		writeLog("Exporting results to Kafka is enabled.", 0, 1);
                g_bools.enableKafkaExport = true;
	}
	else {
                writeLog("Export to Kafka is not enabled.", 0, 1);
	}
}

void process_enable_kafka_tags(ConfVal v){
	if ((strcmp(v.strval, "true") == 0) || (v.intval > 0)) {
		writeLog("Use of tag to Kafka message is enabled.", 0, 1);
                g_bools.enableKafkaTag = true;
	}
	else {
		writeLog("Use of tag to Kafka message is not enabled.", 0, 1);
	}
}

void process_enable_kafka_id(ConfVal v) {
	if ((strcmp(v.strval, "true") == 0) || (v.intval > 0)) {
		writeLog("Use of Kafka id is enabled.", 0, 1);
                g_bools.enableKafkaId = true;
	}
	else {
		writeLog("Use of Kafka id is not enabled.", 0, 1);
       }
}

void process_kafka_start_id(ConfVal val) {
	int i = strtol(val.strval, NULL, 0);
        if (i > 0) {
        	g_sizes.kafka_start_id = i;
        	writeLog("Kafka start id check ok", 0, 1);
        }
        else {
        	writeLog("Could not read g_sizes.kafka_start_id.", 1, 1);
        	g_sizes.kafka_start_id = 0;
        }
}

void process_kafka_brokers(ConfVal value) {
	g_ints.kafkaexportreqs++;
	size_t kf_len = strlen(value.strval) + 1;
	g_strings.kafka_brokers = malloc(kf_len);
	if (g_strings.kafka_brokers == NULL) {
		fprintf(stderr, "Failed to allocate memory for kafka brokers.\n");
		writeLog("Failed to allocate memory [g_strings.kafka_brokers]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	else
		memset(g_strings.kafka_brokers, '\0', (size_t)(strlen(value.strval)+1) * sizeof(char));
	//strncpy(g_strings.kafka_brokers, value.strval, strlen(value.strval));
	snprintf(g_strings.kafka_brokers, kf_len, "%s", value.strval);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka export brokers is set to '%s'", g_strings.kafka_brokers);
	writeLog(trim(g_strings.infostr), 0, 1);
}

void process_kafka_config_file(ConfVal value) {
	size_t cf_len = strlen(value.strval) + 1;
	g_strings.kafkaConfigFile = malloc(cf_len);
	if (g_strings.kafkaConfigFile == NULL) {
		fprintf(stderr, "Failed to allocate memory for kafka config file.\n");
                writeLog("Failed to allocate memory [kafka_config_file]", 2, 1);
                g_ints.config_memalloc_fails++;
                return;
	}
	snprintf(g_strings.kafkaConfigFile, cf_len, "%s", value.strval);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka config file is set to '%s'", g_strings.kafkaConfigFile);
	writeLog(trim(g_strings.infostr), 0, 1);
}

#if 0
void process_kafka_topic(ConfVal val) {
	g_ints.kafkaexportreqs++;
	size_t l = strlen(val.strval) + 1;
	/* allocate once (avoid double allocation/leak) */
	char *tmp = malloc(l);
	if (tmp == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.kafka_topic].\n");
		writeLog("Failed to allocate memory [g_strings.kafka_topic]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	memcpy(tmp, val.strval, l);
	free(g_strings.kafka_topic);
	g_strings.kafka_topic = tmp;
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka export topic is set to '%s'", g_strings.kafka_topic);
        writeLog(trim(g_strings.infostr), 0, 1);
}
#endif

void process_kafka_tag(ConfVal value) {
	/* allocate via strdup and free previous value to avoid leaks */
	char *tmp = strdup(value.strval);
	if (tmp == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.kafka_tag].\n");
		writeLog("Failed to allocate memory [g_strings.kafka_tag]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	free(g_strings.kafka_tag);
	g_strings.kafka_tag = tmp;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka tag is set to '%s'", g_strings.kafka_tag);
	writeLog(trim(g_strings.infostr), 0, 1);
}

void process_enable_kafka_ssl(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Kafka producer will connect to cluster with SSL.", 0, 1);
                writeLog("Make sure you use a certificate with accordance to Kafka ACL list.", 0, 1);
                g_bools.enableKafkaSSL = true;
	}
	else {
		writeLog("Kafka producer will connect with plain text", 0, 1);
	}
}

void process_kafka_ca_certificate(ConfVal val) {
	g_strings.kafkaCACertificate = malloc((size_t)strlen(val.strval)+1);
	if (g_strings.kafkaCACertificate == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.kafkaCACertificate].\n");
		writeLog("Failed to allocate memory [g_strings.kafkaCACertificate]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	strncpy(g_strings.kafkaCACertificate, val.strval, strlen(val.strval));
	g_strings.kafkaCACertificate[strlen(val.strval)] = '\0';
	writeLog("Kafka CA certificate location stored from configuration file.", 0, 1);
}

void process_kafka_producer_certificate(ConfVal value) {
	g_strings.kafkaProducerCertificate = malloc((size_t)strlen(value.strval)+1);
	if (g_strings.kafkaProducerCertificate == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.kafkaProducerCertificate].\n");
		writeLog("Failed to allocate memory [kafkaProducerPertificate", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	strncpy(g_strings.kafkaProducerCertificate, value.strval, strlen(value.strval));
	g_strings.kafkaProducerCertificate[strlen(value.strval)] = '\0';
	writeLog("Kafka Producer certificate location stored from configuration file.", 0, 1);
}

void process_kafka_ssl_key(ConfVal val) {
	g_strings.kafkaSSLKey = malloc((size_t)strlen(val.strval)+1);
	if (g_strings.kafkaSSLKey == NULL) {
		fprintf(stderr, "Failed to allocate memory [g_strings.kafkaSSLKey].\n");
		writeLog("Failed to allocate memory [g_strings.kafkaSSLKey]", 2, 1);
		g_ints.config_memalloc_fails++;
		return;
	}
	strncpy(g_strings.kafkaSSLKey, val.strval, strlen(val.strval));
	g_strings.kafkaSSLKey[strlen(val.strval)] = '\0';
	writeLog("Kafka SSL Key provided from configuration file.", 0, 1);
}

void process_schema_name(ConfVal val) {
	if (val.strval == NULL) {
		fprintf(stderr, "Schema registry name is NULL in config.\n");
                writeLog("Schema registry name is NULL in configuration file.", 1, 1);
		return;
	}
	if (strlen(val.strval) > 100) {
		writeLog("Schema registry name is too long. Should be maximum 100 characters.", 1, 1);
		return;
	}
        strncpy(schemaName, val.strval, sizeof(schemaName)-1);
	schemaName[sizeof(schemaName)-1] = '\0';
    	snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka schema name is set to '%s'", schemaName);
        writeLog(trim(g_strings.infostr), 0, 1);
}

void process_schema_registry_url(ConfVal val) {
        if (val.strval == NULL) {
        	fprintf(stderr, "Schema registry URL is NULL\n");
    		writeLog("Schema registry URL is NULL", 2, 1);
    		g_ints.config_memalloc_fails++;
    		return;
  	}
	size_t len = strlen(val.strval);
        g_strings.schemaRegistryUrl = malloc(len+1);
        if (g_strings.schemaRegistryUrl == NULL) {
                fprintf(stderr, "Failed to allocate memory [kafka_schemaRegistryUrl].\n");
                writeLog("Failed to allocate memory [kafka_schemaRegistryUrl]", 2, 1);
                g_ints.config_memalloc_fails++;
                return;
        }
        strncpy(g_strings.schemaRegistryUrl, val.strval, len);
	g_strings.schemaRegistryUrl[len] = '\0';
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Kafka schema registry url is set to '%s'", g_strings.schemaRegistryUrl);
        writeLog(trim(g_strings.infostr), 0, 1);
}

void process_gardener_run_interval(ConfVal value) {
	int i = strtol(value.strval, NULL, 0);
	if (i < 60)
		i = 43200;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Gardener run interval is %d seconds.", i);
        writeLog(trim(g_strings.infostr), 0, 1);
        g_sizes.gardenerInterval = i;
}

void process_clear_data_cache_interval(ConfVal v) {
	int i = strtol(v.strval, NULL, 0);
	if (i < 60)
		i = 300;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Clear data cache is %d seconds.", i);
	writeLog(trim(g_strings.infostr), 0, 1);
	g_sizes.clearDataCacheInterval = i;
}

void process_data_cache_time_frame(ConfVal val) {
	int i = strtol(val.strval, NULL, 0);
	if (i < 180)
		i = 330;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Data cache time frame is set to %d seconds.", i);
	writeLog(trim(g_strings.infostr), 0, 1);
	g_sizes.dataCacheTimeFrame = i;
}

void process_tune_timer(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Timer tuner is enabled.", 0, 1);
                g_bools.enableTimeTuner = true;
	}
	else {
		writeLog("Timer tuner is not enabled.", 0, 1);
	}
}

void process_tune_cycle(ConfVal val) {
	int i = strtol(val.strval, NULL, 15);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Time tuner cycle is set to %d.", i);
	writeLog(trim(g_strings.infostr), 0, 1);
	g_ints.timeTunerCycle = i;
}

void process_tune_master(ConfVal value) {
	int i = strtol(value.strval, NULL, 1);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Time tuner cycle is set to %d.", i);
	writeLog(trim(g_strings.infostr), 0, 1);
	g_ints.timeTunerMaster = i;
}

void process_run_gardener_at_start(ConfVal v) {
	if ((strcmp(v.strval, "true") == 0) || (v.intval > 0)) {
		writeLog("Gardener will run during startup.", 0, 1);
                g_bools.runGardenerAtStart = true;
        }
}

void process_gardener_script(ConfVal value) {
	if (access(value.strval, F_OK) == 0){
		strncpy(g_files.gardenerScript, value.strval, g_sizes.gardenerscript_size);
		//g_files.gardenerScript[strlen(value.strval)] = '\0';
		g_files.gardenerScript[g_sizes.gardenerscript_size] = '\0';
	}
	else {
		g_bools.enableGardener = false;
		writeLog("Gardener script file could not be found", 1, 1);
		writeLog("Gardener is disabled.", 2, 1);
	}
}

void process_enable_clear_data_cache(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Clear data cache is enabled.", 0, 1);
                g_bools.enableClearDataCache = true;
        }
        else {
                writeLog("Clear data cache is not enabled.", 0, 1);
        }
}

/* process_json_file moved to config.c */
#if 0
void process_json_file(ConfVal value) {
	//strncpy(g_files.jsonFileName, value.strval, strlen(value.strval));
	//g_files.jsonFileName[strlen(value.strval)] = '\0';
	snprintf(g_files.jsonFileName, g_sizes.jsonfilename_size, "%s", value.strval);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Json data will be collected in file: %s.", g_files.jsonFileName);
	writeLog(trim(g_strings.infostr), 0, 1);
}
#endif

/* process_metrics_file moved to config.c */
#if 0
void process_metrics_file(ConfVal val) {
	/*strncpy(g_files.metricsFileName, val.strval, strlen(val.strval));
        g_files.metricsFileName[strlen(val.strval)] = '\0';*/
	snprintf(g_files.metricsFileName, g_sizes.metricsfilename_size, "%s", val.strval);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics will be collected in file: %s", g_files.metricsFileName);
	writeLog(trim(g_strings.infostr), 0, 1);
}
#endif

void process_metrics_output_prefix(ConfVal value) {
	if ((int)strlen(value.strval) <= 30) {
		//size_t len = strlen(value.strval);
		//strncpy(g_strings.metricsOutputPrefix, value.strval, len);
		//g_strings.metricsOutputPrefix[len] = '\0';
		snprintf(g_strings.metricsOutputPrefix, 31, "%s", value.strval);
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics output prefix is set to '%s'", g_strings.metricsOutputPrefix);
		writeLog(trim(g_strings.infostr), 0, 1);
	}
	else {
		writeLog("Could not change g_strings.metricsOutputPrefix. Prefix too long.", 1, 1);
	}
}

void process_save_on_exit(ConfVal value) {
	if ((strcmp(value.strval, "true") == 0) || (value.intval > 0)) {
		writeLog("Data file will be saved in data directory after shutdown.", 0, 1);
		g_bools.saveOnExit = true;
	}
	else {
		writeLog("Json data will be deleted on shutdown.", 0, 1);
	}
}

int getConfigurationValues() {
	char* file_name = NULL;
        char* line = NULL;
        size_t len = 0;
        ssize_t read;
        FILE *fp = NULL;
        int index = 0;
        file_name = "/etc/almond/almond.conf";
        fp = fopen(file_name, "r");
        char confName[MAX_STRING_SIZE] = "";
        char confValue[MAX_STRING_SIZE] = "";
	
	if (fp == NULL)
        {
                perror("Error while opening the configuration file.\n");
                writeLog("Error opening configuration file", 2, 1);
                exit(EXIT_FAILURE);
        }

	while ((read = getline(&line, &len, fp)) != -1) {
		char *trimmed = trim(line);
		if (trimmed[0] == '#' || trimmed[0] == '\0') {
			continue;
		}
		char * token = strtok(trimmed, "=");
		while (token != NULL) {
			if (index == 0) {
				//strncpy(confName, token, sizeof(confName));
				snprintf(confName, sizeof(confName), "%s", token);
                   	}
                   	else {
				//strncpy(confValue, token, sizeof(confValue));
				snprintf(confValue, sizeof(confValue), "%s", token);
                   	}
                   	token = strtok(NULL, "=");
                   	index++;
                   	if (index == 2) index = 0;
           	}
		ConfVal cvu;
		cvu.intval = strtol(trim(confValue), NULL, 0);
		cvu.strval = trim(confValue);
		for (int i = 0; i < sizeof(config_entries)/sizeof(ConfigEntry);i++) {
			if (strcmp(confName, config_entries[i].name) == 0) {
				config_entries[i].process(cvu);
				break;
			}
		}
	}
	g_ints.updateInterval = 60;
	if (g_bools.enableKafkaExport) {
       		if (g_ints.kafkaexportreqs < 2 && !g_bools.useKafkaConfigFile) {
                	writeLog("Not sufficient configuration to export to Kafka. Brokers and or topic is unknown.", 1, 1);
                	writeLog("Kafka export is not enabled.", 0, 1);
                	g_bools.enableKafkaExport = false;
		}
        }
	// Also check Almond SSL like Kafka
        fclose(fp);
        fp = NULL;
        if (line){
                free(line);
                line = NULL;
        }
	if (g_ints.config_memalloc_fails > 0) {
		g_ints.config_memalloc_fails = 0;
		return 2;
	}
        return 0;
}

int truncateLogs() {
	size_t compressed_name_size = g_sizes.logfile_size + 28;
	char* compressed_name = malloc(compressed_name_size * sizeof(char));
	strcpy(compressed_name, g_files.logfile);
	strncat(compressed_name, getCurrentTimestamp(), 20);
	strncat(compressed_name, ".tar.gz", 8);
	if (compress_log(g_files.logfile, compressed_name) == -1) {
		return -1;
	}
	if (truncate(g_files.logfile, 0) == -1) {
		fprintf(stderr, "Failed to truncate log: %s\n", strerror(errno));
		writeLog("Truncation of log file failed.", 1, 1); 
		unlink(compressed_name);
		return -1;
	}
	return 0;
}

int check_file_truncation() {
	struct stat filestat;
	time_t current_time, diff_seconds;

	if (stat(g_files.logfile, &filestat) == -1) {
		writeLog("Failed to get filestat from g_files.logfile. This will make truncation impossible", 1, 1);
		return 0;
	}
	current_time = time(NULL);
	#ifdef HAS_BIRTHTIME
		diff_seconds = current_time - filestat.st_birthtime;
	#else
		diff_seconds = current_time - filestat.st_mtime;
		writeLog("Could not get birthtime from file. Truncation will be omitted.", 1, 1);
	#endif
	if (diff_seconds > g_sizes.truncateLogInterval) {
		printf("Will start truncating the Almond log.");
		writeLog("It is time to truncate the Almond log.", 0, 1);
		sleep(1);
		truncateLogs();
	}
	return diff_seconds;
}

/* apiDryRun moved to api.c */

#if 0
/* apiRunPlugin moved to api.c */
void apiRunPlugin(int plugin_id, int flags) {
	char* pluginName = NULL;
	char* message = NULL;
	int waitCount = 0;

	message = (char *) malloc(sizeof(char) * (g_sizes.apimessage_size+1));
	if (message == NULL) {
		writeLog("Failed to allocate memory for api message", 1, 0);
		return;
	}
	else
		memset(message, '\0', (size_t)(g_sizes.apimessage_size+1) * sizeof(char));
	pluginName = malloc((size_t)(g_sizes.pluginitemname_size + 1) * sizeof(char));
	if (pluginName == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiRunPlugin.\n");
                writeLog("Failed to allocate memory [apiRunPlugin: pluginName]", 2, 0);
                return;
        }
	else
		memset(pluginName, '\0', (size_t)(g_sizes.pluginitemname_size+1) * sizeof(char));
	// In new structure increase id with one
	//plugin_id++;
	pluginName = strdup(g_pointers.g_plugins[plugin_id]->name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
	// Check if same plugin is running in thread, in which case wait...
	while (g_arrays.threadIds[(short)plugin_id] > 0) {
		writeLog("Waiting for thread to finish...", 0, 0);
		sleep(1);
		waitCount++;
		if (waitCount > 10) {
			writeLog("Reached waitCount threshold. Continue.", 1, 0);
			break;
		}
	}
	char p_id[12];
	snprintf(p_id, sizeof(p_id), "%i",plugin_id);
	setApiCmdFile("execute", p_id);
	strcpy(message, "{\n     \"executePlugin\":\"");
	strcat(message, pluginName);
	strcat(message, "\"");
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message, ",\n");
		sleep(10);
		strcat(message, "     \"pluginOutput:\":\"");
		strcat(message, trim(g_pointers.g_plugins[plugin_id]->output.retString));
		strcat(message, "\"");
        }
	strcat(message, "\n}\n");
	g_strings.socket_message = malloc((size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
	if (g_strings.socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiRunPlugin.\n");
                writeLog("Failed to allocate memory [apiRunPlugin: g_strings.socket_message]", 2, 0);
                return;
        }
	else
		memset(g_strings.socket_message, '\0', (size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
	if (strlen(message) > g_sizes.apimessage_size) {
		printf("DEBUG: [apiRunPlugin] Message is larger than size.\n");
		message[g_sizes.apimessage_size-1] = '\0';
	}
	strncpy(g_strings.socket_message, message, (size_t)g_sizes.apimessage_size);
	free(pluginName);
	pluginName = NULL;
	if (message != NULL) {
		free(message);
		message = NULL;
	}	
}
#endif

#if 0
void apiReadData(int plugin_id, int flags) {
	char* pluginName = NULL;
	char rCode[12];
	char* message = NULL;
	unsigned short is_error = 0;

	if (plugin_id < 0) {
		printf("Strange things happen...\n");
		return;
	}

	message = malloc((size_t)g_sizes.apimessage_size * sizeof(char)+1);
	if (message == NULL) {
		writeLog("Failed to allocate memory for api message.", 1, 0);
	}
	else
       		message[0] = '\0';
	pluginName = malloc((size_t)g_sizes.pluginitemname_size * sizeof(char)+1);
	if (pluginName == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiReadData.\n");
		writeLog("Failed to allocate memory [apiReadData:pluginName]", 2, 0);
		return;
	}
	if (plugin_id == 0 && flags == 0) {
		printf("This is an invalid check.\n");
		is_error++;
	}
	if (plugin_id > g_ints.decCount || flags > 100) {
		printf("This is an invalid check.\n");
		is_error++;
	}	
	if (is_error > 0) {
		strcat(message, "{\n     \"almond\":\"Invalid check - no such plugin or flag\"\n}\n");
		g_strings.socket_message = malloc((size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
                if (g_strings.socket_message == NULL) {
                        fprintf(stderr, "Failed to allocate memory.\n");
                        writeLog("Failed to allocate memory in [apiReadData:g_strings.socket_message]", 2, 0);
                        return;
                }
		else
			memset(g_strings.socket_message, '\0', (size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
                strcpy(g_strings.socket_message, message);
                free(message);
                free(pluginName);
                message = pluginName = NULL;
		return;
	}
	// In new structure I need to increase id with 1
	//plugin_id += 1;
	pluginName = strdup(g_pointers.g_plugins[plugin_id]->name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message,"{\n     \"name\":\"");
        	strcat(message, pluginName);
        	strcat(message, "\",\n");
		strcat(message, "     \"description\":\"");
	        strcat(message, g_pointers.g_plugins[plugin_id]->description);
		strcat(message, "\",\n");
		switch (g_pointers.g_plugins[plugin_id]->output.retCode) {
			case 0:
				strcat(message, "     \"pluginStatus\":\"OK\",\n");
				break;
			case 1:
				strcat(message, "     \"pluginStatus\":\"WARNING\",\n");	
				break;
			case 2: 
				strcat(message, "     \"pluginStatus\":\"CRITICAL\",\n");
				break;
			default:
				strcat(message, "     \"pluginStatus\":\"UNKNOWN\",\n");
				break;
		}
		strcat(message, "     \"pluginStatusCode\":\"");
		sprintf(rCode, "%d", g_pointers.g_plugins[plugin_id]->output.retCode); 
	   	strcat(message, trim(rCode));
		strcat(message,  "\",\n");
		strcat(message, "     \"pluginOutput\":\"");
		strcat(message, trim(g_pointers.g_plugins[plugin_id]->output.retString));
		strcat(message, "\",\n");
		strcat(message, "     \"pluginStatusChanged\":\"");
		strcat(message, g_pointers.g_plugins[plugin_id]->statusChanged);
		strcat(message, "\",\n");
		strcat(message, "     \"lastChange\":\"");
		strcat(message, g_pointers.g_plugins[plugin_id]->lastChangeTimestamp);
		strcat(message, "\",\n");
		strcat(message, "     \"lastRun\":\"");
		strcat(message, g_pointers.g_plugins[plugin_id]->lastRunTimestamp);
		strcat(message, "\",\n");
                strcat(message, "     \"nextScheduledRun\":\"");
		strcat(message, g_pointers.g_plugins[plugin_id]->nextRunTimestamp);
		strcat(message, "\"\n");
	}
        else {
		strcat(message,"{\n     \"");
                strcat(message, pluginName);
                strcat(message, "\":\"");
                strcat(message, trim(g_pointers.g_plugins[plugin_id]->output.retString));
                strcat(message, "\"\n");
	}
	strcat(message, "}\n");
	free(pluginName);
	pluginName = NULL;
	g_strings.socket_message = malloc((size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
	if (g_strings.socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		writeLog("Failed to allocate memory in [apiReadData:g_strings.socket_message]", 2, 0);
		return;
	}
	else
		memset(g_strings.socket_message, '\0', (size_t)(g_sizes.apimessage_size + 1) * sizeof(char));
	strncpy(g_strings.socket_message, message, (size_t)g_sizes.apimessage_size);
	free(message);
	message = NULL;
}
#endif

void __deprecated_createUpdateFile(struct PluginItem *item, struct PluginOutput *output, char name[3]) {
	FILE *fp = NULL;
	char filename[30];
       	
	strcpy(filename, "/opt/almond/api_cmd/");
	strncat(filename, name, 3);
	strncat(filename, ".udf", 5);
	filename[strlen(filename)] = '\0';
	fp = fopen(filename, "w");
	fprintf(fp, "item_id\t%s\n", name);
	fprintf(fp, "item_lastruntimestamp\t%s\n", item->lastRunTimestamp);
	fprintf(fp, "item_nextruntimestamp\t%s\n", item->nextRunTimestamp);
	fprintf(fp, "item_lastchangetimestamp\t%s\n", item->lastChangeTimestamp);
	fprintf(fp, "item_statuschanged\t%s\n", item->statusChanged);
	fprintf(fp, "item_nextrun\t");
	//fwrite(&item->nextRun, sizeof(time_t), 1, fp);
	fprintf(fp, "\noutput_retcode\t%i\n", output->retCode);
	fprintf(fp, "output_retstring\t%s\n", output->retString);
	fclose(fp);
	fp = NULL;
}

/* apiGetMetrics moved to api.c */
#if 0
	char* pluginName = NULL;
	char rCode[12];
	char strNum[12];
        char* message = NULL;
	unsigned short is_error = 0;
        
	message = malloc((size_t)g_sizes.apimessage_size+1 * sizeof(char));
	if (message == NULL) {
		writeLog("Could not allocate memory for apimessage", 2, 0);
		return;
	}
	else {
		memset(message, '\0', (size_t)g_sizes.apimessage_size+1 * sizeof(char));
	}
	if (plugin_id == 0 && flags == 0) {
                printf("This is an invalid check.\n");
                is_error++;
        }
        if (plugin_id > g_ints.decCount || flags > 100) {
                printf("This is an invalid check.\n");
                is_error++;
        }
        if (is_error > 0) {
                strcat(message, "{\n     \"almond\":\"Invalid check - no such plugin or flag\"\n}\n");
                g_strings.socket_message = malloc((size_t)strlen(message)+1);
                if (g_strings.socket_message == NULL) {
                        fprintf(stderr, "Failed to allocate memory.\n");
                        writeLog("Failed to allocate memory in [apiReadData:g_strings.socket_message]", 2, 0);
                        return;
                }
                strcpy(g_strings.socket_message, message);
                free(message);
                free(pluginName);
                message = pluginName = NULL;
                return;
        }
	// In new structure increase id with 1
	//plugin_id += 1;
        snprintf(strNum, sizeof(strNum), "%d", plugin_id);
        setApiCmdFile("update", strNum);
	pluginName = (char *)malloc((size_t)(g_sizes.pluginitemname_size+1) * sizeof(char));
	if (pluginName == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		writeLog("Failed to allocate memory [apiRunAndRead:pluginName]", 2, 0);
		return;
	}
	else
		memset(pluginName, '\0', (size_t)(g_sizes.pluginitemname_size+1) * sizeof(char));
        strncpy(pluginName, g_pointers.g_plugins[plugin_id]->name, (size_t)g_sizes.pluginitemname_size+1);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
        //runPlugin(plugin_id, 0);
        PluginItem *item = g_pointers.g_plugins[plugin_id];
        if (item) {
            run_plugin(item);
        }
	if (g_bools.timeScheduler)
		rescheduleChecks();
        createUpdateFile(g_pointers.g_plugins[plugin_id], strNum);
	strcpy(message, "{\n     \"executePlugin\":\"");
        strcat(message, pluginName);
        strcat(message, "\",\n");
        strcat(message, "      \"result\": {\n");
	sleep(10);
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message, "          \"name\":\"");
		strcat(message, pluginName);
		free(pluginName);
		pluginName = NULL;
		strcat(message, "\",\n");
		strcat(message, "          \"description\":\"");
                strcat(message, g_pointers.g_plugins[plugin_id]->description);
                strcat(message, "\",\n");
                switch (g_pointers.g_plugins[plugin_id]->output.retCode) {
                        case 0:
                                strcat(message, "          \"pluginStatus\":\"OK\",\n");
                                break;
                        case 1:
                                strcat(message, "          \"pluginStatus\":\"WARNING\",\n");
                                break;
                        case 2:
                                strcat(message, "          \"pluginStatus\":\"CRITICAL\",\n");
                                break;
                        default:
                                strcat(message, "          \"pluginStatus\":\"UNKNOWN\",\n");
                                break;
                }
                strcat(message, "          \"pluginStatusCode\":\"");
                sprintf(rCode, "%d", g_pointers.g_plugins[plugin_id]->output.retCode);
                strcat(message, trim(rCode));
                strcat(message,  "\",\n");
                strcat(message, "          \"pluginOutput\":\"");
                strcat(message, trim(g_pointers.g_plugins[plugin_id]->output.retString));
                strcat(message, "\",\n");
                strcat(message, "          \"pluginStatusChanged\":\"");
                strcat(message, g_pointers.g_plugins[plugin_id]->statusChanged);
                strcat(message, "\",\n");
                strcat(message, "          \"lastChange\":\"");
                strcat(message, g_pointers.g_plugins[plugin_id]->lastChangeTimestamp);
                strcat(message, "\",\n");
                strcat(message, "          \"lastRun\":\"");
                strcat(message, g_pointers.g_plugins[plugin_id]->lastRunTimestamp);
                strcat(message, "\",\n");
                strcat(message, "          \"nextScheduledRun\":\"");
                strcat(message, g_pointers.g_plugins[plugin_id]->nextRunTimestamp);
                strcat(message, "\"\n     }\n");
	}
	else {
		strcat(message, "          \"returnString\":\"");
		strcat(message, trim(g_pointers.g_plugins[plugin_id]->output.retString));
		strcat(message, "\"\n     }\n");
	}
	strcat(message, "}\n");
	if (g_strings.socket_message != NULL) {
		free(g_strings.socket_message);
		g_strings.socket_message = NULL;
	}
	g_strings.socket_message = malloc((size_t)(g_sizes.apimessage_size+1) * sizeof(char)); 
	if (g_strings.socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		writeLog("Failed to allocate memory [apiRunAndRead:g_strings.socket_message]", 2, 0);
		return;
	}
	if (strlen(message) > g_sizes.apimessage_size) {
		printf("Message is to big. Try increase g_sizes.apimessage_size.\n");
		message[g_sizes.apimessage_size-1] = '\0';
	}
	strncpy(g_strings.socket_message, message, (size_t)g_sizes.apimessage_size);
	if (pluginName != NULL) {
		free(pluginName);
		pluginName = NULL;
	}
	if (message != NULL) {
		free(message);
		message = NULL;
	}
}
#endif

/* apiGetMetrics moved to api.c */
#if 0
void apiGetMetrics() {
	char ch = '/';

	snprintf(g_files.storeName, g_sizes.storename_size, "%s%c%s", g_dirs.storeDir, ch, g_files.metricsFileName);
	apiReadFile(g_files.storeName, 2);
}

#if 0
void apiGetHostName() {
	char nm[9];
	strcpy(nm, "hostname");
	constructSocketMessage(nm, g_strings.hostName);
}
#endif

#if 0
void apiShowVersion() {
	char version[8];
	strcpy(version, "version");
	constructSocketMessage(version, VERSION);
}
#endif

#if 0
void apiShowStatus() {
	FILE *fp;
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

	double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
	double system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec /1e6; 
	pid_t pid = getppid();

        json_object *jobj = json_object_new_object();
	json_object_object_add(jobj, "hostname", json_object_new_string(g_strings.hostName));
        json_object_object_add(jobj, "almond_version", json_object_new_string(VERSION));
	json_object_object_add(jobj, "pid", json_object_new_int(pid));
	fp = fopen("/proc/uptime", "r");
    	if (fp) { 
		double uptime = 0.0;
    		if (fscanf(fp, "%lf", &uptime) == 1) {
        		json_object_object_add(jobj, "uptime_seconds", json_object_new_double(uptime));
    		}
    		fclose(fp);
	}
	json_object_object_add(jobj, "plugin_count", json_object_new_int(g_ints.decCount));
	json_object_object_add(jobj, "user_cpu_time", json_object_new_double(user_time));
	json_object_object_add(jobj, "system_cpu_tume", json_object_new_double(system_time));
	struct mallinfo2 mi = mallinfo2();  // glibc >= 2.33
        json_object_object_add(jobj, "heap_allocated_kb", json_object_new_int64(mi.uordblks / 1024));
        json_object_object_add(jobj, "heap_total_kb",     json_object_new_int64(mi.arena / 1024));
	json_object_object_add(jobj, "max_resident_set_size_kb", json_object_new_int64(usage.ru_maxrss));
	struct rlimit rl;
    	if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        	json_object_object_add(jobj, "stack_size_kb", json_object_new_int64(rl.rlim_cur / 1024));
    	}
	fp = fopen("/proc/self/statm", "r");
    	if (fp) {
		long rss_pages = 0;
    		if (fscanf(fp, "%*s %ld", &rss_pages) == 1) {
        		long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
        		json_object_object_add(jobj, "rss_kb", json_object_new_int64(rss_pages * page_size_kb));
    		}
    		fclose(fp);
	}
	json_object_object_add(jobj, "minor_page_faults", json_object_new_int64(usage.ru_minflt));
	json_object_object_add(jobj, "major_page_faults", json_object_new_int64(usage.ru_majflt));
	json_object_object_add(jobj, "swaps", json_object_new_int64(usage.ru_nswap));
	json_object_object_add(jobj, "block_input_ops", json_object_new_int64(usage.ru_inblock));
	json_object_object_add(jobj, "block_output_ops", json_object_new_int64(usage.ru_oublock));
	json_object_object_add(jobj, "ipc_msgs_sent", json_object_new_int64(usage.ru_msgsnd));
	json_object_object_add(jobj, "ipc_msgs_received", json_object_new_int64(usage.ru_msgrcv));
	json_object_object_add(jobj, "signals_received", json_object_new_int64(usage.ru_nsignals));
	json_object_object_add(jobj, "voluntary_context_switches", json_object_new_int64(usage.ru_nvcsw));
	json_object_object_add(jobj, "involuntary_context_switches", json_object_new_int64(usage.ru_nivcsw));
	json_object_object_add(jobj, "thread_count", json_object_new_int(get_thread_count()));
	json_object_object_add(jobj, "open_file_descriptors", json_object_new_int64(get_fd_count()));
	fp = fopen("/proc/self/io", "r");
	if (fp) {
		char line[256];
    		while (fgets(line, sizeof(line), fp)) {
        		char key[64];
        		unsigned long long value;
			if (sscanf(line, "%63[^:]: %llu", key, &value) == 2) {
            			json_object_object_add(jobj, key, json_object_new_int64(value));
        		}
    		}
		fclose(fp);
	}
        const char *json_str = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
        int size = strlen(json_str) + 2;
        g_strings.socket_message = malloc((size_t)size);
        if (g_strings.socket_message == NULL) {
                printf("Memory allocation failed.\n");
                writeLog("Memory allocation failed [constructSocketMessage:g_strings.socket_message]", 2, 0);
                return;
        }
        else
                memset(g_strings.socket_message, '\0', (size_t)size * sizeof(char));
        snprintf(g_strings.socket_message, (size_t)size, "%s\n", json_str);
        json_object_put(jobj);
}
#endif

#if 0
void apiShowPluginStatus() {
	int num_of_oks = 0, num_of_warnings = 0, num_of_criticals = 0, num_of_unknowns = 0;
	for (int i = 0; i < g_ints.decCount; i++) {
		switch(g_pointers.g_plugins[i]->output.retCode) {
			case 0:
				num_of_oks++;
				break;
			case 1:
				num_of_warnings++;
				break;
			case 2:
				num_of_criticals++;
				break;
			default:
				num_of_unknowns++;
				break;
		}
	}
	json_object *jobj = json_object_new_object();
	json_object_object_add(jobj, "number_of_checks", json_object_new_int(g_ints.decCount));
	json_object_object_add(jobj, "ok", json_object_new_int(num_of_oks));
	json_object_object_add(jobj, "warning", json_object_new_int(num_of_warnings));
	json_object_object_add(jobj, "critical", json_object_new_int(num_of_criticals));
	json_object_object_add(jobj, "unknown", json_object_new_int(num_of_unknowns));
	const char *json_str = json_object_to_json_string(jobj);
	int size = strlen(json_str) + 2;
        g_strings.socket_message = malloc((size_t)size);
        if (g_strings.socket_message == NULL) {
                printf("Memory allocation failed.\n");
                writeLog("Memory allocation failed [constructSocketMessage:g_strings.socket_message]", 2, 0);
                return;
        }
        else
                memset(g_strings.socket_message, '\0', (size_t)size * sizeof(char));
        snprintf(g_strings.socket_message, (size_t)size, "%s\n", json_str);
	json_object_put(jobj);
}
#endif

#if 0
void apiCheckPluginConf() {
	int res = check_plugin_conf_file(g_files.pluginDeclarationFile);
	if (res == 0) {
		constructSocketMessage("pluginconfiguration", "true");
	}
	else
		constructSocketMessage("pluginconfiguration", "false");
}
#endif

#if 0
void apiGetVars(int v) {
	switch (v) {
		case 1:
			if (g_strings.kafka_tag == NULL)
                        	constructSocketMessage("kafkatag", "NULL");
                	else
                        	constructSocketMessage("kafkatag", g_strings.kafka_tag);
			break;
		case 2:
			constructSocketMessage("metricsprefix", g_strings.metricsOutputPrefix);
			break;
		case 3:
			constructSocketMessage("jsonfilename", g_files.jsonFileName);
			break;
		case 4:
			constructSocketMessage("metricsfilename", g_files.metricsFileName);
			break;
		case 5:
			if (g_bools.useKafkaConfigFile) {
				char* currentTopic = getKafkaTopic();
				if (currentTopic != NULL) {
					constructSocketMessage("kafkatopic", currentTopic);
				}
				else {
					constructSocketMessage("kafkatopic", "NULL");
				}
			}
			else if (g_strings.kafka_topic == NULL)
                        	constructSocketMessage("kafkatopic", "NULL");
			else
                        	constructSocketMessage("kafkatopic", g_strings.kafka_topic);
			break;
		case 6:
			int length = snprintf(NULL, 0, "%d", g_ints.schedulerSleep);
			char* sleep_num = malloc(length + 1);
			snprintf(sleep_num, length + 1,  "%d", g_ints.schedulerSleep);
			constructSocketMessage("schedulersleep", sleep_num);
			free(sleep_num);
			break;
		case 7:
			char soe_val[6];
			sprintf(soe_val, "%s", g_bools.saveOnExit ? "true" : "false");
			constructSocketMessage("saveonexit", soe_val);
			break;
		case 8:
			char plo_val[6];
			sprintf(plo_val, "%s", g_bools.logPluginOutput ? "true" : "false");
			constructSocketMessage("pluginoutput", plo_val);
			break;
		case 9:
			char s_kStartId[2];
			sprintf(s_kStartId, "%d", g_sizes.kafka_start_id);
			constructSocketMessage("kafkastartid", s_kStartId);
			break;
		case 10:
			char plts[14];
			sprintf(plts, "%ld", g_time.tPluginFile);
			constructSocketMessage("pluginslastchangets", plts);
			break;
		case 11:
			if (!g_bools.external_scheduler) {
				constructSocketMessage("g_pointers.scheduler", "internal");
			}
			else {
				constructSocketMessage("g_pointers.scheduler", "external");
			}
			break;
		case 12:
			if (g_strings.push_url == NULL) 
				constructSocketMessage("pushurl", "NULL");
			else
				constructSocketMessage("pushurl", g_strings.push_url);
			break;
		case 13:
			char a_port[10];
			sprintf(a_port, "%d", g_ints.push_port);
			constructSocketMessage("pushport", a_port);
			break;
		case 14:
			int length = snprintf(NULL, 0, "%d", g_ints.push_interval);
                        char* p_interval = malloc(length + 1);
                        snprintf(p_interval, length + 1,  "%d", g_ints.push_interval);
                        constructSocketMessage("pushinterval", p_interval);
                        free(p_interval);
			break;
		default:
			constructSocketMessage("getvar", "No matching object found");
	}
}
#endif
#endif

void apiReadAll() {
	//char ch = '/';

	/*strcpy(g_files.fileName, g_dirs.dataDir);
	strncat(g_files.fileName, &ch, 1);
	strcat(g_files.fileName, g_files.jsonFileName);*/
	int written = snprintf(g_files.fileName, g_sizes.filename_size, "%s/%s", g_dirs.dataDir, g_files.jsonFileName);
	if (written < 0) {
		writeLog("Could not read from jsonfile. Encoding error getting file name.", 1, 0);
	}
	else if ((size_t)written >= g_sizes.filename_size) {
		writeLog("Could not get jsonfile. Name is too long.", 1, 0);
	}
	else 
		apiReadFile(g_files.fileName, 0); 
}

/*void collectJsonData(int decLen){
	//char ch = '/';
	char* pluginName = NULL;
	char plts[14];
	FILE *fp = NULL;
        clock_t t;

	if (g_files.fileName == NULL || g_dirs.dataDir == NULL) {
		printf("Variabels in collectJsonData is empty.\n");
		return;
	}
	pthread_mutex_lock(&g_threading.update_mtx);
	//strcpy(g_files.fileName, g_dirs.dataDir);
	//strncat(g_files.fileName, &ch, 1);
	//strcat(g_files.fileName, g_files.jsonFileName)/
	int written = snprintf(g_files.fileName, g_sizes.filename_size, "%s/%s", g_dirs.dataDir, g_files.jsonFileName);
	if (written < 0) {
		writeLog("Could not write to json file", 2, 0);
	}
	if ((size_t)written >= g_sizes.filename_size) {
		writeLog("Json file name truncated. Name is too long.", 1, 0);
	}
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Collecting data to file: %s", g_files.fileName);
	writeLog(trim(g_strings.infostr), 0, 0);
	t = clock();
	fp = fopen(g_files.fileName, "w");
	fputs("{\n", fp);
	fprintf(fp, "   \"host\": {\n");
	fprintf(fp, "      \"name\":\"");
	fputs(g_strings.hostName, fp);
	fprintf(fp, "\",\n");
	fprintf(fp, "      \"pluginfileupdatetime\":\"");
	sprintf(plts, "%ld", g_time.tPluginFile);
        fputs(plts, fp);
        fprintf(fp, "\"\n");
	fputs("   },\n", fp);
	fputs("   \"monitoring\": [\n", fp);
	for (int i = 0; i < decLen; i++) {
		//pluginName = (char *)malloc((size_t)g_sizes.pluginitemname_size * sizeof(char)+1);
		pluginName = strdup(g_pointers.g_plugins[i]->name);
		if (pluginName == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			writeLog("Failed to allocate memory [collectJsonData:pluginName]", 2, 0);
			return;
		}
		removeChar(pluginName, '[');
		removeChar(pluginName, ']');
		fputs("      {\n", fp);
		fprintf(fp, "         \"name\":\"%s\",\n", pluginName);
		free(pluginName);
		pluginName = NULL;
		fprintf(fp, "         \"pluginName\":\"%s\",\n", g_pointers.g_plugins[i]->description);
		switch(g_pointers.g_plugins[i]->output.retCode) {
			case 0:
			   fputs("         \"pluginStatus\":\"OK\",\n", fp);
			   break;
			case 1:
			   fputs("         \"pluginStatus\":\"WARNING\",\n", fp);
			   break;
			case 2:
			   fputs("         \"pluginStatus\":\"CRITICAL\",\n", fp);
                           break;
			default:
			   fputs("         \"pluginStatus\":\"UNKNOWN\",\n", fp);
                           break;
		}
		fprintf(fp, "         \"pluginStatusCode\":\"%d\",\n", g_pointers.g_plugins[i]->output.retCode);
		fprintf(fp, "         \"pluginOutput\":\"%s\",\n", trim(g_pointers.g_plugins[i]->output.retString));
		fprintf(fp, "         \"pluginStatusChanged\":\"%s\",\n", g_pointers.g_plugins[i]->statusChanged);
		if (g_pointers.g_plugins[i]->active > 0)
                        fputs("         \"maintenance\":\"false\",\n", fp);
                else
                        fputs("         \"maintenance\":\"true\",\n", fp);
		fprintf(fp, "         \"lastChange\":\"%s\",\n", g_pointers.g_plugins[i]->lastChangeTimestamp);
		fprintf(fp, "         \"lastRun\":\"%s\", \n", g_pointers.g_plugins[i]->lastRunTimestamp);
		fprintf(fp, "         \"nextRun\":\"%s\"\n", g_pointers.g_plugins[i]->nextRunTimestamp);
		if (i == decLen-1) {
			fputs("      }\n", fp);
		}
		else {
			fputs("      },\n", fp);
		}
	}
        fputs("   ]\n", fp);
	fputs("}\n", fp);
	fclose(fp);
	fp = NULL;
	t = clock() -t;
	//double collection_time = ((double)t)/CLOCKS_PER_SEC;
	//printf("Data collection took %f seconds to execute.\n", collection_time);
	//printf("Data collection took %.0f miliseconds to execute.\n", (double)t);
	//free(dataFName);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Data collection took %.0f miliseconds to execute.", (double)t);
	writeLog(trim(g_strings.infostr), 0, 0);
	pthread_mutex_unlock(&g_threading.update_mtx);
}*/

void collectJsonData(int decLen){
    char *pluginName = NULL;
    char plts[32];
    FILE *tf = NULL;
    int tmpfd = -1;
    char tmpname[1024];
    char targetname[1024];
    clock_t t;

    if (g_files.fileName == NULL || g_dirs.dataDir == NULL) {
        printf("Variables in collectJsonData is empty.\n");
        return;
    }

    pthread_mutex_lock(&g_threading.update_mtx);

    /* build target path */
    int written = snprintf(targetname, sizeof(targetname), "%s/%s", g_dirs.dataDir, g_files.jsonFileName);
    if (written < 0) {
        writeLog("Could not write to json file", 2, 0);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }
    if ((size_t)written >= sizeof(targetname)) {
        writeLog("Json file name truncated. Name is too long.", 1, 0);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    /* build temp template in same directory; mkstemp requires XXXXXX */
    written = snprintf(tmpname, sizeof(tmpname), "%s/.%s.tmpXXXXXX", g_dirs.dataDir, g_files.jsonFileName);
    if (written < 0 || (size_t)written >= sizeof(tmpname)) {
        writeLog("Temp file name truncated. Name is too long.", 1, 0);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    /* create temp file securely */
    tmpfd = mkstemp(tmpname);
    if (tmpfd == -1) {
        writeLog("Failed to create temp file for JSON output", 2, 0);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    /* optionally set desired permissions (e.g., 0644) */
    if (fchmod(tmpfd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
        /* non-fatal, but log */
        writeLog("Warning: fchmod on temp file failed", 1, 0);
    }

    /* get FILE* for convenience */
    tf = fdopen(tmpfd, "w");
    if (tf == NULL) {
        writeLog("fdopen failed for temp file", 2, 0);
        close(tmpfd);
        unlink(tmpname);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    /* write JSON to temp file (same content as before) */
    snprintf(g_strings.infostr, g_sizes.infostr_size, "Collecting data to temp file: %s", tmpname);
    writeLog(trim(g_strings.infostr), 0, 0);

    t = clock();

    // Create JSON objects using json-c
    struct json_object *root = json_object_new_object();
    struct json_object *host_obj = json_object_new_object();
    struct json_object *monitoring_array = json_object_new_array();

    // Host object
    json_object_object_add(host_obj, "name", json_object_new_string(g_strings.hostName));
    sprintf(plts, "%ld", g_time.tPluginFile);
    json_object_object_add(host_obj, "pluginfileupdatetime", json_object_new_string(plts));
    json_object_object_add(root, "host", host_obj);

    // Monitoring array
    for (int i = 0; i < decLen; i++) {
        struct json_object *plugin_obj = json_object_new_object();

        pluginName = strdup(g_pointers.g_plugins[i]->name);
        if (pluginName == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            writeLog("Failed to allocate memory [collectJsonData:pluginName]", 2, 0);
            // Cleanup
            json_object_put(root); // This frees all nested objects
            fclose(tf);
            unlink(tmpname);
            pthread_mutex_unlock(&g_threading.update_mtx);
            return;
        }
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');

        json_object_object_add(plugin_obj, "name", json_object_new_string(pluginName));
        free(pluginName);
        pluginName = NULL;

        json_object_object_add(plugin_obj, "pluginName", json_object_new_string(g_pointers.g_plugins[i]->description));

        const char *status_str;
        switch(g_pointers.g_plugins[i]->output.retCode) {
            case 0: status_str = "OK"; break;
            case 1: status_str = "WARNING"; break;
            case 2: status_str = "CRITICAL"; break;
            default: status_str = "UNKNOWN"; break;
        }
        json_object_object_add(plugin_obj, "pluginStatus", json_object_new_string(status_str));
        json_object_object_add(plugin_obj, "pluginStatusCode", json_object_new_int(g_pointers.g_plugins[i]->output.retCode));
        json_object_object_add(plugin_obj, "pluginOutput", json_object_new_string(trim(g_pointers.g_plugins[i]->output.retString)));
        json_object_object_add(plugin_obj, "pluginStatusChanged", json_object_new_string(g_pointers.g_plugins[i]->statusChanged));
        json_object_object_add(plugin_obj, "maintenance", json_object_new_string(g_pointers.g_plugins[i]->active > 0 ? "false" : "true"));
        json_object_object_add(plugin_obj, "lastChange", json_object_new_string(g_pointers.g_plugins[i]->lastChangeTimestamp));
        json_object_object_add(plugin_obj, "lastRun", json_object_new_string(g_pointers.g_plugins[i]->lastRunTimestamp));
        json_object_object_add(plugin_obj, "nextRun", json_object_new_string(g_pointers.g_plugins[i]->nextRunTimestamp));

        json_object_array_add(monitoring_array, plugin_obj);
    }

    json_object_object_add(root, "monitoring", monitoring_array);

    // Write JSON to temp file
    if (json_object_to_file_ext(tmpname, root, JSON_C_TO_STRING_PRETTY) != 0) {
        writeLog("Failed to write JSON to temp file", 2, 0);
        json_object_put(root);
        fclose(tf);
        unlink(tmpname);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    // Free the JSON object
    json_object_put(root);

    /* close FILE* (this also closes the underlying fd) */
    if (fclose(tf) != 0) {
        writeLog("fclose failed on temp file", 2, 0);
        unlink(tmpname);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }
    tf = NULL;

    /* atomically replace target with temp file */
    if (rename(tmpname, targetname) != 0) {
        snprintf(g_strings.infostr, g_sizes.infostr_size, "[Collect data] Rename failed: %s", strerror(errno));
        writeLog(trim(g_strings.infostr), 2, 0);
        unlink(tmpname);
        pthread_mutex_unlock(&g_threading.update_mtx);
        return;
    }

    t = clock() - t;
    snprintf(g_strings.infostr, g_sizes.infostr_size, "Data collection took %.0f miliseconds to execute.", (double)t);
    writeLog(trim(g_strings.infostr), 0, 0);

    pthread_mutex_unlock(&g_threading.update_mtx);
}

void collectMetrics(int decLen, int style) {
        //char ch = '/';
	char* pluginName = NULL;
	char* serviceName = NULL;
	FILE *mf = NULL;
        clock_t t;
	char *p = NULL;
	int metricsValueLength = 0;
	/*int tmpfd = -1;
    	char tmpname[1024];
    	char targetname[1024];*/

        t = clock();
	pthread_mutex_lock(&g_threading.update_mtx);
        /*strncpy(g_files.storeName, g_dirs.storeDir, g_sizes.storedir_size);
        strncat(g_files.storeName, &ch, 1);
        strcat(g_files.storeName, g_files.metricsFileName);*/
	snprintf(g_files.storeName, g_sizes.storename_size, "%s/%s", g_dirs.storeDir, g_files.metricsFileName); 
        mf = fopen(g_files.storeName, "w");
	if (mf == NULL) {
		writeLog("Failed to open metrics file", 1, 0);
		fprintf(stderr, "Failed to open metrics file\n");
		return;
	}
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Collecting metrics to file: %s", g_files.storeName);
        writeLog(trim(g_strings.infostr), 0, 0);
	for (int i = 0; i < decLen; i++) {
		/*pluginName = (char *)malloc((size_t)g_sizes.pluginitemname_size * sizeof(char)+1);
		memset(pluginName, '\0', g_sizes.pluginitemname_size+1 * sizeof(char));
		if (pluginName == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			writeLog("Memory allocation failed [collectMetrics:pluginName]", 2, 0);
			return;
		}*/
		pluginName = strdup(g_pointers.g_plugins[i]->name);
		if (!pluginName) {
			fprintf(stderr, "Memory allocation failed.\n");
                        writeLog("Memory allocation failed [collectMetrics:pluginName]", 2, 0);
                        return;
		}
        	removeChar(pluginName, '[');
        	removeChar(pluginName, ']');
		for (p = pluginName; *p != '\0'; ++p) {
			//if (*p == '/') *p = '_';
			*p = tolower(*p);
		}
        	// Get metrics
        	char *e;
		char *raw = g_pointers.g_plugins[i]->output.retString;
		char *trimmed_raw = raw ? trim(raw) : "";
		if (raw == NULL || strchr(raw, '|') == NULL) {
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Plugin %s does not provide metrics. Using plain output.",pluginName);
        		writeLog(trim(g_strings.infostr), 1, 0);
		//if (strchr(outputs[i].retString, '|') == NULL) {
		//	snprintf(g_strings.infostr, g_sizes.infostr_size, "Plugin %s does not provide metrics. Using plain output.", pluginName);
		//	writeLog(trim(g_strings.infostr), 1, 0);
			const char *prefix = trim(g_strings.metricsOutputPrefix);
			if (style == 0)
                       		fprintf(mf, "%s_%s{hostname=\"%s\",%s_result=\"%s\"} %d\n", prefix, pluginName, g_strings.hostName, pluginName, trimmed_raw, g_pointers.g_plugins[i]->output.retCode);
			else { 
				// Get service name	
				/*serviceName = (char *)malloc((size_t)g_sizes.pluginitemdesc_size * sizeof(char));
				if (serviceName == NULL) {
					fprintf(stderr, "Failed to allocate memory.\n");
					writeLog("Failed to allocate memory [collectMetrics:serviceName]", 2, 0);
					return;
				}
				memset(serviceName, '\0', g_sizes.pluginitemdesc_size * sizeof(char) + 1);
				strcpy(serviceName, g_pointers.g_plugins[i].description);*/
				const char *service = trim(g_pointers.g_plugins[i]->description);
				fprintf(mf, "%s_%s{hostname=\"%s\", service=\"%s\", value=\"%s\"} %d\n", prefix, pluginName, g_strings.hostName, service, trimmed_raw, g_pointers.g_plugins[i]->output.retCode);
				free(serviceName);
				serviceName = NULL;
			}
		}
                else {
        	 	e = strchr(g_pointers.g_plugins[i]->output.retString, '|');
        	    	int position = (int)(e - g_pointers.g_plugins[i]->output.retString);
			int len = g_sizes.pluginoutput_size;
			size_t srcSize = strlen(g_pointers.g_plugins[i]->output.retString) - position;
			int sublen = (srcSize < len) ? srcSize : len;
			char * metrics = malloc((size_t)sizeof(char) * sublen);
			memset(metrics, 0, sizeof(char) * sublen);
			if (sublen <= srcSize) {
        			//memcpy(metrics,&outputs[i].retString[position+1],sublen);
				memcpy(metrics, &g_pointers.g_plugins[i]->output.retString[position+1],sublen);
			}
			else {
				writeLog("Invalid memcpy operation: size exceeds buffer limit.", 1, 0);
				fprintf(stderr, "Size exceeds buffer [memcpy].\n");
			}
			if (style == 0)
				fprintf(mf, "%s_%s{hostname=\"%s\", %s_result=\"%s\"} %d\n", trim(g_strings.metricsOutputPrefix), pluginName, g_strings.hostName, pluginName, trim(g_pointers.g_plugins[i]->output.retString), g_pointers.g_plugins[i]->output.retCode);
			else {
				serviceName = (char *)malloc((size_t)g_sizes.pluginitemdesc_size * sizeof(char) + 1);
				if (serviceName == NULL) {
					fprintf(stderr, "Memory allocation failed.\n");
					writeLog("Failed to allocate memory [collectMetrics:serviceName]", 2, 0);
					return;
				}
				memset(serviceName, '\0', g_sizes.pluginitemdesc_size * sizeof(char) + 1);
				strcpy(serviceName, g_pointers.g_plugins[i]->description);
				// We need to loop through metrics
				char * token = strtok(metrics, " ");
				while (token != NULL) {
					char* metricsToken;
					char* metricsName;
					char* metricsValue;
					metricsToken = malloc((size_t)strlen(token)+1);
					if (metricsToken == NULL) {
						writeLog("Failed to allocate memory [collectMetrics:metricsToken]", 2, 0);
						return;
					}
					memset(metricsToken, '\0', (size_t)strlen(token)+1 * sizeof(char));
					int do_cut = 0;
					const char *haystring = ";";
					char *c = token;
					while (*c) {
						if (strchr(haystring, *c)) {
							do_cut++;
						}
						c++;
					}
					char *e = strchr(token, ';');
                                        int index = (int)(e - token);
					if (do_cut > 0) {
						strcpy(metricsToken, token);
						metricsToken[index] = '\0';
					}
					else {
						strcpy(metricsToken, token);
					}
					char *f = strchr(metricsToken, '=');
					index = (int)(f - metricsToken);
					metricsName = malloc((size_t)strlen(metricsToken)+1);
					if (metricsName == NULL) {
						writeLog("Failed to allocate memory [collectMetrics:metricsName]", 2, 0);
						return;
					}
					else
						memset(metricsName, '\0', (size_t)strlen(metricsToken)+1);
                                        strcpy(metricsName, metricsToken);
					if (strlen(metricsName) < 5) {
						return;
					}
					char *endOfMetricsName = f+1;
					if (endOfMetricsName != NULL) {
						char *nullTerminator = strchr(endOfMetricsName, '\0');
						if (nullTerminator != NULL) {
							metricsValueLength = strlen(endOfMetricsName);
						}
						else {
							printf("Warn: endOfMetricsName is not null-terminated.\n");
                                                        return;
						}
					}
					else {
						printf("Warn: can not set metric value length.\n");
                                                return;
                                        }

					metricsValue = malloc((size_t)(metricsValueLength+1) * sizeof(char));
					if (metricsValue == NULL) {
						writeLog("Failed to allocate memory [collectMetrics:metricsValue]", 2, 0);
						return;
					}
					else
						memset(metricsValue, '\0', (size_t)(metricsValueLength+1) * sizeof(char));
					strncpy(metricsValue, metricsName + index +1, (size_t)metricsValueLength);
					metricsName[index] = '\0';
					char *pm;
					for (pm = metricsName; *pm != '\0'; ++pm) 
			                        *pm = tolower(*pm);
					removeChar(metricsName, '/');
					 char * cleanMetricsValue = malloc(metricsValueLength +1);
                                        if (!cleanMetricsValue) {
                                                writeLog("Failed to allocate memory [collectMetrics: cleanMetricsValue]", 2, 0);
                                                return;
                                        }
                                        int count = 0;
                                        for (int i = 0; i < metricsValueLength; ++i) {
                                                if (isdigit(metricsValue[i]) || (metricsValue[i] == '.')) {
                                                        cleanMetricsValue[count++] = metricsValue[i];
                                                }
                                        }
                                        cleanMetricsValue[count] = '\0';
					fprintf(mf, "%s_%s_%s{hostname=\"%s\", service=\"%s\", key=\"%s\"} %s\n", trim(g_strings.metricsOutputPrefix), pluginName, metricsName, g_strings.hostName, serviceName, metricsName, cleanMetricsValue);
					free(metricsValue);
					metricsValue = NULL;
					free(metricsName);
					metricsName = NULL;
					free(metricsToken);
					metricsToken = NULL;
					free(cleanMetricsValue);
					cleanMetricsValue = NULL;
					token = strtok(NULL, " ");
				}
				free(serviceName);
				serviceName = NULL;
				free(metrics);
				metrics = NULL;
			}
		}
        	free(pluginName);
		pluginName = NULL;
	}
	fclose(mf);
	mf = NULL;
        t = clock() -t;
	pthread_mutex_unlock(&g_threading.update_mtx);
        snprintf(g_strings.infostr, g_sizes.infostr_size, "Metrics collection took %.0f miliseconds to execute.", (double)t);
        writeLog(trim(g_strings.infostr), 0, 0);
}

void timeTune(int seconds) {
	int i;
	size_t dest_size = 20;
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Tuning up run times %d seconds", seconds);
	writeLog(trim(g_strings.infostr), 0, 0);
	// Loop through and change nextTimeValue
	for (i = 0; i < g_ints.decCount; i++) {
		if (i != g_ints.timeTunerMaster) {
			time_t nextTime = g_pointers.g_plugins[i]->nextRun + seconds;
                	struct tm tNextTime;
                	memset(&tNextTime, '\0', sizeof(struct tm));
               	 	localtime_r(&nextTime, &tNextTime);
                	int len = snprintf(g_pointers.g_plugins[i]->nextRunTimestamp, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			if (len >= dest_size) {
				writeLog("Truncation of timestamp possible in funtion 'timeTune'", 1, 0);
			}
                	g_pointers.g_plugins[i]->nextRun = nextTime;
			if (g_bools.timeScheduler)
				g_pointers.scheduler[g_pointers.g_plugins[i]->id].timestamp = nextTime;
		}
	}
	if (g_bools.timeScheduler) {
		checkSchedulerCount();
		qsort(g_pointers.scheduler, g_ints.decCount, sizeof(struct Scheduler), compare_timestamps);
	}
}

void writePluginResultToFile(int storeIndex, int update) {
	FILE *fp = NULL;
	char* checkName;
	char timestr[35];
	char ch = '/';
	if (update == 0)
		checkName = strdup(g_pointers.g_plugins[storeIndex]->name);
	else
		checkName = strdup(g_pointers.update_g_plugins[storeIndex].name);
	//memmove(checkName, checkName+1,strlen(checkName));
	//checkName[strlen(checkName)-1] = '\0';
	/*strcpy(g_files.fileName, g_dirs.storeDir);
	strncat(g_files.fileName, &ch, 1);
	strcat(g_files.fileName, checkName);*/
	snprintf(g_files.fileName, g_sizes.filename_size, "%s%c%s", g_dirs.storeDir, ch, checkName);
	free(checkName);
	checkName = NULL;
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strcpy(timestr, asctime(timeinfo));
	timestr[strlen(timestr)-1] = '\0';
	if (fileExists(g_files.fileName) == 0) {
		fp = fopen(g_files.fileName, "a");
	}
	else {
		fp = fopen(g_files.fileName, "w+");
	}
	if (update == 0) {
		if (g_pointers.g_plugins[storeIndex]->name && g_strings.pluginReturnString) {
			if (fp != NULL)
				fprintf(fp, "%s, %s, %s\n", timestr, g_pointers.g_plugins[storeIndex]->name, g_pointers.g_plugins[storeIndex]->output.retString);
			else {
				printf("DEBUG: Could not find file stream. Error.\n");
				writeLog("Could not find file stream [writePluginResultToFile]", 1, 0);
				return;
			}
		}
		fflush(fp);
	}
	else
		fprintf(fp, "%s, %s, %s\n", timestr, g_pointers.update_g_plugins[storeIndex].name, g_strings.pluginReturnString);
	fclose(fp);
	fp = NULL;
}

void writeToKafkaTopic(int storeIndex, int update) {
	char *payload;
	char *pluginName;
	char *pluginStatus;
	char currTime[TIME_BUF_LEN];
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);

        int len = snprintf(currTime, g_sizes.max_timestamp_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (len >= dest_size) {
		writeLog("Possible truncation of timestamp in function 'writeToKafkaTopic'.", 1, 0);
	}
	pluginName = malloc((size_t)g_sizes.pluginitemname_size+1 * sizeof(char));
        if (pluginName == NULL) {
        	fprintf(stderr, "Memory allocation failed.\n");
        	writeLog("Failed to allocate memory [runPlugin:g_bools.enableKafkaExport:pluginName]", 2, 0);
        	return;
	}
       	if (update == 0)
       		pluginName = strdup(g_pointers.g_plugins[storeIndex]->name);
	else
       		pluginName = strdup(g_pointers.update_g_plugins[storeIndex].name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
        switch(g_pointers.g_plugins[storeIndex]->output.retCode) {
        	case 0:
        		pluginStatus = malloc(3);
        		strcpy(pluginStatus, "OK");
        		break;
        	case 1:
        		pluginStatus = malloc(8);
        		strcpy(pluginStatus, "WARNING");
        		break;
        	case 2:
        		pluginStatus = malloc(9);
        		strcpy(pluginStatus, "CRITICAL");
        		break;
        	default:
        		pluginStatus = malloc(8);
        		strcpy(pluginStatus, "UNKNOWN");
        		break;
	}
        int count_bytes = strlen(g_strings.hostName) + strlen(g_pointers.g_plugins[storeIndex]->lastChangeTimestamp) + strlen(g_pointers.g_plugins[storeIndex]->lastRunTimestamp) + strlen(g_pointers.g_plugins[storeIndex]->name) + strlen(g_pointers.g_plugins[storeIndex]->nextRunTimestamp);
        count_bytes += g_sizes.pluginitemdesc_size + g_sizes.pluginoutput_size;
        count_bytes += strlen(pluginStatus) + strlen(g_pointers.g_plugins[storeIndex]->statusChanged);
        count_bytes += 185;
        int kafka_export_addons = 0;
        if (g_bools.enableKafkaTag) {
        	count_bytes += strlen(g_strings.kafka_tag);
        	count_bytes += 12; // {"tag":""}
        	kafka_export_addons += 10;
        }
        if (g_bools.enableKafkaId) {
        	count_bytes += 9; // {"id":""}
        	int length = snprintf(NULL, 0, "%d", g_sizes.kafka_start_id);
        	count_bytes += length;
        	kafka_export_addons += 20;
        }
	payload = malloc((size_t)count_bytes);
        if (payload == NULL) {
        	fprintf(stderr, "Could not allocate memory for payload.\n");
        	writeLog("Failed to allocate memory [runPlugin:g_bools.enableKafkaExport:payload]", 2, 0);
        	return;
        }
        if (kafka_export_addons < 1) {
        	sprintf(payload, "{\"name\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", g_strings.hostName, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
        	printf("Payload = %s\n", payload);
        }
        else {
       		if (kafka_export_addons == KAFKA_EXPORT_TAG) {
        		sprintf(payload, "{\"name\":\"%s\", \"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", g_strings.hostName, g_strings.kafka_tag, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
        	}
        	else {
        		int nKafkaId = g_sizes.kafka_start_id + storeIndex;
        		int length = snprintf(NULL, 0, "%d", nKafkaId);
        		char* kafka_id = malloc((size_t)length + 1);
        		snprintf(kafka_id, (size_t)length+1, "%d", nKafkaId);
        		if (kafka_export_addons == KAFKA_EXPORT_ID) {
        			sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", g_strings.hostName, kafka_id, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
        		}
        		else if (kafka_export_addons == KAFKA_EXPORT_IDTAG) {
        			sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\",\"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", g_strings.hostName, kafka_id, g_strings.kafka_tag, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
                        }
                }
	}
	if (g_bools.useKafkaConfigFile) {
		send_message_to_gkafka(payload);
		free(pluginName);
		free(pluginStatus);
		free(payload);
		pluginName = NULL;
		pluginStatus = NULL;
		payload = NULL;
		return;
	}
        if (!g_bools.enableKafkaSSL) {
		if (!g_bools.kafkaAvro)
			send_message_to_kafka(g_strings.kafka_brokers, g_strings.kafka_topic, payload);
		else {
			int nKafkaId = g_sizes.kafka_start_id + storeIndex;
                	int length = snprintf(NULL, 0, "%d", nKafkaId);
                	char* kafka_id = malloc((size_t)length + 1);
                	snprintf(kafka_id, (size_t)length+1, "%d", nKafkaId);
			send_avro_message_to_kafka(g_strings.kafka_brokers, g_strings.kafka_topic, g_strings.hostName, kafka_id, g_strings.kafka_tag, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
		}
	}
        else {
		if (!g_bools.kafkaAvro) {
			send_ssl_message_to_kafka(g_strings.kafka_brokers, g_strings.kafkaCACertificate, g_strings.kafkaProducerCertificate, g_strings.kafkaSSLKey, g_strings.kafka_topic, payload);
		}
		else {
			int nKafkaId = g_sizes.kafka_start_id + storeIndex;
                	int length = snprintf(NULL, 0, "%d", nKafkaId);
                	char* kafka_id = malloc((size_t)length + 1);
                	snprintf(kafka_id, (size_t)length+1, "%d", nKafkaId);
        		send_ssl_avro_message_to_kafka(g_strings.kafka_brokers, g_strings.kafkaCACertificate, g_strings.kafkaProducerCertificate, g_strings.kafkaSSLKey, g_strings.kafka_topic, g_strings.hostName, kafka_id, g_strings.kafka_tag, g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime, pluginName, g_pointers.g_plugins[storeIndex]->nextRunTimestamp, g_pointers.g_plugins[storeIndex]->description, g_pointers.g_plugins[storeIndex]->output.retString, pluginStatus, g_pointers.g_plugins[storeIndex]->statusChanged, g_pointers.g_plugins[storeIndex]->output.retCode);
		}
	}
	free(pluginName);
        free(pluginStatus);
        pluginName = NULL;
        pluginStatus = NULL;
        free(payload);
        payload = NULL;
}

void runPluginCommand(int index, char* command) {
	int prevRetCode = 0;
	clock_t ct;
	time_t t;
	//char currTime[22];
	char currTime[TIME_BUF_LEN];
	int rc = 0;

	if (strlen(command) > 100) {
		writeLog("Command longer than expected. Aborting run.", 1, 0);
		return;
	}
	prevRetCode = g_pointers.g_plugins[index]->output.retCode;
	ct = clock();
	time(&t);
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Running %s.", trim(command));
	writeLog(trim(g_strings.infostr), 0, 0);
	TrackedPopen tp = tracked_popen(trim(command));
	if (tp.fp == NULL) {
		printf("Failed to run command\n");
		writeLog("Failed to run command.", 1, 0);
		// Update with failed run
		g_pointers.g_plugins[index]->output.retCode = 3;
		strncpy(g_pointers.g_plugins[index]->output.retString, "UNKNOWN: Failed to run command", g_sizes.pluginoutput_size);
		return;
	}
	add_plugin_pid(tp.pid);
        while (fgets(g_strings.pluginReturnString, g_sizes.pluginmessage_size, tp.fp) != NULL) {
		// // VERBOSE  printf("%s", g_strings.pluginReturnString);
	}
	rc = tracked_pclose(&tp);
        if (rc == -1) {
        	snprintf(g_strings.infostr, g_sizes.infostr_size,
                     "[runPlugin] tracked_pclose failed: errno %d (%s)",
                     errno, strerror(errno));
        	writeLog(trim(g_strings.infostr), 1, 0);
        }

	if (rc > 0) {
        	if (rc == 256)
        		g_pointers.g_plugins[index]->output.retCode = 1;
        	else if (rc == 512)
        		g_pointers.g_plugins[index]->output.retCode = 2;
        	else
        		g_pointers.g_plugins[index]->output.retCode = rc;
        }
        else
        	g_pointers.g_plugins[index]->output.retCode = rc;
	remove_plugin_pid(tp.pid);
	if (g_strings.pluginReturnString != NULL && g_pointers.g_plugins[index]->output.retString != NULL) {
		char *trimmed = trim(g_strings.pluginReturnString);
		size_t trimmed_len = strlen(trimmed);
		size_t copy_len = (trimmed_len < g_sizes.pluginoutput_size - 1) ? trimmed_len : g_sizes.pluginoutput_size - 1;
		//strncpy(g_pointers.g_plugins[index]->output.retString, trimmed, copy_len);
		snprintf(g_pointers.g_plugins[index]->output.retString, g_sizes.pluginoutput_size, "%s", trimmed);
		g_pointers.g_plugins[index]->output.retString[copy_len] = '\0';
	}
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);
	int tlen = snprintf(currTime, g_sizes.max_timestamp_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (tlen >= dest_size) {
		writeLog("Possible truncation of timestamp in function 'runPluginCommand'.", 1, 0);
	}
	if (g_pointers.g_plugins[index]->output.prevRetCode != -1){
        	//snprintf(currTime, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                if (prevRetCode != g_pointers.g_plugins[index]->output.retCode){
                	strcpy(g_pointers.g_plugins[index]->statusChanged, "1");
                	strcpy(g_pointers.g_plugins[index]->lastChangeTimestamp, currTime);
                }
                else {
                	strcpy(g_pointers.g_plugins[index]->statusChanged, "0");
                }
		strcpy(g_pointers.g_plugins[index]->lastRunTimestamp, currTime);
                time_t nextTime = t + (g_pointers.g_plugins[index]->interval * 60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                int len = snprintf(g_pointers.g_plugins[index]->nextRunTimestamp, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
		if (len >= dest_size) {
			writeLog("Possible truncation of timestamp in 'runPluginCommand'.", 1, 0);
		}
                g_pointers.g_plugins[index]->nextRun = nextTime;
                g_pointers.g_plugins[index]->output.prevRetCode = g_pointers.g_plugins[index]->output.retCode;
                if (g_bools.timeScheduler) {
                	g_pointers.scheduler[0].timestamp = nextTime;
                }
       	}
       	else {
       		g_pointers.g_plugins[index]->output.prevRetCode = 0;
      	}
      	ct = clock() -ct;
        snprintf(g_strings.infostr, g_sizes.infostr_size, "%s executed. Execution took %.0f milliseconds.\n", g_pointers.g_plugins[index]->name, (double)ct);
        writeLog(trim(g_strings.infostr), 0, 0);
        if (g_bools.logPluginOutput == true) {
                char* o_info;
                int o_info_size = g_sizes.pluginmessage_size + 195;
                o_info = malloc((size_t)o_info_size * sizeof(char));
                if (o_info == NULL) {
                        writeLog("Could not allocate memory for variable 'o_info'.", 2, 0);
			return;
                }
                snprintf(o_info, (size_t)o_info_size, "%s : %s", g_pointers.g_plugins[index]->name, g_strings.pluginReturnString);
                writeLog(trim(o_info), 0, 0);
                free(o_info);
                o_info = NULL;
        }
	if (g_bools.pluginResultToFile) {
		writePluginResultToFile(index, 0);
	}
	if (g_bools.enableKafkaExport) {
                writeToKafkaTopic(index, 0);
	}
}

void runPluginOld(int storeIndex, int update) {
	char ch = '/';
	int prevRetCode = 0;
	clock_t ct;
	time_t t;
	//char currTime[22];
	char currTime[TIME_BUF_LEN];
	int rc = 0;
	char sPluginCommand[g_sizes.plugincommand_size];

	if (update > 0)
		prevRetCode = g_pointers.g_plugins[storeIndex]->output.retCode;
	ct = clock();
	time(&t);
	// Test local var
	//strcpy(sPluginCommand, g_dirs.pluginDir);
	//strncat(sPluginCommand, &ch, 1);
	//sPluginCommand[g_sizes.plugincommand_size -1] = '\0';
	snprintf(sPluginCommand, g_sizes.plugincommand_size, "%s%c", g_dirs.pluginDir, ch);
	if (update > 0) {
                strcat(sPluginCommand, g_pointers.update_g_plugins[storeIndex].command);
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Running: %s.", g_pointers.update_g_plugins[storeIndex].command);
        }
        else {
                strcat(sPluginCommand, g_pointers.g_plugins[storeIndex]->command);
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Running: %s.", g_pointers.g_plugins[storeIndex]->command);
        }
	writeLog(trim(g_strings.infostr), 0, 0);
	TrackedPopen tp = tracked_popen(sPluginCommand);
	if (tp.fp == NULL) {
		printf("Failed to run command\n");
		writeLog("Failed to run comman via tracked_popen().", 2, 0);
		rc = -1;
	}
	else {
		add_plugin_pid(tp.pid);
		while (fgets(g_strings.pluginReturnString, g_sizes.pluginmessage_size, tp.fp) != NULL) {
			// VERBOSE  printf("%s", g_strings.pluginReturnString);
			// printf("DEBUG: %s\n", g_strings.pluginReturnString);
		}
		rc = tracked_pclose(&tp);
		if (rc == -1) {
			snprintf(g_strings.infostr, g_sizes.infostr_size, "[runPlugin] tracked_pclose failed with errno %d (%s)", errno, strerror(errno));
			writeLog(trim(g_strings.infostr), 1, 0);
		}
		remove_plugin_pid(tp.pid);
	}
	switch (update) {
		case 0:
			if (rc > 0)
			{
				if (rc == 256)
					g_pointers.g_plugins[storeIndex]->output.retCode = 1;
				else if (rc == 512)
					g_pointers.g_plugins[storeIndex]->output.retCode = 2;
				else
					g_pointers.g_plugins[storeIndex]->output.retCode = rc;
			}
			else
				g_pointers.g_plugins[storeIndex]->output.retCode = rc;
			break;
		case 1:
			if (rc > 0) {
				if (rc == 256)
					printf("Depricated.\n");
				else if (rc == 512)
					printf("Depricated.\n");
				else
					printf("Depricated.\n");
			}
			else
				printf("Depricated.\n");			
			break;
		default:
			switch (rc) {
				case 256:
					g_pointers.g_plugins[storeIndex]->output.retCode = 1;
					//update_outputs[storeIndex].retCode = 1;
					break;
				case 512:
					g_pointers.g_plugins[storeIndex]->output.retCode = 1;
                                        //update_outputs[storeIndex].retCode = 1;
					break;
				default:
					g_pointers.g_plugins[storeIndex]->output.retCode = rc;
                                        //update_outputs[storeIndex].retCode = rc;
			}
	}
	//outout.retString size?
	if (update > 0){ 
		//update_outputs[storeIndex].retString = strdup(trim(g_strings.pluginReturnString));
	}
	else {
		if (g_strings.pluginReturnString != NULL && g_pointers.g_plugins[storeIndex]->output.retString != NULL){
			if (strlen(trim(g_strings.pluginReturnString)) < g_sizes.pluginoutput_size) 
				strncpy(g_pointers.g_plugins[storeIndex]->output.retString, trim(g_strings.pluginReturnString), g_sizes.pluginoutput_size);
			else {
				g_strings.pluginReturnString[g_sizes.pluginoutput_size] = '\0';
				strncpy(g_pointers.g_plugins[storeIndex]->output.retString, trim(g_strings.pluginReturnString), g_sizes.pluginoutput_size);
			}
		}
		else
			printf("WARNING: Want to write to variables that is freed. Is system closing?\n");
	}
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);
        int len = snprintf(currTime, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (len >= dest_size) {
		writeLog("Possible truncation of timestamp while running plugin.", 1, 0);
	}
	if (update == 0) {
		if (g_pointers.g_plugins[storeIndex]->output.prevRetCode != -1){
                	if (prevRetCode != g_pointers.g_plugins[storeIndex]->output.retCode){
				strcpy(g_pointers.g_plugins[storeIndex]->statusChanged, "1");
				strcpy(g_pointers.g_plugins[storeIndex]->lastChangeTimestamp, currTime);
				// Here something is wrong, it updates even if change is 0?
			}
			else {
				strcpy(g_pointers.g_plugins[storeIndex]->statusChanged, "0");
			}
			if (g_bools.enableTimeTuner) {
				if (storeIndex == g_ints.timeTunerMaster) {
					g_ints.timeTunerCounter++;
					if (g_ints.timeTunerCounter == g_ints.timeTunerCycle) {
						g_ints.timeTunerCounter = 0;
						// Get time diff
						char oldTime[20];
						struct tm time;
						strcpy(oldTime, g_pointers.g_plugins[g_ints.timeTunerMaster]->lastRunTimestamp);
						strptime(oldTime, "%04d-%02d-%02d %02d:%02d:%02d", &time);
						time_t ttOldTime = 0, ttCurTime = 0;
						int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
						if (sscanf(oldTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
							struct tm breakdown = {0};
							breakdown.tm_year = year + 1900;
							breakdown.tm_mon = month - 1;
       							breakdown.tm_mday = day;
       							breakdown.tm_hour = hour;
       							breakdown.tm_min = minute;
							breakdown.tm_sec = second;

							if ((ttOldTime = mktime(&breakdown)) == (time_t)-1) {
          							fprintf(stderr, "Could not convert time input to time_t\n");
       							}
						}
						if (sscanf(currTime, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
                                                        struct tm breakdown = {0};
                                                        breakdown.tm_year = year + 1900;
                                                        breakdown.tm_mon = month - 1;
                                                        breakdown.tm_mday = day;
                                                        breakdown.tm_hour = hour;
                                                        breakdown.tm_min = minute;
                                                        breakdown.tm_sec = second;

                                                        if ((ttCurTime = mktime(&breakdown)) == (time_t)-1) {
                                                                fprintf(stderr, "Could not convert time input to time_t\n");
                                                        }
                                                }
						int difference = ttCurTime - ttOldTime - (g_pointers.g_plugins[g_ints.timeTunerMaster]->interval * 60);
						// Apply time diff to all nextRuns :)
						timeTune(difference);
					}
				}
                        }
                	strcpy(g_pointers.g_plugins[storeIndex]->lastRunTimestamp, currTime);
                	time_t nextTime = t + (g_pointers.g_plugins[storeIndex]->interval * 60);
                	struct tm tNextTime;
                	memset(&tNextTime, '\0', sizeof(struct tm));
                	localtime_r(&nextTime, &tNextTime);
                	len = snprintf(g_pointers.g_plugins[storeIndex]->nextRunTimestamp, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			if (len >= dest_size) {
				writeLog("Possible truncation of timestamp in 'runPlugin'.", 1, 0);
			}
			g_pointers.g_plugins[storeIndex]->nextRun = nextTime;
                	g_pointers.g_plugins[storeIndex]->output.prevRetCode = g_pointers.g_plugins[storeIndex]->output.retCode;
			if (g_bools.timeScheduler) {
				g_pointers.scheduler[0].timestamp = nextTime;
			}
		}
		else {
	        	g_pointers.g_plugins[storeIndex]->output.prevRetCode = 0; 
		}
	}
	else {
		// If update = 1 use update_outputs
		// Will this be correct?
		/*if (prevRetCode != update_outputs[storeIndex].retCode){
                	strcpy(g_pointers.update_g_plugins[storeIndex].statusChanged, "1");
                        strcpy(g_pointers.update_g_plugins[storeIndex].lastChangeTimestamp, currTime);
                }
                else {
                	strcpy(g_pointers.update_g_plugins[storeIndex].statusChanged, "0");
                }*/
	}
	ct = clock() -ct;
	if (update == 0)
		snprintf(g_strings.infostr, g_sizes.infostr_size, "%s executed. Execution took %.0f milliseconds.\n", g_pointers.g_plugins[storeIndex]->name, (double)ct);
	else
		snprintf(g_strings.infostr, g_sizes.infostr_size, "%s executed. Execution took %.0f milliseconds.\n", g_pointers.update_g_plugins[storeIndex].name, (double)ct);
        writeLog(trim(g_strings.infostr), 0, 0);
	if (g_bools.logPluginOutput == true) {
		char* o_info;
		int o_info_size = g_sizes.pluginmessage_size + 195; 
		o_info = malloc((size_t)o_info_size * sizeof(char));
		if (o_info == NULL) {
			writeLog("Could not allocate memory for variable 'o_info'.", 2, 0);
		}
		if (update == 0)
			snprintf(o_info, (size_t)o_info_size, "%s : %s", g_pointers.g_plugins[storeIndex]->name, g_strings.pluginReturnString);
		else
			snprintf(o_info, (size_t)o_info_size, "%s : %s", g_pointers.update_g_plugins[storeIndex].name, g_strings.pluginReturnString);
		writeLog(trim(o_info), 0, 0);
		free(o_info);
		o_info = NULL;
	}
	if (g_bools.pluginResultToFile) {
		writePluginResultToFile(storeIndex, update);
	}
	if (g_bools.enableKafkaExport) {
		writeToKafkaTopic(storeIndex, update);
	}
}

void runGardener() {
	int rc = 0;

	TrackedPopen tp = tracked_popen(g_files.gardenerScript);
        if (tp.fp == NULL) {
                printf("Failed to run gardener script\n");
                writeLog("Failed to run gardener script.", 2, 0);
		rc = -1;
        }
	else {
		add_plugin_pid(tp.pid);
        	while (fgets(g_strings.gardenerRetString, g_sizes.gardenermessage_size, tp.fp) != NULL) {
                	// VERBOSE  printf("%s", g_strings.gardenerRetString);
		}
        	rc = tracked_pclose(&tp);
		if (rc == -1) {
			snprintf(g_strings.infostr, g_sizes.infostr_size,
                     		"[runPlugin] tracked_pclose failed: errno %d (%s)",
                     		errno, strerror(errno));
            		writeLog(trim(g_strings.infostr), 1, 0);
		}
		remove_plugin_pid(tp.pid);
        }
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Gardener script executed with return code %i.", rc);
        if (rc > 1) {
		writeLog(trim(g_strings.infostr), 2, 0);
	}
	else writeLog(trim(g_strings.infostr), rc, 0);
}

void runClearDataCache() {
	DIR *d = NULL;
	struct dirent *dir;
	d = opendir(g_dirs.dataDir);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG) {
				char buf[1024];
				struct stat filestat;
				sprintf(buf, "%s/%s", g_dirs.dataDir, dir->d_name);
				stat(buf, &filestat);
                                snprintf(g_strings.infostr, g_sizes.infostr_size, "ClearDataCash checking file: %s", dir->d_name);
                                writeLog(trim(g_strings.infostr), 0, 0);
				// Now check time 
				time_t now = time(NULL);
				// HERE Set current time to timestamp!!!
				time_t ftime = filestat.st_ctime + g_sizes.dataCacheTimeFrame;
				if (now > ftime) {
                                        snprintf(g_strings.infostr, g_sizes.infostr_size, "ClearDataCash remove file: %s", dir->d_name);
                                        writeLog(trim(g_strings.infostr), 0, 0);
					remove(buf);
				}
			}
		}
		closedir(d);
	}
}

void* pluginExeThread(void* data) {
	/*sigset_t sigset;
    	sigemptyset(&sigset);
    	sigaddset(&sigset, SIGCHLD);
    	pthread_sigmask(SIG_BLOCK, &sigset, NULL);*/
	intptr_t storeIndex = (intptr_t)data;
	pthread_detach(pthread_self());
	// VERBOSE printf("Executing %s in pthread %lu\n", g_pointers.g_plugins[storeIndex].description, pthread_self());
	pthread_mutex_lock(&g_threading.mtx);
	g_arrays.threadIds[(short)storeIndex] = 1;
        PluginItem *pi = getPluginItem(storeIndex);
	run_plugin(pi);
	if (g_bools.timeScheduler) {
        	for (size_t i = 0; i < g_ints.decCount; i++) {
                	if (g_pointers.scheduler[i].id == storeIndex) {
                        	g_pointers.scheduler[i].timestamp = g_pointers.g_plugins[storeIndex]->nextRun;
                        	//printf("Updated g_pointers.scheduler[%zu] for plugin_id %ld\n", i, storeIndex);
                        	break;
                	}
        	}
		rescheduleChecks();
	}

	//runPlugin(storeIndex, 0);
	g_sizes.thread_counter--;
	pthread_mutex_unlock(&g_threading.mtx);
        g_arrays.threadIds[(short)storeIndex] = 0;
	pthread_exit(NULL);
	g_sizes.total_threads_run++;
}

void* gardenerExeThread(void* data) {
 	/*sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGCHLD);
        pthread_sigmask(SIG_BLOCK, &sigset, NULL);*/
	pthread_detach(pthread_self());
	runGardener();
	pthread_mutex_lock(&g_threading.mtx);
	g_sizes.thread_counter--;
	pthread_mutex_unlock(&g_threading.mtx);
	pthread_exit(NULL);
	g_sizes.total_threads_run++;
}

void* clearDataCacheThread(void* data) {
	/*sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGCHLD);
        pthread_sigmask(SIG_BLOCK, &sigset, NULL);*/
	pthread_detach(pthread_self());
	runClearDataCache();
	pthread_mutex_lock(&g_threading.mtx);
	g_sizes.thread_counter--;
	pthread_mutex_unlock(&g_threading.mtx);
	pthread_exit(NULL);
	g_sizes.total_threads_run++;
}

int countDeclarations(char *file_name) {
	FILE *fp = NULL;
	int i = 0;
        int ch;

	if (file_name == NULL || strlen(file_name) == 0) {
		writeLog("Filename is not initialized or is empty.", 2, 0);
		fprintf(stderr, "Filename is uninitialized or empty.\n");
	}
        fp = fopen(file_name, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file[countDeclarations].\n");
		writeLog("Error opening and counting g_pointers.g_plugins file.", 2, 0);
                exit(EXIT_FAILURE);
        }
        while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n')
			i++;
	}
	fclose(fp);
	fp = NULL;
	return i-1;
}

/*int loadPluginDeclarations(const char *configFile, int reload) {
	FILE *fp = fopen(configFile, "r");
    	if (!fp) {
        	writeLog("Cannot open plugin g_pointers.g_plugins file", 2, 0);
        	return -1;
    	}

    	char *line = NULL;
    	size_t len = 0;
    	ssize_t read;
    	int count = 0;
    	int lineno = 0;

    	while ((read = getline(&line, &len, fp)) != -1) {
        	lineno++;
        	char *trimmed = trim_line(line);
        	if (*trimmed == '\0' || *trimmed == '#')
            		continue;  

        	if (count >= MAX_DECLS) {
            		writeLog("Too many g_pointers.g_plugins, skipping rest", LOG_LEVEL_WARN, 0);
            		break;
        	}

        	if (parseLine(trimmed, &g_pointers.g_plugins[count], count, lineno)) {
            		snprintf(g_strings.infostr, sizeof(g_strings.infostr),"Loaded declaration [%s] (id=%d)",g_pointers.g_plugins[count].name, count);
            		writeLog(g_strings.infostr, 0, 0);
            		count++;
        	}
    	}

    	free(line);
    	fclose(fp);

	return count;
}*/

/*int loadPluginDeclarations(const char *pluginDeclarationsFile, int reload) {
    	int counter      = 0;
    	int i, index     = 0;
    	int ret          = 0;     // return code, 0 on success, <0 on error
    	char *line       = NULL;
    	char *linecopy   = NULL;
    	size_t len       = 0;
    	ssize_t read;
    	FILE *fp         = NULL;

    	fp = fopen(pluginDeclarationsFile, "r");
    	if (!fp) {
        	writeLog("Error opening plugin g_pointers.g_plugins file.", 2, 0);
        	ret = -1;
        	goto cleanup;
    	}

    	while ((read = getline(&line, &len, fp)) != -1) {
        	index++;
        	if (strchr(line, '#')) {
            		// comment or empty → skip
            		continue;
        	}

        	linecopy = strdup(line);
        	if (!linecopy) {
            		writeLog("Failed to duplicate line", 2, 0);
            		ret = -2;
            		goto cleanup;
        	}

        	{
           		char *token      = NULL;
            		char *name       = NULL;
            		char *saveptr    = NULL;
            		int   parsingErr = 0;

            		for (i = 1; ; i++) {
                		token = strtok_r(i == 1 ? linecopy : NULL, ";", &saveptr);
                		if (!token) 
                    			break;

                		switch (i) {
                  			case 1:
                    				name = strtok(token, " ");
                    				if (!name) {
                        				parsingErr = 1;
                        				break;
                    				}
                    				{
                        				char *desc = strtok(NULL, "?");
                        				if (!reload) {
                            					free(g_pointers.g_plugins[counter].name);
                            					g_pointers.g_plugins[counter].name = strdup(name);
                            					free(g_pointers.g_plugins[counter].description);
                            					g_pointers.g_plugins[counter].description = desc 
                                					? strdup(desc) 
                                					: NULL;
                        				} else {
                            					free(g_pointers.update_g_plugins[counter].name);
                            					g_pointers.update_g_plugins[counter].name = strdup(name);
                            					if (desc) {
                                					free(g_pointers.update_g_plugins[counter].description);
                                					g_pointers.update_g_plugins[counter].description = strdup(desc);
                            					} else {
                                					parsingErr = 1;
                            					}
                        				}
                    				}
                    				break;
                  			case 2:
                    				if (strlen(token) < 5) {
                        				parsingErr = 1;
                        				break;
                    				}
                    				if (!reload) {
                        				free(g_pointers.g_plugins[counter].command);
                        				g_pointers.g_plugins[counter].command = strdup(token);
                    				} else {
                        				free(g_pointers.update_g_plugins[counter].command);
                        				g_pointers.update_g_plugins[counter].command = strdup(token);
                    				}
                    				break;
                  			case 3:
                    				if (!reload)
                        				g_pointers.g_plugins[counter].active = atoi(token);
                    				else
                        				g_pointers.update_g_plugins[counter].active = atoi(token);
                    				break;
                  			case 4:
                    				if (!reload) {
                        				g_pointers.g_plugins[counter].interval = atoi(token);
                        				g_pointers.g_plugins[counter].id = index - 1;
                    				} else {
                        				g_pointers.update_g_plugins[counter].interval = atoi(token);
                        				g_pointers.update_g_plugins[counter].id = index - 1;
                    				}
                    				break;
                  			default:
                    				break;
                		}

                		if (parsingErr)
                    			break;
            		}  // end for‐token loop
            		free(linecopy);
            		linecopy = NULL;
            		if (parsingErr) {
                		continue;
            		}
        	}
        	if (!reload) {
            		g_pointers.g_plugins[counter].lastRunTimestamp[0]     = '\0';
            		g_pointers.g_plugins[counter].nextRunTimestamp[0]    = '\0';
            		g_pointers.g_plugins[counter].lastChangeTimestamp[0] = '\0';
            		g_pointers.g_plugins[counter].statusChanged[0]       = '\0';
        	} else {
            		g_pointers.update_g_plugins[counter].lastRunTimestamp[0]     = '\0';
            		g_pointers.update_g_plugins[counter].nextRunTimestamp[0]    = '\0';
            		g_pointers.update_g_plugins[counter].lastChangeTimestamp[0] = '\0';
            		g_pointers.update_g_plugins[counter].statusChanged[0]       = '\0';
        	}
        	snprintf(g_strings.infostr, g_sizes.infostr_size,"Declaration with index %d is created.\n", counter);
        	writeLog(trim(g_strings.infostr), 0, 0);
        	counter++;
    	}

	cleanup:
    		if (linecopy) free(linecopy);
    		if (line) free(line);
    		if (fp) {
        		fclose(fp);
        		fp = NULL;
    		}

   	return (ret == 0 ? counter : ret);
}*/

void copyPluginItem(PluginItem *dest, const PluginItem *src, int mode) {
    if (!dest || !src) return;  // Defensive check

    if (mode == 0) {
        if (src->name != NULL) {
            snprintf(dest->lastRunTimestamp, g_sizes.max_timestamp_size, "%s", src->lastRunTimestamp);
            snprintf(dest->nextRunTimestamp, g_sizes.max_timestamp_size, "%s", src->nextRunTimestamp);
            snprintf(dest->lastChangeTimestamp, g_sizes.max_timestamp_size, "%s", src->lastChangeTimestamp);
            snprintf(dest->statusChanged, 2, "%s", src->statusChanged);
            dest->active = src->active;
            dest->interval = src->interval;
            dest->nextRun = src->nextRun;
        } else {
            writeLog("copyPluginItem[src->name] is empty. Do not copy.", 0, 0);
        }
    } else if (mode == 2) {
        snprintf(dest->lastRunTimestamp, g_sizes.max_timestamp_size, "%s", src->lastRunTimestamp);
        snprintf(dest->nextRunTimestamp, g_sizes.max_timestamp_size, "%s", src->nextRunTimestamp);
        snprintf(dest->statusChanged, 2, "%s", src->statusChanged);
        dest->nextRun = src->nextRun;
    } else {
        snprintf(dest->name, g_sizes.pluginitemname_size + 1, "%s", src->name);
        snprintf(dest->description, g_sizes.pluginitemdesc_size + 1, "%s", src->description);
        snprintf(dest->command, g_sizes.pluginitemcmd_size + 1, "%s", src->command);
        snprintf(dest->lastRunTimestamp, g_sizes.max_timestamp_size, "%s", src->lastRunTimestamp);
        snprintf(dest->nextRunTimestamp, g_sizes.max_timestamp_size, "%s", src->nextRunTimestamp);
        snprintf(dest->lastChangeTimestamp, g_sizes.max_timestamp_size, "%s", src->lastChangeTimestamp);
        snprintf(dest->statusChanged, 2, "%s", src->statusChanged);
        dest->active = src->active;
        dest->interval = src->interval;
        dest->nextRun = src->nextRun;
    }
}

void plugin_output_init(PluginOutput *o) {
	if (!o) return;
    	o->retCode     = 0;
    	o->prevRetCode = 0;
    	o->retString   = NULL;
}

void plugin_output_destroy(PluginOutput *o) {
    	if (!o) return;
    	free(o->retString);
    	o->retString = NULL;
}

int plugin_output_set(PluginOutput *dest, const PluginOutput *src) {
	size_t len;
	char *dup;

    	if (!dest || !src) return EINVAL;

    	dest->retCode     = src->retCode;
    	dest->prevRetCode = src->prevRetCode;

    	plugin_output_destroy(dest);

    	if (!src->retString) {
        	return 0;
    	}

    	//dest->retString = strdup(src->retString);
    	/*if (!dest->retString) {
        	writeLog("[plugin_output_set] strdup failed", 1, 0);
        	return ENOMEM;
    	}*/
	len = strlen(src->retString);
	dup = malloc(len +1);
	if (!dup) {
		writeLog("[plugin_output_set] malloc failed", 1, 0);
        	return ENOMEM;
	}
	memcpy(dup, src->retString, len + 1);
	dest->retString = dup;

    	return 0;
}

void destroy_g_plugins(PluginItem *decls, size_t count) {
	if (!decls) return;
	for (size_t i = 0; i < count; i++) {
		free(decls[i].name);
		free(decls[i].description);
		free(decls[i].command);
	}
	free(decls);
	decls = NULL;
}

int redeclarePluginDeclarations(int mode, int count) {
	//int c;
	//int rows = 0;
	int check = 0;

	writeLog("Needs to redeclare g_pointers.g_plugins.", 0, 0);
	check = check_plugin_conf_file(g_files.pluginDeclarationFile);
	if (check > 0) {
		writeLog("Errors detected in plugin file. Can not reload.", 1, 0);
	       	return 2;	
	}
	else
		writeLog("Plugin conf file seems in good state. Will try to reload it now.", 0, 0);
	update_plugins();
	flushLog();

	return 0;
}

void checkRetVal(int val) {
	if (val > 1) {
		printf("Caught memory problem redeclaring plugin variables.\nQuiting...");
                writeLog("Memory allocation error redeclaring plugins.", 2, 0);
                writeLog("Check your configs if needed, then restart me.", 0, 0);
                flushLog();
                sig_handler(SIGSTOP);
        }
}

int hardReloadPlugins(int cnt) {
        int qsv = g_bools.quick_start;

        if (g_bools.quick_start != 1) g_bools.quick_start = 1;

        /* free previous plugin items and their per-item allocations */
        if (g_pointers.g_plugins != NULL) {
                for (size_t i = 0; i < cnt; ++i) {
                        if (g_pointers.g_plugins[i]) {
                                free(g_pointers.g_plugins[i]->name);
                                free(g_pointers.g_plugins[i]->description);
                                free(g_pointers.g_plugins[i]->command);
                                free(g_pointers.g_plugins[i]->output.retString);
                                free(g_pointers.g_plugins[i]);
                        }
                }
                free(g_pointers.g_plugins);
                g_pointers.g_plugins = NULL;
        }

        /* allocate array of pointers */
        g_ints.decCount = countDeclarations(g_files.pluginDeclarationFile);
        g_pointers.g_plugins = calloc((size_t)g_ints.decCount, sizeof(PluginItem *));
        if (!g_pointers.g_plugins) {
                perror("Error allocating memory for g_pointers.g_plugins array");
                writeLog("Error allocating memory [g_pointers.g_plugins array]", 2, 0);
                abort();
                return 2;
        }

        g_sizes.declaration_size = (size_t)g_ints.decCount;

        /* allocate each PluginItem and its per-item strings including embedded output */
        for (int i = 0; i < g_ints.decCount; ++i) {
                g_pointers.g_plugins[i] = calloc(1, sizeof(PluginItem));
                if (!g_pointers.g_plugins[i]) {
                        fprintf(stderr, "Error allocating PluginItem %d\n", i);
                        writeLog("Error allocating PluginItem", 2, 0);
                        for (int j = 0; j < i; ++j) {
                                free(g_pointers.g_plugins[j]->name);
                                free(g_pointers.g_plugins[j]->description);
                                free(g_pointers.g_plugins[j]->command);
                                free(g_pointers.g_plugins[j]->output.retString);
                                free(g_pointers.g_plugins[j]);
                        }
                        free(g_pointers.g_plugins);
                        g_pointers.g_plugins = NULL;
                        abort();
                        return 2;
                }
                g_pointers.g_plugins[i]->name = calloc(g_sizes.pluginitemname_size + 1, 1);
                g_pointers.g_plugins[i]->description = calloc(g_sizes.pluginitemdesc_size + 1, 1);
                g_pointers.g_plugins[i]->command = calloc(g_sizes.pluginitemcmd_size + 1, 1);
                g_pointers.g_plugins[i]->output.retString = calloc(g_sizes.pluginoutput_size + 1, 1);
		if (!g_pointers.g_plugins[i]->name || !g_pointers.g_plugins[i]->description || !g_pointers.g_plugins[i]->command || !g_pointers.g_plugins[i]->output.retString) {
                        fprintf(stderr, "Error allocating memory while redeclaring plugins (item %d).\n", i);
                        writeLog("Error allocating memory [g_pointers.update_g_plugins::items].", 2, 0);

                        for (int j = 0; j <= i; ++j) {
                                if (g_pointers.g_plugins[j]) {
                                        free(g_pointers.g_plugins[j]->name);
                                        free(g_pointers.g_plugins[j]->description);
                                        free(g_pointers.g_plugins[j]->command);
                                        free(g_pointers.g_plugins[j]->output.retString);
                                        free(g_pointers.g_plugins[j]);
                                }
                        }
                        free(g_pointers.g_plugins);
                        g_pointers.g_plugins = NULL;
                        abort();
                        return 2;
                }
        }

        /* reload declarations */
        if (check_plugin_conf_file(g_files.pluginDeclarationFile) != 0) {
                writeLog("plugins.conf file seems to be corrupt. Program will shut down.", 2, 0);
                return 2;
        }
        if (g_arrays.threadIds != NULL) {
                free(g_arrays.threadIds);
                g_arrays.threadIds = NULL;
        }
        g_arrays.threadIds = (unsigned short*)malloc((size_t)MAX_PLUGINS * sizeof(unsigned short));
        memset(g_arrays.threadIds, 0, MAX_PLUGINS * sizeof(unsigned short));
        for (int i = 0; i < g_ints.decCount; i++) {
                g_arrays.threadIds[i] = 0;
        }
        g_current_scheduler_cnt = g_ints.decCount;
        checkPluginFileStat(g_files.pluginDeclarationFile, g_time.tPluginFile, 0);
        writeLog("No errors found in plugins.conf", 0, 0);
        if (init_plugins() != 0) {
                logError("Failed to initiate plugins", 2, 0);
                flushLog();
                return 2;
        }
        flushLog();
        // Remove schededuler?
        initScheduler(cnt, 1000, true);
        g_bools.quick_start = qsv;
        return 0;
}

void apiReloadConfigHard() {
	if (check_plugin_conf_file(g_files.pluginDeclarationFile) != 0) {
		constructSocketMessage("reloadpluginshard", "failed");
        }
       	else {
		if (hardReloadPlugins(g_ints.decCount) == 0)
			constructSocketMessage("reloadpluginshard", "success");
		else {
			constructSocketMessage("reloadpluginshard", "fatal");
			sig_handler(SIGSTOP);
		}
	}
}

int checkNewConfig(const char *file_name) {
	FILE *file = NULL;
	char line[512];
        int count = 0;
	char identifier[256];
	char identifiers[150][256] = {0};
	int identifierCount = 0;
	int copies;
	//char c;
	int ch;	

        file = fopen(file_name, "r");
        if (file == NULL)
        {
                perror("Error while opening the file.[checkNewConfig]\n");
                writeLog("Error opening and counting g_pointers.g_plugins file.", 2, 0);
		return -1;
        }

	while (fgets(line, sizeof(line), file)) {
		if (sscanf(line,"[%[^]]]", identifier) == 1) {
			strncpy(identifiers[identifierCount], identifier, sizeof(line));
            		identifierCount++;
        	}
    	}
  	for (int i = 0; i < identifierCount; ++i) {
		copies = 0;
		for (int j = 0; j < identifierCount; j++) {
                    if (strcmp(identifiers[i], identifiers[j]) == 0)
                            copies++;
            	}
		if (copies > 1) {
			writeLog("There are duplicates in plugins.conf. Will abort reloading.", 1, 0);
			writeLog("The plugin file contains duplicates.", 2, 0);
			return -1;
		}
	}
	rewind(file);
	/*for (c = getc(file); c != EOF; c = getc(file)){
                if (c == '\n')
                        count++;
        }*/
	while ((ch = fgetc(file)) != EOF) {
                if (ch == '\n')
                        count++;
        }
        fclose(file);
	file = NULL;
        return count-1;
}

void initNewPlugin(int index) {
	//char currTime[80];
	/*char currTime[TIME_BUF_LEN];
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Initiating new plugin: %s\n", g_pointers.update_g_plugins[index].name);
	writeLog(trim(g_strings.infostr), 0, 0);
	printf("Initiating new plugin with id %d\n", index);
	if (g_pointers.update_g_plugins[index].active == 1) {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "%s is now active. Id %d\n", g_pointers.update_g_plugins[index].name, g_pointers.update_g_plugins[index].id-1);
		writeLog(trim(g_strings.infostr), 0, 0);
		update_outputs[index].prevRetCode = -1;
		//strcpy(g_pointers.update_g_plugins[index].statusChanged, "0");
		snprintf(g_pointers.update_g_plugins[index].statusChanged, 2, "%s", "0");
		//runPlugin(index, 1);
		PluginItem *item = g_pointers.g_plugins[index];
        	if (item && item->active) {
            		run_plugin(item);
        	}
		if (g_bools.timeScheduler)
			rescheduleChecks();
		size_t dest_size = 20;
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                int plen = snprintf(currTime, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		if (plen >= dest_size) {
			writeLog("Possible truncation of timestamp while init new plugin.", 1, 0);
		}
                strcpy(g_pointers.update_g_plugins[index].lastRunTimestamp, currTime);
                strcpy(g_pointers.update_g_plugins[index].lastChangeTimestamp, currTime);
                time_t nextTime = t + (g_pointers.update_g_plugins[index].interval *60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                int len = snprintf(g_pointers.update_g_plugins[index].nextRunTimestamp, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
		if (len >= dest_size) {
			writeLog("[initNewPlugin] possible truncation of timestamp.", 1, 0);
		}
                g_pointers.update_g_plugins[index].nextRun = nextTime;
		usleep(500);
	}
	else
        {
        	snprintf(g_strings.infostr, g_sizes.infostr_size, "%s is not active. Id: %d\n", g_pointers.update_g_plugins[index].name, g_pointers.update_g_plugins[index].id);
        	writeLog(trim(g_strings.infostr), 0, 0);
        }*/
        flushLog();
}

int initTimeScheduler(bool reinit) {
	if (g_ints.decCount == 0) {
		g_pointers.scheduler = NULL;
		printf("Could not initiate a time g_pointers.scheduler of count %d.\n", g_ints.decCount);
		return 1;
	}
	//g_pointers.scheduler = malloc((size_t)sizeof(Scheduler)*g_ints.decCount);
	g_pointers.scheduler = calloc(g_ints.decCount, sizeof(Scheduler));
	if (!g_pointers.scheduler) {
        	printf("Error allocating memory");
        	writeLog("Error allocating memory [initTimeScheduler]", 2, 0);
        	abort();
       		return 2;
        }
	if (reinit) {
		// Populate g_pointers.scheduler
                for (int i = 0; i < g_current_scheduler_cnt; i++) {
                        g_pointers.scheduler[i].id = g_pointers.g_plugins[i]->id;
                        g_pointers.scheduler[i].timestamp = g_pointers.g_plugins[i]->nextRun;
                }
        }
	return 0;
}

void initScheduler(int numOfP, int msSleep, bool reinit) {
	char currTime[TIME_BUF_LEN];
	time_t nextTime;
	float sleepTime = msSleep/1000;
	if (reinit)
                writeLog("Reinitate g_pointers.scheduler to run checks att given intervals", 0, 0);
        else
                logInfo("Initiating g_pointers.scheduler to run checks att given intervals.", 0, 0);
	if (g_bools.timeScheduler) {
		if (!reinit) {
                        logInfo("Initiating a time g_pointers.scheduler.", 0, 0);
                        initTimeScheduler(false);
                }
                else {
                        writeLog("Reinitate time g_pointers.scheduler.", 0, 0);
                        initTimeScheduler(true);
                }
	}
	flushLog();
	for (int i = 0; i < numOfP; i++)
	{
		if (g_pointers.g_plugins[i]->active == 1)
		{
			snprintf(g_strings.infostr, g_sizes.infostr_size, "%s is active. Id %d\n", g_pointers.g_plugins[i]->name, g_pointers.g_plugins[i]->id);
			writeLog(trim(g_strings.infostr), 0, 0);
			//outputs[i].prevRetCode = -1;
			g_pointers.g_plugins[i]->output.prevRetCode = -1;
			snprintf(g_pointers.g_plugins[i]->statusChanged, 2, "%s", "0");
                        
			PluginItem *item = g_pointers.g_plugins[i];
        		if (item) {
            			run_plugin(item);
        		}
			//runPlugin(i, 0);
			size_t dest_size = 20;
			time_t t = time(NULL);
  			struct tm tm = *localtime(&t);
			int len = snprintf(currTime, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			if (len >= dest_size) {
				writeLog("[InitScheduler] possible truncation of timestamp.", 1, 0);
			}
			strcpy(g_pointers.g_plugins[i]->lastRunTimestamp, currTime);
			strcpy(g_pointers.g_plugins[i]->lastChangeTimestamp, currTime);
			if (g_bools.quick_start) {
				int add_time = (int)sleepTime;
				int time_to_add = add_time * i+1;
				nextTime = t + (g_pointers.g_plugins[i]->interval * 60) + time_to_add;
			}
			else {
				nextTime = t + (g_pointers.g_plugins[i]->interval *60);
			}
			struct tm tNextTime;
			memset(&tNextTime, '\0', sizeof(struct tm));
			localtime_r(&nextTime, &tNextTime);
			len = snprintf(g_pointers.g_plugins[i]->nextRunTimestamp, dest_size, "%04d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			if (len >= dest_size) {
				writeLog("[Init g_pointers.scheduler] Possible truncation at nextTimeRuntimestamp", 1, 0);
			}
			g_pointers.g_plugins[i]->nextRun = nextTime;
			if (g_bools.timeScheduler) {
				g_pointers.scheduler[i].id = i;
				g_pointers.scheduler[i].timestamp = nextTime;
			}
			if (!g_bools.quick_start)
				sleep(sleepTime);
		}
		else
		{
			snprintf(g_strings.infostr, g_sizes.infostr_size, "%s is not active. Id: %d\n", g_pointers.g_plugins[i]->name, g_pointers.g_plugins[i]->id);
			writeLog(trim(g_strings.infostr), 0, 0);
			if (g_bools.timeScheduler) {
				g_pointers.scheduler[i].id = i;
				g_pointers.scheduler[i].timestamp = 0;
			}
		}
		flushLog();
	}
	if (!g_bools.standalone) {
		switch (g_sizes.output_type) {
			case JSON_OUTPUT:
				collectJsonData(numOfP);
				break;
			case METRICS_OUTPUT:
				collectMetrics(numOfP, 0);
				break;
			case JSON_AND_METRICS_OUTPUT:
		       		collectJsonData(numOfP);
		       		collectMetrics(numOfP, 0);
		       		break;
			case PROMETHEUS_OUTPUT:
		       		collectMetrics(numOfP, 1);
		       		break;
			case JSON_AND_PROMETHEUS_OUTPUT:
		      		collectJsonData(numOfP);
		     		collectMetrics(numOfP, 1);
				break;
	        	default:
				collectJsonData(numOfP);
		}
	}	
        g_time.tnextGardener = time(0) + g_sizes.gardenerInterval;	
	g_time.tnextClearDataCache = time(0) + g_sizes.clearDataCacheInterval;
	if (g_bools.local_api && !reinit) {
		if (g_bools.use_ssl)
			 SSL_library_init();
		if (g_sizes.socket_is_ready == 1) {
			writeLog("Socket is already happy.", 0, 0);
			return;
		}
		if (initSocket() == SOCKET_READY) {
			startApiSocket();
		}
		else {
			writeLog("Continue without local api.", 0, 0);
		}
	}
	if (g_bools.timeScheduler) {
		checkSchedulerCount();
		qsort(g_pointers.scheduler, g_ints.decCount, sizeof(struct Scheduler), compare_timestamps);
	}
	if (g_bools.runGardenerAtStart && !reinit) {
		writeLog("Running gardener cleanup job", 0, 0);
		runGardener();
	}
	if (reinit)
                writeLog("Scheduler reinitialized.", 0, 0);
        else
                logInfo("Scheduler initialized.", 0, 0);
    	flushLog();
}

void startPluginThread(int plugin_id) {
	int rc;
	pthread_t thread_id;
	intptr_t vpid = (intptr_t)plugin_id;

	vpid = plugin_id;

	rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)vpid);
	if(rc) {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Error: return code from phtread_create is %d\n", rc);
		writeLog(trim(g_strings.infostr), 2, 0);
	}
	else {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Created new thread (%lu) for plugin %s\n", thread_id, g_pointers.g_plugins[plugin_id]->name);
		writeLog(trim(g_strings.infostr), 0, 0);
		g_sizes.total_threads_run++;
		pthread_mutex_lock(&g_threading.mtx);
		g_sizes.thread_counter++;
		pthread_mutex_unlock(&g_threading.mtx);
		pthread_join(thread_id, NULL);
        }
}

void runPluginThreads(int loopVal){
	char currTime[TIME_BUF_LEN];
	pthread_t thread_id;
        int rc;
        int i;
	time_t t = time(NULL);
        struct tm tm = *localtime(&t);

	snprintf(currTime, sizeof(currTime), "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	/*if (g_bools.timeScheduler == 1) {
		i = 1;
		struct Scheduler do_run = g_pointers.scheduler[0];
		while(i > 0) {
			if ((t >= do_run.timestamp) && (g_pointers.g_plugins[do_run.id]->active == 1)) {
				//printf("DEBUG: startPluginThread id %d\n", do_run.id);
				startPluginThread(do_run.id);
				g_ints.tspr++;
			}
			if (do_run.timestamp > t) {
				//printf("Exit..\n");
				break;
			}
			do_run = g_pointers.scheduler[0];
		}
		return;
	}*/
	if (g_bools.timeScheduler) {
                time_t t = time(NULL);
                int currentId = -1;
                time_t currentTimestamp = 0;

                while (g_pointers.scheduler[0].timestamp <= t) {
                        struct Scheduler do_run = g_pointers.scheduler[0];

                        // Prevent infinite loop on same plugin and timestamp
                        if ((currentId == do_run.id) && (currentTimestamp == do_run.timestamp)) {
                                //printf("Loop protection triggered for id %d. Sleeping...\n", do_run.id);
				snprintf(g_strings.infostr, g_sizes.infostr_size, "Loop protextion triggered for id %d. Sleeping...\n", do_run.id);
				writeLog(trim(g_strings.infostr), 0, 0);
                                sleep(1);
                                break;
                        }

                        if (g_pointers.g_plugins[do_run.id]->active == 1) {
                                //printf("DEBUG: startPluginThread id %d\n", do_run.id);
                                startPluginThread(do_run.id);
                                g_ints.tspr++;
                                currentId = do_run.id;
                                currentTimestamp = do_run.timestamp;
                                //printf("After reschedule: g_pointers.scheduler[0].id = %d, timestamp = %ld\n", g_pointers.scheduler[0].id, g_pointers.scheduler[0].timestamp);
                        }
                        // Loop continues as long as g_pointers.scheduler[0].timestamp <= t
                }
                return;
        }

        for (i = 0; i < loopVal; i++) {
           long j = i;
	   if (g_pointers.g_plugins[i]->active == 1) {
		if (t > g_pointers.g_plugins[i]->nextRun)
		{
			rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)j);
           		if(rc) {
                		snprintf(g_strings.infostr, g_sizes.infostr_size, "Error: return code from phtread_create is %d\n", rc);
				writeLog(trim(g_strings.infostr), 2, 0);
           		}
           		else {
                   		snprintf(g_strings.infostr, g_sizes.infostr_size, "Created new thread (%lu) for plugin %s\n", thread_id, g_pointers.g_plugins[i]->name);
				writeLog(trim(g_strings.infostr), 0, 0);
				g_sizes.total_threads_run++;
				pthread_mutex_lock(&g_threading.mtx);
				g_sizes.thread_counter++;
				pthread_mutex_unlock(&g_threading.mtx);
				//pthread_join(thread_id, NULL);
           		}
		}
            }
	}
        //pthread_exit(NULL);
}

void executeGardener() {
	pthread_t thread_id;
	int rc;

	rc = pthread_create(&thread_id, NULL, gardenerExeThread, "gardener 1");
	if(rc) {
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Error: return code from phtread_create is %d\n", rc);
               	writeLog(trim(g_strings.infostr), 2, 0);
		return;
        }
	//pthread_setname_np(thread_id, "Gardener worker");
	pthread_setspecific(thread_id, "Gardener worker");
	snprintf(g_strings.infostr, g_sizes.infostr_size, "Created new thread (%lu) truncating metrics logs (gardener) \n", thread_id);
        writeLog(trim(g_strings.infostr), 0, 0);
	pthread_mutex_lock(&g_threading.mtx);
	g_sizes.thread_counter++;
	pthread_mutex_unlock(&g_threading.mtx);
}

void clearDataCache() {
	pthread_t thread_id;
	int rc;

	rc = pthread_create(&thread_id, NULL, clearDataCacheThread, "clearDataCache 1");
	      if(rc) {
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Error: return code from phtread_create is %d\n", rc);
                writeLog(trim(g_strings.infostr), 2, 0);
        }
        else {
		//pthread_setname_np(thread_id, "DataClearCache");
		pthread_setspecific(thread_id, "DataClearCache");
                snprintf(g_strings.infostr, g_sizes.infostr_size, "Created new thread (%lu) clearing old data files (clearDataCache) \n", thread_id);
                writeLog(trim(g_strings.infostr), 0, 0);
		pthread_mutex_lock(&g_threading.mtx);
		g_sizes.thread_counter++;
		pthread_mutex_unlock(&g_threading.mtx);
		pthread_join(thread_id, NULL);
       }
}

void apiReloadConfigSoft() {
	if (check_plugin_conf_file(g_files.pluginDeclarationFile) != 0) {
                constructSocketMessage("softreloadplugins", "failed");
        }
        else {
                //updatePluginDeclarations();
		update_plugins();
                constructSocketMessage("softreloadplugins", "success");
        }
}

void scheduleChecks(){
	float sleepTime = g_ints.schedulerSleep/1000;
	int i = 1;
	int repeate_write = 0;

	logInfo("Almond started succesfully. Ready to schedule checks.", 0, 0);
	if (g_bools.timeScheduler) {
		writeLog("Start time based g_pointers.scheduler...", 0, 0);
	}
	else {
		writeLog("Start classic g_pointers.scheduler timer...", 0, 0);
		snprintf(g_strings.infostr, g_sizes.infostr_size, "Sleep time is: %.3f\n", sleepTime);
		writeLog(trim(g_strings.infostr), 0, 0);
	}
	flushLog();
	// Timer is an eternal loop :P
	while (i > 0) {
		if (g_threading.is_stopping != 0) i--;
		if (!g_bools.timeScheduler)
			writeLog("Check for command files.", 0, 0);
		else {
			if (repeate_write == 0) {
				writeLog("Check for command files.", 0, 0);
				repeate_write++;
			}
		}
		checkApiCmds();
		if (!g_bools.external_scheduler) {
			runPluginThreads(g_ints.decCount);
		}
		if (!g_bools.timeScheduler) {
			snprintf(g_strings.infostr, g_sizes.infostr_size, "Sleeping for %.3f seconds.\n", sleepTime);
                	writeLog(trim(g_strings.infostr), 0, 0);
			sleep(sleepTime);
		}
		else {
			checkSchedulerCount();
			qsort(g_pointers.scheduler, g_ints.decCount, sizeof(struct Scheduler), compare_timestamps);
			//writeLog("VERBOSE: Scheduler sorted. Sleeping for a second.", 0, 0);
			sleep(1);
		}
		if (!g_bools.timeScheduler || g_ints.tspr > 0) {
			g_ints.tspr = 0;
			repeate_write = 0;
			switch (g_sizes.output_type) {
                		case JSON_OUTPUT:
                        		collectJsonData(g_ints.decCount);
                        		break;
                		case METRICS_OUTPUT:
                        		collectMetrics(g_ints.decCount, 0);
                        		break;
                		case JSON_AND_METRICS_OUTPUT:
                       			collectJsonData(g_ints.decCount);
					collectMetrics(g_ints.decCount, 0);
                       			break;
				case PROMETHEUS_OUTPUT:
					collectMetrics(g_ints.decCount, 1);
					break;
				case JSON_AND_PROMETHEUS_OUTPUT:
					collectJsonData(g_ints.decCount);
                                	collectMetrics(g_ints.decCount, 1);
					break;
                		default:
                        		collectJsonData(g_ints.decCount);
        		}
		}
		// Set this to timestamp
		if (checkPluginFileStat(g_files.pluginDeclarationFile, g_time.tPluginFile, 0)) {
			writeLog("Detected change of plugins file.", 0, 0);
			flushLog();
			//updatePluginDeclarations();
                        update_plugins();
                        printf("Plugins updated. Total live plugins: %d\n", g_ints.g_plugin_count);
		}
		// Time to execute gardener?
		if (g_bools.enableGardener) {
			time_t seconds = time(0);
			if (seconds > g_time.tnextGardener) {
				sleep(10);
				executeGardener();
				g_time.tnextGardener = seconds + g_sizes.gardenerInterval;
				sleep(1);
			}

		}
		if (g_bools.enableClearDataCache) {
			time_t seconds = time(0);
			if (seconds > g_time.tnextClearDataCache) {
                                writeLog("ClearDataCash is ready", 0, 0);
				clearDataCache();
				g_time.tnextClearDataCache = seconds + g_sizes.clearDataCacheInterval;
				sleep(5);
			}
		}
		if (g_bools.use_push || g_bools.use_metrics_push) {
			g_ints.push_interval_cnt++;
			int sleep_push_interval = g_ints.push_interval;
			if (!g_bools.timeScheduler)
				sleep_push_interval = g_ints.push_interval / sleepTime;
			if (g_ints.push_interval_cnt >= sleep_push_interval) {
				char url[1024];
				// Path added if future version would like such an extra param
				const char *path = "/receive";
				build_push_url(url, sizeof(url), g_strings.push_url, g_ints.push_port, path);
				if (url[0] == '\0') {
        				fprintf(stderr, "Failed to build URL\n");
					writeLog("Failed to build push url.", 1, 0);
    				}
				else {
					if (g_bools.use_push) {
						char json_path[g_sizes.filename_size];
						snprintf(g_strings.infostr, g_sizes.infostr_size, "Pushing data to url '%s'.", url);
                                        	writeLog(trim(g_strings.infostr), 0, 0);
                                        	int written = snprintf(json_path, g_sizes.filename_size, "%s/%s", g_dirs.dataDir, g_files.jsonFileName);
                                        	if (written < 0) {
                                                	writeLog("Could not write to push json file", 2, 0);
                                        	}
                                        	if ((size_t)written >= g_sizes.filename_size) {
                                                	writeLog("Push file name truncated. Name is too long.", 1, 0);
                                        	}
						if (post_json_file_stream(url, json_path) == 0) {
							writeLog("Data pushed successfully.", 0, 0);
    						} 
						else {
							writeLog("Failed to push data.", 2, 0);
    						}
						flushLog();
					}
					if (g_bools.use_metrics_push) {
						char metrics_path[g_sizes.storename_size];
    						snprintf(g_strings.infostr, g_sizes.infostr_size, "Pushing metrics to url '%s'.", url);
                                                writeLog(trim(g_strings.infostr), 0, 0);
						int written = snprintf(metrics_path, g_sizes.storename_size, "%s/%s", g_dirs.storeDir, g_files.metricsFileName);
						if (written < 0) {
							writeLog("Could not write to push metrics file", 2, 0);
						}
						if ((size_t)written >= g_sizes.filename_size) {
							writeLog("Push metrics file name truncated. Name is too long.", 1, 0);
						}
						if (post_metrics_file_stream(url, metrics_path) == 0) {
							writeLog("Merics pushed successfully.", 0, 0);
						}
						else {
							writeLog("Failed to push metrics.", 2, 0);
						}
						flushLog(); 
					}
				}
				g_ints.push_interval_cnt = 0;
			}
		}
		else	
			flushLog();
		if (g_bools.truncateLog) {
			if (g_ints.trunc_time == 0) {
				check_file_truncation();
			}
			// Check truncation only every 10th cycle
			g_ints.trunc_time++;
			if (g_ints.trunc_time >= 10) {
				g_ints.trunc_time = 0;
			}
		}
		else {
			//printf("TruncateLog not active.\n");
		}
		if (g_sizes.total_threads_run >= MAX_THREAD_COUNT) {
			writeLog("You are reaching the limit of max_thread_counter. It will be reset to 1.", 1, 0);
			writeLog("Reaching MAX_THREAD_COUNT is an indication the service has been alive too long without restart.", 0, 0);
			g_sizes.total_threads_run = 1;
			flushLog();
		}
	}
}

int isConstantsEnabled () {
	FILE *file = NULL;
	char line[10];
	char *searchString = "enable";

	file = fopen("/etc/almond/memalloc.conf", "r");
	if (file == NULL) {
		printf("No g_arrays.constants file will be used.\n");
		writeLog("No memalloc.conf file was found.", 1, 1);
		return 0;
	}
	while (fgets(line, sizeof(line), file)) {
		if (strstr(line, searchString)) {
			writeLog("Constants file is enabled.", 0, 1);
			fclose(file);
			return 1;
			break;
		}
	}
	fclose(file);
	return 0;
}

void initLogMessages() {
	for (int i = 0; i < 5; i++) {
		g_arrays.logmessage_id[i] = 0;
	}
}

void initialLogging() {
	char lfin[28] = "/var/log/almond/almond.log";

        g_pointers.fptr = fopen(lfin, "a");
	if (!g_pointers.fptr) {
        	perror("Failed to open log file");
        	exit(EXIT_FAILURE);
    	}
        fprintf(g_pointers.fptr, "\n");
        printf("Starting almond version %s.\n", VERSION);
        initConstants();
        writeLog("Almond g_arrays.constants initialized.", 0, 1);
        writeLog("Starting almond (0.9.28)...", 0, 1);
}

int closeFileHandler() {
	fclose(g_pointers.fptr);
	g_pointers.fptr = NULL;
	return EXIT_FAILURE;
}

void setupSignalHandlers() {
	struct sigaction sa;

    	memset(&sa, 0, sizeof(sa));
    	sa.sa_handler = sig_handler;
    	if (sigaction(SIGINT, &sa, NULL) == -1) {
        	logError("Failed to set SIGINT handler", 2, 1);
		printf("Failed to set SIGTERM handler: %s", strerror(errno));
		closeFileHandler();
        	return;
    	}

   	memset(&sa, 0, sizeof(sa));
    	sa.sa_handler = sig_handler;
    	if (sigaction(SIGTERM, &sa, NULL) == -1) {
        	logError("Failed to set SIGTERM handler", 2, 1);
		printf("Failed to set SIGTERM handler: %s", strerror(errno));
		closeFileHandler();
        	return;
    	}
}

int loadConfiguration() {
	int retVal = getConfigurationValues();
        if (retVal == 0) {
                logInfo("Configuration read ok.", 0, 1);
		if (g_bools.useKafkaConfigFile) {
			if (g_strings.kafkaConfigFile != NULL) {
				if (fileExists(g_strings.kafkaConfigFile) == 0) {
					snprintf(g_strings.infostr, g_sizes.infostr_size, "Setting Kafka config file to: %s.", g_strings.kafkaConfigFile);
					logInfo(trim(g_strings.infostr), 0, 1);
					setKafkaConfigFile(g_strings.kafkaConfigFile);
				}
				else {
					snprintf(g_strings.infostr, g_sizes.infostr_size, "File does not exist: %s", g_strings.kafkaConfigFile);
					logInfo(trim(g_strings.infostr), 2, 1);
					logInfo("Kafka will use default config file: /etc/almond/kafka.conf", 0, 1);
				}
			}
			if (loadKafkaConfig() == 0) {
				logInfo("Kafka configuration read ok.", 0, 1);
				if (init_kafka_producer() != 0) {
					logInfo("Error initiating Kafka producer.", 2, 1);
					return 1;
				}
				else {
					logInfo("Kafka producer initiated.", 0, 1);
				}
			}
		}
        }
        else {
                logError("Could not load configuration, due to corruption or memory allocation failure.", 1, 1);
                return 1;
        }
	return 0;
}

void initLoggerThread() {
	fclose(g_pointers.fptr);
        g_pointers.fptr = NULL;
        printf("Initiate logger\n");
        initLogger();
        logInfo("Initiate plugins.", 0, 0);
        fflush(g_pointers.fptr);
}

int loadPlugins() {
	g_ints.decCount = countDeclarations(g_files.pluginDeclarationFile);
        //g_arrays.threadIds = (unsigned short*)malloc((size_t)g_ints.decCount * sizeof(unsigned short));
        for (int i = 0; i < g_ints.decCount; i++) {
                g_arrays.threadIds[i] = 0;
        }
        /*g_pointers.g_plugins = (PluginItem *)malloc((size_t)sizeof(PluginItem) * g_ints.decCount);
        g_sizes.declaration_size = (size_t)g_ints.decCount;
        if (!g_pointers.g_plugins) {
                perror ("Error allocating memory");
                writeLog("Error allocating memory - PluginItem.", 2, 0);
                abort();
        }
        printf("Declarations initiated.\n");
        for (int i = 0; i < g_ints.decCount; i++) {
                g_pointers.g_plugins[i].name = malloc((size_t)g_sizes.pluginitemname_size + 1);
                if (g_pointers.g_plugins[i].name == NULL) {
                        logError("Failed to allocate g_pointers.g_plugins.", 2, 0);
                        exit(2);
                }
                else
                        g_pointers.g_plugins[i].name[0] = '\0';
                g_pointers.g_plugins[i].description = malloc((size_t)g_sizes.pluginitemdesc_size + 1);
                if (g_pointers.g_plugins[i].description == NULL){
                        logError("Failed to allocate g_pointers.g_plugins.", 2, 0);
                        exit(2);
                }
                else
                        g_pointers.g_plugins[i].description[0] = '\0';
                g_pointers.g_plugins[i].command = malloc((size_t)g_sizes.pluginitemcmd_size + 1);
                if (g_pointers.g_plugins[i].command == NULL) {
                        logError("Failed to allocate g_pointers.g_plugins.", 2, 0);
                        exit(2);
                }
                else
                        g_pointers.g_plugins[i].command[0] = '\0';
        }*/
	//init_plugins(g_files.pluginDeclarationFile, &g_sizes.declaration_size);
	init_plugins();
        logInfo("Declarations read.", 0, 0);
        /*outputs = malloc((size_t)sizeof(PluginOutput)*g_ints.decCount);
        if (!outputs){
                perror("Error allocating memory");
                writeLog("Error allocating memory - PluginOutput.", 2, 0);
                abort();
        }*/
        for (size_t i = 0; i < g_ints.decCount; ++i) {
    		//plugin_output_init(&outputs[i]);
	}
        /*for (int i = 0; i < g_ints.decCount; i++) {
                outputs[i].retString = malloc((size_t)g_sizes.pluginoutput_size);
                if (outputs[i].retString == NULL) {
                        logError("Failed to allocate outputs.", 2, 0);
                        exit(2);
                }
                else
                        outputs[i].retString[0] = '\0';
        }*/
        g_sizes.output_size = (size_t)g_ints.decCount;
        //int pluginDeclarationResult = loadPluginDeclarations(g_files.pluginDeclarationFile, 0);
	// This should be deprecated
	int pluginDeclarationResult = 9;
        time_t dummy = time(NULL);
        checkPluginFileStat(g_files.pluginDeclarationFile, dummy, 1);
        if (pluginDeclarationResult <= 0){
                logInfo("Problem reading from plugin declaration file.", 1, 0);
        }
        else {
                logInfo("Plugin g_pointers.g_plugins file loaded.", 0, 0);
        }
	//printf("DEBUG: pluginDeclarationResult = %d\n", pluginDeclarationResult);
	return 0;
}

void apiReload() {
	// Reinitiate all Almond vars, copy needed if failed?
	if (loadConfiguration() != 0) {
		constructSocketMessage("almond_reload", "failed");
	}
	else {
		constructSocketMessage("almond_reload", "true");
	}
}

void* zombieReaper(void* arg) {
	sigset_t set;
    	sigemptyset(&set);
    	sigaddset(&set, SIGCHLD);

    	int sig;

	while(!g_threading.is_stopping) {
		if (sigwait(&set, &sig) == 0 && sig == SIGCHLD) {
			pid_t pid;
			int status;
			while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
				if (is_plugin_pid(pid)) {
					continue;
				}
				snprintf(g_strings.infostr, g_sizes.infostr_size, "Reaper thread cleaned up orphan PID %d", pid);
				writeLog(trim(g_strings.infostr), 0, 0);
			}
		}
	}
	writeLog("Reaper thread exiting gracefully", 0, 0);
	return NULL;
}

static int init_system(void) {
	install_signals();
        initialLogging();
        int configResult = loadConfiguration();
        if (configResult != 0) {
                logError("Failed to load configuration", 1, 1);
                return 1;
        }
        else
                printf("Configuration read.\n");

        if (strcmp(g_strings.hostName, "None") == 0) {
                char *tempHost = getHostName();
                snprintf(g_strings.hostName, 255, "%s", tempHost);
                free(tempHost);
        }
        writeLog("Initiate logger thread.", 0, 1);
        initLoggerThread();
        if (check_plugin_conf_file(g_files.pluginDeclarationFile) != 0) {
                logError("plugins.conf file seems to be corrupt. Program will shut down.", 2, 0);
                return 2;
        }
        g_arrays.threadIds = (unsigned short*)malloc((size_t)MAX_PLUGINS * sizeof(unsigned short));
        memset(g_arrays.threadIds, 0, MAX_PLUGINS * sizeof(unsigned short));
        checkPluginFileStat(g_files.pluginDeclarationFile, g_time.tPluginFile, 0);
        logInfo("No errors found in plugins.conf", 0, 0);
        g_ints.decCount = countDeclarations(g_files.pluginDeclarationFile);
        for (int i = 0; i < g_ints.decCount; i++) {
                g_arrays.threadIds[i] = 0;
        }
        g_current_scheduler_cnt = g_ints.decCount;
        if (init_plugins() != 0) {
                logError("Failed to initiate plugins", 2, 0);
                flushLog();
                return 2;
        }
        flushLog();
        initScheduler(g_ints.decCount, g_ints.initSleep, false);
        return 0;
}

static void run_check_loop(void) {
    while (!g_threading.is_stopping) {
        scheduleChecks();
    }
}

static void shutdown_system(void) {
	switch (g_threading.shutdown_reason) {
                case SR_SIGINT:
                        writeLog("Caught SIGINT, exiting program.", 0, 0);
                        break;
                case SR_SIGKILL:
                        writeLog("Caught SIGKILL, exiting program.", 0, 0);
                        break;
                default:
                        writeLog("Normal program termination.", 0, 0);
                        break;
        }
        sig_exit_app();
	if (g_arrays.threadIds) {
        	free(g_arrays.threadIds);
        	g_arrays.threadIds = NULL;
    	}

    	flushLog();
}

int main(int argc, char* argv[]) {
	#if defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
		#define HAS_BIRTHTIME 1
	#else
		#define HAS_BIRTHTIME 0
	#endif
        int rc = init_system();
	if (rc != 0) {
		return rc;
    	}

	run_check_loop();
	shutdown_system();

   	return 0;
}
