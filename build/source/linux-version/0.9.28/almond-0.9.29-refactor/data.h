#ifndef ALMOND_DATA_STRUCTURES_HEADER
#define ALMOND_DATA_STRUCTURES_HEADER

#define TIMESTAMP_SIZE 64
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "uthash.h"    // or your favorite C hash library

#ifndef MAX_HOSTS
#define MAX_HOSTS 256
#endif

typedef struct PluginOutput {
        int retCode;
        int prevRetCode;
        char* retString;
} PluginOutput;

typedef struct Scheduler {
        int id;
        time_t timestamp;
} Scheduler;

typedef struct TrackedPopen {
        FILE *fp;
        pid_t pid;
} TrackedPopen;

typedef struct PluginItem {
    char *name;
    char *description;
    char *command;                
    int active;
    int interval;
    int id;
    PluginOutput output;
    char lastRunTimestamp[TIMESTAMP_SIZE];
    char nextRunTimestamp[TIMESTAMP_SIZE];
    char lastChangeTimestamp[TIMESTAMP_SIZE];
    char statusChanged[2];
    time_t nextRun;
    bool touched;
    UT_hash_handle hh;
} PluginItem;

typedef struct GlobalDirectories {
    char* confDir;
    char* dataDir;
    char* storeDir;
    char* pluginDir;
    char* logDir;
    char* backupDirectory;
} GlobalDirectories;

typedef struct GlobalFiles {
    char* fileName;
    char* jsonFileName;
    char* metricsFileName;
    char* pluginDeclarationFile;
    char* gardenerScript;
    char* dataFileName;
    char* newFileName;
    char* storeName;
    char* logfile;
} GlobalFiles;

typedef struct GlobalStrings {
    char* hostName;
    char* metricsOutputPrefix;
    char* infostr;
    char* socket_message;
    char* server_message;
    char* almondCertificate;
    char* almondKey;
    char* schemaRegistryUrl;
    char* kafkaConfigFile;
    char* customMonitorVals;
    char *iam_public_key;
    char *iam_public_key_file;
    char *iam_issuer;
    char *iam_aud;
    char* logmessage;
    char* gardenerRetString;
    char* pluginCommand;
    char* pluginReturnString;
    char* push_url;
    char* kafka_brokers;
    char* kafka_topic;
    char* kafka_tag;
    char* kafkaCACertificate;
    char* kafkaSSLKey;
    char* kafkaProducerCertificate;
    char* api_args;
} GlobalStrings;

typedef struct GlobalBooleans {
    bool confDirSet;
    bool dataDirSet;
    bool storeDirSet;
    bool logDirSet;
    bool pluginDirSet;
    bool logPluginOutput;
    bool pluginResultToFile;
    bool saveOnExit;
    bool dockerLog;
    bool enableGardener;
    bool runGardenerAtStart;
    bool enableClearDataCache;
    bool enableIamAud;
    bool enableIamRoles;
    bool enableKafkaExport;
    bool enableKafkaSSL;
    bool enableKafkaTag;
    bool enableKafkaId;
    bool kafkaAvro;
    bool enableTimeTuner;
    bool standalone;
    bool quick_start;
    bool local_api;
    bool use_push;
    bool use_metrics_push;
    bool external_scheduler;
    bool useKafkaConfigFile;
    bool use_ssl;
    bool truncateLog;
    bool timeScheduler;
    bool allowAllHosts;
} GlobalBooleans;

typedef struct GlobalIntegers {
    int initSleep;
    int updateInterval;
    int push_interval;
    int push_interval_cnt;
    int hosts_allowed_count;
    int iam_roles_count;
    int decCount;
    int kafkaexportreqs;
    int schedulerSleep;
    int timeTunerMaster;
    int timeTunerCycle;
    int timeTunerCounter;
    int local_port;
    int tspr;
    int config_memalloc_fails;
    int trunc_time;
    int max_try;
    int push_port;
    int g_plugin_count;
    int g_current_scheduler_cnt;
    int is_file_open;
    int api_action;
    int args_set;
    int logrecord;
    int shutdown_phase;
} GlobalIntegers;

typedef struct GlobalSizes {
    size_t infostr_size;
    size_t gardenermessage_size;
    size_t pluginmessage_size;
    size_t storename_size;
    size_t apimessage_size;
    size_t socketservermessage_size;
    size_t socketclientmessage_size;
    size_t logmessage_size;
    size_t confdir_size;
    size_t datadir_size;
    size_t plugindeclarationfile_size;
    size_t metricsoutputprefix_size;
    size_t datafilename_size;
    size_t jsonfilename_size;
    size_t metricsfilename_size;
    size_t gardenerscript_size;
    size_t logdir_size;
    size_t hostname_size;
    size_t plugindir_size;
    size_t pluginitemname_size;
    size_t pluginitemdesc_size;
    size_t pluginitemcmd_size;
    size_t pluginoutput_size;
    size_t plugincommand_size;
    size_t newfilename_size;
    size_t storedir_size;
    size_t backupdirectory_size;
    size_t filename_size;
    size_t logfile_size;
    size_t max_timestamp_size;
    size_t declaration_size;
    size_t output_size;
    size_t update_output_size;
    size_t update_declaration_size;
    signed int truncateLogInterval;
    unsigned int socket_is_ready;
    unsigned int gardenerInterval;
    unsigned int clearDataCacheInterval;
    unsigned int dataCacheTimeFrame;
    unsigned int kafka_start_id;
    unsigned int total_threads_run;
    unsigned int thread_counter;
    unsigned char output_type;
} GlobalSizes;

typedef struct GlobalArrays {
    char constants[50][50];
    int values[50];
    unsigned short *threadIds;
    int logmessage_id[5];
    pid_t plugin_pid_set[256];
    char **iam_roles_accepted;
    char *hosts_allowed[MAX_HOSTS];
} GlobalArrays;

typedef struct GlobalTime {
    time_t tLastUpdate;
    time_t tnextUpdate;
    time_t tnextGardener;
    time_t tnextClearDataCache;
    time_t tPluginFile;
} GlobalTime;

typedef struct GlobalNetwork {
    SSL_CTX *ctx;
    SSL *ssl;
    struct sockaddr_in address;
    int server_fd;
} GlobalNetwork;

typedef struct GlobalThreading {
    volatile sig_atomic_t is_stopping;
    volatile sig_atomic_t shutdown_reason;
    volatile sig_atomic_t already_exiting;
    pthread_mutex_t plugin_set_mtx;
    pthread_cond_t file_opened;
    pthread_mutex_t mtx;
    pthread_mutex_t update_mtx;
    pthread_mutex_t hostname_mutex;
} GlobalThreading;

typedef struct GlobalPointers {
    PluginItem **g_plugins;
    PluginItem *g_plugin_map;
    PluginItem *update_g_plugins;
    Scheduler *scheduler;
    FILE *fptr;
} GlobalPointers;

#endif // ALMOND_DATA_STRUCTURES_HEADER
