#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <json-c/json.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include "structures.h"
#include "mod_kafka.h"

#define MAX_COLUMNS 2
#define MAX_STRING_SIZE 50
#define MAX_CONSTANTS 50
#define JSON_OUTPUT 0
#define METRICS_OUTPUT 1
#define JSON_AND_METRICS_OUTPUT 2
#define PROMETHEUS_OUTPUT 3
#define JSON_AND_PROMETHEUS_OUTPUT 4
#define HOWRU_API 10
#define ALMOND_API_PORT 9165
#define SOCKET_READY 1
#define NO_SOCKET -1
#define API_READ 10
#define API_RUN 15
#define API_EXECUTE_AND_READ 25
#define API_GET_METRICS 30
#define API_READ_ALL 100
#define API_FLAGS_VERBOSE 1
#define API_DRY_RUN 6
#define API_EXECUTE_GARDENER 17
#define API_NAME_START 50
#define API_ENABLE_TIMETUNER 50
#define API_DISABLE_TIMETUNER 51
#define API_ENABLE_GARDENER 52
#define API_DISABLE_GARDENER 53
#define API_ENABLE_CLEARCACHE 54
#define API_DISABLE_CLEARCACHE 55
#define API_ENABLE_QUICKSTART 56
#define API_DISABLE_QUICKSTART 57
#define API_ENABLE_STANDALONE 58
#define API_DISABLE_STANDALONE 59
#define API_SET_PLUGINOUTPUT 70
#define API_SET_SAVEONEXIT 71
#define API_SET_KAFKATAG 72
#define API_SET_SLEEP 73
#define API_SET_KAFKA_START_ID 74
#define API_SET_HOSTNAME 75
#define API_SET_METRICSPREFIX 76
#define API_SET_JSONFILENAME 77
#define API_SET_METRICSFILENAME 78
#define API_SET_KAFKATOPIC 79
#define API_GET_PLUGINOUTPUT 81
#define API_GET_KAFKATAG 82
#define API_GET_SLEEP 83
#define API_GET_SAVEONEXIT 84
#define API_GET_HOSTNAME 85
#define API_GET_METRICSPREFIX 86
#define API_GET_JSONFILENAME 87
#define API_GET_METRICSFILENAME 88
#define API_GET_KAFKATOPIC 89
#define API_GET_KAFKA_START_ID 90
#define API_GET_PLUGIN_RELOAD_TS 91
#define API_CHECK_PLUGIN_CONFIG 92
#define API_RELOAD_ALMOND 93
#define API_RELOAD_CONFIG_HARD 94
#define API_RELOAD_CONFIG_SOFT 95
#define API_NAME_END 96
#define API_DENIED 66
#define KAFKA_EXPORT_TAG 10
#define KAFKA_EXPORT_ID 20
#define KAFKA_EXPORT_IDTAG 30
#define VERSION "0.9.8"

char constantsFile[26] = "/opt/almond/memalloc.alm";
char* confDir = NULL;
char* dataDir = NULL;
char* storeDir = NULL;
char* pluginDir = NULL;
char* logDir = NULL;
char* pluginDeclarationFile = NULL;
char* hostName = NULL;
char* fileName = NULL;
char* jsonFileName = NULL;
char* metricsFileName = NULL;
char* gardenerScript = NULL;
char* metricsOutputPrefix = NULL;
char* infostr = NULL;
char* socket_message = NULL;
char* kafka_brokers = NULL;
char* kafka_topic = NULL;
char* kafka_tag = NULL;
char* kafkaCACertificate = NULL;
char* kafkaSSLKey = NULL;
char* kafkaProducerCertificate = NULL;
char* logmessage = NULL;
char* logfile = NULL;
char* dataFileName = NULL;
char* backupDirectory = NULL;
char* newFileName = NULL;
char* gardenerRetString = NULL;
char* pluginCommand = NULL;
char* pluginReturnString = NULL;
char* storeName = NULL;
char* server_message = NULL;
char* client_message = NULL;
char* almondCertificate = NULL;
char* almondKey = NULL;
//char* apiMessage;
PluginItem *declarations = NULL;
PluginOutput *outputs = NULL;
PluginItem *update_declarations = NULL;
PluginOutput *update_outputs = NULL;
Scheduler *scheduler = NULL;
struct sockaddr_in address;
SSL_CTX *ctx;
SSL *ssl;
int initSleep;
int updateInterval;
int schedulerSleep = 5000;
int confDirSet = 0;
int dataDirSet = 0;
int storeDirSet = 0;
int logDirSet = 0;
int pluginDirSet = 0;
int logPluginOutput = 0;
int pluginResultToFile = 0;
int decCount = 0;
int saveOnExit = 0;
int dockerLog = 0;
int enableGardener = 0;
int enableClearDataCache = 0;
int kafkaexportreqs = 0;
int enableKafkaExport = 0;
int enableKafkaSSL = 0;
int enableKafkaTag = 0;
int enableKafkaId = 0;
int enableTimeTuner = 0;
int timeTunerMaster = 1;
int timeTunerCycle = 15;
int timeTunerCounter = 0;
int local_port = 9909;
int local_api = 0;
int standalone = 0;
int quick_start = 0;
int use_ssl = 0;
int timeScheduler = 0;
int tspr = 0;
size_t infostr_size = 400;
size_t gardenermessage_size = 1035;
size_t pluginmessage_size = 2300;
size_t storename_size = 100;
size_t apimessage_size = 2000;
size_t socketservermessage_size = 2000;
size_t socketclientmessage_size = 2000;
size_t logmessage_size = 1545;
int server_fd;
size_t confdir_size = 50;
size_t datadir_size = 50;
size_t plugindeclarationfile_size = 75;
size_t metricsoutputprefix_size = 30;
size_t datafilename_size = 100;
size_t jsonfilename_size = 50;
size_t metricsfilename_size = 50;
size_t gardenerscript_size = 75;
size_t logdir_size = 50;
size_t hostname_size = 255;
size_t plugindir_size = 50;
size_t pluginitemname_size = 50;
size_t pluginitemdesc_size = 100;
size_t pluginitemcmd_size = 255;
size_t pluginoutput_size = 1500;
size_t plugincommand_size = 100;
size_t newfilename_size = 250;
size_t storedir_size = 50;
size_t backupdirectory_size = 100;
size_t filename_size = 100;
size_t logfile_size = 100;
int is_file_open = 0;
size_t declaration_size = 0;
size_t output_size = 0;
size_t update_output_size = 0;
size_t update_declaration_size = 0;
unsigned int socket_is_ready = 0;
unsigned int gardenerInterval = 43200;
unsigned int clearDataCacheInterval = 300;
unsigned int dataCacheTimeFrame = 330;
unsigned int kafka_start_id = 0;
unsigned int volatile thread_counter = 0;
unsigned char output_type = 0;
time_t tLastUpdate, tnextUpdate;
time_t tnextGardener;
time_t tnextClearDataCache;
time_t tPluginFile;
struct sockaddr_in address;
int server_fd;
int api_action = 0;
char* api_args = NULL;
int args_set = 0;
FILE *fptr = NULL;
char constants[MAX_CONSTANTS][50];
int values[20];
unsigned short *threadIds = NULL;
//char *logmessages[5];
int logmessage_id[5];
int logrecord = 0;
unsigned short volatile is_stopping = 0;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t file_opened = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hostname_mutex = PTHREAD_MUTEX_INITIALIZER;

void flushLog();
int isConstantsEnabled();
int getConstants();
void initNewPlugin(int index);
void initScheduler(int, int);
void apiReadData(int, int);
void apiDryRun(int);
void apiRunPlugin(int, int);
void apiRunAndRead(int, int);
void apiGetMetrics();
void apiReadAll();
void apiGetHostName();
void apiGetVars(int);
void apiCheckPluginConf();
void apiReloadConfigHard();
void runPluginCommand(int, char*);
void runPlugin(int, int);
void runPluginArgs(int, int, int);
void executeGardener();
int createSocket(int);
int initTimeScheduler();
void sig_handler(int);

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

int getNextMessage() {
	int count = 0;
	for (int i = 0; i < 5; ++i) {
		if (logmessage_id[i] == 0) {
			logmessage_id[i] = 1;
			//printf("DEBUG count is %d\n", count);
			return count;
		}
		count++;
	}
	// Buffer full clear it
	/*for (int j = 0; j < 5; j++) {
		logmessage_id[j] = 0;
		//memset(logmessages[j], 0, logmessage_size * sizeof(char));
		if (logmessages[j] != NULL && logmessage_size > 0) {
			 printf("DEBUG: logmessage[%d] = %s\n", j, logmessages[j]);
			 printf("Now clear with memset...\n"); 
   			 memset(logmessages[j], 0, logmessage_size * sizeof(char));
		} else {
    			// Handle the error, e.g., log an error message or exit the program
    			fprintf(stderr, "Error: logmessages[j] is NULL or logmessage_size is invalid.\n");
    			exit(EXIT_FAILURE);
		}
	}*/
	return 0;
}

void writeLog(const char *message, int level, int startup) {
        char timeStamp[20];
        size_t dest_size = 20;
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
	//int message_id = 0;
	
        snprintf(timeStamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (logrecord > 0) {
                sleep(0.5);
        }
        logrecord = 1;
	if (logmessage == NULL) {
		printf("Failed to write logmessage, since memory allocation is already freed.\n");
		printf("Message not written [Level] Message: %d, %s\n", level, message);
		return;
	}
	else {
		memset(logmessage, 0, (size_t)logmessage_size * sizeof(char)+1);
		logmessage[0] = '\0';
	}
        strncpy(logmessage, timeStamp, (size_t)(sizeof(timeStamp)-1));
	/*if (logmessage != NULL) {
		free(logmessage);
		logmessage = NULL;
		logmessage = malloc(1545 * sizeof(char));
		memset(logmessage, 0, logmessage_size * sizeof(char));
	}*/
	//message_id = getNextMessage();
	strncpy(logmessage, timeStamp, logmessage_size -1);
        strcat(logmessage, " | ");
        switch (level) {
                case 0:
                        strcat(logmessage, "[INFO]\t");
                        break;
                case 1:
                        strcat(logmessage, "[WARNING]\t");
                        break;
                case 2:
                        strcat(logmessage, "[ERROR]\t");
                        break;
                default:
                        strcat(logmessage, "[DEBUG]\t");
        }
        strcat(logmessage, message);
	//strncpy(logmessages[message_id], logmessage, logmessage_size);
	//logmessages[message_id][logmessage_size -1] = '\0';
	//printf("DEBUG: logmessage[%d] = %s\n", message_id, logmessages[message_id]);
	if (startup < 1) {
	 	pthread_mutex_lock(&file_mutex);
		while (!is_file_open) {
			pthread_cond_wait(&file_opened, &file_mutex);
		}
        	fprintf(fptr, "%s\n", logmessage);
		pthread_mutex_unlock(&file_mutex);
	}
	else {
		fprintf(fptr, "%s\n", logmessage);
	}
	if (dockerLog > 0) {
		printf("%s\n", logmessage);
	}
	logrecord = 0;
}

void logError(const char* message, int severity, int mode) {
        writeLog(message, severity, mode);
        fprintf(stderr, "%s\n", message);
}

void logInfo(const char*message, int severity,int mode) {
        writeLog(message, severity, mode);
        printf("%s\n", message);
}

void initLogger() {
      	char ch = '/';

        snprintf(logfile, logfile_size, "%s%c%s", logDir, ch, "almond.log");
        pthread_mutex_lock(&file_mutex);
        fptr = fopen(logfile, "a");
        if (fptr == NULL) {
		printf("Could not open '%s'\n", logfile);
                perror("Error opening logfile");
                exit(EXIT_FAILURE);
        }
        is_file_open = 1;
        pthread_cond_broadcast(&file_opened);
        pthread_mutex_unlock(&file_mutex);
        //writeLog("Logger is initiated.", 0, 0);
}

void checkCtMemoryAlloc() {
	if (confDir == NULL) {
                fprintf(stderr, "Failed to allocate memory.\n");
        }
	if (dataDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [dataDir].\n");
        }
	if (pluginDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [pluginDir].\n");
        }
	if (pluginDeclarationFile == NULL) {
                fprintf(stderr, "Failed to allocate memory [pluginDeclarationFile].\n");
        }
	if (storeDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [storeDir].\n");
        }
        if (infostr == NULL) {
                fprintf(stderr, "Failed to allocate memory [infostr].\n");
        }
        if (logDir == NULL) {
                fprintf(stderr, "Failed to allocate memory [logDir].\n");
        }
        if (fileName == NULL) {
                fprintf(stderr, "Failed to allocate memory [fileName].\n");
        }
	if (logfile == NULL ) {
		fprintf(stderr, "Failed to allocate memory [logfile].\n");
	}
        if (dataFileName == NULL ) {
                fprintf(stderr, "Failed to allocate memory [dataFileName].\n");
        }
        if (backupDirectory == NULL ) {
                fprintf(stderr, "Failed to allocate memory [backupDirectory].\n");
        }
        if (newFileName == NULL ) {
                fprintf(stderr, "Failed to allocate memory [newFileName].\n");
        }
	if (gardenerRetString == NULL) {
		fprintf(stderr, "Failed to allocate memory [gardenerRetString].\n");
	}
	if (pluginCommand == NULL) {
		fprintf(stderr, "Failed to allocate memory [pluginCommand].\n");
	}
	if (pluginReturnString == NULL) {
		fprintf(stderr, "Failed to allocate memory [pluginReturnString].\n");
	}
	if (storeName == NULL) {
		fprintf(stderr, "Failed to allocare memory [storeName].\n");
	}
	/*if (apiMessage == NULL) {
		fprintf(stderr, "Failed to allocate memory [apiMessage].\n");
	}*/
}

void updateHostName(char * str) {
	for (int i = 0; i < 255; i++) {
                hostName[i] = str[i];
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
        char *p = strtok(buf, ";");
        char *array[4];
        while (p != NULL) {
                array[i++] = p;
                p = strtok(NULL, ";");
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

int toggleHostName(char *name) {
        FILE * fPtr = NULL;
        FILE * fTemp = NULL;
        char * filename = NULL;
        char * tempfile = NULL;

        char buffer[1000];
        char fhost[20] = "scheduler.hostName=";
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
                char *pch;
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
        		oldName[c] = jsonFileName[c];
        		jsonFileName[c] = '\0';
        	}
	}
	else if (mode == 1) {
		for (int c = 0; c < sizeof(oldName); c++) {
                        oldName[c] = metricsFileName[c];
                        metricsFileName[c] = '\0';
                }
	}
        for (int i = 0; i < strlen(value); i++) {
        	if (value[i] == '\n')
                	break;
        	else {
			if (mode == 0)
               			jsonFileName[i] = value[i];
			if (mode == 1)
				metricsFileName[i] = value[i];
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
        		strcat(jsonFileName, ".json");
               		jsonFileName[50] = '\0';
		}
		else if (mode == 1) {
			strcat(metricsFileName, ".metrics");
			metricsFileName[50] = '\0';
		}
        }
        else {
		if (mode == 0) {
        		char *ext = strrchr(jsonFileName, '.');
                	if (ext) {
                		if (*(ext+1) == '\0') {
                        		writeLog("New jsonfile name is ending with a dot.", 1, 0);
                        	}
                        	jsonFileName[strlen(value)+1] = '\0';
                	}
                	else {
                		strcat(jsonFileName, ".json");
                        	jsonFileName[strlen(value)+6] = '\0';
                	}
			snprintf(infostr, infostr_size, "Json export file name changed to '%s'.", jsonFileName);
		}
		else if (mode == 1) {
			char *ext = strrchr(metricsFileName, '.');
                        if (ext) {
                                if (*(ext+1) == '\0') {
                                        writeLog("New metrics filname is ending with a dot.", 1, 0);
                                }
                                metricsFileName[strlen(value)+1] = '\0';
                        }
                        else {
                                strcat(metricsFileName, ".metrics");
                                metricsFileName[strlen(value)+9] = '\0';
                        }
			snprintf(infostr, infostr_size, "Metrics filename changed to '%s'.", metricsFileName);
		}

	}
       	writeLog(infostr, 1, 0);
       	writeLog("If using howru api together with Almond, you need to restart Howru service.", 2, 0);
	if (mode == 0)
       		toggleExportFileName(jsonFileName, 0);
	else if (mode == 1)
		toggleExportFileName(metricsFileName, 1);
       	char * removeFileName;
        if (mode == 0)
		removeFileName = malloc(datafilename_size);
	else if (mode == 1)
		removeFileName = malloc(100);
	if (removeFileName == NULL) {
       		writeLog("Could not allocate memory for removing old file.", 1, 0);
        }
        else {
		if (mode == 0) {
        		memset(removeFileName, '\0', datafilename_size);
                	snprintf(removeFileName, datafilename_size, "%s%c%s", dataDir, '/', oldName);
			if (remove(removeFileName) == 0) {
                        	snprintf(infostr, infostr_size, "Json export file '%s' is removed.", oldName);
                        	writeLog(infostr, 1, 0);
                	}
                	else {
                        	snprintf(infostr, infostr_size, "Could not remove old export file '%s'.", oldName);
                        	writeLog(infostr, 1, 0);
                	}
		}
		else if (mode == 1) {
			memset(removeFileName, '\0',100);
                        snprintf(removeFileName, 100, "%s%c%s", storeDir, '/', oldName);
			if (remove(removeFileName) == 0) {
                                snprintf(infostr, infostr_size, "Metrics export file '%s' is removed.", oldName);
                                writeLog(infostr, 1, 0);
                        }
                        else {
                                snprintf(infostr, infostr_size, "Could not remove old metrics file '%s'.", oldName);
                                writeLog(infostr, 1, 0);
                        }
		}
        }
        if (removeFileName != NULL) {
        	free(removeFileName);
                removeFileName = NULL;
        }
}

int compare_timestamps(const void* a, const void* b) {
        struct Scheduler* sa = (struct Scheduler*)a;
        struct Scheduler* sb = (struct Scheduler*)b;
        if (sb->timestamp > sa->timestamp) return -1;
        if (sb->timestamp < sa->timestamp) return 1;
        if (sa->id > sb->id) return -1;
        if (sa->id < sb->id) return 1;
        return 0;
}

int check_plugin_conf_file(char *pluginDeclarationFile) {
        FILE * fPtr = NULL;
        int i;
        char buffer[1000];
        int retval = 0;

        fPtr = fopen(pluginDeclarationFile, "r");
        if (fPtr == NULL)
        {
                perror("Error while opening the file.\n");
                writeLog("Error opening the plugin declarations file.", 2, 0);
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

void rescheduleChecks() {
	/*for (int i = 0; i < decCount; i++) {
                printf("ID = %d\t", scheduler[i].id);
                printf("Timestamp = %ld\n", (long)scheduler[i].timestamp);
        }*/
        writeLog("Schedule new exectution times.", 0, 0);
        qsort(scheduler, decCount, sizeof(struct Scheduler), compare_timestamps);
        /*for (int i = 0; i < decCount; i++) {
                printf("ID = %d\t", scheduler[i].id);
                printf("Timestamp = %ld\n", (long)scheduler[i].timestamp);
        }*/
        flushLog();
}

int updateValuesFromUdfFile(char id[3]) {
	FILE *fp = NULL;
	char* token;
	char filename[30];
	size_t buffer_size = pluginoutput_size + 100;
	char buffer[buffer_size];
	char columns[2][pluginoutput_size];
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
		token = strtok(buffer, "\t");
		while (token != NULL) {
			strcpy(columns[columnCount], token);
			columnCount++;
			token = strtok(NULL, "\t");
		}
		if (strcmp(columns[0], "item_id") == 0) {
			snprintf(infostr, infostr_size, "Updating pluginitem with id '%s' from update file.", trim(columns[1]));
			writeLog(infostr, 0, 0);
			pId = atoi(columns[1]);
		}
		if (pId != -1) {
			if (strcmp(columns[0], "item_lastruntimestamp") == 0) {
				strncpy(declarations[pId].lastRunTimestamp, trim(columns[1]), 20);
			}
			else if (strcmp(columns[0], "item_lastchangetimestamp") == 0) {
				strncpy(declarations[pId].lastChangeTimestamp, trim(columns[1]), 20);
			}
			else if (strcmp(columns[0], "item_nextruntimestamp") == 0) {
				strncpy(declarations[pId].nextRunTimestamp, trim(columns[1]), 20);
			}
			else if (strcmp(columns[0], "item_statuschanged") == 0) {
				strncpy(declarations[pId].statusChanged, trim(columns[1]), 1);
			}
			else if (strcmp(columns[0], "output_retcode") == 0) {
				//strcpy(outputs[pId].retCode, trim(columns[1]));
				outputs[pId].retCode = atoi(trim(columns[1]));
			}
			else if (strcmp(columns[0], "output_retstring") == 0) {
				strcpy(outputs[pId].retString, trim(columns[1]));
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
		char *timestamp = declarations[pId].nextRunTimestamp;
		if (strptime(timestamp,"%Y-%m-%d %H:%M:%S", &tm_struct)) {
			time_var = mktime(&tm_struct);
			if (time_var != -1) {
				declarations[pId].nextRun = time_var;
				if (timeScheduler == 1) {
					scheduler[pId].timestamp = time_var;
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
        fp = fopen(filename, "w");
	if (fp == NULL) {
		perror("Failed to open command file.");
		writeLog("Failed to open command file.", 2, 0);
		return;
	}
        strncpy(content, name, sizeof(content)-1);
        strcat(content, "\t");
        strcat(content, value);
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

        char filename[100] = "/opt/almond/api_cmd/";
        strcat(filename, cmd);

        cmdfile = fopen(filename, "r");
        if (cmdfile == NULL) {
                perror("Failed to open file");
		writeLog("Could not open command file.", 1, 0);
                return 1;
        }
        while (fgets(line, sizeof(line), cmdfile)) {
                token = strtok(line, "\t");
                while (token != NULL) {
                        strcpy(columns[columnCount], token);
                        columnCount++;
                        token = strtok(NULL, "\t");
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
		 if (kafka_tag == NULL) {
                 	kafka_tag = malloc((size_t)strlen(columns[1])+1);
                 	if (kafka_tag != NULL)
                 		memset(kafka_tag, '\0', (size_t)strlen(columns[1])+1 * sizeof(char));
		 	else {
				 writeLog("Failed to allocate memory for variable 'kafka_tag'.", 1, 0);
				 return 2;
		 	}
		 }
		 for (int i = 0; i < strlen(columns[1]); i++) {
		        if (columns[1][i] == '\n')
				break;
			else
                		kafka_tag[i] = columns[1][i];
                	if (columns[1][i] == '\0')
                        	break;
        	 }
		 kafka_tag[strlen(columns[1])+1] = '\0';
                 snprintf(infostr, infostr_size, "Kafka tag is set to '%s'", kafka_tag);
		 writeLog(infostr, 0, 0);
	}
	else if (strcmp(columns[0], "kafkatopic") == 0) {
		if (kafka_topic == NULL) {
			kafka_topic = malloc((size_t)strlen(columns[1])+1);
			if (kafka_topic != NULL)
				memset(kafka_topic, '\0', (size_t)strlen(columns[1])+1 * sizeof(char));
			else {
				writeLog("Failed to allocate memory for variable 'kafka_topic'.", 1, 0);
				return 2;
			}
		}
		for (int i = 0; i < strlen(columns[1]); i++) {
			if (columns[1][i] == '\n')
				break;
			else
				kafka_topic[i] = columns[1][i];
			if (columns[1][i] == '\0')
				break;
		}
		kafka_topic[strlen(columns[1])+1] = '\0';
		snprintf(infostr, infostr_size, "Kafka topic is set to '%s'.", kafka_topic);
		writeLog(infostr, 0, 0);
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
		runPlugin(id, 0);
		if (timeScheduler == 1) {
			rescheduleChecks();
		}
	}
	else if (strcmp(columns[0], "executeargs") == 0) {
		//int id = atoi([columns[1]);
		//command = trim(columns[2]);
		writeLog("Execute plugin with added arguments from command file.", 0, 0);
		parseExArgsCmd(columns[1]);
		//runPluginArgs
		if (timeScheduler == 1) {
			rescheduleChecks();
		}
	}

	else if (strcmp(columns[0], "metricsprefix") == 0) {
                memset(metricsOutputPrefix, '\0', metricsoutputprefix_size);
		for (int i = 0; i < strlen(columns[1]); i++) {
			if (columns[1][i] == '\n')
				break;
			else
				metricsOutputPrefix[i] = columns[1][i];
			if (columns[1][i] == '\0')
				break;
		}
	        snprintf(infostr, infostr_size, "Metrics prefix is set to '%s'", metricsOutputPrefix);
		writeLog(infostr, 0, 0);

	}
	else if (strcmp(columns[0], "update") == 0) {
		writeLog("Ready to run updates from udf-file.", 0, 0);
		updateValuesFromUdfFile(columns[1]);
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
	//writeLog("Check for command files.", 0, 0);
        if (!(d = opendir("/opt/almond/api_cmd"))) {
                perror("Failed to open directory");
		writeLog("Failed to open command file directory.", 1, 0);
                return 1;
        }
        while ((entry = readdir(d)) != NULL) {
                if (entry->d_type == DT_REG && strcmp(entry->d_name + strlen(entry->d_name) -4, ".cmd") == 0) {
                        runApiCmds(entry->d_name);
                }
        }
        return 0;
}

void initConstants() {
	logmessage = calloc(logmessage_size+1, sizeof(char));
	if (logmessage == NULL) {
                fprintf(stderr, "Failed to allocate memory [logmessage].\n");
        }
        else {
                strncpy(logmessage, "", logmessage_size+1);
		logmessage[logmessage_size] = '\0';
	}
        logfile = malloc(logfile_size);
        if (logfile == NULL) {
                 fprintf(stderr, "Failed to allocate memory [logFile].\n");
        }
        else
                memset(logfile, '\0', logfile_size);
	if (isConstantsEnabled() > 0) {
		getConstants();
        }
	confDir = malloc(confdir_size);
	if (confDir != NULL) {
		memset(confDir, 0, confdir_size);
	}
	dataDir = malloc(datadir_size);
	if (dataDir != NULL)
		memset(dataDir, '\0', datadir_size);
	pluginDir = malloc(plugindir_size);
	if (pluginDir != NULL)
		memset(pluginDir, '\0', plugindir_size);
	pluginDeclarationFile = malloc(plugindeclarationfile_size);
	if (pluginDeclarationFile != NULL)
		memset(pluginDeclarationFile, '\0', plugindeclarationfile_size);
	jsonFileName = calloc(jsonfilename_size+1, sizeof(char));
	if (jsonFileName == NULL) {
                fprintf(stderr, "Failed to allocate memory [jsonFileName].\n");
        }
	else
		strncpy(jsonFileName, "monitor_data.json", 18);
	metricsFileName = calloc(metricsfilename_size+1, sizeof(char));
	if (metricsFileName == NULL) {
		fprintf(stderr, "Failed to allocate memory [metricsFileName].\n");
	}
	else
		strncpy(metricsFileName, "monitor.metrics", 16);
	gardenerScript = calloc(gardenerscript_size+1, sizeof(char));
	if (gardenerScript == NULL) {
                fprintf(stderr, "Failed to allocate memory [gardenerScript].\n");
        }
        else
		strncpy(gardenerScript, "/opt/almond/gardener.py", 24);
	storeDir = malloc(storedir_size);
	if (storeDir == NULL) {
		fprintf(stderr, "Failed to allocate memory [storeDir].\n");
	}
	else
		memset(storeDir, '\0', storedir_size);
	logDir = malloc(logdir_size);
	if (logDir != NULL)
		memset(logDir, '\0', logdir_size);
	infostr = malloc((size_t)infostr_size * sizeof(char));
	if (infostr == NULL) {
		fprintf(stderr, "Failed to allocate memory [infostr].\n");
	}
	else
		memset(infostr, '\0', (size_t)infostr_size * sizeof(char));
	hostName = calloc(hostname_size+1, sizeof(char));
	if (hostName == NULL) {
		fprintf(stderr, "Failed to allocate memory [hostName].\n");
	}
	else
		strncpy(hostName, "None", 5);
	fileName = malloc((size_t)filename_size * sizeof(char));
	if (fileName == NULL) {
		fprintf(stderr, "Failed to allocate memory [fileName].\n");
	}
	else
		memset(fileName, '\0', (size_t)filename_size * sizeof(char));
	metricsOutputPrefix = calloc(metricsoutputprefix_size+1, sizeof(char));
	if (metricsOutputPrefix == NULL) {
		fprintf(stderr, "Failed to allocate memory [metricsOutputPrefix].\n");
	}
	else
		strncpy(metricsOutputPrefix, "almond", 7);
	dataFileName = malloc(datafilename_size);
	memset(dataFileName, '\0', datafilename_size);
	backupDirectory = malloc(backupdirectory_size);
	memset(backupDirectory, '\0', backupdirectory_size);
	newFileName = malloc(newfilename_size);
	memset(newFileName, '\0', (size_t)(150 * sizeof(char)));
	gardenerRetString = malloc((size_t)gardenermessage_size * sizeof(char));
	memset(gardenerRetString, '\0', (size_t)(sizeof(char) * gardenermessage_size));
	pluginCommand = malloc((size_t)100 * sizeof(char));
	memset(pluginCommand, '\0', sizeof(char) * 100);
	pluginReturnString = malloc((size_t)pluginmessage_size * sizeof(char));
	memset(pluginReturnString, '\0', (size_t)(pluginmessage_size * sizeof(char)));
	storeName = malloc((size_t)storename_size * sizeof(char));
	memset(storeName, '\0', (size_t)(sizeof(char) * storename_size));
	//apiMessage = malloc(apimessage_size * sizeof(char));
	checkCtMemoryAlloc();
}

int getConstants() {
	int count = 0;

	if (logmessage == NULL) {
		memset(logmessage, 0, logmessage_size);
                logmessage[0] = '\0';
	}

	writeLog("Reading memory variable constants.", 0, 1);

        FILE *file = fopen(constantsFile, "r");
        if (file == NULL) {
		printf("Could not read constants file. Not found.");
                return 1;
        }

        while (fscanf(file, "%s %d", constants[count], &values[count]) == 2) {
                count++;
		if (count == MAX_CONSTANTS) break;
        }
        for (int i = 0; i < count; i++) {
                if (strcmp(constants[i], "CONFDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'confDir' will be allocated by constants file.", 0, 1);
			confdir_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "DATADIR_SIZE") == 0) {
			writeLog("Memory for variable 'dataDir' will be reallocated by constants file.", 0, 1);
			datadir_size = (size_t)(values[i] * sizeof(char) + 1);
		}
		else if (strcmp(constants[i], "PLUGINDECLARATIONFILE_SIZE") == 0) {
			writeLog("Memory for variable 'pluginDeclarationSize' will be allocated by constants file.", 0, 1);
			plugindeclarationfile_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "JSONFILENAME_SIZE") == 0) {
			writeLog("Memory for variable 'jsonFileName' will be allocated by constants file.", 0, 1);
			jsonfilename_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "METRICSFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'metricsFileName' will be allocated by constants file.", 0, 1);
			metricsfilename_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "GARDENERSCRIPT_SIZE") == 0) {
                        writeLog("Memory for variable 'gardenerScript' will be allocated by constants file.", 0, 1);
			gardenerscript_size = (size_t)(values[i] * sizeof(char)+1);
		} 
		else if (strcmp(constants[i], "HOSTNAME_SIZE") == 0) {
			writeLog("Memory for variable 'hostName' will be allocated by constants file.", 0, 1);
			hostname_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "METRICSOUTPUTPREFIX_SIZE") == 0) {
                        writeLog("Memory for variable 'metricsOutputPrefix' will be allocated by constants file.", 0, 1);
			metricsoutputprefix_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "STOREDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'storeDir' will be allocated by constants file.", 0, 1);
			storedir_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "LOGDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'logDir' will be allocated by constants file.", 0, 1);
			logdir_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "INFOSTR_SIZE") == 0) {
			writeLog("Memory for 'info_str' will be allocated by constants file.", 0, 1);
			infostr_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "PLUGINDIR_SIZE") == 0) {
                        writeLog("Memory for variable 'pluginDir' will be allocated by constants file.", 0, 1);
			plugindir_size =  (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "FILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'fileName' will be allocated by constants file.", 0, 1);
			filename_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "LOGMESSAGE_SIZE") == 0) {
			writeLog("Memory for variable 'logmessage' will be allocated by constants file.", 0, 1);
			logmessage_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "LOGFILE_SIZE") == 0) {
			writeLog("Memory for variable 'logfile' will be allocated by constants file.", 0, 1);
			logfile_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "DATAFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'dataFileName' will be allocated by constants file.", 0, 1);
			datafilename_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "BACKUPDIRECTORY_SIZE") == 0) {
                        writeLog("Memory for variable 'backupDirectory' will be allocated by constants file.", 0, 1);
			backupdirectory_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "NEWFILENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'newFileName' will be allocated by constants file.", 0, 1);
			newfilename_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "GARDENERMESSAGE_SIZE") == 0) {
			writeLog("Memory for gardener return message will be allocated by constants file.", 0, 1);
			gardenermessage_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "PLUGINCOMMAND_SIZE") == 0) {
			writeLog("Memory for plugin command size will be allocated by constants file.", 0, 1);
			plugincommand_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "PLUGINMESSAGE_SIZE") == 0) {
                        writeLog("Memory for plugin message size will be allocated by constants file.", 0, 1);
			pluginmessage_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "STORENAME_SIZE") == 0) {
                        writeLog("Memory for variable 'storeName' will be allocated by constants file.", 0, 1);
                        storename_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "APIMESSAGE_SIZE") == 0) {
                        writeLog("Memory for API messages will dynamically be allocated by size inconstants file.", 0, 1);
                        apimessage_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "SOCKETSERVERMESSAGE_SIZE") == 0) {
			writeLog("Memory for socket server messages will dynamically be allocated by size in constants file.", 0, 1);
			socketservermessage_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "SOCKETCLIENTMESSAGE_SIZE") == 0) {
			writeLog("Memory for socket client messages will dynamically be allocated by size in constants file.", 0, 1);
			socketclientmessage_size = (size_t)(values[i] * sizeof(char)+1);
		}
		else if (strcmp(constants[i], "PLUGINITEMNAME_SIZE") == 0) {
			writeLog("Memory for pluginitem name size will dynamically be allocated by size in constants file.", 0, 1);
                        pluginitemname_size = (size_t)(values[i] * sizeof(char)+1);
                }
 		else if (strcmp(constants[i], "PLUGINITEMDESC_SIZE") == 0) {
                        writeLog("Memory for pluginitem description size will dynamically be allocated by size in constants file.", 0, 1);
                        pluginitemdesc_size = (size_t)(values[i] * sizeof(char)+1);
                }
 		else if (strcmp(constants[i], "PLUGINITEMCMD_SIZE") == 0) {
                        writeLog("Memory for pluginitem command size will dynamically be allocated by size in constants file.", 0, 1);
                        pluginitemcmd_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else if (strcmp(constants[i], "PLUGINOUTPUT_SIZE") == 0) {
                        writeLog("Memory for plugin output will dynamically be allocated by size in constants file.", 0, 1);
                        pluginoutput_size = (size_t)(values[i] * sizeof(char)+1);
                }
		else {
			snprintf(infostr, infostr_size, "Constant '%s' not implemented by Almond %s", constants[i], VERSION);
			writeLog(trim(infostr), 1, 1);
		}
        }
        return 0;
}

void constructSocketMessage(const char* action, const char* message) {
	int size = strlen(action) + strlen(message);
	size += 11;
	socket_message = malloc((size_t)size);
    	if (socket_message == NULL) {
        	printf("Memory allocation failed.\n");
		writeLog("Memory allocation failed [constructSocketMessage:socket_message]", 2, 0);
        	return;
    	}
	else
		memset(socket_message, '\0', (size_t)size * sizeof(char));
    	snprintf(socket_message, (size_t)size, "{ \"%s\":\"%s\" }\n", action, message);
}

int directoryExists(const char *checkDir, size_t length) {
        snprintf(infostr, infostr_size, "Checking directory %s", checkDir);
        writeLog(trim(infostr), 0, 1);

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

void flushLog() {
	pthread_mutex_lock(&file_mutex);
	if (fptr != NULL) {
		fclose(fptr);
		fptr = NULL;
		//fflush(fptr);
		is_file_open = 0;
	}
	pthread_mutex_unlock(&file_mutex);
	initLogger();
}

int getIdFromName(char *plugin_name) {
	char* pluginName = NULL;
	int retVal = -1;

	for (int i = 0; i < decCount; i++) {
		pluginName = malloc((size_t)pluginitemname_size * sizeof(char)+1);
		if (pluginName == NULL) {
			fprintf(stderr, "Failed to allocate memory.\n");
			writeLog("Failed to allocate memory [getIdFromName:pluginName]", 2, 0);
			return -1;
		}
		else
			memset(pluginName, '\0', (size_t)pluginitemname_size+1 * sizeof(char));
                strncpy(pluginName, declarations[i].name, pluginitemname_size);
		removeChar(pluginName, '[');
		removeChar(pluginName, ']');
		if (strcmp(trim(plugin_name), pluginName) == 0) {
			retVal = declarations[i].id;
			break;
		}
		free(pluginName);
		pluginName = NULL;
	}
	return retVal;
}

void* apiThread(void* data) {
	int retrys = 3;
	int retry_count = 0;
	int createSocketRetVal = 0;
        pthread_detach(pthread_self());
	createSocketRetVal = createSocket(server_fd);
        while ((createSocketRetVal != 0)  && (retry_count > retrys)) {
		perror("Create socket.");
		printf("Could not create socket!\n");
		writeLog("Could not create socket for API thread.", 1, 0);
		sleep(1);
		createSocketRetVal = createSocket(server_fd);
		retry_count++;
	}
	pthread_mutex_lock(&mtx);
	thread_counter--;
	pthread_mutex_unlock(&mtx);
        pthread_exit(NULL);
}

void startApiSocket() {
        pthread_t thread_id;
        int rc;

        rc = pthread_create(&thread_id, NULL, apiThread, "almondapi");
        if(rc) {
		printf("Error creating phtread\n");
                snprintf(infostr, infostr_size, "Error: return code from phtread_create is %d\n", rc);
                writeLog(trim(infostr), 2, 0);
		return;
        }
	//pthread_setname_np(thread_id, "API Connection Listener");
	pthread_setspecific(thread_id, "API Connection Listener");
	printf("New thread accepting socket created.\n");
        snprintf(infostr, infostr_size, "Created new thread (%lu) listening for connections on port %d \n", thread_id, local_port);
        writeLog(trim(infostr), 0, 0);
	pthread_mutex_lock(&mtx);
	thread_counter++;
	pthread_mutex_unlock(&mtx);
}

void changeSetValue(int id, int newval) {
	if (id != 3) {
		if (newval > 0)
			newval = 1;
		else
			newval = 0;
	}
	switch (id) {
		case 1:
			logPluginOutput = newval;
			break;
		case 2:
			saveOnExit = newval;
			break;
		case 3:
                        if ((newval < 1000) || (newval > 60000)) {
				writeLog("API call is trying to set sleep to unsupported value.", 1, 0);
				writeLog("Scheduler sleep value is unchanged.", 0, 0);
			}
			else
				schedulerSleep = newval;
			break;
		case 4:
			kafka_start_id = newval;
			break;
		default:
			writeLog("changeSetValue called with wrong index", 1, 0);
	}
}

void setPluginOutput(int newval) {
	if (newval > 0)
	       	newval = 1 ;
	else newval = 0;
	logPluginOutput = newval;
}

int toggleQuickStart(int on) {
	FILE * fPtr = NULL;
	FILE * fTemp = NULL;
	char * filename = NULL;
	char * tempfile = NULL;

	char buffer[1000];
	char enable[30] = "scheduler.quickStart=1";
	char disable[30] = "scheduler.quickStart=0";
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
        char header[100] = "HTTP/1.1 200 OK\nContent-Type:application/txt\nContent-Length: ";
	char * send_message = NULL;
	size_t content_length = 0;
	char len[4];

	if (args_set == 0) {
		switch (api_action) {
        		case API_READ:
				apiReadData(id, aflags);
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
                                enableTimeTuner = 1;
                                writeLog("Time tuner enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Time tuner is now enabled.");
                                break;
                        case API_DISABLE_TIMETUNER:
                                enableTimeTuner = 0;
                                writeLog("Time tuner disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Time tuner is now disabled.");
                                break;
                        case API_ENABLE_GARDENER:
                                enableGardener = 1;
                                writeLog("Gardener enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Gardener is now enabled.");
                                break;
                        case API_DISABLE_GARDENER:
                                enableGardener = 0;
                                writeLog("Gardener disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Gardener is now disabled.");
                                break;
			case API_ENABLE_CLEARCACHE:
                                enableClearDataCache = 1;
                                writeLog("ClearDataCache enabled through API call.", 0, 0);
				constructSocketMessage("enable", "ClearDataCache is now enabled.");
                                break;
                        case API_DISABLE_CLEARCACHE:
                                enableClearDataCache = 0;
                                writeLog("ClearDataCache disabled through API call.", 0, 0);
				constructSocketMessage("disable", "ClearDataCache is now disabled.");
                                break;
                        case API_ENABLE_QUICKSTART:
                                quick_start = 1;
				toggleQuickStart(1);
                                writeLog("Quick start enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Quick start is now enabled.");
                                break;
                        case API_DISABLE_QUICKSTART:
                                quick_start = 0;
				toggleQuickStart(0);
                                writeLog("Quick start disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Quick start is now disabled");
                                break;
                        case API_ENABLE_STANDALONE:
                                standalone = 1;
                                writeLog("Standalone mode enabled through API call.", 0, 0);
				constructSocketMessage("enable", "Standalone mode is now enabled");
                                break;
                        case API_DISABLE_STANDALONE:
                                standalone = 0;
                                writeLog("Standalone mode disabled through API call.", 0, 0);
				constructSocketMessage("disable", "Standalone mode is now disabled.");
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
			case API_CHECK_PLUGIN_CONFIG:
				apiCheckPluginConf();
				break;
			case API_RELOAD_CONFIG_HARD:
				apiReloadConfigHard();
				break;
			case API_RELOAD_CONFIG_SOFT:
				//apiReloadConfigSoft();
				break;
			case API_RELOAD_ALMOND:
				//apiReload();
				break;
			case API_DENIED:
				constructSocketMessage("return", "Access denied: You need a valid token.");
                                break;
                	default:
                        	//printf("The request did not trigger any action.\n");
				constructSocketMessage("return", "The request id did not trigger any action.");
		}
        }
	else args_set = 0;
	content_length = (size_t)strlen(socket_message); 
	sprintf(len, "%li", content_length);
        strcat(header, trim(len));
        strcat(header, "\n\n");
	content_length += (size_t)strlen(header);
	/*if (send_message != NULL) {
		printf("This is strange...Is it the threads spinning around?\n");
		free(send_message);
		send_message = NULL;
	}*/
	send_message = malloc((size_t)content_length+1 * sizeof(char));
	if (send_message == NULL) {
		perror("Failed to allocate memory for send_message");
		writeLog("Could not allocate memory [send_socket_message:send_message]", 2, 0);
		return;
	}
	else
		memset(send_message, '\0', (content_length+1) * sizeof(char));
        strncpy(send_message, header, (size_t)(sizeof(header)));
	strcat(send_message, socket_message);
	if (use_ssl > 0) {
		if (SSL_write(ssl, send_message, strlen(send_message)) <= 0) {
			writeLog("Could not send ssl message to client", 1, 0);
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
	send_message = NULL;
	if (socket_message != NULL) {
		free(socket_message);
		socket_message = NULL;
	}
}

struct json_object* getJsonValue(struct json_object *jobj, const char* key) {
        struct json_object *tmp;
        if (json_object_object_get_ex(jobj, key, &tmp)) {
                return tmp;
        }
        return NULL;
}

void parseClientMessage(char str[], int arr[]) {
        struct json_object *jobj, *jaction, *jid, *jname,  *jflags, *jargs, *jvalue, *jmode;
	struct json_object *jtoken;
        char *value = NULL;
        char action[10];
        char sid[5];
	char flags[10];
	char args[100];
	char sval[100];
	char name[50];
	char mode[5];
	char * fname = NULL;
        char * lname = NULL;
        char username[40];
        char* token = NULL;
        char line[100];
        int id = -1;
	int aflags = 0;
	int bExecute = 0;
        enum json_tokener_error jerr;

	args_set = 0;
        json_tokener *tok = json_tokener_new();
        jobj = json_tokener_parse_ex(tok, str, (size_t)(strlen(str)));
        jerr = json_tokener_get_error(tok);
        if (jerr != 0) {
                printf("jerr = %s\n", json_tokener_error_desc(jerr));
                printf("j = %p\n", jobj);
                printf("jerr_raw = %d\n", jerr);
		snprintf(infostr, infostr_size, "Json error: %s", json_tokener_error_desc(jerr));
		writeLog(trim(infostr), 1, 0);
		writeLog("Could not parse API call. Wrong syntax.", 1, 0);
                return;
        }
        json_object_object_foreach(jobj, key, val) {
                value = (char *) json_object_get_string(val);
        }
        jaction = getJsonValue(jobj, "action");
        jid = getJsonValue(jobj, "id");
	jname = getJsonValue(jobj, "name");
	jflags = getJsonValue(jobj, "flags");
	jargs = getJsonValue(jobj, "args");
	jtoken = getJsonValue(jobj, "token");
	jvalue = getJsonValue(jobj, "value");
	jmode = getJsonValue(jobj, "mode");
	if (jid != NULL) {
        	strncpy(sid, json_object_to_json_string_ext(jid, JSON_C_TO_STRING_PLAIN), 5);
        	removeChar(sid, '"');
	}
	if (jaction != NULL) {
        	strncpy(action, json_object_to_json_string_ext(jaction, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 10);
        	removeChar(action, '"');
	}
	if (jname != NULL) {
		strncpy(name, json_object_to_json_string_ext(jname, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 50);
		removeChar(name, '"');
	}
	if (jmode != NULL) {
		strncpy(mode, json_object_to_json_string_ext(jmode, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 5);
		removeChar(mode, '"');
	}
        if (jflags != NULL) {
		strncpy(flags, json_object_to_json_string_ext(jflags, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 10);
		removeChar(flags, '"');
		if (strcmp(trim(flags), "verbose") == 0) {
			aflags = 1;
		}
		else if (strcmp(trim(flags), "dry") == 0) {
			aflags = API_DRY_RUN;
			api_action = API_DRY_RUN;
		}
		else if (strcmp(trim(flags), "all") == 0) {
			aflags = 10;
		}
		else aflags = 0;
	}
	if (jargs != NULL) {
		strncpy(args, json_object_to_json_string_ext(jargs, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 100);
		removeChar(args, '"');
		args_set++;
	}
	else args_set = 0;
	if (jvalue != NULL) {
		strncpy(sval, json_object_to_json_string_ext(jvalue, JSON_C_TO_STRING_PLAIN | JSON_C_TO_STRING_PRETTY), 100);
		removeChar(sval, '"');
	}
	if (jtoken != NULL) {
		token = malloc(30);
		if (token == NULL) {
			writeLog("Could not allocate memory for execute token", 1, 0);
		}
		else
			memset(token, '\0', 30 * sizeof(char));
                strncpy(token, json_object_to_json_string_ext(jtoken, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY), 30);
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
					fname = malloc((size_t)sizeof(line)+1);
					if (fname == NULL) {
						writeLog("Could not allocate message [parseClientMessage:fname]", 2, 0);
						return;
					}
					else
						memset(fname, '\0', (size_t)sizeof(line)+1 * sizeof(char));
					strncpy(fname, trim(line), sizeof(line));
                                }
                                if (i == 2){
                                        lname = malloc((size_t)sizeof(line)+1);
					if (lname == NULL) {
						writeLog("Could not allocate memory [parseClientMessage:lname]", 2, 0);
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
					strncpy(username, "", 2);
					strcat(username, fname);
                                        strcat(username, " ");
                                        strcat(username, lname);
                                        snprintf(infostr, infostr_size, "User '%s' granted API execution rights from token.", username);
                                        writeLog(trim(infostr), 0, 0);
                                        flushLog();
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
			api_action = API_READ_ALL;
		}
		else {
                	api_action = API_READ;
		}
        }
        else if ((strcmp(trim(action), "execute") == 0)|| (strcmp(trim(action), "run") == 0)) {
		if (bExecute > 0) {
                        if (strcmp(trim(name), "gardener") == 0) {
                                api_action = API_EXECUTE_GARDENER;
                        }
                        else if (api_action != API_DRY_RUN)
                                api_action = API_RUN;
                }
                else api_action = API_DENIED;
        }
	else if ((strcmp(trim(action), "runread") == 0) || (strcmp(trim(action), "exread") == 0)) {
		if (bExecute != 0)
			api_action = API_EXECUTE_AND_READ;
		else 
			api_action = API_DENIED;
	}
	else if ((strcmp(trim(action), "metrics") == 0) || (strcmp(trim(action), "getm") == 0)) { 
		api_action = API_GET_METRICS;
	}	
	else if ((strcmp(trim(action), "enable") == 0) || (strcmp(trim(action), "disable") == 0)) {
		printf("Action is enable or disable\n");
 		if (bExecute != 0) {
 			if (strcmp(trim(name), "timetuner") == 0) {
 				if (strcmp(trim(action), "enable") == 0)
 					api_action = API_ENABLE_TIMETUNER;
 				else if (strcmp(trim(action), "disable") == 0)
 					api_action = API_DISABLE_TIMETUNER;
 			}
 			if (strcmp(trim(name), "gardener") == 0) {
 				if (strcmp(trim(action), "enable") == 0)
 					api_action = API_ENABLE_GARDENER;
 				else if (strcmp(trim(action), "disable") == 0)
 					api_action = API_DISABLE_GARDENER;
 			}
                        if (strcmp(trim(name), "cleancache") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        api_action = API_ENABLE_CLEARCACHE;
                                else if (strcmp(trim(action), "disable") == 0)
                                        api_action = API_DISABLE_CLEARCACHE;
                        }
			if (strcmp(trim(name), "quickstart") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        api_action = API_ENABLE_QUICKSTART;
                                else if (strcmp(trim(action), "disable") == 0)
                                        api_action = API_DISABLE_QUICKSTART;
                        }
			if (strcmp(trim(name), "standalone") == 0) {
                                if (strcmp(trim(action), "enable") == 0)
                                        api_action = API_ENABLE_STANDALONE;
                                else if (strcmp(trim(action), "disable") == 0)
                                        api_action = API_DISABLE_STANDALONE;
                        }
 		}
		else
			api_action = API_DENIED;
	}
	else if ((strcmp(trim(action), "set") == 0) || (strcmp(trim(action), "setvar") == 0)) {
		if (bExecute != 0) {
			pthread_mutex_lock(&update_mtx);
			if (strcmp(trim(name), "pluginoutput") == 0) {
				int val = atoi(trim(sval));
				setPluginOutput(val);
				changeSetValue(1, val);
				api_action = API_SET_PLUGINOUTPUT;
			}
			else if (strcmp(trim(name), "saveonexit") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(2, val);
				api_action = API_SET_SAVEONEXIT;
			}
			else if (strcmp(trim(name), "sleep") == 0) {
				int val = atoi(trim(sval));
				changeSetValue(3, val);
				api_action = API_SET_SLEEP;
			}
			else if (strcmp(trim(name), "kafkatag") == 0) {
				setApiCmdFile("kafkatag", trim(sval));
				writeLog("A command file for changing kafkatag has been created.", 0, 0);
				api_action = API_SET_KAFKATAG;
			}
			else if (strcmp(trim(name), "kafkatopic") == 0) {
				setApiCmdFile("kafkatopic", trim(sval));
				writeLog("A command file for changing Kafka topic name has been created.", 0, 0);
				api_action = API_SET_KAFKATOPIC;
			}
			else if (strcmp(trim(name), "jsonfilename") == 0) {
				setApiCmdFile("jsonfilename", trim(sval));
				writeLog("A command file for changing json export file name has been created.", 0, 0);
				api_action = API_SET_JSONFILENAME;
			}
			else if (strcmp(trim(name), "metricsfilename") == 0) {
				setApiCmdFile("metricsfilename", trim(sval));
				writeLog("A command file for changinf metrics file name has been created.", 0, 0);
				api_action = API_SET_METRICSFILENAME;
			}
			else if (strcmp(trim(name), "kafkastartid") == 0) {
				int val = atoi(trim(sval));
				if (val > 0) {
					changeSetValue(4, val);
                                	snprintf(infostr, infostr_size, "Kafka start id is set to '%d'", val);
                                	writeLog("Kafka start id is toggled through API call.", 0, 0);
                                	writeLog(trim(infostr), 0, 0);
				}
				else {
					snprintf(infostr, infostr_size, "Could not set Kafka start id to '%s'", sval);
					writeLog("Kafka start id was toggled through API call.", 0, 0);
					writeLog(trim(infostr), 1, 0);
				}
				api_action = API_SET_KAFKA_START_ID;
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
				snprintf(infostr,  infostr_size, "Virtal hostname set to '%s'", newname);
				writeLog("Hostname (virtual) is toggled through API call.", 1, 0);
				writeLog(trim(infostr), 1, 0);
				free(newname);
				setApiCmdFile("hostname", trim(sval));
				api_action = API_SET_HOSTNAME;
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
				api_action = API_SET_METRICSPREFIX;
			}
			else {
				api_action = -1;
			}
			pthread_mutex_unlock(&update_mtx);
		}
		else {
			writeLog("API action was denied. Wrong or no token supplied.", 1, 0);
			api_action = API_DENIED;
		}
	}
	else if (strcmp(trim(action), "getvar") == 0) {
                if (strcmp(trim(name), "hostname") == 0) {
                        api_action = API_GET_HOSTNAME;
                }
		else if (strcmp(trim(name), "kafkatag") == 0) {
			api_action = API_GET_KAFKATAG;
		}
		else if (strcmp(trim(name), "metricsprefix") == 0) {
			api_action = API_GET_METRICSPREFIX;
		}
		else if (strcmp(trim(name), "jsonfilename") == 0) {
			api_action = API_GET_JSONFILENAME;
		}
		else if (strcmp(trim(name), "metricsfilename") == 0) {
			api_action = API_GET_METRICSFILENAME;
		}
		else if (strcmp(trim(name), "kafkatopic") == 0) {
			api_action = API_GET_KAFKATOPIC;
		}
		else if (strcmp(trim(name), "sleep") == 0) {
                        api_action = API_GET_SLEEP;
                }
		else if (strcmp(trim(name), "saveonexit") == 0) {
			api_action = API_GET_SAVEONEXIT;
		}
		else if (strcmp(trim(name), "pluginoutput") == 0) {
			api_action = API_GET_PLUGINOUTPUT;
		}
		else if (strcmp(trim(name), "kafkastartid") == 0) {
			api_action = API_GET_KAFKA_START_ID;
		}
		else {
			api_action = -1;
		}
        }
	else if (strcmp(trim(action), "check") == 0) {
		if (strcmp(trim(name), "pluginconfig") == 0) {
			api_action = API_CHECK_PLUGIN_CONFIG;
		}
		else if (strcmp(trim(name), "pluginconfigts") == 0) {
			api_action = API_GET_PLUGIN_RELOAD_TS;
		}
		else {
			api_action = -1;
		}
	}
	else if (strcmp(trim(action), "reload") == 0) {
		if (strcmp(trim(name), "almond") == 0) {
			// Reload Almond
			api_action = API_RELOAD_ALMOND;
		}
		else if (strcmp(trim(name), "plugins") == 0) {
			if (strcmp(trim(mode), "hard") == 0) {
				// Hard reload
				api_action = API_RELOAD_CONFIG_HARD;
			}
			else if (strcmp(trim(mode), "soft") == 0) {
				// Soft reload
				api_action = API_RELOAD_CONFIG_SOFT;
			}
			else {
				api_action = -1;
			}
		}
		else {
			api_action = -1;
		}
	}
        else {
                api_action = 0;
        }
        if (api_action > 0) {
                id = atoi(sid);
                if (id == 0) {
			if (jname != NULL) {
				id = getIdFromName(name);
				if (id == -1) {
					// Some api action does not need name
					if (api_action > API_NAME_END && api_action < API_NAME_START) {
						snprintf(infostr, infostr_size, "Try to run API command with name '%s', which does not exist.", name);
                                        	writeLog(trim(infostr), 1, 0);
						api_action = 0;
						return;
					}
					else return;
				}
			}	
			else {
				writeLog("Received a bad json-request. API call is aborted.", 1, 0);
				api_action = 0;
                        	return;
			}
			if (id < 0) {
				writeLog("Could not get id from name. This might cause strange things to happen. Aborting API call.", 1, 0);
				api_action = 0;
				return;
			}
                }
                id--;
		if (args_set > 0 && (api_action == API_RUN || api_action == API_DRY_RUN || api_action == API_EXECUTE_AND_READ)) {
			api_args = malloc((size_t)strlen(args)+1);
			if (api_args == NULL) {
				fprintf(stderr, "Could not allocate memory.\n");
				writeLog("Could not allocate memory [parseClientMessage:api_args]", 2, 0);
				return;
			}
			else
				memset(api_args, '\0', (size_t)strlen(args)+1 * sizeof(char));
			strncpy(api_args, args, strlen(args));
			runPluginArgs(id, aflags, api_action);
			if (timeScheduler == 1) {
				rescheduleChecks();
			}
			free(api_args);
			api_args = NULL;
		}
        }
	arr[0] = id;
	arr[1] = aflags;
}

SSL_CTX *create_context() {
    	const SSL_METHOD *method;
    	SSL_CTX *ctx;

   	method = TLS_server_method();

    	ctx = SSL_CTX_new(method);
	if (!ctx) {
        	perror("Unable to create SSL context");
        	ERR_print_errors_fp(stderr);
        	exit(EXIT_FAILURE);
    	}
    	return ctx;
}

void configure_context(SSL_CTX *ctx) {
	/* Set the key and cert */
   	if (SSL_CTX_use_certificate_file(ctx, almondCertificate, SSL_FILETYPE_PEM) <= 0) {
        	ERR_print_errors_fp(stderr);
        	exit(EXIT_FAILURE);
    	}
    	if (SSL_CTX_use_PrivateKey_file(ctx, almondKey, SSL_FILETYPE_PEM) <= 0 ) {
        	ERR_print_errors_fp(stderr);
        	exit(EXIT_FAILURE);
    	}
	/*if (!SSL_CTX_use_certificate_chain_file(ctx, "almonds.crt"))
        	ERR_print_errors_fp(stderr);*/
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    	if(!SSL_CTX_check_private_key(ctx)) {
        	fprintf(stderr, "Private key does not match the certificate public key\n");
        	exit(EXIT_FAILURE);
    	}
}

int initSocket () {
        int opt = 1;
        if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                perror("socket failed");
                writeLog("Could not initiate socket.", 2, 0);
                return -1;
        }
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt))) {
                perror("setsockopt");
                writeLog("Setsockopt failed.", 2, 0);
                return -1;
        }
	bzero((char *)&address, sizeof(address));
	//memset(&address, 0, sizeof(address);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        if (local_port == ALMOND_API_PORT)
                address.sin_port = htons((uint16_t)ALMOND_API_PORT);
        else
                address.sin_port = htons((uint16_t)local_port);
        if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0) {
                perror("bind failed");
                writeLog("Failed to bind port.", 2, 0);
                return -1;
        }
	if (use_ssl > 0) {
    		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
        	ctx = create_context();
                configure_context(ctx);
        }
        writeLog("Almond socket initialized.", 0, 0);
        socket_is_ready = 1;
        return socket_is_ready;
}

int createSocket(int server_fd) {
        int client_socket;
        socklen_t client_size;
        struct sockaddr_in client_addr;
	int params[2];
	SSL *ssl;

	server_message = malloc((size_t)socketservermessage_size+1);
	if (server_message == NULL) {
		fprintf(stderr, "Failed to allocate memory for servermessage.\n");
		writeLog("Failed to allocate memory [createSocket:servermessage].", 1, 0);
		return -1;
	}
	client_message = malloc((size_t)socketclientmessage_size+1);
	if (client_message == NULL) {
		fprintf(stderr, "Failed to allocate memory for clientmessage.\n");
                writeLog("Failed to allocate memory [createSocket:clientmessage].", 1, 0);
                return -1;
	}
        memset(server_message, '\0', (size_t)socketservermessage_size);
        memset(client_message, '\0', (size_t)socketclientmessage_size);
        if (listen(server_fd, 5) < 0) {
                perror("listen");
                writeLog("Failed listening...", 2, 0);
                socket_is_ready = 0;
                return -1;
        }
        snprintf(infostr, infostr_size, "Ready listening on port %d", local_port);
        writeLog(trim(infostr), 0, 0);
	free(infostr);
	infostr = NULL;
	infostr = malloc((size_t)infostr_size * sizeof(char));
	if (infostr != NULL) {
		memset(infostr, '\0', infostr_size * sizeof(char));
		strncpy(infostr, "", 2);
	}
	else
		printf("Failed to allocate memory for 'infostr'.\n");
        // Accept incoming connections
        client_size = sizeof(client_addr);
	while(1) {
        	client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_size);
        	if (client_socket < 0){
			if (is_stopping == 0) {
				perror("ERROR on accept.");
                		printf("Can't accept any socket requests.\n");
                		writeLog("Could not accept client socket.", 1, 0);
			}
                	return -1;
			//continue;
        	}
		if (use_ssl > 0) {
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, client_socket);

			/*if (SSL_accept(ssl) <= 0) {
				ERR_print_errorsfp(stderr);
			}*/
			if (SSL_accept(ssl) <= 0) {
				ERR_print_errors_fp(stderr);
				writeLog("Error while authenticating SSL connection.", 0, 0);
				return -1;
			}
			else
				writeLog("SSL client authenticated succesfully", 0, 0); 
		}
        	printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        	snprintf(infostr, infostr_size, "Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        	writeLog(trim(infostr), 0, 0);
		pid_t pid = fork();
		if (pid == 0) {
			close(server_fd);
			if (client_message == NULL) return -1;
			if (use_ssl > 0) {
			       	SSL_CTX_free(ctx);
				int bytes = SSL_read(ssl, client_message, socketclientmessage_size);
				if (bytes > 0)
				       writeLog("SSL client message received.", 0, 0);
				else {
					ERR_print_errors_fp(stderr);
					writeLog("Failed to receive ssl client message.", 1, 0);
					free(server_message);
					free(client_message);
					server_message = NULL;
					client_message = NULL;
					return -1;
				}
			}
			else {	
        			if (recv(client_socket, client_message, socketclientmessage_size, 0) < 0){
					perror("recv failed\n");
                			printf("Couldn't receive\n");
                			writeLog("Could not receieve client message on socket.", 1, 0);
					free(server_message);
        				free(client_message);
        				server_message = client_message = NULL;
                			return -1;
        			}
			}
			if (client_message == NULL) {
				printf("Could not receive client message.\n");
				char message[100] = "Received empty message. Nothing to reply.";
				if (use_ssl > 0) {
					SSL_write(ssl, message, strlen(message));
					writeLog("Could not send message to client.", 1, 0);
				}
				else {
                			if (send(client_socket, message, 100, 0) < 0) {
						writeLog("Could not send message to client.", 1, 0);
        				}
				}	
				writeLog("Message sent on socket. Closing connection.", 0, 0);
                		close(client_socket);
				free(server_message);
				free(client_message);
				server_message = client_message = NULL;
                		return -1;
			}
        		char *e;
       	 		int index;
        		e = strchr(client_message, '{');
        		index = (int)(e - client_message);
        		char message[150];
			strncpy(message, client_message + index, strlen(client_message) - index);
			if ((strlen(client_message)-index) > sizeof(message)) {
				writeLog("Client message is longer than expected. [createSocket]", 1, 1);
				message[150] = '\0';
			}
        		parseClientMessage(message, params);
        		writeLog("Message received on socket.", 0, 0);
			int id = params[0];
			int aflags = params[1];
			if (use_ssl > 0)
				send_socket_message(NO_SOCKET, ssl, id, aflags);
			else
        			send_socket_message(client_socket, NULL, id, aflags);
			if (server_message != NULL)
				free(server_message);
			if (client_message != NULL)
				free(client_message);
			server_message = client_message = NULL;
			printf("Close client socket %i\n", pid);
			if (use_ssl > 0) {
				//SSL_shutdown(ssl);
				SSL_free(ssl);
				SSL_CTX_free(ctx);
			}
			close(client_socket);
        		return 0;
		}
		else if (pid > 0) {
			printf("Close client socket %i\n", pid);
			close(client_socket);
		}
		else {
			perror("Fork failed.");
			return -2;
		}
	}
	return 0;
}

void closeSocket() {
        writeLog("Closing socket.", 0, 0);
        shutdown(server_fd, SHUT_RDWR);
}

void closejsonfile() {
	const char bFolderName[7] = "backup";
	char ch = '/';
	char dot = '.';
        
	snprintf(dataFileName, datafilename_size, "%s%c%s", dataDir, ch, jsonFileName);


	if (saveOnExit == 0) {
		remove(dataFileName);
	}
	else {
		char date[13];
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
                strftime(date, sizeof(date), "%Y%m%d%H%M", t);
		snprintf(backupDirectory, backupdirectory_size, "%s%c%s", dataDir, ch, bFolderName);
		if (directoryExists(backupDirectory, 100) != 0) {
			int status = mkdir(trim(backupDirectory), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (status != 0 && errno != EEXIST) {
				printf("Failed to create backup directory. Errno: %d\n", errno);
				return;
			}
		}
		char bd[backupdirectory_size];
		char jfn[filename_size];
		memset(bd, 0, sizeof(bd));
		memset(jfn, 0, sizeof(jfn));
		strncpy(bd, backupDirectory, backupdirectory_size);
		strncpy(jfn, jsonFileName, filename_size);
		snprintf(newFileName, newfilename_size, "%s%c%s%c%s", bd, ch, jfn, dot, date);
		rename(dataFileName, newFileName);
	}	
}

void safe_free(void** ptr) {
	if (*ptr != NULL) {
		free(*ptr);
		*ptr = NULL;
	}
}

void safe_free_str(void* ptr) {
	if (ptr != NULL) {
		free(ptr);
		ptr = NULL;
	}
}

void free_kafka_vars() {
	if (kafkaexportreqs > 0) {
		free(kafka_brokers);
		if (kafka_topic != NULL) {
			free(kafka_topic);
			kafka_topic = NULL;
		}
		if (kafka_tag != NULL) { 
			free(kafka_tag);
			kafka_tag = NULL;
		}
		free(kafkaCACertificate);
		free(kafkaProducerCertificate);
		free(kafkaSSLKey);
		kafka_brokers = NULL;
		kafka_topic = NULL;
		kafka_tag = NULL;
		kafkaCACertificate = NULL;
		kafkaProducerCertificate = NULL;
		kafkaSSLKey = NULL;
	}
}

void free_constants() {
	safe_free_str(confDir);
	safe_free_str(dataDir);
	safe_free_str(storeDir);
	safe_free_str(logDir);
	safe_free_str(pluginDeclarationFile);
	safe_free_str(jsonFileName);
	safe_free_str(metricsFileName);
	safe_free_str(gardenerScript);
	safe_free_str(infostr);
	safe_free_str(pluginDir);
	safe_free_str(hostName);
	safe_free_str(fileName);
	safe_free_str(metricsOutputPrefix);
	//safe_free_str(logfile);
	safe_free_str(dataFileName);
	safe_free_str(backupDirectory);
	safe_free_str(newFileName);
	safe_free_str(gardenerRetString);
	safe_free_str(pluginCommand);
	safe_free_str(pluginReturnString);
	safe_free_str(storeName);
	safe_free_str(socket_message);
	safe_free_str(client_message);
	writeLog("All constants freed from memory.", 0, 0);
}

void free_structures(int numOfS) {
	for (int i = 0; i < numOfS; i++) {
		free(declarations[i].name);
		free(declarations[i].description);
		free(declarations[i].command);
		free(outputs[i].retString);
		declarations[i].name = NULL;
		declarations[i].description = NULL;
		declarations[i].command = NULL;
		outputs[i].retString = NULL;
	}
	if (scheduler != NULL) {
		free(scheduler);
	}
}

void freemem() {
	confDir = NULL;
	dataDir = NULL;
	storeDir = NULL;
	pluginDir = NULL;
	logDir = NULL;
	pluginDeclarationFile = NULL;
	hostName = NULL;
	fileName = NULL;
	jsonFileName = NULL;
	metricsFileName = NULL;
	gardenerScript = NULL;
	infostr = NULL;
	if (socket_message != NULL) {
		free(socket_message);
		socket_message = NULL;
	}
	/*if (logfile != NULL) {
		free(logfile);
		logfile = NULL;
	}*/
	dataFileName = NULL;
	backupDirectory = NULL;
	newFileName = NULL;
	gardenerRetString = NULL;
	pluginCommand = NULL;
	pluginReturnString = NULL;
	storeName = NULL;
        if (update_declarations != NULL) {
		for (int i = 0; i < update_declaration_size; i++) {
                	free(update_declarations[i].name);
                        free(update_declarations[i].description);
                        free(update_declarations[i].command);
			update_declarations[i].name = NULL;
			update_declarations[i].description = NULL;
			update_declarations[i].command = NULL;
                }
                free(update_declarations);
		update_declarations = NULL;
	}
	if (update_outputs != NULL) {
		for (int i=0; i < update_output_size-1; i++) {
			free(update_outputs[i].retString);
			update_outputs[i].retString = NULL;
		}
		free(update_outputs);
	}
	if (api_args != NULL) {
		free(api_args);
		api_args = NULL;
	}
}

void destroy_mutexes() {
	pthread_mutex_destroy(&mtx);
	pthread_mutex_destroy(&file_mutex);
	pthread_mutex_destroy(&update_mtx);
}

void sig_exit_app() {
	//safe_free_str(logmessage);
	//safe_free_str(logfile);
	if (fptr != NULL) {
		fclose(fptr);
        	fptr = NULL;
	}
	destroy_mutexes();
        fflush(stdout);
        fflush(stderr);
        printf("Exiting application.\n");
}

void sig_handler(int signal){
	int max_try = 60;
	int try_count = 0;
    	switch (signal) {
        	case SIGINT:
			is_stopping = 1;
			writeLog("Caught SIGINT, exiting program.", 0, 0);
			flushLog();
			closejsonfile();
			closeSocket();
			free_structures(decCount);
			free(declarations);
			free(outputs);
			free_kafka_vars();
			while (thread_counter > 0) {
                                writeLog("Waiting for threads to finish...", 0, 0);
                                fflush(fptr);
                                sleep(2);
                                printf("There are %i threads waiting to finish.\n", thread_counter);
				try_count++;
				if (try_count >= max_try) break;
                        }
			free_constants();
			writeLog("Almond says goodbye.", 0, 0);
			free(threadIds);
			freemem();
			sig_exit_app();
            		exit(0);
		case SIGKILL:
			is_stopping = 1;
			writeLog("Caught SIGKILL, exiting progam.", 0, 0);
			closejsonfile();
			closeSocket();
			free_structures(decCount);
			free(declarations);
			free(outputs);
			free_kafka_vars();
			while (thread_counter > 0) {
                                writeLog("Waiting for threads to finish...", 0, 0);
                                fflush(fptr);
                                sleep(2);
                                printf("There are %i threads waiting to finish.\n", thread_counter);
				try_count++;
				if (try_count >= max_try) break;
                        }
			free_constants();
			writeLog("Almond says goodbye.", 0, 0);
			printf("Let threads finsih...\n");
			free(threadIds);
			freemem();
			sig_exit_app();
			exit(0);
		case SIGTERM:
		case SIGSTOP:
			is_stopping = 1;
			printf("Caught signal to quit program.\n");
			closejsonfile();
			closeSocket();
                        writeLog("Caught signal to terminate program.", 0, 0);
			free_structures(decCount);
			free(declarations);
			free(outputs);
			free_kafka_vars();
			while (thread_counter > 0) {
				writeLog("Waiting for threads to finish...", 0, 0);
				fflush(fptr);
				sleep(2);
				printf("There are %i threads waiting to finish.\n", thread_counter);
				try_count++;
				if (try_count >= max_try)
					break;
			}
                        free(threadIds);
                        freemem();
			free_constants();
			writeLog("Almond says goodbye.", 0, 0);
			sig_exit_app();
			printf("Application is stopped.\n");
                        exit(0);
    	}
//	stop = 1;
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
	tPluginFile = file_stat.st_mtime;
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
      		perror("Error while opening the file.\n");
		writeLog("Error opening configuration file", 2, 1);
      		exit(EXIT_FAILURE);
   	}

	while ((read = getline(&line, &len, fp)) != -1) {
	   char * token = strtok(line, "=");
	   while (token != NULL)
	   {
		   if (index == 0)
		   {
			   strncpy(confName, token, sizeof(confName));
		   }
		   else
		   {
			   strncpy(confValue, token, sizeof(confValue));
		   }
		   token = strtok(NULL, "=");
		   index++;
		   if (index == 2) index = 0;
           }
	   if (strcmp(confName, "almond.api") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i >= 1) {
			   local_api = 1;
		   }
	   }
	   if (strcmp(confName, "almond.standalone") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i >= 1) {
			   writeLog("Almond will run standalone. No monitor data will be sent to HowRU.", 0, 1);
			   standalone = 1;
		   }
	   }
           if (strcmp(confName, "almond.port") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i >= 1) {
			   local_port = i;
		   }
		   else local_port = ALMOND_API_PORT;
		   if (local_api > 0) {
			   writeLog("Almond will enable local api.", 0, 1);
		   }
	   }
	   if (strcmp(confName, "scheduler.confDir") == 0) {
		   confDir = malloc((size_t)50 * sizeof(char));
		   if (confDir != NULL)
		   	   memset(confDir, '\0', 50 * sizeof(char));
		   if (directoryExists(confValue, 255) == 0) {
			   strncpy(confDir,trim(confValue), strlen(confValue));
			   confDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			   if(status != 0 && errno != EEXIST){
                               printf("Failed to create directory. Errno: %d\n", errno);
			       writeLog("Error creating configuration directory.", 2, 1);
                           }
			   else{
			       strncpy(confDir, trim(confValue), strlen(confValue));
			       confDirSet = 1;
			   }
		  }
		  writeLog("Configuration directory is set.", 0, 1);
	   }
	   if (strcmp(confName, "scheduler.quickStart") == 0) {
                   int i = strtol(trim(confValue), NULL, 0);
                   if (i >= 1) {
                           writeLog("Almond scheduler have quick start activated.", 0, 1);
                           quick_start = 1;
                   }
           }
	   if ((strcmp(confName, "scheduler.useTLS") == 0) || (strcmp(confName, "almond.useSSL") == 0)) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i >= 1) {
			   writeLog("Almond scheduler use TLS encryption.", 0, 1);
			   use_ssl = 1;
		   }
	   }
	   if ((strcmp(confName, "scheduler.certificate") == 0) || (strcmp(confName, "almond.certificate") == 0)) {
                   almondCertificate = malloc((size_t)strlen(confValue)+1);
                   if (almondCertificate == NULL) {
                           fprintf(stderr, "Failed to allocate memory [almondCertificate].\n");
                           writeLog("Failed to allocate memory [almondCertificate]", 2, 1);
                           return 2;
                   }
                   strncpy(almondCertificate, trim(confValue), strlen(confValue));
                   if (use_ssl > 0) {
                        writeLog("Certificate for Almond is not provided. Almond API will not run with TLS encryption.", 1, 1);
                        use_ssl = 0;
                   }
           }
           if ((strcmp(confName, "scheduler.key") == 0) || (strcmp(confName, "almond.key") == 0)) {
                   almondKey = malloc((size_t)strlen(confValue)+1);
                   if (almondKey == NULL) {
                           fprintf(stderr, "Failed to allocate memory [almondSSLKey].\n");
                           writeLog("Failed to allocate memory [almondSSLKey]", 2, 1);
                           return 2;
                   }
                   strcpy(almondKey, trim(confValue));
                   if (use_ssl > 0) {
                        writeLog("No SSL key for Almond certificate provided. Almond API will not run with SSL encryption.", 1, 1);
                        use_ssl = 0;
                   }
           }
	   if (strcmp(confName, "scheduler.format") == 0) {
           	if (strcmp(trim(confValue), "json") == 0){
			printf ("Export to json\n");
		      	output_type= JSON_OUTPUT;
	      	}
	      	else if (strcmp(trim(confValue), "metrics") == 0) {
			printf ("Export to metrics file\n");
		      	output_type = METRICS_OUTPUT;
	      	}
	      	else if (strcmp(trim(confValue), "jsonmetrics") == 0) {
			printf ("Export both to json and metrics file.\n");
		      	writeLog("Exporting both to json and to metrics file.", 0, 1);
		      	output_type = JSON_AND_METRICS_OUTPUT;
	      	}
	      	else if (strcmp(trim(confValue), "prometheus") == 0) {
			printf("Export to prometheus.\n");
		      	writeLog("Export to prometheus style metrics.", 0, 1);
		      	output_type = PROMETHEUS_OUTPUT;
	      	}
	      	else if (strcmp(trim(confValue), "jsonprometheus") == 0) {
                      	printf("Export to both json and Prometheus style metrics.\n");
                      	writeLog("Exporting to both json and prometheus style metrics.", 0, 1);
                      	output_type = JSON_AND_PROMETHEUS_OUTPUT;
              	}
	      	else {
		      	printf("%s is not a valid value.  supported at this moment.\n", confValue);
		      	writeLog("Unsupported value in configuration scheduler.format.", 1, 1);
		      	writeLog("Using standard output (JSON_OUTPUT).", 0, 1);
		      	output_type = JSON_OUTPUT;
	   	}
	   }
	   if (strcmp(confName, "scheduler.initSleepMs") == 0) {
              int i = strtol(trim(confValue), NULL, 0);
	      if (i < 5000)
		      i = 7000;
	      initSleep = i;
	      writeLog("Init sleep for scheduler read.", 0, 1);
	   }
	   if (strcmp(confName, "scheduler.type") == 0) {
		   if (strcmp(trim(confValue), "time") == 0){
			   timeScheduler = 1;
			   writeLog("Almond will use a time scheduler.", 0, 1);
		   }
		   else {
			   writeLog("Almond will useclassic scheduler.", 0, 1);
		   }
	   }
	   if (strcmp(confName, "scheduler.sleepMs") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i < 2000)
			   i = 2000;
                   snprintf(infostr, infostr_size, "Scheduler sleep time is %d ms.", i);
		   writeLog(trim(infostr), 0, 1);
		   schedulerSleep = i;
	   }
	   if (strcmp(confName, "scheduler.dataDir") == 0) {
		   if (directoryExists(confValue, 255) == 0) {
			   strncpy(dataDir, trim(confValue), strlen(confValue));
			   dataDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
			   if (status != 0 && errno != EEXIST) {
				   printf("Failed to create directory. Errno: %d\n", errno);
				   writeLog("Error creating HowRU dataDir.", 2, 1);
			   }
			   else {
				   strncpy(dataDir, trim(confValue), strlen(confValue));
				   dataDirSet = 1;
			   }
		   }
	   }
	   if (strcmp(confName, "scheduler.storeDir") == 0) {
                   if (directoryExists(confValue, 255) == 0) {
			   strncpy(storeDir, trim(confValue), storedir_size);
                           storeDirSet = 1;
                   }
                   else {
                           int status = mkdir(trim(confValue), 0755);
                           if (status != 0 && errno != EEXIST) {
                                   printf("Failed to create directory. Errno: %d\n", errno);
                                   writeLog("Error creating HowRU storeDir.", 2, 1);
                           }
                           else {
                                   strncpy(storeDir, trim(confValue), strlen(confValue));
                                   storeDirSet = 1;
                           }
                   }
           }
	   if (strcmp(confName, "scheduler.logToStdout") == 0) {
 		   printf("Found logToStdout\n");
 		   dockerLog = atoi(confValue);
 	   }
	   if (strcmp(confName, "scheduler.logDir") == 0) {
		   if (directoryExists(confValue, 255) == 0) {
			   strncpy(logDir, trim(confValue), strlen(confValue));
			   logDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
			   if (status != 0 && errno != EEXIST) {
				   printf("Failed to create directory. Errno: %d\n", errno);
				   writeLog("Error creating log directory.", 2, 1);
			   }
			   else {
				   strncpy(logDir, trim(confValue), strlen(confValue));
				   logDirSet = 1;
			   }
		   }
		   if (strcmp(confValue, "/var/log/almond") != 0) {
			   char ch =  '/';
			   FILE *logFile;
			   strcpy(fileName, logDir);
        		   strncat(fileName, &ch, 1);
                           strcat(fileName, "almond.log");
			   writeLog("Closing logfile...", 0, 1);
			   fclose(fptr);
			   fptr = NULL;
			   sleep(0.2);
                           logFile = fopen("/var/log/almond/almond.log", "r");
			   fptr = fopen(fileName, "a");
			   if (fptr == NULL) {
				   fclose(logFile);
				   logFile = NULL;
				   fptr = fopen("/var/log/almond/almond.log", "a");
				   writeLog("Could not create new logfile.", 1, 1);
				   writeLog("Reopened logfile '/var/log/almond/almond.log'.", 0, 1);
				   strcpy(logfile, "/var/log/almond/almond.log");
			   }
			   else {
				   while ( (ch = fgetc(logFile)) != EOF)
					   fputc(ch, fptr);
				   fclose(logFile);
				   logFile = NULL;
				   writeLog("Created new logfile.", 0, 1);
				   strcpy(logfile, fileName);
			   }
		   }
		   else {
			   strcpy(logfile, "/var/log/almond/almond.log");
		   }

	   }
	   if (strcmp(confName, "scheduler.logPluginOutput") == 0) {
	   	if (atoi(confValue) == 0) {
			writeLog("Plugin outputs will not be written in the log file", 0, 1);
		}
		else {
			writeLog("Plugin outputs will be written to the log file", 0, 1);
			logPluginOutput = 1;
		}
	   }
           if (strcmp(confName, "scheduler.storeResults") == 0) {
		   if (atoi(confValue) == 0) {
			   writeLog("Plugin results is not stored in specific csv file.", 0, 1);
		   }
		   else {
			   writeLog("Plugin results will be stored in csv file.", 0, 1);
			   pluginResultToFile = 1;
		   }
	   }
	   if (strcmp(confName, "scheduler.hostName") == 0) {
		  strncpy(hostName, trim(confValue), strlen(confValue));
		  snprintf(infostr, infostr_size, "Scheduler will name this host: %s", hostName);
		  writeLog(trim(infostr), 0, 1);
	   }
	   if (strcmp(confName, "plugins.directory") == 0) {
		   if (directoryExists(confValue, 255) == 0) {
			   strcpy(pluginDir, trim(confValue));
			   pluginDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
			   if (status != 0 && errno != EEXIST) {
				   printf("Failed to create directory. Errno: %d\n", errno);
				   writeLog("Error creating plugins directory.", 2, 1);
			   }
			   else {
				   strncpy(pluginDir, trim(confValue), strlen(confValue));
				   pluginDirSet = 1;
			   }
		   }
	   }
	   if (strcmp(confName, "plugins.declaration") == 0) {
		   if (access(trim(confValue), F_OK) == 0){
			   strncpy(pluginDeclarationFile, trim(confValue), strlen(confValue));
		   }
		   else {
			   printf("ERROR: Plugin declaration file does not exist.");
			   writeLog("Plugin declaration file does not exist.", 2, 1);
			   return 1;
		   }
	   }
	   if (strcmp(confName, "scheduler.enableGardener") == 0) {
		   if (atoi(confValue) == 0) {
                           writeLog("Metrics gardener is not enabled.", 0, 1);
                   }
                   else {
                           writeLog("Metrics gardener is enabled.", 0, 1);
                           enableGardener = 1;
                   }
           }
	   if (strcmp(confName, "scheduler.gardenerScript") == 0) {
                   if (access(trim(confValue), F_OK) == 0){
                        strncpy(gardenerScript, trim(confValue), strlen(confValue));
                   }
                   else {
                        enableGardener = 0;
                        writeLog("Gardener script file could not be found", 1, 1);
                        writeLog("Metrics gardener is disabled.", 2, 1);
                   }
           }
	   if (strcmp(confName, "scheduler.enableClearDataCache") == 0) {
                   if (atoi(confValue) == 0) {
                           writeLog("Clear data cache is not enabled.", 0, 1);
                   }
                   else {
                           writeLog("Clear data cache is enabled.", 0, 1);
                           enableClearDataCache = 1;
                   }
           }
	   if (strcmp(confName, "scheduler.enableKafkaExport") == 0) {
		   if (atoi(confValue) == 0) {
			   writeLog("Export to Kafka is not enabled.", 0, 1);
		   }
		   else {
			   writeLog("Exporting results to Kafka is enabled.", 0, 1);
			   enableKafkaExport = 1;
		   }
	   }
           if (strcmp(confName, "scheduler.enableKafkaTag") == 0) {
                   if (atoi(confValue) == 0) {
                           writeLog("Use of tag to Kafka message is not enabled.", 0, 1);
                   }
                   else {
                           writeLog("Use of tag to Kafka message is enabled.", 0, 1);
                           enableKafkaTag = 1;
                  }
           }
	   if (strcmp(confName, "scheduler.enableKafkaId") == 0) {
                   if (atoi(confValue) == 0) {
                           writeLog("Use of Kafka id is not enabled.", 0, 1);
                   }
                   else {
                           writeLog("Use of Kafka id is enabled.", 0, 1);
                           enableKafkaId = 1;
                  }
           }
	   if (strcmp(confName, "scheduler.kafkaStartId") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i > 0) {
			   kafka_start_id = i;
			   writeLog("Kafka start id check ok", 0, 1);
		   }
		   else {
			   writeLog("Could not read kafka_start_id.", 1, 1);
			   kafka_start_id = 0;
		   }
	   }
	   if (strcmp(confName, "scheduler.kafkaBrokers") == 0) {
		   kafkaexportreqs++;
		   kafka_brokers = malloc((size_t)strlen(confValue)+1);
		   if (kafka_brokers == NULL) {
			   fprintf(stderr, "Failed to allocate memory for kafka brokers.\n");
			   writeLog("Failed to allocate memory [kafka_brokers]", 2, 1);
			   return 2;
		   }
		   else
			   memset(kafka_brokers, '\0', (size_t)(strlen(confValue)+1) * sizeof(char));
		   strncpy(kafka_brokers, trim(confValue), strlen(confValue));
		   snprintf(infostr, infostr_size, "Kafka export brokers is set to '%s'", kafka_brokers);
		   writeLog(trim(infostr), 0, 1);
	   }
	   if (strcmp(confName, "scheduler.kafkaTopic") == 0) {
		   kafkaexportreqs++;
		   kafka_topic = malloc((size_t)strlen(confValue)+1);
		   if (kafka_topic == NULL) {
			   fprintf(stderr, "Failed to allocate memory [kafka_topic].\n");
			   writeLog("Failed to allocate memory [kafka_topic]", 2, 1);
			   return 2;
		   }
		   else
			   memset(kafka_topic, '\0', (size_t)(strlen(confValue)+1) * sizeof(char));
		   strncpy(kafka_topic, trim(confValue), strlen(confValue));
		   snprintf(infostr, infostr_size, "Kafka export topic is set to '%s'", kafka_topic);
		   writeLog(trim(infostr), 0, 1);
	   }
           if (strcmp(confName, "scheduler.kafkaTag") == 0) {
                  kafka_tag = malloc((size_t)strlen(confValue)+1);
		  if (kafka_tag == NULL) {
		  	fprintf(stderr, "Failed to allocate memory [kafka_tag].\n");
			writeLog("Failed to allocate memory [kafka_tag]", 2, 1);
			return 2;
		  }
                  strncpy(kafka_tag, trim(confValue), strlen(confValue));
                  snprintf(infostr, infostr_size, "Kafka tag is set to '%s'", kafka_tag);
                  writeLog(trim(infostr), 0, 1);
           }
	   if (strcmp(confName, "scheduler.enableKafkaSSL") == 0) {
		   if (atoi(confValue) == 0) {
			   writeLog("Kafka producer will connect with plain text", 0, 1);
                   }
                   else {
                           writeLog("Kafka producer will connect to cluster with SSL.", 0, 1);
                           writeLog("Make sure you use a certificate with accordance to Kafka ACL list.", 0, 1);
                           enableKafkaSSL = 1;
                  }
           }
           if (strcmp(confName, "scheduler.kafkaCACertificate") == 0) {
		   kafkaCACertificate = malloc((size_t)strlen(confValue)+1);
		   if (kafkaCACertificate == NULL) {
			   fprintf(stderr, "Failed to allocate memory [kafkaCACertificate].\n");
			   writeLog("Failed to allocate memory [kafkaCACertificate]", 2, 1);
			   return 2;
		   }
		   strncpy(kafkaCACertificate, trim(confValue), strlen(confValue));
		   if (enableKafkaExport > 0) {
                        writeLog("Kafka CACertificate not provided. Will try to connect to Kafka in plain text.", 1, 1);
			enableKafkaSSL = 0;
                   }
	   }
           if (strcmp(confName, "scheduler.kafkaProducerCertificate") == 0) {
		   kafkaProducerCertificate = malloc((size_t)strlen(confValue)+1);
		   if (kafkaProducerCertificate == NULL) {
			   fprintf(stderr, "Failed to allocate memory [kafkaProducerCertificate].\n");
			   writeLog("Failed to allocate memory [kafkaProducerPertificate", 2, 1);
			   return 2;
		   }
		   strncpy(kafkaProducerCertificate, trim(confValue), strlen(confValue));
		   if (enableKafkaExport > 0){
                   	writeLog("Certificate for Almond Kafka Producer functionality not provided. Will try to connect to Kafka in plain text.", 1, 1);
                   	enableKafkaSSL = 0;
	   	}
	   }
           if (strcmp(confName, "scheduler.kafkaSSLKey") == 0) {
		   kafkaSSLKey = malloc((size_t)strlen(confValue)+1);
		   if (kafkaSSLKey == NULL) {
			   fprintf(stderr, "Failed to allocate memory [kafkaSSLKey].\n");
			   writeLog("Failed to allocate memory [kafkaSSLKey]", 2, 1);
			   return 2;
		   }
		   strcpy(kafkaSSLKey, trim(confValue));
		   if (enableKafkaExport > 0) {
           		writeLog("No SSL key for Kafka producer certificate provided. Will try to connect to Kafka in plain text.", 1, 1);
                   	enableKafkaSSL = 0;
		   }
	   }
	   if (strcmp(confName, "scheduler.gardenerRunInterval") == 0) {
                   int i = strtol(trim(confValue), NULL, 0);
                   if (i < 60)
                           i = 43200;
                   snprintf(infostr, infostr_size, "Gardener run interval is %d seconds.", i);
                   writeLog(trim(infostr), 0, 1);
                   gardenerInterval = i;
           }
	   if (strcmp(confName, "scheduler.clearDataCacheInterval") == 0) {
                   int i = strtol(trim(confValue), NULL, 0);
                   if (i < 60)
                           i = 300;
                   snprintf(infostr, infostr_size, "Clear data cache is %d seconds.", i);
                   writeLog(trim(infostr), 0, 1);
                   clearDataCacheInterval = i;
           }
  	   if (strcmp(confName, "scheduler.dataCacheTimeFrame") == 0) {
                   int i = strtol(trim(confValue), NULL, 0);
                   if (i < 180)
                           i = 330;
                   snprintf(infostr, infostr_size, "Data cache time frame is set to %d seconds.", i);
                   writeLog(trim(infostr), 0, 1);
                   dataCacheTimeFrame = i;
           }
	   if (strcmp(confName, "scheduler.tuneTimer") == 0) {
		   if (atoi(confValue) == 0) {
			   writeLog("Timer tuner is not enabled.", 0, 1);
		   }
		   else {
			   writeLog("Timer tuner is enabled.", 0, 1);
			   enableTimeTuner = 1;
		   }
	   }
	   if (strcmp(confName, "scheduler.tunerCycle") == 0) {
		   int i = strtol(trim(confValue), NULL, 15);
                   snprintf(infostr, infostr_size, "Time tuner cycle is set to %d.", i);
		   writeLog(trim(infostr), 0, 1);
		   timeTunerCycle = i;
	   }
           if (strcmp(confName, "scheduler.tuneMaster") == 0) {
                   int i = strtol(trim(confValue), NULL, 1);
                   snprintf(infostr, infostr_size, "Time tuner cycle is set to %d.", i);
                   writeLog(trim(infostr), 0, 1);
                   timeTunerMaster = i;
           } 
	   if (strcmp(confName, "data.jsonFile") == 0) {
		   strncpy(jsonFileName, trim(confValue), strlen(confValue));
		   jsonFileName[strlen(confValue)] = '\0';
		   snprintf(infostr, infostr_size, "Json data will be collected in file: %s.", jsonFileName);
		   writeLog(trim(infostr), 0, 1);
	   }
	   if (strcmp(confName, "data.metricsFile") == 0) {
		strncpy(metricsFileName, trim(confValue), strlen(confValue));
		snprintf(infostr, infostr_size, "Metrics will be collected in file: %s", metricsFileName);
		writeLog(trim(infostr), 0, 1);
	   }
	   if (strcmp(confName, "data.metricsOutputPrefix") == 0) {
                   if ((int)strlen(confValue) <= 30) {
                        strncpy(metricsOutputPrefix, trim(confValue), strlen(confValue));
                        snprintf(infostr, infostr_size, "Metrics output prefix is set to '%s'", metricsOutputPrefix);
                        writeLog(trim(infostr), 0, 1);
                   }
                   else {
                        writeLog("Could not change metricsOutputPrefix. Prefix too long.", 1, 1);
                   }
           }
	   if (strcmp(confName, "data.saveOnExit") == 0) {
		if (atoi(confValue) == 0) {
			writeLog("Json data will be deleted on shutdown.", 0, 1);
		}
		else {
			writeLog("Data file will be saved in data directory after shutdown.", 0, 1);
			saveOnExit = 1;
		}
	   }
 	}

	updateInterval = 60;
	if ((kafkaexportreqs < 2) && (enableKafkaExport > 0)) {
		writeLog("Not sufficient configuration to export to Kafka. Brokers and or topic is unknown.", 1, 1);
               	writeLog("Kafka export is not enabled.", 0, 1);
               	enableKafkaExport = 0;
     	}

   	fclose(fp);
	fp = NULL;
   	if (line){
        	free(line);
		line = NULL;
	}
   	return 0;
}

void apiDryRun(int plugin_id) {
	char* pluginName = NULL;
	char* message = NULL;
        char retString[2280];
        char ch = '/';
        PluginOutput output;
        int rc = 0;
	FILE *fp = NULL;
	
	output.retString = malloc((size_t)pluginoutput_size);
	message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
	if (message == NULL) {
		writeLog("Failed to allocate memory for api message", 1, 0);
		return;
	}
	else
		memset(message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
	pluginName = malloc((size_t)(pluginitemname_size + 1) * sizeof(char));
	if (pluginName == NULL) {
        	fprintf(stderr, "Failed to allocate memory in apiDryRun.\n");
                writeLog("Failed to allocate memory [apiDryRun: pluginName", 2, 0);
                return;
        }
	else
		memset(pluginName, '\0', (size_t)(pluginitemname_size + 1) * sizeof(char));
        strncpy(pluginName, declarations[plugin_id].name, pluginitemname_size);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
        strcpy(message, "{\n     \"dryExecutePlugin\":\"");
        strcat(message, pluginName);
        strcat(message, "\"");
        strcat(message, ",\n");
        strcpy(pluginCommand, pluginDir);
        strncat(pluginCommand, &ch, 1);
        strcat(pluginCommand, declarations[plugin_id].command);
        snprintf(infostr, infostr_size, "Running: %s.", declarations[plugin_id].command);
        writeLog(trim(infostr), 0, 0);
        fp = popen(pluginCommand, "r");
        if (fp == NULL) {
                printf("Failed to run command\n");
                writeLog("Failed to run command.", 2, 0);
        }
        while (fgets(retString, sizeof(retString), fp) != NULL) {
                // VERBOSE  printf("%s", retString);
        }
        rc = pclose(fp);
        if (rc > 0)
        {
                if (rc == 256)
                        output.retCode = 1;
                else if (rc == 512)
                        output.retCode = 2;
                else
                        output.retCode = rc;
        }
        else
                output.retCode = rc;
        strncpy(output.retString, trim(retString), strlen(retString));
        strcat(message, "     \"pluginOutput:\":\"");
	strcat(message, trim(output.retString));
        strcat(message, "\"");
	strcat(message, "\n}\n");
	socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
	if (socket_message == NULL) {
        	fprintf(stderr, "Failed to allocate memory for socket messages.\n");
                writeLog("Failed to allocate memory [apiDryRun:socket message]", 2, 0);
                return;
	}
	else
		memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
        strncpy(socket_message, message, apimessage_size);
	free(pluginName);
	pluginName = NULL;
	free(output.retString);
	output.retString = NULL;
	free(message);
	message = NULL;
}

void apiRunPlugin(int plugin_id, int flags) {
	char* pluginName = NULL;
	char* message = NULL;
	int waitCount = 0;

	message = (char *) malloc(sizeof(char) * (apimessage_size+1));
	if (message == NULL) {
		writeLog("Failed to allocate memory for api message", 1, 0);
		return;
	}
	else
		memset(message, '\0', (size_t)(apimessage_size+1) * sizeof(char));
	pluginName = malloc((size_t)(pluginitemname_size + 1) * sizeof(char));
	if (pluginName == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiRunPlugin.\n");
                writeLog("Failed to allocate memory [apiRunPlugin: pluginName]", 2, 0);
                return;
        }
	else
		memset(pluginName, '\0', (size_t)(pluginitemname_size+1) * sizeof(char));
	pluginName = strdup(declarations[plugin_id].name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
	// Check if same plugin is running in thread, in which case wait...
	while (threadIds[(short)plugin_id] > 0) {
		writeLog("Waiting for thread to finish...", 0, 0);
		sleep(1);
		waitCount++;
		if (waitCount > 10) {
			writeLog("Reached waitCount threshold. Continue.", 1, 0);
			break;
		}
	}
	char p_id[3];
	snprintf(p_id, sizeof(p_id), "%i",plugin_id);
	setApiCmdFile("execute", p_id);
	strcpy(message, "{\n     \"executePlugin\":\"");
	strcat(message, pluginName);
	strcat(message, "\"");
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message, ",\n");
		sleep(10);
		strcat(message, "     \"pluginOutput:\":\"");
		strcat(message, trim(outputs[plugin_id].retString));
		strcat(message, "\"");
        }
	strcat(message, "\n}\n");
	socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
	if (socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiRunPlugin.\n");
                writeLog("Failed to allocate memory [apiRunPlugin: socket_message]", 2, 0);
                return;
        }
	else
		memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
	if (strlen(message) > apimessage_size) {
		printf("DEBUG: Message is larger than size.\n");
		message[apimessage_size-1] = '\0';
	}
	strncpy(socket_message, message, (size_t)apimessage_size);
	free(pluginName);
	pluginName = NULL;
	if (message != NULL) {
		free(message);
		message = NULL;
	}	
}

char* createRunArgsStr(int num, const char* str) {
	char int_str[3];
	sprintf(int_str, "%d", num);
	size_t tot_len = strlen(str) + strlen(int_str) + 1;
	char* result = malloc(tot_len * sizeof(char));
	if (result == NULL) {
		return NULL;
	}
	strcpy(result, int_str);
	strcat(result, ";");
	strcat(result, str);
	return result;
}

void runPluginArgs(int id, int aflags, int api_action) {
	//const char space[1] = " ";
	char* command = NULL;
	char* newcmd = NULL;
	char* pluginName = NULL;
	char* runArgsStr = NULL;
	FILE *fp = NULL;
        char ch = '/';
        PluginOutput output;
        char currTime[22];
	char rCode[3];
        int rc = 0;
	char* message = NULL;

	// TODO Validate args
	message = (char *) malloc(sizeof(char) * (apimessage_size+1));
	if (message == NULL) {
		fprintf(stderr, "Failed to allocate memory in [runPluginArgs:message].\n");
		writeLog("Failed to allocate memory in [runPluginArgs:message].", 2, 0);
		return;
	}
	else
		memset(message, '\0', (size_t)(apimessage_size+1) * sizeof(char));
	newcmd = malloc(200);
	if (newcmd == NULL) {
		fprintf(stderr, "Failed to allocate memory in runPluginArgs.\n");
		writeLog("Failed to allocate memory [runPluginArgs: newcmd]", 2, 0);
		return;
	}
	else
		memset(newcmd, '\0', 200);
	command = malloc((size_t)(pluginitemcmd_size + 1) * sizeof(char));
	if (command == NULL) {
                fprintf(stderr, "Failed to allocate memory in runPluginArgs.\n");
                writeLog("Failed to allocate memory [runPluginArgs: command]", 2, 0);
                return;
        }
	else
		memset(command, '\0', (size_t)(pluginitemcmd_size + 1) * sizeof(char));
	pluginName = malloc((size_t)(pluginitemname_size + 1) * sizeof(char));
	if (pluginName == NULL) {
                fprintf(stderr, "Failed to allocate memory in runPluginArgs.\n");
                writeLog("Failed to allocate memory [runPluginArgs: pluginName]", 2, 0);
                return;
        }
	else
		memset(pluginName, '\0', (size_t)(pluginitemname_size + 1) * sizeof(char));
	output.retString = malloc((size_t)(pluginoutput_size + 1) * sizeof(char));
	if (output.retString == NULL) {
		fprintf(stderr, "Failed to allocate memory for plugin output string.\n");
		writeLog("Failed to allocate memory [runPluginArgs:output.retString).", 2, 0);
		return;
	}
	else
		memset(output.retString, '\0', (size_t)(pluginoutput_size + 1) * sizeof(char));
        strncpy(pluginName, declarations[id].name, pluginitemname_size);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
	strcpy(command, declarations[id].command);
        strcpy(newcmd, pluginDir);
        strncat(newcmd, &ch, 1);
	char * token = strtok(command, " ");
	strcat(newcmd, token);
	strcat(newcmd, " ");
	strcat(newcmd, api_args);
	runArgsStr = createRunArgsStr(id, newcmd);
	if (runArgsStr != NULL) {
		setApiCmdFile("executeargs", runArgsStr);
		free(runArgsStr);
	}
	else {
		fprintf(stderr, "Failed to allocate memory for execute arguments command file.\n");
		writeLog("Failed to allocate memory [setAPiCmdsFile: executeargs].", 2, 0);
	}
	fp = popen(newcmd, "r");
        if (fp == NULL) {
                printf("Failed to run command\n");
                writeLog("Failed to run command.", 2, 0);
		strcpy(message, "\n{ \"failedToRun\":\"");
	 	strcat(message, newcmd);
		strcat(message, "\"}");
		socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
		if (socket_message == NULL) {
                	fprintf(stderr, "Failed to allocate memory in runPluginArgs.\n");
                	writeLog("Failed to allocate memory [runPluginArgs: socketmessage]", 2, 0);
                	return;
        	}
		else
			memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
        	strncpy(socket_message, message, apimessage_size);
		free(api_args);
		free(command);
		memset(&newcmd[0], 0, sizeof(*newcmd));
		free(newcmd);
		free(pluginName);
		api_args = NULL;
		command = NULL;
		pluginName = NULL;
		return;
        }
	while (fgets(pluginReturnString, pluginmessage_size, fp) != NULL) {
                // VERBOSE  printf("%s", pluginReturnString);
        }
        rc = pclose(fp);
        if (rc > 0)
        {
                if (rc == 256)
                        output.retCode = 1;
                else if (rc == 512)
                        output.retCode = 2;
                else
                        output.retCode = rc;
        }
        else
                output.retCode = rc;
        strcpy(output.retString, trim(pluginReturnString));
	size_t dest_size = 20;
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        
        if (api_action == API_DRY_RUN)
		strcpy(message, "{\n     \"dryExecutePlugin\":\"");
	else {
                if (output.retCode != outputs[id].retCode){
                	strcpy(declarations[id].statusChanged, "1");
                	strcpy(declarations[id].lastChangeTimestamp, currTime);
		}
                else {
                	strcpy(declarations[id].statusChanged, "0");
                }
		strcpy(message, "{\n     \"executePlugin\":\"");
		strcpy(declarations[id].lastRunTimestamp, currTime);
                time_t nextTime = t + (declarations[id].interval * 60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                snprintf(declarations[id].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
                declarations[id].nextRun = nextTime;
		if (timeScheduler == 1) {
			scheduler[declarations[id].id].timestamp = nextTime;
			rescheduleChecks();
		}
                output.prevRetCode = output.retCode;
                outputs[id] = output;
	}
        strcat(message, pluginName);
        strcat(message, "\",\n");
        strcat(message, "      \"result\": {\n");
        if (aflags == API_FLAGS_VERBOSE || aflags == API_DRY_RUN) {
                strcat(message, "          \"name\":\"");
                strcat(message, pluginName);
                free(pluginName);
		pluginName = NULL;
                strcat(message, "\",\n");
                strcat(message, "          \"description\":\"");
                strcat(message, declarations[id].description);
                strcat(message, "\",\n");
                switch (output.retCode) {
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
                sprintf(rCode, "%d", output.retCode);
                strcat(message, trim(rCode));
                strcat(message,  "\",\n");
                strcat(message, "          \"pluginOutput\":\"");
                strcat(message, trim(output.retString));
                strcat(message, "\",\n");
		if (aflags == API_FLAGS_VERBOSE) {
                	strcat(message, "          \"pluginStatusChanged\":\"");
                	strcat(message, declarations[id].statusChanged);
                	strcat(message, "\",\n");
                	strcat(message, "          \"lastChange\":\"");
                	strcat(message, declarations[id].lastChangeTimestamp);
                	strcat(message, "\",\n");
		}
                strcat(message, "          \"lastRun\":\"");
                strcat(message, currTime);
                strcat(message, "\",\n");
		if (aflags == API_FLAGS_VERBOSE) {
                	strcat(message, "          \"nextScheduledRun\":\"");
                	strcat(message, declarations[id].nextRunTimestamp);
                	strcat(message, "\"\n     }\n");
		}
		else {
			strcat(message, "     }\n");
		}
        }
        else {
                strcat(message, "          \"returnString\":\"");
                strcat(message, trim(outputs[id].retString));
                strcat(message, "\"\n     }\n");
        }
        strcat(message, "}\n");
	socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
	if (socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		writeLog("Failed to allocate memory [runPluginArgs:socket_message]", 2, 0);
		return;
	}
	else
		memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
        strncpy(socket_message, message, (size_t)apimessage_size);
	free(api_args);
	api_args = NULL;
	free(command);
	command = NULL;
	memset(&newcmd[0], 0, sizeof(*newcmd));
	free(newcmd);
	newcmd = NULL;
	free(output.retString);
	output.retString = NULL;
	free(message);
	message = NULL;
}

void apiReadData(int plugin_id, int flags) {
	char* pluginName = NULL;
	char rCode[3];
	char* message = NULL;
	unsigned short is_error = 0;

	if (plugin_id < 0) {
		printf("Strange things happen...\n");
		return;
	}

	message = malloc((size_t)apimessage_size * sizeof(char)+1);
	if (message == NULL) {
		writeLog("Failed to allocate memory for api message.", 1, 0);
	}
	else
       		message[0] = '\0';
	pluginName = malloc((size_t)pluginitemname_size * sizeof(char)+1);
	if (pluginName == NULL) {
		fprintf(stderr, "Failed to allocate memory in apiReadData.\n");
		writeLog("Failed to allocate memory [apiReadData:pluginName]", 2, 0);
		return;
	}
	if (plugin_id == 0 && flags == 0) {
		printf("This is an invalid check.\n");
		is_error++;
	}
	if (plugin_id > decCount || flags > 100) {
		printf("This is an invalid check.\n");
		is_error++;
	}	
	if (is_error > 0) {
		strcat(message, "{\n     \"almond\":\"Invalid check - no such plugin or flag\"\n}\n");
		socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
                if (socket_message == NULL) {
                        fprintf(stderr, "Failed to allocate memory.\n");
                        writeLog("Failed to allocate memory in [apiReadData:socket_message]", 2, 0);
                        return;
                }
		else
			memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
                strcpy(socket_message, message);
                free(message);
                free(pluginName);
                message = pluginName = NULL;
		return;
	}
	pluginName = strdup(declarations[plugin_id].name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message,"{\n     \"name\":\"");
        	strcat(message, pluginName);
        	strcat(message, "\",\n");
		strcat(message, "     \"description\":\"");
	        strcat(message, declarations[plugin_id].description);
		strcat(message, "\",\n");
		switch (outputs[plugin_id].retCode) {
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
		sprintf(rCode, "%d", outputs[plugin_id].retCode); 
	   	strcat(message, trim(rCode));
		strcat(message,  "\",\n");
		strcat(message, "     \"pluginOutput\":\"");
		strcat(message, trim(outputs[plugin_id].retString));
		strcat(message, "\",\n");
		strcat(message, "     \"pluginStatusChanged\":\"");
		strcat(message, declarations[plugin_id].statusChanged);
		strcat(message, "\",\n");
		strcat(message, "     \"lastChange\":\"");
		strcat(message, declarations[plugin_id].lastChangeTimestamp);
		strcat(message, "\",\n");
		strcat(message, "     \"lastRun\":\"");
		strcat(message, declarations[plugin_id].lastRunTimestamp);
		strcat(message, "\",\n");
                strcat(message, "     \"nextScheduledRun\":\"");
		strcat(message, declarations[plugin_id].nextRunTimestamp);
		strcat(message, "\"\n");
	}
        else {
		strcat(message,"{\n     \"");
                strcat(message, pluginName);
                strcat(message, "\":\"");
                strcat(message, trim(outputs[plugin_id].retString));
                strcat(message, "\"\n");
	}
	strcat(message, "}\n");
	free(pluginName);
	pluginName = NULL;
	socket_message = malloc((size_t)(apimessage_size + 1) * sizeof(char));
	if (socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		writeLog("Failed to allocate memory in [apiReadData:socket_message]", 2, 0);
		return;
	}
	else
		memset(socket_message, '\0', (size_t)(apimessage_size + 1) * sizeof(char));
	strncpy(socket_message, message, (size_t)apimessage_size);
	free(message);
	message = NULL;
}

void createUpdateFile(struct PluginItem *item, struct PluginOutput *output, char name[3]) {
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

void apiRunAndRead(int plugin_id, int flags) {
	char* pluginName = NULL;
	char rCode[3];
	char strNum[3];
        char* message = NULL;
	unsigned short is_error = 0;
        
	message = malloc((size_t)apimessage_size+1 * sizeof(char));
	if (message == NULL) {
		writeLog("Could not allocate memory for apimessage", 2, 0);
		return;
	}
	else {
		memset(message, '\0', (size_t)apimessage_size+1 * sizeof(char));
	}
	if (plugin_id == 0 && flags == 0) {
                printf("This is an invalid check.\n");
                is_error++;
        }
        if (plugin_id > decCount || flags > 100) {
                printf("This is an invalid check.\n");
                is_error++;
        }
        if (is_error > 0) {
                strcat(message, "{\n     \"almond\":\"Invalid check - no such plugin or flag\"\n}\n");
                socket_message = malloc((size_t)strlen(message)+1);
                if (socket_message == NULL) {
                        fprintf(stderr, "Failed to allocate memory.\n");
                        writeLog("Failed to allocate memory in [apiReadData:socket_message]", 2, 0);
                        return;
                }
                strcpy(socket_message, message);
                free(message);
                free(pluginName);
                message = pluginName = NULL;
                return;
        }
        snprintf(strNum, sizeof(strNum), "%d", plugin_id);
        setApiCmdFile("update", strNum);
	pluginName = (char *)malloc((size_t)(pluginitemname_size+1) * sizeof(char));
	if (pluginName == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		writeLog("Failed to allocate memory [apiRunAndRead:pluginName]", 2, 0);
		return;
	}
	else
		memset(pluginName, '\0', (size_t)(pluginitemname_size+1) * sizeof(char));
        strncpy(pluginName, declarations[plugin_id].name, (size_t)pluginitemname_size);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
        runPlugin(plugin_id, 0);
	if (timeScheduler ==1)
		rescheduleChecks();
        createUpdateFile(&declarations[plugin_id], &outputs[plugin_id], strNum);
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
                strcat(message, declarations[plugin_id].description);
                strcat(message, "\",\n");
                switch (outputs[plugin_id].retCode) {
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
                sprintf(rCode, "%d", outputs[plugin_id].retCode);
                strcat(message, trim(rCode));
                strcat(message,  "\",\n");
                strcat(message, "          \"pluginOutput\":\"");
                strcat(message, trim(outputs[plugin_id].retString));
                strcat(message, "\",\n");
                strcat(message, "          \"pluginStatusChanged\":\"");
                strcat(message, declarations[plugin_id].statusChanged);
                strcat(message, "\",\n");
                strcat(message, "          \"lastChange\":\"");
                strcat(message, declarations[plugin_id].lastChangeTimestamp);
                strcat(message, "\",\n");
                strcat(message, "          \"lastRun\":\"");
                strcat(message, declarations[plugin_id].lastRunTimestamp);
                strcat(message, "\",\n");
                strcat(message, "          \"nextScheduledRun\":\"");
                strcat(message, declarations[plugin_id].nextRunTimestamp);
                strcat(message, "\"\n     }\n");
	}
	else {
		strcat(message, "          \"returnString\":\"");
		strcat(message, trim(outputs[plugin_id].retString));
		strcat(message, "\"\n     }\n");
	}
	strcat(message, "}\n");
	if (socket_message != NULL) {
		free(socket_message);
		socket_message = NULL;
	}
	socket_message = malloc((size_t)(apimessage_size+1) * sizeof(char)); 
	if (socket_message == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		writeLog("Failed to allocate memory [apiRunAndRead:socket_message]", 2, 0);
		return;
	}
	if (strlen(message) > apimessage_size) {
		printf("Message is to big. Try increase apimessage_size.\n");
		message[apimessage_size-1] = '\0';
	}
	strncpy(socket_message, message, (size_t)apimessage_size);
	if (pluginName != NULL) {
		free(pluginName);
		pluginName = NULL;
	}
	if (message != NULL) {
		free(message);
		message = NULL;
	}
}

void apiReadFile(char *fileName, int type) {
	FILE *f = NULL;
	char info[70];
	char * message = NULL;
        long length;
        int err = 0;

	f = fopen(fileName, "r");
        if (f) {
                fseek(f, 0, SEEK_END);
                length = ftell(f);
                fseek(f, 0, SEEK_SET);
                message = malloc((size_t)length);
		if (message == NULL) {
			writeLog("Failed to allocate memory [apiReadFile:message]", 2, 0);
			return;
		}
                if (message) {
                        fread(message, 1, length, f);
                }
                fclose(f);
		f = NULL;
        }
        else err++;

        if (message) {
		socket_message = malloc((size_t)length);
		if (socket_message == NULL) {
			fprintf(stderr,"Memory allocation failed.\n");
			writeLog("Failed to allocate memory [apiReadFile:socket_message]", 2, 0);
			return;
		}
		else
			memset(socket_message, '\0', (size_t)length);
                strncpy(socket_message, message, (size_t)length);
        }
        else err++;
        if (err > 0) {
		if (type == 2)
                	snprintf(info, 70, "{ \"return_info\":\"Could not read metrics file. No results found.\"}\n");
		else
			snprintf(info, 70, "{ \"return_info\":\"Could not read almond file. No results found.\"}\n");
                socket_message = malloc(71);
		if (socket_message == NULL) {
			writeLog("Failed to allocate memory in apiReadFile:err:socket_message", 2, 0);
			return;
		}
		memset(socket_message, '\0', 71);
                strcpy(socket_message, info);
        }
        free(message);
	message = NULL;
}

void apiGetMetrics() {
	char ch = '/';

	snprintf(storeName, storename_size, "%s%c%s", storeDir, ch, metricsFileName);
	apiReadFile(storeName, 2);
}

void apiGetHostName() {
	char nm[9];
	strcpy(nm, "hostname");
	constructSocketMessage(nm, hostName);
}

void apiCheckPluginConf() {
	int res = check_plugin_conf_file(pluginDeclarationFile);
	if (res == 0) {
		constructSocketMessage("pluginconfiguration", "true");
	}
	else
		constructSocketMessage("pluginconfiguration", "false");
}

void apiGetVars(int v) {
	switch (v) {
		case 1:
			if (kafka_tag == NULL)
                        	constructSocketMessage("kafkatag", "NULL");
                	else
                        	constructSocketMessage("kafkatag", kafka_tag);
			break;
		case 2:
			constructSocketMessage("metricsprefix", metricsOutputPrefix);
			break;
		case 3:
			constructSocketMessage("jsonfilename", jsonFileName);
			break;
		case 4:
			constructSocketMessage("metricsfilename", metricsFileName);
			break;
		case 5:
			if (kafka_topic == NULL)
                        	constructSocketMessage("kafkatopic", "NULL");
			else
                        	constructSocketMessage("kafkatopic", kafka_topic);
			break;
		case 6:
			int length = snprintf(NULL, 0, "%d", schedulerSleep);
			char* sleep_num = malloc(length + 1);
			snprintf(sleep_num, length + 1,  "%d", schedulerSleep);
			constructSocketMessage("schedulersleep", sleep_num);
			free(sleep_num);
			break;
		case 7:
			char soe_num[2];
			sprintf(soe_num, "%d", saveOnExit);
			constructSocketMessage("saveonexit", soe_num);
			break;
		case 8:
			char plo_num[2];
			sprintf(plo_num, "%d", logPluginOutput);
			constructSocketMessage("pluginoutput", plo_num);
			break;
		case 9:
			char s_kStartId[2];
			sprintf(s_kStartId, "%d", kafka_start_id);
			constructSocketMessage("kafkastartid", s_kStartId);
			break;
		case 10:
			char plts[14];
			sprintf(plts, "%ld", tPluginFile);
			constructSocketMessage("pluginslastchangets", plts);
			break;
		default:
			constructSocketMessage("getvar", "No matching object found");
	}
}

void apiReadAll() {
	char ch = '/';

	strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);
	apiReadFile(fileName, 0); 
}

void collectJsonData(int decLen){
	char ch = '/';
	char* pluginName = NULL;
	FILE *fp = NULL;
        clock_t t;

	if (fileName == NULL || dataDir == NULL) {
		printf("Variabels in collectJsonData is empty.\n");
		return;
	}
	pthread_mutex_lock(&update_mtx);
	strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);
	snprintf(infostr, infostr_size, "Collecting data to file: %s", fileName);
	writeLog(trim(infostr), 0, 0);
	t = clock();
	fp = fopen(fileName, "w");
	fputs("{\n", fp);
	fprintf(fp, "   \"host\": {\n");
	fprintf(fp, "      \"name\":\"");
	fputs(hostName, fp);
	fprintf(fp, "\"\n");
	fputs("   },\n", fp);
	fputs("   \"monitoring\": [\n", fp);
	for (int i = 0; i < decLen; i++) {
		pluginName = (char *)malloc((size_t)pluginitemname_size * sizeof(char)+1);
		if (pluginName == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			writeLog("Failed to allocate memory [collectJsonData:pluginName]", 2, 0);
			return;
		}
		pluginName = strdup(declarations[i].name);
		removeChar(pluginName, '[');
		removeChar(pluginName, ']');
		fputs("      {\n", fp);
		fprintf(fp, "         \"name\":\"%s\",\n", pluginName);
		free(pluginName);
		pluginName = NULL;
		fprintf(fp, "         \"pluginName\":\"%s\",\n", declarations[i].description);
		switch(outputs[i].retCode) {
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
		fprintf(fp, "         \"pluginStatusCode\":\"%d\",\n", outputs[i].retCode);
		fprintf(fp, "         \"pluginOutput\":\"%s\",\n", trim(outputs[i].retString));
		fprintf(fp, "         \"pluginStatusChanged\":\"%s\",\n", declarations[i].statusChanged);
		if (declarations[i].active > 0)
                        fputs("         \"maintenance\":\"false\",\n", fp);
                else
                        fputs("         \"maintenance\":\"true\",\n", fp);
		fprintf(fp, "         \"lastChange\":\"%s\",\n", declarations[i].lastChangeTimestamp);
		fprintf(fp, "         \"lastRun\":\"%s\", \n", declarations[i].lastRunTimestamp);
		fprintf(fp, "         \"nextRun\":\"%s\"\n", declarations[i].nextRunTimestamp);
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
	snprintf(infostr, infostr_size, "Data collection took %.0f miliseconds to execute.", (double)t);
	writeLog(trim(infostr), 0, 0);
	pthread_mutex_unlock(&update_mtx);
}

void collectMetrics(int decLen, int style) {
        char ch = '/';
	char* pluginName = NULL;
	char* serviceName = NULL;
	FILE *mf = NULL;
        clock_t t;
	char *p = NULL;
	int metricsValueLength = 0;

        t = clock();
	pthread_mutex_lock(&update_mtx);
        strncpy(storeName, storeDir, storedir_size);
        strncat(storeName, &ch, 1);
        strcat(storeName, metricsFileName);
        mf = fopen(storeName, "w");
	if (mf == NULL) {
		writeLog("Failed to open metrics file", 1, 0);
		fprintf(stderr, "Failed to open metrics file\n");
		return;
	}
        snprintf(infostr, infostr_size, "Collecting metrics to file: %s", storeName);
        writeLog(trim(infostr), 0, 0);
	for (int i = 0; i < decLen; i++) {
		pluginName = (char *)malloc((size_t)pluginitemname_size * sizeof(char)+1);
		memset(pluginName, '\0', pluginitemname_size+1 * sizeof(char));
		if (pluginName == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			writeLog("Memory allocation failed [collectMetrics:pluginName]", 2, 0);
			return;
		}
		pluginName = strdup(declarations[i].name);
        	removeChar(pluginName, '[');
        	removeChar(pluginName, ']');
		for (p = pluginName; *p != '\0'; ++p) {
			//if (*p == '/') *p = '_';
			*p = tolower(*p);
		}
        	// Get metrics
        	char *e;
		if (strchr(outputs[i].retString, '|') == NULL) {
			snprintf(infostr, infostr_size, "Plugin %s does not provide metrics. Using plain output.", pluginName);
			writeLog(trim(infostr), 1, 0);
			if (style == 0)
                       		fprintf(mf, "%s_%s{hostname=\"%s\",%s_result=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
			else { 
				// Get service name	
				serviceName = (char *)malloc((size_t)pluginitemdesc_size * sizeof(char));
				if (serviceName == NULL) {
					fprintf(stderr, "Failed to allocate memory.\n");
					writeLog("Failed to allocate memory [collectMetrics:serviceName]", 2, 0);
					return;
				}
				memset(serviceName, '\0', pluginitemdesc_size * sizeof(char));
				strcpy(serviceName, declarations[i].description);
				fprintf(mf, "%s_%s{hostname=\"%s\", service=\"%s\", value=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, serviceName, trim(outputs[i].retString), outputs[i].retCode);
				free(serviceName);
				serviceName = NULL;
			}
		}
                else {
        	 	e = strchr(outputs[i].retString, '|');
        	    	int position = (int)(e - outputs[i].retString);
			int len = pluginoutput_size;
			size_t srcSize = strlen(outputs[i].retString) - position;
			int sublen = (srcSize < len) ? srcSize : len;
			char * metrics = malloc((size_t)sizeof(char) * sublen);
			memset(metrics, 0, sizeof(char) * sublen);
			if (sublen <= srcSize) {
        			memcpy(metrics,&outputs[i].retString[position+1],sublen);
			}
			else {
				writeLog("Invalid memcpy operation: size exceeds buffer limit.", 1, 0);
				fprintf(stderr, "Size exceeds buffer [memcpy].\n");
			}
			if (style == 0)
				fprintf(mf, "%s_%s{hostname=\"%s\", %s_result=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
			else {
				serviceName = (char *)malloc((size_t)pluginitemdesc_size * sizeof(char));
				if (serviceName == NULL) {
					fprintf(stderr, "Memory allocation failed.\n");
					writeLog("Failed to allocate memory [collectMetrics:serviceName]", 2, 0);
					return;
				}
				memset(serviceName, '\0', pluginitemdesc_size * sizeof(char));
				strcpy(serviceName, declarations[i].description);
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
					fprintf(mf, "%s_%s_%s{hostname=\"%s\", service=\"%s\", key=\"%s\"} %s\n", trim(metricsOutputPrefix), pluginName, metricsName, hostName, serviceName, metricsName, cleanMetricsValue);
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
	pthread_mutex_unlock(&update_mtx);
        snprintf(infostr, infostr_size, "Metrics collection took %.0f miliseconds to execute.", (double)t);
        writeLog(trim(infostr), 0, 0);
}

void timeTune(int seconds) {
	int i;
	size_t dest_size = 20;
	snprintf(infostr, infostr_size, "Tuning up run times %d seconds", seconds);
	writeLog(trim(infostr), 0, 0);
	// Loop through and change nextTimeValue
	for (i = 0; i < decCount; i++) {
		if (i != timeTunerMaster) {
			time_t nextTime = declarations[i].nextRun + seconds;
                	struct tm tNextTime;
                	memset(&tNextTime, '\0', sizeof(struct tm));
               	 	localtime_r(&nextTime, &tNextTime);
                	snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
                	declarations[i].nextRun = nextTime;
			if (timeScheduler == 1)
				scheduler[declarations[i].id].timestamp = nextTime;
		}
	}
	if (timeScheduler > 0) {
		qsort(scheduler, decCount, sizeof(struct Scheduler), compare_timestamps);
	}
}

void writePluginResultToFile(int storeIndex, int update) {
	FILE *fp = NULL;
	char* checkName;
	char timestr[35];
	char ch = '/';
	checkName = malloc((size_t)pluginitemname_size * sizeof(char)+1);
	if (checkName == NULL) {
		writeLog("Failed to allocate memory [runPlugin:checkName].", 2, 0);
		return;
	}
	if (update == 0)
		checkName = strdup(declarations[storeIndex].name);
	else
		checkName = strdup(update_declarations[storeIndex].name);
	memmove(checkName, checkName+1,strlen(checkName));
	checkName[strlen(checkName)-1] = '\0';
	strcpy(fileName, storeDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, checkName);
	free(checkName);
	checkName = NULL;
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strcpy(timestr, asctime(timeinfo));
	timestr[strlen(timestr)-1] = '\0';
	if (fileExists(fileName) == 0) {
		fp = fopen(fileName, "a");
	}
	else {
		fp = fopen(fileName, "w+");
	}
	if (update == 0) {
		if (declarations[storeIndex].name && pluginReturnString) {
			if (fp != NULL)
				fprintf(fp, "%s, %s, %s\n", timestr, declarations[storeIndex].name, pluginReturnString);
			else {
				printf("DEBUG: Could not find file stream. Error.\n");
				writeLog("Could not find file stream [writePluginResultToFile]", 1, 0);
				return;
			}
		}
		fflush(fp);
	}
	else
		fprintf(fp, "%s, %s, %s\n", timestr, update_declarations[storeIndex].name, pluginReturnString);
	fclose(fp);
	fp = NULL;
}

void writeToKafkaTopic(int storeIndex, int update) {
	char *payload;
	char *pluginName;
	char *pluginStatus;
	char currTime[22];
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);

        snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	pluginName = malloc((size_t)pluginitemname_size * sizeof(char));
        if (pluginName == NULL) {
        	fprintf(stderr, "Memory allocation failed.\n");
        	writeLog("Failed to allocate memory [runPlugin:enableKafkaExport:pluginName]", 2, 0);
        	return;
	}
       	if (update == 0)
       		pluginName = strdup(declarations[storeIndex].name);
	else
       		pluginName = strdup(update_declarations[storeIndex].name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
        switch(outputs[storeIndex].retCode) {
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
        int count_bytes = strlen(hostName) + strlen(declarations[storeIndex].lastChangeTimestamp) + strlen(declarations[storeIndex].lastRunTimestamp) + strlen(declarations[storeIndex].name) + strlen(declarations[storeIndex].nextRunTimestamp);
        count_bytes += pluginitemdesc_size + pluginoutput_size;
        count_bytes += strlen(pluginStatus) + strlen(declarations[storeIndex].statusChanged);
        count_bytes += 185;
        int kafka_export_addons = 0;
        if (enableKafkaTag > 0) {
        	count_bytes += strlen(kafka_tag);
        	count_bytes += 12; // {"tag":""}
        	kafka_export_addons += 10;
        }
        if (enableKafkaId > 0) {
        	count_bytes += 9; // {"id":""}
        	int length = snprintf(NULL, 0, "%d", kafka_start_id);
        	count_bytes += length;
        	kafka_export_addons += 20;
        }
	payload = malloc((size_t)count_bytes);
        if (payload == NULL) {
        	fprintf(stderr, "Could not allocate memory for payload.\n");
        	writeLog("Failed to allocate memory [runPlugin:enableKafkaExport:payload]", 2, 0);
        	return;
        }
        if (kafka_export_addons < 1) {
        	sprintf(payload, "{\"name\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
        	printf("Payload = %s\n", payload);
        }
        else {
       		if (kafka_export_addons == KAFKA_EXPORT_TAG) {
        		sprintf(payload, "{\"name\":\"%s\", \"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_tag, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
        	}
        	else {
        		int nKafkaId = kafka_start_id + storeIndex;
        		int length = snprintf(NULL, 0, "%d", nKafkaId);
        		char* kafka_id = malloc((size_t)length + 1);
        		snprintf(kafka_id, (size_t)length+1, "%d", nKafkaId);
        		if (kafka_export_addons == KAFKA_EXPORT_ID) {
        			sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_id, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
        		}
        		else if (kafka_export_addons == KAFKA_EXPORT_IDTAG) {
        			sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\",\"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_id, kafka_tag, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
                        }
                }
	}
        free(pluginName);
        free(pluginStatus);
        pluginName = NULL;
        pluginStatus = NULL;
        if (enableKafkaSSL == 0)
        	send_message_to_kafka(kafka_brokers, kafka_topic, payload);
        else
        	send_ssl_message_to_kafka(kafka_brokers, kafkaCACertificate, kafkaProducerCertificate, kafkaSSLKey, kafka_topic, payload);
        free(payload);
        payload = NULL;
}

void runPluginCommand(int index, char* command) {
	int prevRetCode = 0;
	clock_t ct;
	time_t t;
	FILE *fp = NULL;
	char currTime[22];
	int rc = 0;

	if (strlen(command) > 100) {
		writeLog("Command longer than expected. Aborting run.", 1, 0);
		return;
	}
	prevRetCode = outputs[index].retCode;
	ct = clock();
	time(&t);
	snprintf(infostr, infostr_size, "Running %s.", trim(command));
	writeLog(trim(infostr), 0, 0);
	fp = popen(trim(command), "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		writeLog("Failed to run command.", 1, 0);
	}
        while (fgets(pluginReturnString, pluginmessage_size, fp) != NULL) {
		// // VERBOSE  printf("%s", pluginReturnString);
	}
	rc = pclose(fp);
	if (rc > 0) {
        	if (rc == 256)
        		outputs[index].retCode = 1;
        	else if (rc == 512)
        		outputs[index].retCode = 2;
        	else
        		outputs[index].retCode = rc;
        }
        else
        	outputs[index].retCode = rc;
	if (pluginReturnString != NULL && outputs[index].retString != NULL) {
		if (strlen(trim(pluginReturnString)) < pluginoutput_size)
                	strncpy(outputs[index].retString, trim(pluginReturnString), pluginoutput_size);
                else {
                	pluginReturnString[pluginoutput_size] = '\0';
                	strncpy(outputs[index].retString, trim(pluginReturnString), pluginoutput_size);
             	}
	}
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);
	snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (outputs[index].prevRetCode != -1){
        	//snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                if (prevRetCode != outputs[index].retCode){
                	strcpy(declarations[index].statusChanged, "1");
                	strcpy(declarations[index].lastChangeTimestamp, currTime);
                }
                else {
                	strcpy(declarations[index].statusChanged, "0");
                }
		strcpy(declarations[index].lastRunTimestamp, currTime);
                time_t nextTime = t + (declarations[index].interval * 60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                snprintf(declarations[index].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
                declarations[index].nextRun = nextTime;
                outputs[index].prevRetCode = outputs[index].retCode;
                if (timeScheduler == 1) {
                	scheduler[0].timestamp = nextTime;
                }
       	}
       	else {
       		outputs[index].prevRetCode = 0;
      	}
      	ct = clock() -ct;
        snprintf(infostr, infostr_size, "%s executed. Execution took %.0f milliseconds.\n", declarations[index].name, (double)ct);
        writeLog(trim(infostr), 0, 0);
        if (logPluginOutput == 1) {
                char* o_info;
                int o_info_size = pluginmessage_size + 195;
                o_info = malloc((size_t)o_info_size * sizeof(char));
                if (o_info == NULL) {
                        writeLog("Could not allocate memory for variable 'o_info'.", 2, 0);
                }
                snprintf(o_info, (size_t)o_info_size, "%s : %s", declarations[index].name, pluginReturnString);
                writeLog(trim(o_info), 0, 0);
                free(o_info);
                o_info = NULL;
        }
	if (pluginResultToFile == 1) {
		writePluginResultToFile(index, 0);
	}
	if (enableKafkaExport == 1) {
                writeToKafkaTopic(index, 0);
	}
}

void runPlugin(int storeIndex, int update) {
	FILE *fp = NULL;
	char ch = '/';
	int prevRetCode = 0;
	clock_t ct;
	time_t t;
	char currTime[22];
	int rc = 0;
	char sPluginCommand[plugincommand_size];

	if (update > 0)
		prevRetCode = outputs[storeIndex].retCode;
	ct = clock();
	time(&t);
	// Test local var
	strcpy(sPluginCommand, pluginDir);
	strncat(sPluginCommand, &ch, 1);
	if (update > 0) {
                strcat(sPluginCommand, update_declarations[storeIndex].command);
                snprintf(infostr, infostr_size, "Running: %s.", update_declarations[storeIndex].command);
        }
        else {
                strcat(sPluginCommand, declarations[storeIndex].command);
                snprintf(infostr, infostr_size, "Running: %s.", declarations[storeIndex].command);
        }
	writeLog(trim(infostr), 0, 0);
	fp = popen(sPluginCommand, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		writeLog("Failed to run command.", 2, 0);
	}
	while (fgets(pluginReturnString, pluginmessage_size, fp) != NULL) {
		// VERBOSE  printf("%s", pluginReturnString);
		// printf("DEBUG: %s\n", pluginReturnString);
	}
	rc = pclose(fp);
	switch (update) {
		case 0:
			if (rc > 0)
			{
				if (rc == 256)
					outputs[storeIndex].retCode = 1;
				else if (rc == 512)
					outputs[storeIndex].retCode = 2;
				else
					outputs[storeIndex].retCode = rc;
			}
			else
				outputs[storeIndex].retCode = rc;
			break;
		case 1:
			if (rc > 0) {
				if (rc == 256)
					update_outputs[storeIndex].retCode = 1;
				else if (rc == 512)
					update_outputs[storeIndex].retCode = 2;
				else
					update_outputs[storeIndex].retCode = rc;
			}
			else
				update_outputs[storeIndex].retCode = rc;
			break;
		default:
			switch (rc) {
				case 256:
					outputs[storeIndex].retCode = 1;
					update_outputs[storeIndex].retCode = 1;
					break;
				case 512:
					outputs[storeIndex].retCode = 1;
                                        update_outputs[storeIndex].retCode = 1;
					break;
				default:
					outputs[storeIndex].retCode = rc;
                                        update_outputs[storeIndex].retCode = rc;
			}
	}
	//outout.retString size?
	if (update > 0){ 
		update_outputs[storeIndex].retString = strdup(trim(pluginReturnString));
	}
	else {
		if (pluginReturnString != NULL && outputs[storeIndex].retString != NULL){
			if (strlen(trim(pluginReturnString)) < pluginoutput_size) 
				strncpy(outputs[storeIndex].retString, trim(pluginReturnString), pluginoutput_size);
			else {
				pluginReturnString[pluginoutput_size] = '\0';
				strncpy(outputs[storeIndex].retString, trim(pluginReturnString), pluginoutput_size);
			}
		}
		else
			printf("WARNING: Want to write to variables that is freed. Is system closing?\n");
	}
	size_t dest_size = 20;
        time_t tTime = time(NULL);
        struct tm tm = *localtime(&tTime);
        snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (update == 0) {
		if (outputs[storeIndex].prevRetCode != -1){
			/*size_t dest_size = 20;
                	time_t t = time(NULL);
                	struct tm tm = *localtime(&t);
                	snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);*/
                	if (prevRetCode != outputs[storeIndex].retCode){
				strcpy(declarations[storeIndex].statusChanged, "1");
				strcpy(declarations[storeIndex].lastChangeTimestamp, currTime);
				// Here something is wrong, it updates even if change is 0?
			}
			else {
				strcpy(declarations[storeIndex].statusChanged, "0");
			}
			if (enableTimeTuner == 1) {
				if (storeIndex == timeTunerMaster) {
					timeTunerCounter++;
					if (timeTunerCounter == timeTunerCycle) {
						timeTunerCounter = 0;
						// Get time diff
						char oldTime[20];
						struct tm time;
						strcpy(oldTime, declarations[timeTunerMaster].lastRunTimestamp);
						strptime(oldTime, "%d-%02d-%02d %02d:%02d:%02d", &time);
						time_t ttOldTime = 0, ttCurTime = 0;
						int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
						if (sscanf(oldTime, "%d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
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
						if (sscanf(currTime, "%d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second) == 6) {
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
						int difference = ttCurTime - ttOldTime - (declarations[timeTunerMaster].interval * 60);
						// Apply time diff to all nextRuns :)
						timeTune(difference);
					}
				}
                        }
                	strcpy(declarations[storeIndex].lastRunTimestamp, currTime);
                	time_t nextTime = t + (declarations[storeIndex].interval * 60);
                	struct tm tNextTime;
                	memset(&tNextTime, '\0', sizeof(struct tm));
                	localtime_r(&nextTime, &tNextTime);
                	snprintf(declarations[storeIndex].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			declarations[storeIndex].nextRun = nextTime;
                	outputs[storeIndex].prevRetCode = outputs[storeIndex].retCode;
			if (timeScheduler == 1) {
				scheduler[0].timestamp = nextTime;
			}
		}
		else {
	        	outputs[storeIndex].prevRetCode = 0; 
		}
	}
	else {
		// If update = 1 use update_outputs
		// Will this be correct?
		if (prevRetCode != update_outputs[storeIndex].retCode){
                	strcpy(update_declarations[storeIndex].statusChanged, "1");
                        strcpy(update_declarations[storeIndex].lastChangeTimestamp, currTime);
                }
                else {
                	strcpy(update_declarations[storeIndex].statusChanged, "0");
                }
	}
	ct = clock() -ct;
	if (update == 0)
		snprintf(infostr, infostr_size, "%s executed. Execution took %.0f milliseconds.\n", declarations[storeIndex].name, (double)ct);
	else
		snprintf(infostr, infostr_size, "%s executed. Execution took %.0f milliseconds.\n", update_declarations[storeIndex].name, (double)ct);
        writeLog(trim(infostr), 0, 0);
	if (logPluginOutput == 1) {
		char* o_info;
		int o_info_size = pluginmessage_size + 195; 
		o_info = malloc((size_t)o_info_size * sizeof(char));
		if (o_info == NULL) {
			writeLog("Could not allocate memory for variable 'o_info'.", 2, 0);
		}
		if (update == 0)
			snprintf(o_info, (size_t)o_info_size, "%s : %s", declarations[storeIndex].name, pluginReturnString);
		else
			snprintf(o_info, (size_t)o_info_size, "%s : %s", update_declarations[storeIndex].name, pluginReturnString);
		writeLog(trim(o_info), 0, 0);
		free(o_info);
		o_info = NULL;
	}
	if (pluginResultToFile == 1) {
		writePluginResultToFile(storeIndex, update);
		/*char* checkName;
		char timestr[35];
		char ch = '/';
		checkName = malloc((size_t)pluginitemname_size * sizeof(char)+1);
		if (checkName == NULL) {
			writeLog("Failed to allocate memory [runPlugin:checkName].", 2, 0);
			return;
		}
		if (update == 0) 
			checkName = strdup(declarations[storeIndex].name);
		else
			checkName = strdup(update_declarations[storeIndex].name);
		memmove(checkName, checkName+1,strlen(checkName));
                checkName[strlen(checkName)-1] = '\0';
        	strcpy(fileName, storeDir);
        	strncat(fileName, &ch, 1);
        	strcat(fileName, checkName);
		free(checkName);
                checkName = NULL;
		time_t  rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strcpy(timestr, asctime(timeinfo));
		timestr[strlen(timestr)-1] = '\0';
		if (fileExists(fileName) == 0) {
			fp = fopen(fileName, "a");
		}
		else {
			fp = fopen(fileName, "w+");
		}
		if (update == 0) {
			if (declarations[storeIndex].name && pluginReturnString) {
				if (fp != NULL)
					fprintf(fp, "%s, %s, %s\n", timestr, declarations[storeIndex].name, pluginReturnString);
				else {
					printf("DEBUG: Could not find file stream. Error.\n");
					return;
				}
			}
			fflush(fp);
		}
		else
			fprintf(fp, "%s, %s, %s\n", timestr, update_declarations[storeIndex].name, pluginReturnString);
		fclose(fp);
		fp = NULL;*/
	}
	if (enableKafkaExport == 1) {
		writeToKafkaTopic(storeIndex, update);
		/*char *payload;
		char *pluginName;
		char *pluginStatus;
		pluginName = malloc((size_t)pluginitemname_size * sizeof(char));
		if (pluginName == NULL) {
			fprintf(stderr, "Memory allocation failed.\n");
			writeLog("Failed to allocate memory [runPlugin:enableKafkaExport:pluginName]", 2, 0);
			return;
		}
		if (update == 0)
			pluginName = strdup(declarations[storeIndex].name);
		else
			pluginName = strdup(update_declarations[storeIndex].name);
		removeChar(pluginName, '[');
                removeChar(pluginName, ']');
		switch(outputs[storeIndex].retCode) {
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
		int count_bytes = strlen(hostName) + strlen(declarations[storeIndex].lastChangeTimestamp) + strlen(declarations[storeIndex].lastRunTimestamp) + strlen(declarations[storeIndex].name) + strlen(declarations[storeIndex].nextRunTimestamp);
		count_bytes += pluginitemdesc_size + pluginoutput_size;
                count_bytes += strlen(pluginStatus) + strlen(declarations[storeIndex].statusChanged);
		count_bytes += 185;
		int kafka_export_addons = 0;
                if (enableKafkaTag > 0) {
			count_bytes += strlen(kafka_tag);
                        count_bytes += 12; // {"tag":""}
			kafka_export_addons += 10;
		}
		if (enableKafkaId > 0) {
			count_bytes += 9; // {"id":""}
			int length = snprintf(NULL, 0, "%d", kafka_start_id);
                        count_bytes += length;
			kafka_export_addons += 20;
		}
		payload = malloc((size_t)count_bytes);
		if (payload == NULL) {
			fprintf(stderr, "Could not allocate memory for payload.\n");
			writeLog("Failed to allocate memory [runPlugin:enableKafkaExport:payload]", 2, 0);
			return;
		}
                if (kafka_export_addons < 1) {
			sprintf(payload, "{\"name\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
			printf("Payload = %s\n", payload);
                }
                else {
			if (kafka_export_addons == KAFKA_EXPORT_TAG) {
				sprintf(payload, "{\"name\":\"%s\", \"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_tag, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
			}
			else {
				int nKafkaId = kafka_start_id + storeIndex;
                                int length = snprintf(NULL, 0, "%d", nKafkaId);
                                char* kafka_id = malloc((size_t)length + 1);
                                snprintf(kafka_id, (size_t)length+1, "%d", nKafkaId);
				if (kafka_export_addons == KAFKA_EXPORT_ID) {
					sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_id, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
				}
				else if (kafka_export_addons == KAFKA_EXPORT_IDTAG) {
					sprintf(payload, "{\"name\":\"%s\", \"id\":\"%s\",\"tag\":\"%s\", \"data\": {\"lastChange\":\"%s\", \"lastRun\":\"%s\", \"name\":\"%s\", \"nextRun\":\"%s\", \"pluginName\":\"%s\", \"pluginOutput\":\"%s\", \"pluginStatus\":\"%s\", \"pluginStatusChanged\":\"%s\", \"pluginStatusCode\":\"%d\"}}", hostName, kafka_id, kafka_tag, declarations[storeIndex].lastChangeTimestamp, currTime, pluginName, declarations[storeIndex].nextRunTimestamp, declarations[storeIndex].description, outputs[storeIndex].retString, pluginStatus, declarations[storeIndex].statusChanged, outputs[storeIndex].retCode);
				}
			}
                }
		free(pluginName);
		free(pluginStatus);
		pluginName = NULL;
		pluginStatus = NULL;
		if (enableKafkaSSL == 0)
			send_message_to_kafka(kafka_brokers, kafka_topic, payload);
		else
			send_ssl_message_to_kafka(kafka_brokers, kafkaCACertificate, kafkaProducerCertificate, kafkaSSLKey, kafka_topic, payload);
		free(payload);
		payload = NULL;*/
	}
}

void runGardener() {
	FILE *fp = NULL;
	int rc = 0;

	fp = popen(gardenerScript, "r");
        if (fp == NULL) {
                printf("Failed to run gardener script\n");
                writeLog("Failed to run gardener script.", 2, 0);
        }
        while (fgets(gardenerRetString, gardenermessage_size, fp) != NULL) {
                // VERBOSE  printf("%s", gardenerRetString);
        }
        rc = pclose(fp);
	snprintf(infostr, infostr_size, "Gardener script executed with return code %i.", rc);
        if (rc > 1) {
		writeLog(trim(infostr), 2, 0);
	}
	else writeLog(trim(infostr), rc, 0);
}

void runClearDataCache() {
	DIR *d = NULL;
	struct dirent *dir;
	d = opendir(dataDir);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_REG) {
				char buf[1024];
				struct stat filestat;
				sprintf(buf, "%s/%s", dataDir, dir->d_name);
				stat(buf, &filestat);
                                snprintf(infostr, infostr_size, "ClearDataCash checking file: %s", dir->d_name);
                                writeLog(trim(infostr), 0, 0);
				// Now check time 
				time_t now = time(NULL);
				// HERE Set current time to timestamp!!!
				time_t ftime = filestat.st_ctime + dataCacheTimeFrame;
				if (now > ftime) {
                                        snprintf(infostr, infostr_size, "ClearDataCash remove file: %s", dir->d_name);
                                        writeLog(trim(infostr), 0, 0);
					remove(buf);
				}
			}
		}
		closedir(d);
	}
}

void* pluginExeThread(void* data) {
	long storeIndex = (long)data;
	pthread_detach(pthread_self());
	// VERBOSE printf("Executing %s in pthread %lu\n", declarations[storeIndex].description, pthread_self());
	pthread_mutex_lock(&mtx);
	threadIds[(short)storeIndex] = 1;
	runPlugin(storeIndex, 0);
	if (timeScheduler == 1){
		rescheduleChecks();
	}
	thread_counter--;
	pthread_mutex_unlock(&mtx);
	pthread_exit(NULL);
	threadIds[(short)storeIndex] = 0;
}

void* gardenerExeThread(void* data) {
	pthread_detach(pthread_self());
	runGardener();
	pthread_mutex_lock(&mtx);
	thread_counter--;
	pthread_mutex_unlock(&mtx);
	pthread_exit(NULL);
}

void* clearDataCacheThread(void* data) {
	pthread_detach(pthread_self());
	runClearDataCache();
	pthread_mutex_lock(&mtx);
	thread_counter--;
	pthread_mutex_unlock(&mtx);
	pthread_exit(NULL);
}

int countDeclarations(char *file_name) {
	FILE *fp = NULL;
	int i = 0;
	//char c;
        int ch;

	if (file_name == NULL || strlen(file_name) == 0) {
		writeLog("Filename is not initialized or is empty.", 2, 0);
		fprintf(stderr, "Filename is uninitialized or empty.\n");
	}
        fp = fopen(file_name, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file.\n");
		writeLog("Error opening and counting declarations file.", 2, 0);
                exit(EXIT_FAILURE);
        }
	/*for (c = getc(fp); c != EOF; c = getc(fp)){
                printf("%c", c); 
		if (c == '\n')
			i++;
	}*/
        while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n')
			i++;
	}
	fclose(fp);
	fp = NULL;
        //printf("Declaration count = %i\n", i);
	return i-1;
}

int loadPluginDeclarations(char *pluginDeclarationsFile, int reload) {
	int counter = 0;
	int i;
	int index = 0;
        char* line = NULL;
	char *token = NULL;
	char *name = NULL;
        size_t len = 0;
        ssize_t read;
        FILE *fp = NULL;
	char *saveptr = NULL;

        fp = fopen(pluginDeclarationsFile, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file.\n");
		writeLog("Error opening the plugin declarations file.", 2, 0);
                exit(EXIT_FAILURE);
        }
	while ((read = getline(&line, &len, fp)) != -1) {
		index++;
		if (strchr(line, '#') == NULL){
			for (i = 1, line;; i++, line=NULL) {
		        	token = strtok_r(line, ";", &saveptr);
			       	if (token == NULL)
				       break;
			       	switch(i) {
					case 1:
                                        	name = strtok(token, " ");
					       	if (reload == 0)
					       		declarations[counter].name = strdup(name);
						else
							update_declarations[counter].name = strdup(name);
					       	token = strtok(NULL, "?");
						if (reload == 0)
					       		declarations[counter].description = strdup(token);
						else {
							if (token != NULL && update_declarations[counter].description != NULL)
								update_declarations[counter].description = strdup(token);
							else
								printf("Could not update. This is an error.\n");
						}
					       	break;
				       	case 2: 
						if (strlen(token) < 5) {
							printf("Command seems well to short. Config error.\n");
							return 2;
						}
					       	if (reload == 0 )
					       		declarations[counter].command = strdup(token);
						else
							update_declarations[counter].command = strdup(token);
					       	break;
				       	case 3: 
						if (reload == 0)
					       		declarations[counter].active = atoi(token);
						else
							update_declarations[counter].active = atoi(token);
					       	break;
				       	case 4: 
						if (reload == 0) {
					       		declarations[counter].interval = atoi(token);
					       		declarations[counter].id = index-1;
						}
						else {
							update_declarations[counter].interval = atoi(token);
                                                        update_declarations[counter].id = index-1;
						}
			       	}
		       	}
		       	if (reload == 0) {
		       		strcpy(declarations[counter].lastRunTimestamp, "");
		       		strcpy(declarations[counter].nextRunTimestamp, "");
		       		strcpy(declarations[counter].lastChangeTimestamp, "");
		       		strcpy(declarations[counter].statusChanged, "");
			}
			else {
				strcpy(update_declarations[counter].lastRunTimestamp, "");
                                strcpy(update_declarations[counter].nextRunTimestamp, "");
                                strcpy(update_declarations[counter].lastChangeTimestamp, "");
                                strcpy(update_declarations[counter].statusChanged, "");
			}
		       	snprintf(infostr, infostr_size, "Declaration with index %d is created.\n", counter);
		       	writeLog(trim(infostr), 0, 0);
                       	counter++;
		}
	}
        fclose(fp);
	fp = NULL;
        if (line) {
                free(line);
		line = NULL;
	}
	return 0;
}

void copyPluginItem(PluginItem *dest, const PluginItem *src, int mode) {
	if (mode == 0) {
		if (src->name != NULL) {
			strncpy(dest->lastRunTimestamp, src->lastRunTimestamp, 20);
			strncpy(dest->nextRunTimestamp, src->nextRunTimestamp, 20);
			strncpy(dest->lastChangeTimestamp, src->lastChangeTimestamp, 20);
			strncpy(dest->statusChanged, src->statusChanged, 1);
			dest->active = src->active;
			dest->interval = src->interval;
			dest->nextRun = src->nextRun;
		}
		else {
			//printf("Source is empty, do not copy\n");
			writeLog("copyPluginItem[src->name] is empty. Do not copy.", 0, 0);
		}
	}
	else if (mode == 2) {
		strncpy(dest->lastRunTimestamp, src->lastRunTimestamp,20);
		strncpy(dest->nextRunTimestamp, src->nextRunTimestamp, 20);
		strncpy(dest->statusChanged, src->statusChanged, 1);
		dest->nextRun = src->nextRun;
	}
	else {
		strncpy(dest->name, src->name, pluginitemname_size);
        	strncpy(dest->description, src->description, pluginitemdesc_size);
        	strncpy(dest->command, src->command, pluginitemcmd_size);
		strncpy(dest->lastRunTimestamp, src->lastRunTimestamp, 20);
                strncpy(dest->nextRunTimestamp, src->nextRunTimestamp, 20);
		strncpy(dest->lastChangeTimestamp, src->lastChangeTimestamp, 20);
                strncpy(dest->statusChanged, src->statusChanged, 1);
                dest->active = src->active;
                dest->interval = src->interval;
                dest->nextRun = src->nextRun;
	}
}

void copyOutputItem(PluginOutput *dest, const PluginOutput *src) {
	dest->retCode = src->retCode;
	dest->prevRetCode = src->prevRetCode;
	if (src->retString != NULL) {
		strncpy(dest->retString, src->retString, pluginoutput_size);
	}
	else {
		writeLog("copyOutputItem source->retString is NULL", 1, 0);
	}
}

int redeclarePluginDeclarations(int mode, int count) {
	int c;
	int rows = 0;
	int check = 0;

	writeLog("Needs to redeclare declarations.", 0, 0);
	check = check_plugin_conf_file(pluginDeclarationFile);
	if (check > 0) {
		writeLog("Errors detected in plugin file. Can not reload.", 1, 0);
	       	return 2;	
	}
	else
		writeLog("Plugin conf file seems in good state. Will try to reload it now.", 0, 0);
	update_declarations = (PluginItem *)calloc(count, sizeof(PluginItem));
	update_declaration_size = (size_t)count;
	if (!update_declarations) {
		perror ("Error allocating memory");
		writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations]", 2, 0);
		abort();
		return 2;
	}
	for (int i = 0; i < count; i++) {
		update_declarations[i].name = (char *) malloc((size_t)pluginitemname_size * sizeof(char));
		update_declarations[i].description = (char *) malloc((size_t)pluginitemdesc_size * sizeof(char));
		update_declarations[i].command = (char *) malloc((size_t)pluginitemcmd_size * sizeof(char));
		if (update_declarations[i].name == NULL || update_declarations[i].description == NULL || update_declarations[i].command == NULL) {
			fprintf(stderr, "Error allocating memory while redeclaring plugins.\n");
			writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations::items].", 2, 0);
			abort();
			return 2;
		}
	}
	writeLog("Needs to reallocate memory for outputs.", 0, 0);
	update_outputs = (PluginOutput *)calloc((size_t)count, sizeof(PluginOutput));
	update_output_size = (size_t)count;
	if (!update_outputs){
		perror("Error allocating memory");
		writeLog("Error allocating memory [redeclarePluginDeclarations:update_outputs].", 2, 0);
		abort();
		return 2;
	}
	for (int j = 0; j < count; j++) {
		update_outputs[j].retString = (char *) malloc((size_t)pluginoutput_size * sizeof(char));
		if (!update_outputs[j].retString) {
			fprintf(stderr, "Error allocating memory while redeclaring outputs.\n");
			writeLog("Error allocating memory [redeclarePluginDeclarations:update_outputs:returnString].", 2, 0);
			abort();
			return 2;
		}
	}
	int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile, 1);
	if (pluginDeclarationResult != 0){
		printf("ERROR: Problem reading plugin declaration file.\n");
		writeLog("Problem reading from plugin declaration file.", 1, 0);
	}
	else {
		printf("Declarations read.\n");
		writeLog("Plugin declarations file reloaded.", 0, 0);
	}
	int update[count];
	switch(mode) {
		case 0:
			if (count > 0 && threadIds != NULL) {
				threadIds = realloc(threadIds, (size_t)(count * sizeof(int)));
				if (threadIds == NULL) {
					free(threadIds);
					printf("Could not reallocate memory for threadIds.\n");
					exit(EXIT_FAILURE);
				}
			}
			for (int i = 0; i < decCount; i++) {
                                int missing = 0;
                                for (int j = 0; j < decCount; j++) {
                                        if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
                                                if (i == j) {
                                                        snprintf(infostr, infostr_size, "Redeclare %s with id %d\n", update_declarations[i].name, i+1);
                                                        writeLog(trim(infostr), 0, 0);
                                                        update[i] = 0;
                                                }
                                                else {
                                                        snprintf(infostr, infostr_size, "Redeclare %s with new id. Id is now %d\n", update_declarations[i].name, i);
                                                        writeLog(trim(infostr), 1, 0);
                                                        update[i] = j;
                                                }
                                                missing++;
                                                break;
                                        }
                                }
                                if (missing == 0) {
                                        printf("Plugin is missing. Needs declaration.\n");
                                        writeLog("Needs to declare new plugin.", 0, 0);
                                        initNewPlugin(i);
                                        update[i] = 1000;
                                }
                        }
                        for (int i = decCount; i < count; ++i) {
                                printf("Check for new plugins.\n");
                                initNewPlugin(i);
                                update[i] = 1000;
                        }
                        for (int i = 0; i < count; i++) {
                                if (update[i] == 0){
                                        copyPluginItem(&update_declarations[i], &declarations[i],0);
                                        copyOutputItem(&update_outputs[i], &outputs[i]);
                                }
                                else {
                                        copyPluginItem(&update_declarations[i], &declarations[update[i]], 0);
                                        copyOutputItem(&update_outputs[i], &outputs[update[i]]);
                                }
                        }
			free_structures(decCount);
                        free(declarations);
                        free(outputs);
                        declarations = NULL;
                        declarations = (PluginItem *)malloc((size_t)sizeof(PluginItem) * count);
                        outputs = (PluginOutput *)malloc((size_t)sizeof(PluginOutput) * count);
			declaration_size = (size_t)count;
			output_size = (size_t)count;
                        if (!declarations) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_declaraions]", 2, 0);
                                abort();
                                return 2;
                        }
                        if (!outputs) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_outputs]", 2, 0);
                                abort();
                                return 2;
                        }
                        for (int i = 0; i < count; i++) {
                                declarations[i].name = (char *) malloc((size_t)pluginitemname_size * sizeof(char));
                                declarations[i].description = (char *) malloc((size_t)pluginitemdesc_size * sizeof(char));
                                declarations[i].command = (char *) malloc((size_t)pluginitemcmd_size * sizeof(char));
                                outputs[i].retString = (char *) malloc((size_t)pluginoutput_size * sizeof(char));
                                if (declarations[i].name == NULL || declarations[i].description == NULL || declarations[i].command == NULL || outputs[i].retString == NULL) {
                                        fprintf(stderr, "Error allocating memory while redeclaring plugins.\n");
                                        writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations::items].", 2, 0);
                                        abort();
                                        return 2;
                                }
                        }
                        for (int e = 0; e < count; e++) {
                                copyPluginItem(&declarations[e], &update_declarations[e], 5);
                                copyOutputItem(&outputs[e], &update_outputs[e]);
                        }
                        for (int i = 0; i < count; i++) {
                                free(update_declarations[i].name);
                                free(update_declarations[i].description);
                                free(update_declarations[i].command);
                                if (i < decCount) {
                                        // Why is not update_outputs[count_max] exisiting...?
                                        free(update_outputs[i].retString);
                                }
                        }
                        free(update_declarations);
                        free(update_outputs);
                        update_declarations = NULL;
                        update_outputs = NULL;
                        decCount = count;
			break;
		case 1:
			if (count > 0 && threadIds != NULL) {
                                threadIds = realloc(threadIds, (size_t)(count * sizeof(int)));
                                if (threadIds == NULL) {
                                        free(threadIds);
                                        printf("Could not reallocate memory for threadIds.\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
			for (int i = 0; i < count; i++) {
                                int found = 0;
                                for (int j = 0; j < decCount; j++) {
                                        if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
                                                if (i == j) {
                                                        snprintf(infostr, infostr_size, "Redeclare %s with id %d\n", update_declarations[i].name, i+1);
                                                        writeLog(trim(infostr), 0, 0);
                                                        update[i] = 0;
                                                }
                                                else {
                                                        snprintf(infostr, infostr_size, "Redeclare %s with new id. Id is now %d\n", update_declarations[i].name, i);
                                                        writeLog(trim(infostr), 1, 0);
                                                        update[i] = j;
                                                }
                                                found++;
                                                break;
                                        }
                                        else {
                                                if (j == decCount-1) {
                                                        // Write this if not found in new declaration only
                                                        snprintf(infostr, infostr_size, "Old plugin declaration '%s' with id %d marked for deletion.", declarations[j].name, declarations[j].id);
                                                        writeLog(trim(infostr), 1, 0);
                                                }
                                        }
                                }
                                if (found == 0) {
                                        initNewPlugin(i);
                                }
                        }
                        for (int a = 0; a < count; a++) {
                                 if (update[a] == 0){
                                        copyPluginItem(&update_declarations[a], &declarations[a],0);
                                        copyOutputItem(&update_outputs[a], &outputs[a]);
                                }
                                else {
                                        copyPluginItem(&update_declarations[a], &declarations[update[a]], 0);
                                        copyOutputItem(&update_outputs[a], &outputs[update[a]]);
                                }
                        }
                        free_structures(decCount);
                        free(outputs);
                        outputs = NULL;
                        free(declarations);
                        declarations = NULL;
                        declarations = (PluginItem *)malloc((size_t)sizeof(PluginItem) * count);
			declaration_size = (size_t)count;
                        outputs = (PluginOutput *)malloc((size_t)sizeof(PluginOutput) * count);
			output_size = (size_t)count;
                        if (!declarations) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_declaraions]", 2, 0);
                                abort();
                                return 2;
                        }
                        if (!outputs) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_outputs]", 2, 0);
                                abort();
                                return 2;
                        }
                        for (int i = 0; i < count; i++) {
                                declarations[i].name = (char *) malloc((size_t)pluginitemname_size * sizeof(char));
                                declarations[i].description = (char *) malloc((size_t)pluginitemdesc_size * sizeof(char));
                                declarations[i].command = (char *) malloc((size_t)pluginitemcmd_size * sizeof(char));
                                outputs[i].retString = (char *) malloc((size_t)pluginoutput_size * sizeof(char));
                                if (declarations[i].name == NULL || declarations[i].description == NULL || declarations[i].command == NULL || outputs[i].retString == NULL) {
                                        fprintf(stderr, "Error allocating memory while redeclaring plugins.\n");
                                        writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations::items].", 2, 0);
                                        abort();
                                        return 2;
                                }
                        }
                        for (int z = 0; z < count; z++) {
                                copyPluginItem(&declarations[z], &update_declarations[z], 3);
                                copyOutputItem(&outputs[z], &update_outputs[z]);
                        }
			for (int i = 0; i < count; i++) {
                                free(update_declarations[i].name);
                                free(update_declarations[i].description);
                                free(update_declarations[i].command);
                                if (i < decCount) {
                                        free(update_outputs[i].retString);
                                }
                        }
                        free(update_declarations);
                        free(update_outputs);
                        update_declarations = NULL;
                        update_outputs = NULL;
                        decCount = count;
			break;
		case 2:
			c = 0;
                        for (int i = 0; i < decCount; i++) {
				if (strcmp(update_declarations[i].name, declarations[i].name) != 0) {
					c++;
					rows++;
					if (strcmp(update_declarations[i].description, declarations[i].description) == 0)
						c--;
					else
						c++;
					if (strcmp(update_declarations[i].command, declarations[i].command) == 0) 
						c--;
					else 
						c++;
				}
				if (c < 0) c = 0;
			}
			for (int i = 0; i < decCount; i++) {
                                free(update_declarations[i].name);
                                free(update_declarations[i].description);
                                free(update_declarations[i].command);
                                free(update_outputs[i].retString);
                        }
                        free(update_declarations);
                        free(update_outputs);
		       	update_declarations = NULL;
			update_outputs = NULL;
			return c + rows;
			break;
		case 3:
			for (int i = 0; i < decCount; i++) {
				int found = 0;
				int id = -1;
				for (int j = 0; j < decCount; j++) {
					if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
						found++;
						id = j;
					}
					if (strcmp(update_declarations[i].description, declarations[j].description) == 0) {
						found++;
						id = j;
					}
					if (strcmp(update_declarations[i].command, declarations[j].command) == 0) {
						found++;
						id = j;
					}
				}
				if (found > 0) {
					printf("Found update_declaration id %d on declaration id %d\n", i, id);
					copyOutputItem(&update_outputs[i], &outputs[id]);
					copyPluginItem(&update_declarations[i], &declarations[id], 2);
				}
				else {
					printf("Did not find declaration.name = %s", update_declarations[i].name);
					initNewPlugin(i);
				}
			}
			free_structures(decCount);
                        free(outputs);
                        outputs = NULL;
                        free(declarations);
                        declarations = NULL;
                        declarations = (PluginItem *)malloc((size_t)sizeof(PluginItem) * count);
			declaration_size = (size_t)count;
                        outputs = (PluginOutput *)malloc((size_t)sizeof(PluginOutput) * count);
			output_size = (size_t)count;
                        if (!declarations) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_declaraions]", 2, 0);
                                abort();
                                return 2;
                        }
                        if (!outputs) {
                                perror ("Error allocating memory");
                                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_outputs]", 2, 0);
                                abort();
                                return 2;
                        }
                        for (int i = 0; i < count; i++) {
                                declarations[i].name = (char *) malloc((size_t)pluginitemname_size * sizeof(char));
                                declarations[i].description = (char *) malloc((size_t)pluginitemdesc_size * sizeof(char));
                                declarations[i].command = (char *) malloc((size_t)pluginitemcmd_size * sizeof(char));
                                outputs[i].retString = (char *) malloc((size_t)pluginoutput_size * sizeof(char));
                                if (declarations[i].name == NULL || declarations[i].description == NULL || declarations[i].command == NULL || outputs[i].retString == NULL) {
                                        fprintf(stderr, "Error allocating memory while redeclaring plugins.\n");
                                        writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations::items].", 2, 0);
                                        abort();
                                        return 2;
                                }
                                copyPluginItem(&declarations[i], &update_declarations[i], 0);
                                copyOutputItem(&outputs[i], &update_outputs[i]);
                        }
                        for (int i = 0; i < count; i++) {
                                free(update_declarations[i].name);
                                free(update_declarations[i].description);
                                free(update_declarations[i].command);
                                if (i < decCount) {
                                        free(update_outputs[i].retString);
                                }
                        }
                        free(update_declarations);
                        free(update_outputs);
                        update_declarations = NULL;
                        update_outputs = NULL;
			break;

			// This case in not working. Make corrupt data. Force restart?
                        /*for (int i = 0; i < decCount; i++) {
                                printf("i = %d\n", i);
                                int write_this = 0;
                                int id = 0;
                                for (int j = 0; j < count; j++) {
                                        printf ("j = %d\n", j);
                                        printf ("update_i = %s\n", update_declarations[i].name);
                                        printf ("dec_j = %s | outputs[%d] = %s\n", declarations[j].name, j, outputs[j].retString);
                                        if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
                                                printf("NAME\n");
                                                printf("Update_declarations = %s | declarations = %s\n", update_declarations[i].name, declarations[j].name);
                                                id = j;
                                                printf("ID = %d\n", id);
                                                write_this++;
                                                break;
                                        }
                                        else if (strcmp(update_declarations[i].description, declarations[j].description) == 0) {
                                                id = j;
                                                printf("DESCRIPTION\n");
                                                printf("Update_declarations = %s | declarations = %s\n", update_declarations[i].description, declarations[j].description);
                                                printf("ID = %d\n", id);
                                                write_this++;
                                                break;
                                        }
                                        else if (strcmp(update_declarations[i].command,declarations[j].command) == 0) {
                                                id = j;
                                                printf ("COMMAND\n");
                                                printf("Update_declarations = %s | declarations = %s\n", update_declarations[i].command, declarations[j].command);
                                                printf("ID = %d\n", id);
                                                write_this++;
                                                break;
                                        }
                                }
                                if (write_this == 0) {
                                        writeLog("Needs to redeclare plugin. You should not mess around with conf files so much.", 1);
                                        initNewPlugin(i);
                                        flushLog();
                                }
                                else {
                                        snprintf(loginfo, 400, "Found old outputs for plugin declaration '%s'.", update_declarations[i].name);
                                        writeLog(trim(loginfo), 0);
                                        flushLog();
                                        printf ("ID = %d\n", id);
                                        printf ("Update_declarations = %s | declarations = %s\n", update_declarations[i].name, declarations[id].name);
                                        printf ("Outputs[%d] = ", id);
                                        printf ("%s\n", outputs[id].retString);
                                        update_outputs[i] = outputs[id];
                                        strcpy(update_declarations[i].lastRunTimestamp,declarations[id].lastRunTimestamp);
                                        strcpy(update_declarations[i].nextRunTimestamp,declarations[id].nextRunTimestamp);
                                        strcpy(update_declarations[i].statusChanged,declarations[id].statusChanged);
                                        update_declarations[i].nextRun = declarations[id].nextRun;
                                }

                        }*/
	}
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
	int qsv = quick_start;

	if (quick_start != 1) quick_start = 1;
	free_structures(decCount);
        free(outputs);
        outputs = NULL;
        free(declarations);
        declarations = NULL;
        declarations = (PluginItem *)malloc((size_t)sizeof(PluginItem) * cnt);
	declaration_size = (size_t)cnt;
        outputs = (PluginOutput *)malloc((size_t)sizeof(PluginOutput) * cnt);
	output_size = (size_t)cnt;
        if (!declarations) {
        	perror ("Error allocating memory");
       		writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_declaraions]", 2, 0);
        	abort();
        	return 2;
        }
        if (!outputs) {
        	perror ("Error allocating memory");
                writeLog("Error allocating memory [redeclarePluginDeclarations:redeclare_outputs]", 2, 0);
                abort();
                return 2;
       }
       for (int i = 0; i < cnt; i++) {
       		declarations[i].name = (char *) malloc((size_t)pluginitemname_size * sizeof(char));
                declarations[i].description = (char *) malloc((size_t)pluginitemdesc_size * sizeof(char));
                declarations[i].command = (char *) malloc((size_t)pluginitemcmd_size * sizeof(char));
                outputs[i].retString = (char *) malloc((size_t)pluginoutput_size * sizeof(char));
                if (declarations[i].name == NULL || declarations[i].description == NULL || declarations[i].command == NULL || outputs[i].retString == NULL) {
                	fprintf(stderr, "Error allocating memory while redeclaring plugins.\n");
                        writeLog("Error allocating memory [redeclarePluginDeclarations:update_declarations::items].", 2, 0);
                        abort();
                        return 2;
       		}
       }

       if (loadPluginDeclarations(pluginDeclarationFile, 0) == 0) {
	       writeLog("Plugin file reloaded.", 0, 0);
       }
       else {
	       writeLog("Error reloading plugins file.", 2, 0);
	       sig_handler(SIGSTOP);
       }
       decCount = cnt;
       initScheduler(cnt, 1000);
       quick_start = qsv;
       return 0;
}	

void apiReloadConfigHard() {
	if (check_plugin_conf_file(pluginDeclarationFile) != 0) {
		constructSocketMessage("reloadpluginshard", "failed");
        }
       	else {
		hardReloadPlugins(decCount);
		constructSocketMessage("reloadpluginshard", "success");
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
                perror("Error while opening the file.\n");
                writeLog("Error opening and counting declarations file.", 2, 0);
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

int updatePluginDeclarations() {
	FILE *fp = NULL;
	char* line = NULL;
	char* name = NULL;
        char *token = NULL;
	char *saveptr = NULL;
	int i;
	int index = 0;
	int counter = 0;
	int reload_required = 0;
	size_t len = 0;
	ssize_t read;
	PluginItem item;

	item.name = (char *)malloc((size_t)pluginitemname_size * sizeof(char));
	item.description = (char *)malloc((size_t)pluginitemdesc_size * sizeof(char));
	item.command = (char *)malloc((size_t)pluginitemcmd_size * sizeof(char));

	int newCount = checkNewConfig(pluginDeclarationFile);
	if (newCount < 0) {
		// Abort if duplicates
		return newCount;
	}
	// Temporary use of hard reload until redeclare is fine.
	//return hardReloadPlugins(newCount);

	if (newCount > decCount) {
		int retVal = redeclarePluginDeclarations(0, newCount);
		checkRetVal(retVal);
		free(item.name);
                free(item.description);
                free(item.command);
		return 1;
	}
	else if (newCount < decCount) {
		int retVal = redeclarePluginDeclarations(1, newCount);
		checkRetVal(retVal);
		free(item.name);
                free(item.description);
                free(item.command);
		return 1;
	}
	else {
		// Read plugin declarations file and update declarations	
		// This causes errors if changing orders
		// Adapt to new redeclare function
		// This only works if you edit line in current positions
		if (redeclarePluginDeclarations(2, newCount) > 4) {
                	printf ("Needs total reload...\n");
                        /*int retVal = redeclarePluginDeclarations(3, newCount);
                        checkRetVal(retVal);*/
			hardReloadPlugins(newCount);
			free(item.name);
                	free(item.description);
                	free(item.command);
                        return 1;
                }
		fp = fopen(pluginDeclarationFile, "r");
       	 	if (fp == NULL)
        	{
                	perror("Error while opening the file.\n");
                	writeLog("Error opening the plugin declarations file.", 2, 0);
			free(item.name);
                	free(item.description);
                	free(item.command);
               		exit(EXIT_FAILURE);
        	}
        	while ((read = getline(&line, &len, fp)) != -1) {
                	index++;
                	if (strchr(line, '#') == NULL){
                        	for (i = 1, line;; i++, line=NULL) {
                               		token = strtok_r(line, ";", &saveptr);
                               		if (token == NULL)
                                       		break;
                               		switch(i) {
                                       		case 1:
                                               		name = strtok(token, " ");
                                               		strcpy(item.name, name);
                                               		token = strtok(NULL, "?");
                                               		strcpy(item.description, token);
                                               		break;
                                       		case 2: strcpy(item.command, token);
                                               		break;
                                       		case 3: item.active = atoi(token);
                                               		break;
                                       		case 4: item.interval = atoi(token);
                                               		item.id = index-1;
                               		}
                       		}
                       		strcpy(item.lastRunTimestamp, "");
                       		strcpy(item.nextRunTimestamp, "");
                       		strcpy(item.lastChangeTimestamp, "");
                       		strcpy(item.statusChanged, "");
				if (strcmp(declarations[counter].name, item.name) != 0) {
					snprintf(infostr, infostr_size, "Declaration name for item %i changed from %s to %s.", counter, declarations[counter].name, item.name);
					strncpy(declarations[counter].name, item.name, strlen(item.name) + 1);
					reload_required = 1;
                                        writeLog(trim(infostr), 0, 0);
				}
				if (strcmp(declarations[counter].description, item.description) != 0) {
					snprintf(infostr, infostr_size, "Declaration description for %s changed from %s to %s.", declarations[counter].name, declarations[counter].description, item.description);
					strncpy(declarations[counter].description, item.description, strlen(item.description) + 1);
                                        writeLog(trim(infostr), 0, 0);
				}
				if (strcmp(declarations[counter].command, item.command) != 0) {
					snprintf(infostr, infostr_size, "Declaration command for %s changed to %s.", declarations[counter].name, item.command);
					strncpy(declarations[counter].command, item.command, strlen(item.command) + 1);
					writeLog(trim(infostr), 0, 0);
				}
				if (declarations[counter].active != item.active) {
					if (item.active == 0) {
						snprintf(infostr, infostr_size, "Declaration %s is now inactive.", declarations[counter].name);
						declarations[counter].active = 0;
					}
					else {
						snprintf(infostr, infostr_size, "Declaration %s is now active", declarations[counter].name);
						declarations[counter].active = 1;
					}
					writeLog(trim(infostr), 0, 0);
				}
				if (declarations[counter].interval != item.interval) {
					snprintf(infostr, infostr_size, "Declaration %s interval changed from %i to %i.", declarations[counter].name, declarations[counter].interval, item.interval);
					declarations[counter].interval = item.interval;
					writeLog(trim(infostr), 0, 0);
				}
				/* int x = ((declarations[counter].command == item.command) && (declarations[counter].active == item.active) &&
						(declarations[counter].interval == item.interval)) ? 1 : 0;
				if (x != 0) {
					strncpy(declarations[counter].command, item.command, strlen(item.command) + 1);
					declarations[counter].active = item.active;
					declarations[counter].interval = item.interval;
					snprintf(loginfo, 100, "Declaration parameters for item %s is updated.\n",item.name);
                                	writeLog(trim(loginfo), 0);
				}*/
                       		counter++;
                	}
        	}
       	 	fclose(fp);
		fp = NULL;
       		if (line) {
       		         free(line);
			 line = NULL;
		}
		if (reload_required) {
			printf("Reload required.\n");
			writeLog("Changed declaration name might cause inconsistencies. Will reload all plugins.", 1, 0);
			flushLog();
			int retVal = redeclarePluginDeclarations(2, newCount);
                	checkRetVal(retVal);
			free(item.name);
                	free(item.description);
                	free(item.command);
                	return 1;
		}
		free(item.name);
                free(item.description);
                free(item.command);
		if (timeScheduler == 1) {
			free(scheduler);
			scheduler = NULL;
			initTimeScheduler(); 
			for (int i = 0; i < decCount; i++) {
                                scheduler[i].id = i;
                                scheduler[i].timestamp = declarations[i].nextRun;
                        }
			rescheduleChecks();
		}
		return 0;
	}
}

void initNewPlugin(int index) {
	char currTime[22];
	snprintf(infostr, infostr_size, "Initiating new plugin: %s\n", update_declarations[index].name);
	writeLog(trim(infostr), 0, 0);
	printf("Initiating new plugin with id %d\n", index);
	if (update_declarations[index].active == 1) {
		snprintf(infostr, infostr_size, "%s is now active. Id %d\n", update_declarations[index].name, update_declarations[index].id-1);
		writeLog(trim(infostr), 0, 0);
		update_outputs[index].prevRetCode = -1;
		strcpy(update_declarations[index].statusChanged, "0");
		runPlugin(index, 1);
		if (timeScheduler == 1)
			rescheduleChecks();
		size_t dest_size = 20;
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
                strcpy(update_declarations[index].lastRunTimestamp, currTime);
                strcpy(update_declarations[index].lastChangeTimestamp, currTime);
                time_t nextTime = t + (update_declarations[index].interval *60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                snprintf(update_declarations[index].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
                update_declarations[index].nextRun = nextTime;
		usleep(500);
	}
	else
        {
        	snprintf(infostr, infostr_size, "%s is not active. Id: %d\n", update_declarations[index].name, update_declarations[index].id);
        	writeLog(trim(infostr), 0, 0);
        }
        flushLog();
}

int initTimeScheduler() {
	scheduler = malloc((size_t)sizeof(Scheduler)*decCount);
	if (!scheduler) {
        	printf("Error allocating memory");
        	writeLog("Error allocating memory [initTimeScheduler]", 2, 0);
        	abort();
       		return 2;
        }
	return 0;
}

void initScheduler(int numOfP, int msSleep) {
	char currTime[22];
	time_t nextTime;
	float sleepTime = msSleep/1000;
	//printf("Initiating scheduler\n");
	logInfo("Initiating scheduler to run checks att given intervals.", 0, 0);
	if (timeScheduler != 0) {
		logInfo("Initiating a time scheduler.", 0, 0);
		initTimeScheduler();
	}
	flushLog();
	for (int i = 0; i < numOfP; i++)
	{
		if (declarations[i].active == 1)
		{
			snprintf(infostr, infostr_size, "%s is active. Id %d\n", declarations[i].name, declarations[i].id);
			writeLog(trim(infostr), 0, 0);
			outputs[i].prevRetCode = -1;
			strncpy(declarations[i].statusChanged, "0", 2);
			runPlugin(i, 0);
			size_t dest_size = 20;
			time_t t = time(NULL);
  			struct tm tm = *localtime(&t);
			snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			strcpy(declarations[i].lastRunTimestamp, currTime);
			strcpy(declarations[i].lastChangeTimestamp, currTime);
			if (quick_start == 1) {
				int add_time = (int)sleepTime;
				int time_to_add = add_time * i+1;
				nextTime = t + (declarations[i].interval * 60) + time_to_add;
			}
			else {
				nextTime = t + (declarations[i].interval *60);
			}
			struct tm tNextTime;
			memset(&tNextTime, '\0', sizeof(struct tm));
			localtime_r(&nextTime, &tNextTime);
			snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			declarations[i].nextRun = nextTime;
			if (timeScheduler == 1) {
				scheduler[i].id = i;
				scheduler[i].timestamp = nextTime;
			}
			if (quick_start < 1)
				sleep(sleepTime);
		}
		else
		{
			snprintf(infostr, infostr_size, "%s is not active. Id: %d\n", declarations[i].name, declarations[i].id);
			writeLog(trim(infostr), 0, 0);
			if (timeScheduler > 0) {
				scheduler[i].id = i;
				scheduler[i].timestamp = 0;
			}
		}
		flushLog();
	}
	if (standalone == 0) {
		switch (output_type) {
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
        tnextGardener = time(0) + gardenerInterval;	
	tnextClearDataCache = time(0) + clearDataCacheInterval;
	if (local_api > 0) {
		if (use_ssl > 0)
			 SSL_library_init();
		if (socket_is_ready == 1) {
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
	if (timeScheduler != 0) {
		qsort(scheduler, decCount, sizeof(struct Scheduler), compare_timestamps);
	}
	logInfo("Scheduler initialized.", 0, 0);
    	flushLog();
}

void startPluginThread(int plugin_id) {
	int rc;
	pthread_t thread_id;
	long vpid;

	vpid = plugin_id;

	rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)vpid);
	if(rc) {
		snprintf(infostr, infostr_size, "Error: return code from phtread_create is %d\n", rc);
		writeLog(trim(infostr), 2, 0);
	}
	else {
		snprintf(infostr, infostr_size, "Created new thread (%lu) for plugin %s\n", thread_id, declarations[plugin_id].name);
		writeLog(trim(infostr), 0, 0);
		pthread_mutex_lock(&mtx);
		thread_counter++;
		pthread_mutex_unlock(&mtx);
		pthread_join(thread_id, NULL);
        }
}

void runPluginThreads(int loopVal){
	char currTime[22];
	pthread_t thread_id;
        int rc;
        int i;
	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
	size_t dest_size = 20;

	snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	if (timeScheduler == 1) {
		i = 1;
		struct Scheduler do_run = scheduler[0];
		//printf("do_run timestamp = %ld\n", (long)do_run.timestamp);
		//printf("t timestamp = %ld\n", (long)t);
		while(i > 0) {
			if ((t >= do_run.timestamp) && (declarations[do_run.id].active == 1)) {
				//printf("DEBUG: startPluginThread id %d\n", do_run.id);
				startPluginThread(do_run.id);
				tspr++;
			}
			if (do_run.timestamp > t) {
				//printf("Exit..\n");
				break;
			}
			do_run = scheduler[0];
                       	//printf("do_run new id = %d\n", do_run.id);
                        //printf("do_run timestamp = %ld\n", (long)do_run.timestamp);
                        //printf("t timestamp = %ld\n", (long)t);
		}
		return;
	}

        for (i = 0; i < loopVal; i++) {
           long j = i;
	   if (declarations[i].active == 1) {
		if (t > declarations[i].nextRun)
		{
			rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)j);
           		if(rc) {
                		snprintf(infostr, infostr_size, "Error: return code from phtread_create is %d\n", rc);
				writeLog(trim(infostr), 2, 0);
           		}
           		else {
                   		snprintf(infostr, infostr_size, "Created new thread (%lu) for plugin %s\n", thread_id, declarations[i].name);
				writeLog(trim(infostr), 0, 0);
				pthread_mutex_lock(&mtx);
				thread_counter++;
				pthread_mutex_unlock(&mtx);
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
		snprintf(infostr, infostr_size, "Error: return code from phtread_create is %d\n", rc);
               	writeLog(trim(infostr), 2, 0);
		return;
        }
	//pthread_setname_np(thread_id, "Gardener worker");
	pthread_setspecific(thread_id, "Gardener worker");
	snprintf(infostr, infostr_size, "Created new thread (%lu) truncating metrics logs (gardener) \n", thread_id);
        writeLog(trim(infostr), 0, 0);
	pthread_mutex_lock(&mtx);
	thread_counter++;
	pthread_mutex_unlock(&mtx);
}

void clearDataCache() {
	pthread_t thread_id;
	int rc;

	rc = pthread_create(&thread_id, NULL, clearDataCacheThread, "clearDataCache 1");
	      if(rc) {
                snprintf(infostr, infostr_size, "Error: return code from phtread_create is %d\n", rc);
                writeLog(trim(infostr), 2, 0);
        }
        else {
		//pthread_setname_np(thread_id, "DataClearCache");
		pthread_setspecific(thread_id, "DataClearCache");
                snprintf(infostr, infostr_size, "Created new thread (%lu) clearing old data files (clearDataCache) \n", thread_id);
                writeLog(trim(infostr), 0, 0);
		pthread_mutex_lock(&mtx);
		thread_counter++;
		pthread_mutex_unlock(&mtx);
		pthread_join(thread_id, NULL);
       }
}

void scheduleChecks(){
	float sleepTime = schedulerSleep/1000;
	const int i = 1;
	int repeate_write = 0;

	logInfo("Almond started succesfully. Ready to schedule checks.", 0, 0);
	if (timeScheduler == 1) {
		writeLog("Start time based scheduler...", 0, 0);
	}
	else {
		writeLog("Start classic scheduler timer...", 0, 0);
		snprintf(infostr, infostr_size, "Sleep time is: %.3f\n", sleepTime);
		writeLog(trim(infostr), 0, 0);
	}
	flushLog();
	// Timer is an eternal loop :P
	while (i > 0) {
		if (timeScheduler != 1)
			writeLog("Check for command files.", 0, 0);
		else {
			if (repeate_write == 0) {
				writeLog("Check for command files.", 0, 0);
				repeate_write++;
			}
		}
		checkApiCmds();
		runPluginThreads(decCount);
		if (timeScheduler != 1) {
			snprintf(infostr, infostr_size, "Sleeping for %.3f seconds.\n", sleepTime);
                	writeLog(trim(infostr), 0, 0);
			sleep(sleepTime);
		}
		else {
			qsort(scheduler, decCount, sizeof(struct Scheduler), compare_timestamps);
			//writeLog("VERBOSE: Scheduler sorted. Sleeping for a second.", 0, 0);
			sleep(1);
		}
		if (timeScheduler != 1 || tspr > 0) {
			tspr = 0;
			repeate_write = 0;
			switch (output_type) {
                		case JSON_OUTPUT:
                        		collectJsonData(decCount);
                        		break;
                		case METRICS_OUTPUT:
                        		collectMetrics(decCount, 0);
                        		break;
                		case JSON_AND_METRICS_OUTPUT:
                       			collectJsonData(decCount);
					collectMetrics(decCount, 0);
                       			break;
				case PROMETHEUS_OUTPUT:
					collectMetrics(decCount, 1);
					break;
				case JSON_AND_PROMETHEUS_OUTPUT:
					collectJsonData(decCount);
                                	collectMetrics(decCount, 1);
					break;
                		default:
                        		collectJsonData(decCount);
        		}
		}
		// Set this to timestamp
		if (checkPluginFileStat(pluginDeclarationFile, tPluginFile, 0)) {
			writeLog("Detected change of plugins file.", 0, 0);
			flushLog();
			updatePluginDeclarations();
		}
		// Time to execute gardener?
		if (enableGardener != 0) {
			time_t seconds = time(0);
			if (seconds > tnextGardener) {
				sleep(10);
				executeGardener();
				tnextGardener = seconds + gardenerInterval;
				sleep(10);
			}

		}
		if (enableClearDataCache != 0) {
			time_t seconds = time(0);
			if (seconds > tnextClearDataCache) {
                                writeLog("ClearDataCash is ready", 0, 0);
				clearDataCache();
				tnextClearDataCache = seconds + clearDataCacheInterval;
				sleep(5);
			}
		}
		flushLog();
	}
}

int isConstantsEnabled () {
	FILE *file = NULL;
	char line[10];
	char *searchString = "enable";

	file = fopen("/etc/almond/memalloc.conf", "r");
	if (file == NULL) {
		printf("No constants file will be used.\n");
		writeLog("No memalloc.conf file was found.", 1, 1);
		return 0;
	}
	while (fgets(line, sizeof(line), file)) {
		if (strstr(line, searchString)) {
			writeLog("Constants file is enabled.", 0, 1);
			return 1;
			break;
		}
	}
	return 0;
}

void initLogMessages() {
	for (int i = 0; i < 5; i++) {
		logmessage_id[i] = 0;
	}
}

void initialLogging() {
	char lfin[28] = "/var/log/almond/almond.log";

        fptr = fopen(lfin, "a");
        fprintf(fptr, "\n");
        printf("Starting almond version %s.\n", VERSION);
        initConstants();
        writeLog("Almond constants initialized.", 0, 1);
        writeLog("Starting almond (0.9.8)...", 0, 1);
}

int closeFileHandler() {
	fclose(fptr);
	fptr = NULL;
	return EXIT_FAILURE;
}

void setupSignalHandlers() {
	/*if (signal(SIGINT, sig_handler) == SIG_ERR) {
                logError("An error occurred while setting the SIGINT signal handler.", 2, 1);
                fclose(fptr);
                fptr = NULL;
                return EXIT_FAILURE;
        }
        if (signal(SIGTERM, sig_handler) == SIG_ERR) {
                logError("An error occured while setting sigterm signal handler.", 2, 1);
                fclose(fptr);
                fptr = NULL;
                return EXIT_FAILURE;
        }*/
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
        }
        else {
                logError("Configuration is not valid", 1, 1);
                return 1;
        }
	return 0;
}

void initLoggerThread() {
	fclose(fptr);
        fptr = NULL;
        printf("Initaite logger\n");
        initLogger();
        logInfo("Initiate plugins.", 0, 0);
        fflush(fptr);
}

int loadPlugins() {
	decCount = countDeclarations(pluginDeclarationFile);
        threadIds = (unsigned short*)malloc((size_t)decCount * sizeof(unsigned short));
        for (int i = 0; i < decCount; i++) {
                threadIds[i] = 0;
        }
        declarations = (PluginItem *)malloc((size_t)sizeof(PluginItem) * decCount);
        declaration_size = (size_t)decCount;
        if (!declarations) {
                perror ("Error allocating memory");
                writeLog("Error allocating memory - PluginItem.", 2, 0);
                abort();
        }
        printf("Declarations initiated.\n");
        for (int i = 0; i < decCount; i++) {
                declarations[i].name = malloc((size_t)pluginitemname_size);
                if (declarations[i].name == NULL) {
                        logError("Failed to allocate declarations.", 2, 0);
                        exit(2);
                }
                else
                        declarations[i].name[0] = '\0';
                declarations[i].description = malloc((size_t)pluginitemdesc_size);
                if (declarations[i].description == NULL){
                        logError("Failed to allocate declarations.", 2, 0);
                        exit(2);
                }
                else
                        declarations[i].description[0] = '\0';
                declarations[i].command = malloc((size_t)pluginitemcmd_size);
                if (declarations[i].command == NULL) {
                        logError("Failed to allocate declarations.", 2, 0);
                        exit(2);
                }
                else
                        declarations[i].command[0] = '\0';
        }
        logInfo("Declarations read.", 0, 0);
        outputs = malloc((size_t)sizeof(PluginOutput)*decCount);
        if (!outputs){
                perror("Error allocating memory");
                writeLog("Error allocating memory - PluginOutput.", 2, 0);
                abort();
        }
        for (int i = 0; i < decCount; i++) {
                outputs[i].retString = malloc((size_t)pluginoutput_size);
                if (outputs[i].retString == NULL) {
                        logError("Failed to allocate outputs.", 2, 0);
                        exit(2);
                }
                else
                        outputs[i].retString[0] = '\0';
        }
        output_size = (size_t)decCount;
        int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile, 0);
        time_t dummy; //= time(NULL);
        checkPluginFileStat(pluginDeclarationFile, dummy, 1);
        if (pluginDeclarationResult != 0){
                logInfo("Problem reading from plugin declaration file.", 1, 0);
        }
        else {
                logInfo("Plugin declarations file loaded.", 0, 0);
        }
	return 0;
}

int main(int argc, char* argv[]) {
	initialLogging();
	setupSignalHandlers();
	int configResult = loadConfiguration();
	if (configResult != 0) {
		logError("Failed to load configuration", 1, 1);
		return 1;
	}
	else
		printf("Configuration read.\n");

	if (strcmp(hostName, "None") == 0) { 
		strncpy(hostName, getHostName(), 255);
	}
	writeLog("Initiate logger thread.", 0, 1);
	initLoggerThread();
	if (check_plugin_conf_file(pluginDeclarationFile) != 0) {
                logError("plugins.conf file seems to be corrupt. Program will shut down.", 2, 0);
                return 2;
        }
	logInfo("No errors found in plugins.conf", 0, 0);
	if (loadPlugins() != 0) {
		logError("Failed to load plugin declarations", 2, 0);
		flushLog();
		return 2;
	}
	flushLog();
        initScheduler(decCount, initSleep);
        scheduleChecks();
	sig_handler(SIGSTOP);

   	return 0;
}
