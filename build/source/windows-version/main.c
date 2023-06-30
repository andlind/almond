/*
    NOTE: Due to gcc limitations on Windows this version of Almond
    will not run plugins multithreaded in the current version.
                                                    */
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <winbase.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <winsock.h>
#include <process.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>
#include "structures.h"
#include "jparse.h"

#pragma comment(lib, "w2_32.lib") // Winsock lib

#define MAX_STRING_SIZE 50
#define JSON_OUTPUT 0
#define METRICS_OUTPUT 1
#define JSON_AND_METRICS_OUTPUT 2
#define PROMETHEUS_OUTPUT 3
#define JSON_AND_PROMETHEUS_OUTPUT 4
#define HOWRU_API 10
#define ALMOND_API_PORT 9165
#define SOCKET_READY 1
#define API_READ 10
#define API_RUN 15
#define API_EXECUTE_AND_READ 25
#define API_GET_METRICS 30
#define API_READ_ALL 100
#define API_FLAGS_VERBOSE 1
#define API_DRY_RUN 6
#define INFO_BUFFER_SIZE 32767
#define CONF_BUFFER_LENGTH 255
#define MAX_THREADS 64

char confDir[50];
char dataDir[50];
char storeDir[50];
char logDir[50];
char pluginDir[50] = "plugins/";
char pluginDeclarationFile[75] = "/conf/plugins.conf";
char fileName[100];
char hostName[255] = "None";
char jsonFileName[50] = "monitor_data_c.json";
char metricsFileName[50] = "monitor.metrics";
char gardenerScript[75] = "scripts/gardener.bat";
char metricsOutputPrefix[30] = "almond";
char* socket_message;
char working_directory[100];
PluginItem *declarations;
PluginOutput *outputs;
PluginItem *update_declarations;
PluginOutput *update_outputs;
struct sockaddr_in address;
int initSleep;
int updateInterval;
int schedulerSleep = 5000;
int confDirSet = 0;
int dataDirSet = 0;
int storeDirSet = 0;
//int templateDirSet = 0;
int logDirSet = 0;
int pluginDirSet = 0;
int logPluginOutput = 0;
int pluginResultToFile = 0;
int decCount = 0;
int saveOnExit = 0;
int dockerLog = 0;
int enableGardener = 0;
int enableTimeTuner = 0;
int timeTunerMaster = 1;
int timeTunerCycle = 15;
int timeTunerCounter = 0;
int local_port = 9165;
int local_api = 0;
int standalone = 0;
int quick_start = 0;
int server_fd;
unsigned int socket_is_ready = 0;
unsigned int gardenerInterval = 43200;
unsigned char output_type = 0;
time_t tLastUpdate, tnextUpdate;
time_t tnextGardener;
time_t tPluginFile;
struct sockaddr_in address;
int server_fd;
int api_action = 0;
char* api_args;
int args_set = 0;
FILE *fptr;
char logDir[50];
WSADATA wsa;
SOCKET s, client_socket;
struct sockaddr_in server;
static volatile int g_exit = 0;
HANDLE hApiThread; // hGardener, hPluginThread;

void flushLog();
int getConfigurationValues();
int updatePluginDeclarations();
int redeclarePluginDeclarations(int, int);
int initSocket();
int createSocket(int);
void startApiSocket();
void runGardener();
void executeGardner();
void getHostName();
void timeTune(int);
void checkRetVal(int);
void collectJsonData(int);
void initScheduler(int, int);
void runPlugin(int, int);
void runPluginThreads(int);
void runPluginArgs(int, int, int);
void initNewPlugin(int index);
void collectMetrics(int, int);
void scheduleChecks();
void send_socket_message(int, int, int);
BOOL directoryExists(LPCTSTR szPath);

static INT WINAPI console_ctrl_handler(DWORD dwCtrlType){
    g_exit = 1;
    switch (dwCtrlType){
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT: // Closing the console window
        case CTRL_LOGOFF_EVENT: // User logs off. Passed only to services!
        case CTRL_SHUTDOWN_EVENT: // System is shutting down. Passed only to services!
            //closejsonfile();
			closeSocket();
			fclose(fptr);
			free(declarations);
			free(outputs);
            break;
    }

    return 1;
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

void writeLog(const char *message, int level) {
    char timeStamp[20];
    char wmes[1845] = "";
    size_t dest_size = 20;
    time_t t = time(NULL);
    struct tm * tm;

    time(&t);
    tm = localtime(&t);

    snprintf(timeStamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon +1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    //snprintf(timeStamp, asctime(tm))
    //sprintf(timeStamp, "%d-%d-%d %d:%d:%d")
    strcpy(wmes, timeStamp);
    strcat(wmes, " | ");
    switch(level) {
        case 0:
            strcat(wmes, "[INFO]\t");
            break;
        case 1:
            strcat(wmes, "[WARNING]\t");
            break;
        case 2:
            strcat(wmes, "[ERROR]\t");
            break;
        default:
            strcat(wmes, "[DEBUG]\t");
    }
    strcat(wmes, message);
    fprintf(fptr, "%s\n", wmes);
}

void removeChar(char *str, char garbage) {
        char *src, *dest;
        for (src = dest = str; *src != '\0'; src++){
                *dest = *src;
                if (*dest != garbage) dest++;
        }
        *dest ='\0';
}

/*DWORD WINAPI ThreadFunc(void* data){
    long storeIndex = (long)data;
    if (createSocket(server_fd) != 0) {
        writeLog("Could not create socket.", 1);
    }
    return 0;
}*/

DWORD WINAPI apiThread(void* data){
    long storeIndex = (long)data;
    if (createSocket(server_fd) != 0) {
        writeLog("Could not create socket.", 1);
    }
    return 0;
}

/*DWORD WINAPI gardenerExeThread(void* data) {
    int threadid = (int) data;
	runGardener();
	WaitForSingleObject(hGardener, INFINITE);
	CloseHandle(hGardener);
	return 0;
}*/

/*DWORD WINAPI pluginExeThread(void* data){
    long storeIndex = (long)data;
	runPlugin(storeIndex, 0);
    WaitForSingleObject(hPluginThread, INFINITE);
    CloseHandle(hPluginThread);
}*/

void apiDryRun(int plugin_id) {
	char* pluginName;
	char message[500];
    char command[100];
    char retString[2280];
    char ch = '/';
    PluginOutput output;
    char info[295];
    int rc = 0;
	FILE *fp;

	memset(message, 0, sizeof message);
    //printf("Dry running plugin with id %d\n", plugin_id);
    pluginName = malloc(strlen(declarations[plugin_id].name)+1);
    strcpy(pluginName, declarations[plugin_id].name);
    removeChar(pluginName, '[');
    removeChar(pluginName, ']');
    strcpy(message, "{\n     \"dryExecutePlugin\":\"");
    strcat(message, pluginName);
    strcat(message, "\"");
    strcat(message, ",\n");

    strcpy(command, pluginDir);
    strncat(command, &ch, 1);
    strcat(command, declarations[plugin_id].command);
    snprintf(info, 295, "Running: %s.", declarations[plugin_id].command);
    writeLog(trim(info), 0);
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        writeLog("Failed to run command.", 2);
    }
    while (fgets(retString, sizeof(retString), fp) != NULL) {
        // VERBOSE  printf("%s", retString);
    }
    rc = pclose(fp);
    if (rc > 0) {
        if (rc == 256)
            output.retCode = 1;
        else if (rc == 512)
            output.retCode = 2;
        else
            output.retCode = rc;
    }
    else
        output.retCode = rc;
    strcpy(output.retString, trim(retString));
    strcat(message, "     \"pluginOutput:\":\"");
	strcat(message, trim(output.retString));
    strcat(message, "\"");
	strcat(message, "\n}\n");
    socket_message = malloc(strlen(message)+1);
    strcpy(socket_message, message);
}

void apiRunPlugin(int plugin_id, int flags) {
	char* pluginName;
	char message[500];

	memset(message, 0, sizeof message);
	//printf("Executing plugin with id %d\n", plugin_id);
	pluginName = malloc(strlen(declarations[plugin_id].name)+1);
    strcpy(pluginName, declarations[plugin_id].name);
    removeChar(pluginName, '[');
    removeChar(pluginName, ']');
	runPlugin(plugin_id, 0);
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
	socket_message = malloc(strlen(message)+1);
	strcpy(socket_message, message);
}

void runPluginArgs(int id, int aflags, int api_action) {
	const char space[1] = " ";
	char* command;
	char* newcmd;
	char* pluginName;
	int pos;
	FILE *fp;
    char retString[2280];
    char ch = '/';
    PluginOutput output;
    //clock_t t;
    char currTime[20];
    char info[295];
	char message[2000];
	char rCode[3];
    int rc = 0;

    //t = clock();
	memset(message, 0, sizeof message);
	newcmd = malloc(200);
	command = malloc(strlen(declarations[id].command)+1);
	pluginName = malloc(strlen(declarations[id].name)+1);
    strcpy(pluginName, declarations[id].name);
    removeChar(pluginName, '[');
    removeChar(pluginName, ']');
	strcpy(command, declarations[id].command);
    strcpy(newcmd, pluginDir);
    strncat(newcmd, &ch, 1);
	char * token = strtok(command, " ");
	strcat(newcmd, token);
	strcat(newcmd, space);
	strcat(newcmd, api_args);

	fp = popen(newcmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        writeLog("Failed to run command.", 2);
		strcpy(message, "\n{ \"failedToRun\":\"");
	 	strcat(message, newcmd);
		strcat(message, "\"}");
        socket_message = malloc(strlen(message)+1);
        strcpy(socket_message, message);
		free(api_args);
		free(command);
		memset(&newcmd[0], 0, sizeof(newcmd));
		free(newcmd);
		free(pluginName);
		return;
    }
    while (fgets(retString, sizeof(retString), fp) != NULL) {
        // VERBOSE  printf("%s", retString);
    }
    rc = pclose(fp);

    if (rc > 0) {
        if (rc == 256)
            output.retCode = 1;
        else if (rc == 512)
            output.retCode = 2;
        else
            output.retCode = rc;
    }
    else
        output.retCode = rc;
    strcpy(output.retString, trim(retString));
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
        //struct tm tNextTime;
        //memset(&tNextTime, '\0', sizeof(struct tm));
        //localtime_r(&nextTime, &tNextTime);
        //struct tm *tNextTime;
        struct tm *tNextTime;
        tNextTime = localtime(&nextTime);
        snprintf(declarations[id].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime->tm_year + 1900, tNextTime->tm_mon +1, tNextTime->tm_mday, tNextTime->tm_hour, tNextTime->tm_min, tNextTime->tm_sec);
        declarations[id].nextRun = nextTime;
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
    socket_message = malloc(strlen(message)+1);
    strcpy(socket_message, message);
	free(api_args);
	free(command);
	memset(&newcmd[0], 0, sizeof(newcmd));
	free(newcmd);
}

void apiRunAndRead(int plugin_id, int flags) {
	char* pluginName;
	char rCode[3];
    char message[2000];

	memset(message, 0, sizeof message);
    //printf("Executing plugin with id %d\n", plugin_id);
    pluginName = malloc(strlen(declarations[plugin_id].name)+1);
    strcpy(pluginName, declarations[plugin_id].name);
    removeChar(pluginName, '[');
    removeChar(pluginName, ']');
    runPlugin(plugin_id, 0);
	strcpy(message, "{\n     \"executePlugin\":\"");
    strcat(message, pluginName);
    strcat(message, "\",\n");
    strcat(message, "      \"result\": {\n");
	sleep(10);
	if (flags == API_FLAGS_VERBOSE) {
		strcat(message, "          \"name\":\"");
		strcat(message, pluginName);
		free(pluginName);
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
	socket_message = malloc(strlen(message)+1);
	strcpy(socket_message, message);
}

void apiReadFile(char fileName[100], int type) {
	FILE *f;
	char info[70];
	char * message;
    long length;
    int err = 0;

	f = fopen(fileName, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        message = malloc(length);
        if (message) {
            fread(message, 1, length, f);
        }
        fclose(f);
    }
    else err++;

    if (message) {
        socket_message = malloc(strlen(message)+1);
        strcpy(socket_message, message);
    }
    else err++;
    if (err > 0) {
        if (type == 2)
            snprintf(info, 70, "{ \"return_info\":\"Could not read metrics file. No results found.\"}\n");
		else
			snprintf(info, 70, "{ \"return_info\":\"Could not read almond file. No results found.\"}\n");
        socket_message = malloc(71);
        strcpy(socket_message, info);
    }
    free(message);
}

void apiGetMetrics() {
	char ch = '/';
    char storeName[100];

    strcpy(storeName, storeDir);
    strncat(storeName, &ch, 1);
    strcat(storeName, metricsFileName);
	apiReadFile(storeName, 2);
}

void apiReadAll() {
	char ch = '/';
    char fileName[100];

	strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);
	apiReadFile(fileName, 0);
}

void apiReadData(int plugin_id, int flags){
    char* pluginName;
	char rCode[3];
	char message[2000];

    memset(message, 0, sizeof message);
	printf("Collecting data from plugin id %d\n", plugin_id);
	pluginName = malloc(strlen(declarations[plugin_id].name)+1);
    strcpy(pluginName, declarations[plugin_id].name);
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
	strcat(message, "}");
	free(pluginName);
	socket_message = malloc(strlen(message)+1);
	strcpy(socket_message, message);
}

void send_socket_message(int socket, int id, int aflags) {
    char header[100] = "HTTP/1.1 200 OK\nContent-Type:application/txt\nContent-Length: ";
    char message[60];
    char* send_message;
	int content_length;
	char len[4];

	printf("DEBUG: api_action = %d\n", api_action);

	switch (api_action) {
        case API_READ:
            printf("DEBUG: API_READ\n");
            apiReadData(id, aflags);
            break;
        case API_RUN:
            //strcat(socket_message, apiRunPlugin(id, aflags));
            apiRunPlugin(id, aflags);
            break;
        case API_DRY_RUN:
            //strcat(socket_message, apiRunPlugin(id, aflags));
            apiDryRun(id);
            break;
        case API_EXECUTE_AND_READ:
            //strcat(socket_message, apiRunAndRead(id, aflags));
            apiRunAndRead(id, aflags);
            break;
        case API_GET_METRICS:
            apiGetMetrics();
            break;
        case API_READ_ALL:
            apiReadAll();
            break;
        default:
            printf("DEBUG: No action\n");
            socket_message = malloc(61);
            strcat(message, "{     \"return\":\"The request did not trigger any action\" }\n");
            strcpy(socket_message, message);
	}

    printf("socket message = %s\n", socket_message);

    content_length = strlen(socket_message);
	sprintf(len, "%d", content_length);
    strcat(header, trim(len));
    strcat(header, "\n\n");
	content_length += strlen(header);
	send_message = malloc(content_length+1);
    strcpy(send_message, header);
	strcat(send_message, socket_message);
	printf("send_message = %s\n", send_message);
    if (send(socket, send_message, strlen(send_message), 0) < 0) {
        writeLog("Could not send message to client.", 1);
    }
	writeLog("Message sent on socket. Closing connection.", 0);
    closesocket(socket);
    //WSACleanup();
	free(send_message);
	memset(&socket_message[0], 0, sizeof(socket_message));
	socket_message[0] = (char) 0;
	free(socket_message);
	CloseHandle(hApiThread);
	flushLog();
    startApiSocket();
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

void runGardener() {
	FILE *fp;
	char retString[1035];
	char info[255];
	int rc = 0;

	fp = popen(gardenerScript, "r");
    if (fp == NULL) {
        printf("Failed to run gardener script\n");
        writeLog("Failed to run gardener script.", 2);
    }
    while (fgets(retString, sizeof(retString), fp) != NULL) {
            // VERBOSE  printf("%s", retString);
    }
    rc = pclose(fp);
    snprintf(info, 225, "Gardener script executed with return code %i.", rc);
    if (rc > 1) {
        writeLog(trim(info), 2);
    }
    else writeLog(trim(info), rc);
}

void executeGardener() {
    char logInfo[200];
    //DWORD lpThreadId;
    // This should run in thread
    /*hGardener = CreateThread(NULL, 0, gardenerExeThread, NULL, 0, &lpThreadId);
    if (hApiThread == NULL){
        writeLog("Error creating API thread.", 2);
    }
    else {
        snprintf(logInfo, 200, "Created new thread (%lu) truncating metricd logs (gardener) %d \n", lpThreadId);
        writeLog(trim(logInfo), 0);
    }*/
    runGardener();
    flushLog();
}

int fileExists(const char *checkFile) {
	if (access(checkFile, F_OK) == 0)
		return 0;
	else
		return 1;
}

int initSocket() {
    printf("Init Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error code :%d\n", WSAGetLastError());
        writeLog("Could not initiate Winsock.", 2);
        return -1;
    }
    printf("Initialised");
    if (( s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
        printf("Could not create socket: %d\n", WSAGetLastError());
        writeLog("Could not create socket.", 2);
        return -1;
    }
    printf("Socket created.\n");
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9165);
    if (local_port == ALMOND_API_PORT){
        server.sin_port = htons(ALMOND_API_PORT);
    }
    else
        server.sin_port = htons(local_port);
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR){
        printf("Bind failed with error code: %d\n", WSAGetLastError());
        writeLog("Failed to bind port.", 2);
        return -1;
    }
    puts("Bind done");
    writeLog("Almond socket initialized.", 0);
    socket_is_ready = 1;
    return socket_is_ready;
}

int createSocket(int server_fd) {
    struct sockaddr_in client;
    int c;
    char info[100];
    char buffer[1024] = { 0 };
    char server_message[2000], client_message[2000];
    char message[100];
    char *e;
    int index;
    int params[2];
    int iResult;

    api_action = 0;
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    if (listen(s, 1) < 0) {
        writeLog("Failed listening..", 2);
        socket_is_ready = 0;
        return -1;
    }
    puts("Wait for incomming connections.");
    snprintf(info, 100, "Ready listening on port %d.", local_port);
    writeLog(trim(info), 0);
    c = sizeof(struct sockaddr_in);
    client_socket = accept(s, (struct sockaddr *)&client, &c);
    if (client_socket == INVALID_SOCKET){
        printf("Accept failed with error code: %d\n", WSAGetLastError());
        writeLog("Could not accept client socket. ", 1);
        return -1;
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    snprintf(info, 100, "Client connected at IP: %s and port: %i\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    writeLog(trim(info), 0);
    /*if (recv(client_socket, client_message, sizeof(client_message), 0) < 0){
        printf("Couldn't receive\n");
        writeLog("Recv failed.\n", 2);
        writeLog("Could not receieve client message on socket.", 1);
        flushLog();
        //return -1;
    }
    e = strchr(client_message, '{');
    index = (int)(e - client_message);
    strncpy(message, client_message + index, strlen(client_message) - index);
    printf("Message: %s\n", message);*/
    //parseClientMessage(message, params);
    writeLog("Message received on socket.", 0);
    //do {
    iResult = recv(client_socket, client_message, sizeof(client_message), 0);
    if (iResult > 0){
        e = strchr(client_message, '{');
        index = (int)(e - client_message);
        if (index > 0)
            strncpy(message, client_message + index, strlen(client_message) - index);
        else
            strncpy(message, client_message, strlen(client_message));
        printf("Bytes received: %d\n", iResult);
    }
    else if (iResult == 0)
        printf("Connection closing...\n");
    else {
        printf("recv failed with error: %d\n", WSAGetLastError());
    }
    //} while (iResult > 0);
    flushLog();
    //printf("Message: %s\n", message);
    api_action = parseMessage(message, params);
    printf("DEBUG: Return action = %d", api_action);
	int id = params[0];
	int aflags = params[1];
	printf("DEBUG : id = %d, aflags = %d\n", id, aflags);
    send_socket_message(client_socket, id, aflags);
    return 0;
}

void startApiSocket() {
    char logInfo[200];
    DWORD lpThreadId;
    // This should run in thread
    hApiThread = CreateThread(NULL, 0, apiThread, NULL, 0, &lpThreadId);
    if (hApiThread == NULL){
        writeLog("Error creating API thread.", 2);
    }
    else {
        snprintf(logInfo, 200, "Created new thread (%lu) listening for connections on port %d \n", lpThreadId, local_port);
        writeLog(trim(logInfo), 0);
        //printf("New thread accepting socket created.\n");
    }
    flushLog();
}

int countDeclarations(char *file_name) {
	FILE *fp;
	int i = 0;
	char c;

	writeLog("Count declarations.", 0);

    fp = fopen(file_name, "r");
	if (fp == NULL)
    {
        perror("Error while opening the file.\n");
		writeLog("Error opening and counting declarations file.", 2);
        exit(EXIT_FAILURE);
    }
	for (c = getc(fp); c != EOF; c = getc(fp)){
        //printf("Count = %d\n", i);
		if (c == '\n')
			i++;
	}
	fclose(fp);
	return i-1;
	//return i;
}

int loadPluginDeclarations(char *pluginDeclarationsFile, int reload) {
	int hasFaults = 0;
	int counter = 0;
	int i;
	int index = 0;
	char *token;
	char *name;
	char *description;
	char loginfo[60];
    FILE *fp;
	char *saveptr;
	PluginItem item;
	char line[CONF_BUFFER_LENGTH];

    fp = fopen(pluginDeclarationsFile, "r");
	if (fp == NULL)
    {
        perror("Error while opening the file.\n");
		writeLog("Error opening the plugin declarations file.", 2);
        exit(EXIT_FAILURE);
    }
	//while ((read = getline(&line, &len, fp)) != -1) {
	i = 0;
    while (fgets(line, CONF_BUFFER_LENGTH, fp)) {
		index++;
		if (strchr(line, '#') == NULL){
            char* strline;
            strline = malloc(CONF_BUFFER_LENGTH);
            strncpy(strline, line, strlen(line));
            for (i = 1, strline;; i++, strline=NULL) {
                token = strtok_r(strline, ";", &saveptr);
                if (token == NULL)
                    break;
                //printf("%d: %s\n", i, token);
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
            snprintf(loginfo, 60, "Declaration with index %d is created.\n", counter);
            writeLog(trim(loginfo), 0);
            if (reload == 0)
                declarations[counter] = item;
            else
                update_declarations[counter] = item;
            counter++;
		}
	}
    fclose(fp);
	return 0;
}

void flushLog() {
    char logFile[100];
    char ch = '\\';

    strcpy(logFile, logDir);
    strncat(logFile, &ch, 1);
    strcat(logFile, "almond.log");
    fclose(fptr);
    //printf("%s\n", logFile);
    sleep(0.10);
    //fptr = fopen(logFile, "a");
    fptr = fopen("almond.log", "a");
}

int getConfigurationValues() {
	char* file_name;
	char line[CONF_BUFFER_LENGTH];
	FILE *fp;
	int index = 0;
	file_name = "conf/almond.conf";
    fp = fopen(file_name, "r");
	char confName[MAX_STRING_SIZE] = "";
    char confValue[MAX_STRING_SIZE] = "";

	if (fp == NULL)
   	{
        perror("Error while opening the file.\n");
		writeLog("Error opening configuration file", 2);
        exit(EXIT_FAILURE);
   	}

	//while ((read = getline(&line, &len, fp)) != -1) {
	while (fgets(line, CONF_BUFFER_LENGTH, fp)) {
        line[strcspn(line, "\n")] = 0;
        char * token = strtok(line, "=");
        while (token != NULL)
        {
            if (index == 0)
            {
                strcpy(confName, token);
                //printf("%s\n", confName);
            }
            else
            {
                strcpy(confValue, token);
                //printf("%s\n", confValue);
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
                writeLog("Almond will run standalone. No monitor data will be sent to HowRU.", 0);
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
                writeLog("Almond will enable local api.", 0);
            }
        }
        if (strcmp(confName, "scheduler.confDir") == 0) {
            if (directoryExists(confValue) == 0) {
                size_t dest_size = sizeof(confValue);
                snprintf(confDir, dest_size, "%s", confValue);
                confDirSet = 1;
            }
            else {
                //int status = mkdir(trim(confValue), 0777);
                int status = mkdir("conf");
                if(status != 0 && errno != EEXIST){
                    printf("Failed to create directory. Errno: %d\n", errno);
                    writeLog("Error creating configuration directory.", 2);
                }
                else{
                    strncpy(confDir, trim(confValue), strlen(confValue));
                    confDirSet = 1;
                }
            }
            //printf("%s\n", confDir);
            writeLog("Configuration directory is set.", 0);
        }
        if (strcmp(confName, "scheduler.quickStart") == 0) {
            int i = strtol(trim(confValue), NULL, 0);
            if (i >= 1) {
                writeLog("Almond scheduler have quick start activated.", 0);
                quick_start = 1;
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
                writeLog("Exporting both to json and to metrics file.", 0);
                output_type = JSON_AND_METRICS_OUTPUT;
            }
            else if (strcmp(trim(confValue), "prometheus") == 0) {
                printf("Export to prometheus.\n");
                writeLog("Export to prometheus style metrics.\n", 0);
                output_type = PROMETHEUS_OUTPUT;
            }
            else if (strcmp(trim(confValue), "jsonprometheus") == 0) {
                printf("Export to both json and Prometheus style metrics.\n");
                writeLog("Exporting to both json and prometheus style metrics.", 0);
                output_type = JSON_AND_PROMETHEUS_OUTPUT;
            }
            else {
                printf("%s is not a valid value.  supported at this moment.\n", confValue);
                writeLog("Unsupported value in configuration scheduler.format.", 1);
                writeLog("Using standard output (JSON_OUTPUT).", 0);
                output_type = JSON_OUTPUT;
            }
        }
        if (strcmp(confName, "scheduler.initSleepMs") == 0) {
            int i = strtol(trim(confValue), NULL, 0);
            if (i < 5000)
                i = 7000;
            initSleep = i;
            writeLog("Init sleep for scheduler read.", 0);
        }
        if (strcmp(confName, "scheduler.sleepMs") == 0) {
            char mes[40];
            int i = strtol(trim(confValue), NULL, 0);
            if (i < 2000)
                i = 2000;
            snprintf(mes, 40, "Scheduler sleep time is %d ms.", i);
            writeLog(trim(mes), 0);
            schedulerSleep = i;
        }
        if (strcmp(confName, "scheduler.dataDir") == 0) {
            if (directoryExists(confValue) == 0) {
                size_t dest_size = sizeof(confValue);
                snprintf(dataDir, dest_size, "%s", confValue);
                dataDirSet = 1;
            }
            else {
                int status = mkdir("conf");
                if (status != 0 && errno != EEXIST) {
                    printf("Failed to create directory. Errno: %d\n", errno);
                    writeLog("Error creating HowRU dataDir.", 2);
                }
                else {
                    strncpy(dataDir, trim(confValue), strlen(confValue));
                    dataDirSet = 1;
                }
            }
        }
        if (strcmp(confName, "scheduler.storeDir") == 0) {
            if (directoryExists(confValue) == 0) {
                size_t dest_size = sizeof(confValue);
                snprintf(storeDir, dest_size, "%s", confValue);
                storeDirSet = 1;
            }
            else {
                int status = mkdir("data");
                if (status != 0 && errno != EEXIST) {
                    printf("Failed to create directory. Errno: %d\n", errno);
                    writeLog("Error creating HowRU storeDir.", 2);
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
            if (directoryExists(confValue) == 0) {
                size_t dest_size = sizeof(confValue);
                snprintf(logDir, dest_size, "%s", confValue);
                logDirSet = 1;
            }
            else {
                int status = mkdir("log");
                if (status != 0 && errno != EEXIST) {
                    printf("Failed to create directory. Errno: %d\n", errno);
                    writeLog("Error creating log directory.", 2);
                }
                else {
                    strncpy(logDir, trim(confValue), strlen(confValue));
                    logDirSet = 1;
                }
            }
            if (strcmp(confValue, "/var/log/almond") != 0) {
                char ch =  '/';
                char fileName[100];
                FILE *logFile;
                strcpy(fileName, logDir);
                strncat(fileName, &ch, 1);
                strcat(fileName, "almond.log");
                writeLog("Closing logfile...", 0);
                fclose(fptr);
                sleep(0.2);
                logFile = fopen("/var/log/almond/almond.log", "r");
                fptr = fopen(fileName, "a");
                if (fptr == NULL) {
                    fclose(logFile);
                    fptr = fopen("/var/log/almond/almond.log", "a");
                    writeLog("Could not create new logfile.", 1);
                    writeLog("Reopened logfile '/var/log/almond/almond.log'.", 0);
                }
                else {
                    while ( (ch = fgetc(logFile)) != EOF)
                        fputc(ch, fptr);
                    fclose(logFile);
                    writeLog("Created new logfile.", 0);
                }
            }
        }
        if (strcmp(confName, "scheduler.logPluginOutput") == 0) {
            if (atoi(confValue) == 0) {
                writeLog("Plugin outputs will not be written in the log file", 0);
            }
            else {
                writeLog("Plugin outputs will be written to the log file", 0);
                logPluginOutput = 1;
            }
        }
        if (strcmp(confName, "scheduler.storeResults") == 0) {
            if (atoi(confValue) == 0) {
                writeLog("Plugin results is not stored in specific csv file.", 0);
            }
            else {
                writeLog("Plugin results will be stored in csv file.", 0);
                pluginResultToFile = 1;
            }
        }
        if (strcmp(confName, "scheduler.hostName") == 0) {
            char info[300];
            strncpy(hostName, trim(confValue), strlen(confValue));
            snprintf(info, 300, "Scheduler will name this host: %s", hostName);
            writeLog(trim(info), 0);
        }
        if (strcmp(confName, "plugins.directory") == 0) {
            if (directoryExists(confValue) == 0) {
                size_t dest_size = sizeof(confValue);
                printf("DEBUG: confValue = %s\n", confValue);
                snprintf(pluginDir, dest_size, "%s", confValue);
                printf("DEBUG: pluginDir = %s\n", pluginDir);
                pluginDirSet = 1;
            }
            else {
                int status = mkdir("plugins");
                if (status != 0 && errno != EEXIST) {
                    printf("Failed to create directory. Errno: %d\n", errno);
                    writeLog("Error creating plugins directory.", 2);
                }
                else {
                    strncpy(pluginDir, trim(confValue), strlen(confValue));
                    pluginDirSet = 1;
                }
            }
        }
        if (strcmp(confName, "plugins.declarations") == 0) {
            writeLog(trim(confValue), 3);
            if (access(trim(confValue), F_OK) == 0){
                strncpy(pluginDeclarationFile, trim(confValue), strlen(confValue));
            }
            else {
                printf("ERROR: Plugin declaration file does not exist.");
                writeLog("Plugin declaration file does not exist.", 2);
                return 1;
            }
        }
        if (strcmp(confName, "scheduler.metricsOutputPrefix") == 0) {
            char info[300];
            if (strlen(confValue) <= 30) {
                strncpy(metricsOutputPrefix, trim(confValue), strlen(confValue));
                snprintf(info, 300, "Metrics output prefix is set to '%s'", metricsOutputPrefix);
                writeLog(trim(info), 0);
            }
            else {
                writeLog("Could not change metricsOutputPrefix. Prefix too long.", 1);
            }
        }
        if (strcmp(confName, "scheduler.enableGardener") == 0) {
            if (atoi(confValue) == 0) {
                writeLog("Metrics gardener is not enabled.", 0);
            }
            else {
                writeLog("Metrics gardener is enabled.", 0);
                enableGardener = 1;
            }
        }
        if (strcmp(confName, "scheduler.gardenerScript") == 0) {
            if (access(trim(confValue), F_OK) == 0){
                strncpy(gardenerScript, trim(confValue), strlen(confValue));
            }
            else {
                enableGardener = 0;
                writeLog("Gardener script file could not be found", 1);
                writeLog("Metrics gardener is disabled.", 2);
            }
        }
        if (strcmp(confName, "scheduler.gardenerRunInterval") == 0) {
            char mes[40];
            int i = strtol(trim(confValue), NULL, 0);
            if (i < 60)
                i = 43200;
            snprintf(mes, 40, "Gardener run interval is %d seconds.", i);
            writeLog(trim(mes), 0);
            gardenerInterval = i;
        }
        if (strcmp(confName, "scheduler.tuneTimer") == 0) {
            if (atoi(confValue) == 0) {
                writeLog("Timer tuner is not enabled.", 0);
            }
            else {
                writeLog("Timer tuner is enabled.", 0);
                enableTimeTuner = 1;
            }
        }
        if (strcmp(confName, "scheduler.tunerCycle") == 0) {
            char info[40];
            int i = strtol(trim(confValue), NULL, 15);
            snprintf(info, 40, "Time tuner cycle is set to %d.", i);
            writeLog(trim(info), 0);
            timeTunerCycle = i;
        }
        if (strcmp(confName, "scheduler.tuneMaster") == 0) {
            char info[40];
            int i = strtol(trim(confValue), NULL, 1);
            snprintf(info, 40, "Time tuner cycle is set to %d.", i);
            writeLog(trim(info), 0);
            timeTunerMaster = i;
        }
        if (strcmp(confName, "data.jsonFile") == 0) {
            char info[100];
            strncpy(jsonFileName, trim(confValue), strlen(confValue));
            snprintf(info, 100, "Json data will be collected in file: %s.", jsonFileName);
            writeLog(trim(info), 0);
        }
        if (strcmp(confName, "data.metricsFile") == 0) {
            char info[100];
            strncpy(metricsFileName, trim(confValue), strlen(confValue));
            snprintf(info, 100, "Metrics will be collected in file: %s", metricsFileName);
            writeLog(trim(info), 0);
        }
        if (strcmp(confName, "data.metricsOutputPrefix") == 0) {
            char info[300];
            if ((int)strlen(confValue) <= 30) {
                strncpy(metricsOutputPrefix, trim(confValue), strlen(confValue));
                snprintf(info, 300, "Metrics output prefix is set to '%s'", metricsOutputPrefix);
                writeLog(trim(info), 0);
            }
            else {
                writeLog("Could not change metricsOutputPrefix. Prefix too long.", 1);
            }
        }
        if (strcmp(confName, "data.saveOnExit") == 0) {
            if (atoi(confValue) == 0) {
                writeLog("Json data will be deleted on shutdown.", 0);
            }
            else {
                writeLog("Data file will be saved in data directory after shutdown.", 0);
                saveOnExit = 1;
            }
        }
    }
	updateInterval = 60;
   	fclose(fp);
   	return 0;
}

void collectJsonData(int decLen){
	int retVal = 0;
	char ch = '/';
	char fileName[100];
	char* pluginName;
	FILE *fp;
    clock_t t;
	char info[225];

	/*strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);*/
	strcpy(fileName, "data/monitor_data.json");
	snprintf(info, 225, "Collecting data to file: %s", fileName);
	writeLog(trim(info), 0);
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
		pluginName = malloc(strlen(declarations[i].name)+1);
		strcpy(pluginName, declarations[i].name);
		removeChar(pluginName, '[');
		removeChar(pluginName, ']');
		fputs("      {\n", fp);
		fprintf(fp, "         \"name\":\"%s\",\n", pluginName);
		free(pluginName);
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
	t = clock() -t;
	//double collection_time = ((double)t)/CLOCKS_PER_SEC;
	//printf("Data collection took %f seconds to execute.\n", collection_time);
	//printf("Data collection took %.0f miliseconds to execute.\n", (double)t);
	snprintf(info, 225, "Data collection took %.0f miliseconds to execute.", (double)t);
	writeLog(trim(info), 0);
}

int redeclarePluginDeclarations(int mode, int count) {
	char loginfo[400];
	int c;
	int rows = 0;

	//int newCount = countDeclarations(pluginDeclarationFile);
	writeLog("Needs to redeclare declarations.", 0);
	update_declarations = malloc(sizeof(PluginItem) * count);
	if (!update_declarations) {
		perror ("Error allocating memory");
		writeLog("Error allocating memory - PluginItem.", 2);
		abort();
		return 2;
	}
	writeLog("Needs to reallocate memory for outputs.", 0);
	update_outputs = malloc(sizeof(PluginOutput) * count);
	if (!update_outputs){
		perror("Error allocating memory");
		writeLog("Error allocating memory - PluginOutput.", 2);
		abort();
		return 2;
	}
	int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile, 1);
	if (pluginDeclarationResult != 0){
		printf("ERROR: Problem reading plugin declaration file.\n");
		writeLog("Problem reading from plugin declaration file.", 1);
	}
	else {
		printf("Declarations read.\n");
		writeLog("Plugin declarations file reloaded.", 0);
	}
	switch(mode) {
		case 0:
			for (int i = 0; i < decCount; i++) {
                // Compare before assign
                int missing = 0;
                for (int j = 0; j < decCount; j++) {
                    if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
                        if (i == j) {
                            snprintf(loginfo, 400, "Redeclare %s with id %d\n", update_declarations[i].name, i+1);
                            writeLog(trim(loginfo), 0);
                        }
                        else {
                            snprintf(loginfo, 400, "Redeclare %s with new id. Id is now %d\n", update_declarations[i].name, i);
                            writeLog(trim(loginfo), 1);
                        }
                        update_declarations[i] = declarations[j];
                        update_outputs[i] = outputs[j];
                        missing++;
                        break;
                    }
                }
                if (missing == 0) {
                    writeLog("Needs to declare new plugin.", 0);
                    initNewPlugin(i);
                    flushLog();
					//update_declarations[i] = declarations[i];
					//update_outputs[i] = outputs[i];
                }
			}
            for (int i = decCount; i < count; ++i) {
                printf("Check for new plugins.\n");
                initNewPlugin(i);
            }
			free(declarations);
            declarations = update_declarations;
            free(outputs);
            outputs = update_outputs;
            update_declarations = NULL;
            update_outputs = NULL;
			break;
		case 1:
			for (int i = 0; i < count; i++) {
				int found = 0;
				for (int j = 0; j < decCount; j++) {
					if (strcmp(update_declarations[i].name, declarations[j].name) == 0) {
						if (i == j) {
						 	snprintf(loginfo, 400, "Redeclare %s with id %d\n", update_declarations[i].name, i+1);
                            writeLog(trim(loginfo), 0);
                        }
                        else {
                            snprintf(loginfo, 400, "Redeclare %s with new id. Id is now %d\n", update_declarations[i].name, i);
                            writeLog(trim(loginfo), 1);
                        }
						update_outputs[i] = outputs[j];
                        strcpy(update_declarations[i].lastRunTimestamp,declarations[j].lastRunTimestamp);
                        strcpy(update_declarations[i].nextRunTimestamp,declarations[j].nextRunTimestamp);
                        strcpy(update_declarations[i].statusChanged,declarations[j].statusChanged);
						found++;
						break;
					}
					else {
						snprintf(loginfo, 400, "Old plugin declaration '%s' with id %d marked for deletion.", declarations[j].name, declarations[j].id);
                        writeLog(trim(loginfo), 1);
					}
				}
				if (found == 0) {
					initNewPlugin(i);
				}
			}
			free(declarations);
			declarations = update_declarations;
			free(outputs);
			outputs = update_outputs;
			update_declarations = NULL;
			update_outputs = NULL;
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
				//printf ("count = %d\n", c);
				//printf ("rows = %d\n", rows);
			}
            update_declarations = NULL;
			update_outputs = NULL;
			/*if (rows > 0) {
				printf ("Changed on %d rows\n", rows);
			}*/
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
					update_outputs[i] = outputs[id];
                    strcpy(update_declarations[i].lastRunTimestamp,declarations[id].lastRunTimestamp);
                    strcpy(update_declarations[i].nextRunTimestamp,declarations[id].nextRunTimestamp);
                    strcpy(update_declarations[i].statusChanged,declarations[id].statusChanged);
                    update_declarations[i].nextRun = declarations[id].nextRun;
				}
				else {
					printf("Did not find declaration.name = %s", update_declarations[i].name);
					initNewPlugin(i);
				}
			}
			free(declarations);
            declarations = update_declarations;
            free(outputs);
            outputs = update_outputs;
			update_declarations = NULL;
            update_outputs = NULL;
			break;
	}
    decCount = count;
	flushLog();
	return 0;
}

int updatePluginDeclarations() {
	FILE *fp;
	char* line;
	char* name;
    char *token;
	char *saveptr;
	int i;
	int index = 0;
	int counter = 0;
	int reload_required = 0;
	char loginfo[400];
	size_t len = 0;
	ssize_t read;
	PluginItem item;

	int newCount = countDeclarations(pluginDeclarationFile);

	if (newCount > decCount) {
		int retVal = redeclarePluginDeclarations(0, newCount);
		checkRetVal(retVal);
		return 1;
	}
	else if (newCount < decCount) {
		int retVal = redeclarePluginDeclarations(1, newCount);
		checkRetVal(retVal);
		return 1;
	}
	else {
		// Read plugin declarations file and update declarations
		// This causes errors if changing orders
		// Adapt to new redeclare function
		// This only works if you edit line in current positions
		if (redeclarePluginDeclarations(2, newCount) > 4) {
            printf ("Needs total reload...\n");
            int retVal = redeclarePluginDeclarations(3, newCount);
            checkRetVal(retVal);
            return 1;
        }
		fp = fopen(pluginDeclarationFile, "r");
        if (fp == NULL) {
            perror("Error while opening the file.\n");
            writeLog("Error opening the plugin declarations file.", 2);
            exit(EXIT_FAILURE);
        }
        while (fgets(line, CONF_BUFFER_LENGTH, fp)) {
		index++;
		if (strchr(line, '#') == NULL){
            char* strline;
            strline = malloc(CONF_BUFFER_LENGTH);
            strncpy(strline, line, strlen(line));
            for (i = 1, strline;; i++, strline=NULL) {
                token = strtok_r(strline, ";", &saveptr);
                    if (token == NULL)
                        break;
                    //printf("%d: %s\n", i, token);
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
                int x = 0;
                if (strcmp(declarations[counter].name, item.name) != 0) {
                    snprintf(loginfo, 300, "Declaration name for item %i changed from %s to %s.", counter, declarations[counter].name, item.name);
                    strncpy(declarations[counter].name, item.name, strlen(item.name) + 1);
                    reload_required = 1;
                    writeLog(trim(loginfo), 0);
                }
                if (strcmp(declarations[counter].description, item.description) != 0) {
                    snprintf(loginfo, 300, "Declaration description for %s changed from %s to %s.", declarations[counter].name, declarations[counter].description, item.description);
                    strncpy(declarations[counter].description, item.description, strlen(item.description) + 1);
                    writeLog(trim(loginfo), 0);
                }
                if (strcmp(declarations[counter].command, item.command) != 0) {
                    snprintf(loginfo, 400, "Declaration command for %s changed to %s.", declarations[counter].name, item.command);
                    strncpy(declarations[counter].command, item.command, strlen(item.command) + 1);
                    writeLog(trim(loginfo), 0);
                }
                if (declarations[counter].active != item.active) {
                    if (item.active == 0) {
                        snprintf(loginfo, 300, "Declaration %s is now inactive.", declarations[counter].name);
                        declarations[counter].active = 0;
                    }
                    else {
                        snprintf(loginfo, 300, "Declaration %s is now active", declarations[counter].name);
                        declarations[counter].active = 1;
                    }
                    writeLog(trim(loginfo), 0);
                }
                if (declarations[counter].interval != item.interval) {
                    snprintf(loginfo, 300, "Declaration %s interval changed from %i to %i.", declarations[counter].name, declarations[counter].interval, item.interval);
                    declarations[counter].interval = item.interval;
                    writeLog(trim(loginfo), 0);
                }
                counter++;
            }
        }
        fclose(fp);
        if (reload_required) {
            printf("Reload required.\n");
            writeLog("Changed declaration name might cause inconsistencies. Will reload all plugins.", 1);
            flushLog();
            int retVal = redeclarePluginDeclarations(2, newCount);
            checkRetVal(retVal);
            return 1;
        }
    return 0;
    }
}

void runPlugin(int storeIndex, int update) {
    FILE *fp;
	char command[100];
	char readcmd[255];
	char retString[2280];
	//char ch = '/';
	PluginOutput output;
	clock_t t;
	char currTime[20];
	char o_currTime[20];
	char info[295];
	int rc = 0;

	t = clock();
	//strcpy(command, pluginDir);
	//printf("DEBUG: pluginDir = %s\n", pluginDir);
	//printf("DEBUG: command = %s\n", command);
	//strncat(command, &ch, 1);
	//printf("DEBUG: command = %s\n", command);
	getcwd(working_directory, 100);
	if (update > 0) {
		strcat(command, update_declarations[storeIndex].command);
		snprintf(info, 295, "Running: %s.", update_declarations[storeIndex].command);
	}
	else {
        printf("declarations_command = %s\n", declarations[storeIndex].command);
        strncpy(readcmd, declarations[storeIndex].command +1, strlen(declarations[storeIndex].command) -1);
        printf("readcmd = %s\n", readcmd);
		strcat(command, readcmd);
		snprintf(info, 295, "Running: %s.", declarations[storeIndex].command);
		printf("DEBUG: command = %s\n", command);
	}
	writeLog(trim(info), 0);
	chdir("plugins");
	fp = popen(readcmd, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		chdir(working_directory);
		writeLog("Failed to run command.", 2);
		return;
	}
	while (fgets(retString, sizeof(retString), fp) != NULL) {
		// VERBOSE  printf("%s", retString);
		printf("%s", retString);
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
	strcpy(output.retString, trim(retString));
	if (update == 0) {
		if (outputs[storeIndex].prevRetCode != -1){
			size_t dest_size = 20;
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            if (output.retCode != outputs[storeIndex].retCode){
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
						strftime(oldTime, 20, "%d-%02d-%02d %02d:%02d:%02d", &time);
						time_t ttOldTime = 0, ttCurTime = 0;
						int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
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
            // Here we have a problrm with time which is wrong in the scheduler

            /*time_t o_nextTime;
            size_t dest_size = 20;
            time_t o_t = time(NULL);
            struct tm o_tm = *localtime(&o_t);
            snprintf(o_currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", o_tm.tm_year + 1900, o_tm.tm_mon +1, o_tm.tm_mday, o_tm.tm_hour, o_tm.tm_min, o_tm.tm_sec);
            o_nextTime = o_t + (declarations[storeIndex].interval *60);
            struct tm *o_tNextTime;
            o_tNextTime = localtime(&o_nextTime);
            printf("DEBUG: [init->plugin] o_tNextTime year = %d\n", o_tNextTime->tm_year);*/

            //printf("DEBUG Store index = %d\n", storeIndex);
            //printf("DEBUG declarations[storeIndex].interval = %d\n", declarations[storeIndex].interval);
            time_t nextTime;
            //size_t dest_size = 20;
            time_t o_t = time(NULL);
            nextTime = t + (declarations[storeIndex].interval * 60);
            struct tm *tNextTime;
            tNextTime = localtime(&nextTime);
            /*memset(&tNextTime, '\0', sizeof(struct tm));
            //localtime_r(&nextTime, &tNextTime);
            tNextTime = localtime(&nextTime);*/

            //printf("DEBUG: [runPlugin] tNextTime year = %d\n", tNextTime->tm_year);
            snprintf(declarations[storeIndex].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime->tm_year + 1900, tNextTime->tm_mon +1, tNextTime->tm_mday, tNextTime->tm_hour, tNextTime->tm_min, tNextTime->tm_sec);
			declarations[storeIndex].nextRun = nextTime;
            output.prevRetCode = output.retCode;
		}
		else {
            output.prevRetCode = 0;
		}
		outputs[storeIndex] = output;
	}
	else {
		output.prevRetCode = 0;
		update_outputs[storeIndex] = output;
	}
	t = clock() -t;
	if (update == 0)
		snprintf(info, 295, "%s executed. Execution took %.0f milliseconds.\n", declarations[storeIndex].name, (double)t);
	else
		snprintf(info, 295, "%s executed. Execution took %.0f milliseconds.\n", update_declarations[storeIndex].name, (double)t);
    chdir(working_directory);
    writeLog(trim(info), 0);
	if (logPluginOutput == 1) {
		char o_info[2395];
		if (update == 0)
			snprintf(o_info, 2395, "%s : %s", declarations[storeIndex].name, retString);
		else
			snprintf(o_info, 2395, "%s : %s", update_declarations[storeIndex].name, retString);
		writeLog(trim(o_info), 0);
	}
	if (pluginResultToFile == 1) {
		char fileName[100];
		char checkName[20];
		char timestr[35];
		char ch = '/';
		char csv = ',';
		FILE *fpt;

		if (update == 0)
			strcpy(checkName, declarations[storeIndex].name);
		else
			strcpy(checkName, update_declarations[storeIndex].name);
		memmove(checkName, checkName+1,strlen(checkName));
		checkName[strlen(checkName)-1] = '\0';
        strcpy(fileName, storeDir);
        strncat(fileName, &ch, 1);
        strcat(fileName, checkName);
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
		if (update == 0)
			fprintf(fp, "%s, %s, %s\n", timestr, declarations[storeIndex].name, retString);
		else
			fprintf(fp, "%s, %s, %s\n", timestr, update_declarations[storeIndex].name, retString);
		fclose(fp);
	}
}

void runPluginThreads(int loopVal){
	char currTime[20];
	char logInfo[200];
    int i;
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);
	size_t dest_size = 20;
    //DWORD lpThreadId;

	snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    for (i = 0; i < loopVal; i++) {
        long j = i;
        if (declarations[i].active == 1) {
            snprintf(logInfo, 200, "Current time is: %s", trim(currTime));
            writeLog(logInfo, 0);
            snprintf(logInfo, 200, "Next run time schedulation is: %s", trim(declarations[i].nextRunTimestamp));
            writeLog(logInfo, 0);
            if (t > declarations[i].nextRun) {
                writeLog("Time to run---", 0);
                //rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)j);
                /*hPluginThread = CreateThread(NULL, 0, pluginExeThread, NULL, 0, (void *)j);
                if (hPluginThread == NULL){
                    snprintf(logInfo, 200, "Error creating thread for plugin %s", declarations[i].name);
                    writeLog(trim(logInfo), 2);
                }
           		else {
                    snprintf(logInfo, 200, "Created new thread (%lu) for plugin %s\n", lpThreadId, declarations[i].name);
                    writeLog(trim(logInfo), 0);
                    flushLog();
                    writeLog("Closing thread.", 0);
           		}*/
           		runPlugin(i, 0);
           		flushLog();
            }
        }
	}
}

void collectMetrics(int decLen, int style) {
    char ch = '/';
	char* pluginName;
	char* serviceName;
    char fileName[100];
	char storeName[100];
	FILE *mf;
    clock_t t;
    char info[225];
	char *p;

    t = clock();
    strcpy(storeName, storeDir);
    strncat(storeName, &ch, 1);
    strcat(storeName, metricsFileName);
    mf = fopen(storeName, "w");
    snprintf(info, 225, "Collecting metrics to file: %s", storeName);
    writeLog(trim(info), 0);
	for (int i = 0; i < decLen; i++) {
        pluginName = malloc(strlen(declarations[i].name)+1);
        strcpy(pluginName, declarations[i].name);
        removeChar(pluginName, '[');
        removeChar(pluginName, ']');
		for (p = pluginName; *p != '\0'; ++p) {
			//if (*p == '/') *p = '_';
			*p = tolower(*p);
		}
        	// Get metrics
        char *e;
		if (strchr(outputs[i].retString, '|') == NULL) {
            snprintf(info, 255, "Plugin %s does not provide metrics. Using plain output.", pluginName);
			writeLog(trim(info), 1);
			//printf("Metrics = %s\n", trim(outputs[i].retString));
			if (style == 0)
                fprintf(mf, "%s_%s{hostname=\"%s\",%s_result=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
			else {
				// Get service name
				serviceName = malloc(strlen(declarations[i].description)+1);
				strcpy(serviceName, declarations[i].description);
				fprintf(mf, "%s_%s{hostname=\"%s\", service=\"%s\", value=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, serviceName, trim(outputs[i].retString), outputs[i].retCode);
				free(serviceName);
			}
		}
        else {
            e = strchr(outputs[i].retString, '|');
            int position = (int)(e - outputs[i].retString);
            int len = strlen(outputs[i].retString);
            int sublen = len - position;
            sublen++;
            char metrics[sublen];
            memcpy(metrics,&outputs[i].retString[position+1],sublen);
            //printf("Metrics = %s\n", trim(metrics));
			if (style == 0)
				fprintf(mf, "%s_%s{hostname=\"%s\", %s_result=\"%s\"} %d\n", trim(metricsOutputPrefix), pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
			else {
				serviceName = malloc(strlen(declarations[i].description)+1);
				strcpy(serviceName, declarations[i].description);
				// We need to loop through metrics
				char * token = strtok(metrics, " ");
				while (token != NULL) {
					char*  metricsToken;
					char* metricsName;
					char* metricsValue;
					metricsToken = malloc(strlen(token)+1);
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
					metricsName = malloc(strlen(metricsToken)+1);
                    strcpy(metricsName, metricsToken);
					metricsValue = malloc(strlen(metricsName-index)+1);
					strcpy(metricsValue, metricsName + index +1);
					metricsName[index] = '\0';
					char *pm;
					for (pm = metricsName; *pm != '\0'; ++pm)
                        *pm = tolower(*pm);
					removeChar(metricsName, '/');
					fprintf(mf, "%s_%s_%s{hostname=\"%s\", service=\"%s\", key=\"%s\"} %s\n", trim(metricsOutputPrefix), pluginName, metricsName, hostName, serviceName, metricsName, metricsValue);
					free(metricsValue);
					free(metricsName);
					free(metricsToken);
					token = strtok(NULL, " ");
				}
				//fprintf(mf, "almond_%s{hostname=\"%s\",service =\"%s\", value=\"%s\"} %d\n", pluginName, hostName, serviceName, trim(outputs[i].retString), outputs[i].retCode);
				free(serviceName);
			}
		}
        free(pluginName);
	}
	fclose(mf);
    t = clock() -t;
    //double collection_time = ((double)t)/CLOCKS_PER_SEC;
    //printf("Data collection took %f seconds to execute.\n", collection_time);
    //printf("Data collection took %.0f miliseconds to execute.\n", (double)t);
    snprintf(info, 225, "Metrics collection took %.0f miliseconds to execute.", (double)t);
    writeLog(trim(info), 0);
}

void timeTune(int seconds) {
	int i;
	size_t dest_size = 20;
	char info[200];
	snprintf(info, 200, "Tuning up run times %d seconds", seconds);
	writeLog(trim(info), 0);
	// Loop through and change nextTimeValue
	for (i = 0; i < decCount; i++) {
		if (i != timeTunerMaster) {
			time_t nextTime = declarations[i].nextRun + seconds;
            struct tm *tNextTime;
            memset(&tNextTime, '\0', sizeof(struct tm));
            //localtime_r(&nextTime, &tNextTime);
            tNextTime = localtime(&nextTime);
            snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime->tm_year + 1900, tNextTime->tm_mon +1, tNextTime->tm_mday, tNextTime->tm_hour, tNextTime->tm_min, tNextTime->tm_sec);
            declarations[i].nextRun = nextTime;
        }
	}
}

void getHostName() {
    // Get FQDN instead?
    DWORD bufCharCount = INFO_BUFFER_SIZE;
    char infoBuf[INFO_BUFFER_SIZE];

    if (!GetComputerName(infoBuf, &bufCharCount)) {
        printf("Could not retrieve computer name.\n");
        strcpy(infoBuf, "None");
    }
    printf("%s\n", infoBuf);
    strncpy(hostName, infoBuf, sizeof(hostName));
}

BOOL directoryExists(LPCTSTR szPath) {
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void initNewPlugin(int index) {
	char currTime[20];
	char logInfo[100];
	snprintf(logInfo, 100, "Initiating new plugin: %s\n", update_declarations[index].name);
	writeLog(trim(logInfo), 0);
	printf("Initiating new plugin with id %d", index);
	if (update_declarations[index].active == 1) {
		snprintf(logInfo, 100, "%s is now active. Id %d\n", update_declarations[index].name, update_declarations[index].id-1);
		writeLog(trim(logInfo), 0);
		update_outputs[index].prevRetCode = -1;
		strcpy(update_declarations[index].statusChanged, "0");
		runPlugin(index, 1);
		size_t dest_size = 20;
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcpy(update_declarations[index].lastRunTimestamp, currTime);
        strcpy(update_declarations[index].lastChangeTimestamp, currTime);
        time_t nextTime = t + (update_declarations[index].interval *60);
        struct tm *tNextTime;
        memset(&tNextTime, '\0', sizeof(struct tm));
        tNextTime = localtime(&nextTime);
        //localtime_r(&nextTime, &tNextTime);
        snprintf(update_declarations[index].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime->tm_year + 1900, tNextTime->tm_mon +1, tNextTime->tm_mday, tNextTime->tm_hour, tNextTime->tm_min, tNextTime->tm_sec);
        update_declarations[index].nextRun = nextTime;
		usleep(500);
	}
	else{
        snprintf(logInfo, 100, "%s is not active. Id: %d\n", update_declarations[index].name, update_declarations[index].id);
        writeLog(trim(logInfo), 0);
    }
    flushLog();
}

void checkRetVal(int val) {
	if (val > 1) {
		printf("Caught memory problem redeclaring plugin variables.\nQuiting...");
        writeLog("Memory allocation error redeclaring plugins.", 2);
        writeLog("Check your configs if needed, then restart me.", 0);
        flushLog();
        exit(0);
    }
}

void initScheduler(int numOfP, int msSleep) {
	char currTime[20];
	char logInfo[100];
	time_t nextTime;
	float sleepTime = msSleep/1000;
	printf("Initiating scheduler\n");

	for (int i = 0; i < numOfP; i++)
	{
		if (declarations[i].active == 1)
		{
			snprintf(logInfo, 100, "%s is active. Id %d\n", declarations[i].name, declarations[i].id);
			writeLog(trim(logInfo), 0);
			outputs[i].prevRetCode = -1;
			strcpy(declarations[i].statusChanged, "0");
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
			struct tm *tNextTime;
			tNextTime = localtime(&nextTime);
			//printf("DEBUG: [init] tNextTime year = %d\n", tNextTime->tm_year);
			snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime->tm_year + 1900, tNextTime->tm_mon +1, tNextTime->tm_mday, tNextTime->tm_hour, tNextTime->tm_min, tNextTime->tm_sec);
			declarations[i].nextRun = nextTime;
			//printf("Sleep time is: %.3f\n", sleepTime);
			if (quick_start < 1)
				sleep(sleepTime);
		}
		else
		{
			snprintf(logInfo, 100, "%s is not active. Id: %d\n", declarations[i].name, declarations[i].id);
			writeLog(trim(logInfo), 0);
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
	if (local_api > 0) {
		if (initSocket() == SOCKET_READY) {
			startApiSocket();
		}
		else {
			writeLog("Continue without local api.", 0);
		}
	}
}

void scheduleWinChecks(){
    char logInfo[100];
    float sleepTime = schedulerSleep/1000;
    const int eternal = 1;

    writeLog("Start timer...", 0);
    snprintf(logInfo, 100, "Sleep time is: %.3f seconds\n", sleepTime);
	writeLog(trim(logInfo), 0);
    flushLog();
    while (eternal == 1){
        runPluginThreads(decCount);
        snprintf(logInfo, 100, "Sleeping for  %.3f seconds.\n", sleepTime);
		writeLog(trim(logInfo), 0);
		flushLog();
		sleep(sleepTime);
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
		// Set this to timestamp
		if (checkPluginFileStat(pluginDeclarationFile, tPluginFile, 0)) {
			writeLog("Detected change of plugins file.", 0);
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
		flushLog();
    }
}

void scheduleChecks(){
	char logInfo[100];
	float sleepTime = schedulerSleep/1000;
    //const int i = 1;

	writeLog("Start timer...", 0);
	snprintf(logInfo, 100, "Sleep time is: %.3f\n", sleepTime);
	writeLog(trim(logInfo), 0);
	flushLog();
	// Timer is an eternal loop :P
	// while (g_exit < 1 { TRY THIS!!! }
	while (g_exit < 0) {
        //runPluginThreads(numOfT);
		runPluginThreads(decCount);
		snprintf(logInfo, 100, "Sleeping for  %.3f seconds.\n", sleepTime);
		writeLog(trim(logInfo), 0);
		flushLog();
		sleep(sleepTime);
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
		// Set this to timestamp
		if (checkPluginFileStat(pluginDeclarationFile, tPluginFile, 0)) {
			writeLog("Detected change of plugins file.", 0);
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
		flushLog();
	}
}

void closeSocket(){
    writeLog("Closing socket.", 0);
    closesocket(s);
    writeLog("Application received a quit signal.", 0);
    writeLog("Walmond says goodbye..", 0);
    flushLog();
    printf("Exiting application...");
    WSACleanup();
}

int main() {
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    fptr = fopen("almond.log", "a");
    fprintf(fptr, "\n");
    writeLog("Starting Almond win version 0.5", 0);
    flushLog();
    printf("Run config\n");
    int retVal = getConfigurationValues();
    printf("Check retval\n");
    if (retVal == 0) {
		printf("Configuration read ok.\n");
		writeLog("Configuration read ok.", 0);
	}
	else {
		printf("ERROR: Configuration is not valid.\n");
		writeLog("Configuration is not valid", 1);
		return 1;
	}
    flushLog();
    if (strcmp(hostName, "None") == 0) {
        getHostName();
	}
	writeLog("Hostname is set.", 0);
	flushLog();
	decCount = countDeclarations(pluginDeclarationFile);
	if (decCount < 1) {
        writeLog("Did not find any plugin declarations.", 2);
        flushLog();
        return 2;
	}
    writeLog("So far so good as the slenderman said.", 0);
    flushLog();
    declarations = malloc(sizeof(PluginItem) * decCount);
	if (!declarations) {
		perror ("Error allocating memory");
                writeLog("Error allocating memory - PluginItem.", 2);
		abort();
	}
	writeLog("Declarations memory heap is set.", 0);
	memset(declarations, 0, sizeof(PluginItem)*decCount);
	outputs = malloc(sizeof(PluginOutput)*decCount);
	if (!outputs){
		perror("Error allocating memory");
		writeLog("Error allocating memory - PluginOutput.", 2);
		abort();
	}
	memset(outputs, 0, sizeof(PluginOutput)*decCount);
	writeLog("Outputs memory heap is set.", 0);
	flushLog();
	int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile, 0);
	if (pluginDeclarationResult != 0){
		printf("ERROR: Problem reading plugin declaration file.\n");
		writeLog("Problem reading from plugin declaration file.", 1);
	}
	else {
		printf("Declarations read.\n");
		writeLog("Plugin declarations file loaded.", 0);
	}
    time_t dummy;
	checkPluginFileStat(pluginDeclarationFile, dummy, 1);
	writeLog("The stuff is loaded. Ready to shoot.", 0);
	flushLog();
	initScheduler(decCount, initSleep);
	writeLog("Initiating scheduler to run checks att given intervals.", 0);
    printf("Scheduler started.\n");
    flushLog();
    scheduleWinChecks();
    writeLog("Walmond hope to be started again. Soon.", 0);
    flushLog();
    return 0;
}
