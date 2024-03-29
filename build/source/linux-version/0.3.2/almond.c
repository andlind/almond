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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_STRING_SIZE 50
#define JSON_OUTPUT 0
#define METRICS_OUTPUT 1
#define JSON_AND_METRICS_OUTPUT 2
#define PROMETHEUS_OUTPUT 3
#define JSON_AND_PROMETHEUS_OUTPUT 4

struct PluginItem {
        char name[50];
        char description[100];
        char command[255];
	char lastRunTimestamp[20];
	char nextRunTimestamp[20];
	char lastChangeTimestamp[20];
	char statusChanged[1];
        int active;
        int interval;
        int id;
	time_t nextRun;
};

struct PluginOutput {
	int retCode;
	int prevRetCode;
	char retString[1500];
};

char confDir[50];
char dataDir[50];
char storeDir[50];
char templateDir[50];
char logDir[50];
char pluginDir[50];
char pluginDeclarationFile[75];
char fileName[100];
char template[500];
char hostName[255] = "None";
char templateName[50] = "metrics.template";
char jsonFileName[50] = "monitor_data_c.json";
char metricsFileName[50] = "monitor.metrics";
char gardenerScript[75] = "/opt/almond/gardener.py";
struct PluginItem *declarations;
struct PluginOutput *outputs;
int initSleep;
int updateInterval;
int schedulerSleep = 5000;
int confDirSet = 0;
int dataDirSet = 0;
int storeDirSet = 0;
int templateDirSet = 0;
int logDirSet = 0;
int pluginDirSet = 0;
int logPluginOutput = 0;
int pluginResultToFile = 0;
int decCount = 0;
int saveOnExit = 0;
int dockerLog = 0;
int enableGardener = 0;
unsigned int gardenerInterval = 43200;
unsigned char output_type = 0;
time_t tLastUpdate, tnextUpdate;
time_t tnextGardener;

FILE *fptr;

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
	char *dest = malloc(strlen(sentence)-strlen(find)+strlen(replace)+1);
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
        	memcpy(insert_point, tmp, p - tmp);
        	insert_point += p - tmp;
		memcpy(insert_point, replace, repl_len);
        	insert_point += repl_len;
        	tmp = p + needle_len;
    }
    strcpy(dest, buffer);
    return dest;
}
			
void writeLog(const char *message, int level) {
        char timeStamp[20];
        char wmes[1545] = "";
        size_t dest_size = 20;
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        snprintf(timeStamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        strcpy(wmes, timeStamp);
        strcat(wmes, " | ");
        switch (level) {
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
	if (dockerLog > 0) {
		printf("%s\n", wmes);
	}
}

int directoryExists(const char *checkDir, size_t length) {
        char mes[50];
        snprintf(mes, 50, "Checking directory %s", checkDir);
        writeLog(trim(mes), 0);

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
        char logFile[100];
	char ch = '/';

        strcpy(logFile, logDir);
        strncat(logFile, &ch, 1);
        strcat(logFile, "almond.log");
	fclose(fptr);
	sleep(0.10);
	fptr = fopen(logFile, "a");

}

void closejsonfile() {
	const char *bFolderName = "backup";
	char dataFileName[100];
	char backupDirectory[100];
	char newFileName[150];
	char ch = '/';
	char dot = '.';
        
	strcpy(dataFileName, dataDir);
        strncat(dataFileName, &ch, 1);
        strcat(dataFileName, jsonFileName);


	if (saveOnExit == 0) {
		remove(dataFileName);
	}
	else {
		char date[12];
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
                strftime(date, sizeof(date)-1, "%Y%m%d%H_", t);
		strcpy(backupDirectory, dataDir);
                strncat(backupDirectory, &ch, 1);
		strncat(backupDirectory, bFolderName, strlen(bFolderName));
		if (directoryExists(backupDirectory, 100) != 0) {
			int status = mkdir(trim(backupDirectory), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (status != 0 && errno != EEXIST) {
				printf("Failed to create backup directory. Errno: %d\n", errno);
				return;
			}
		}
		strcpy(newFileName, backupDirectory);
		strncat(newFileName, &ch, 1);
		strncat(newFileName, jsonFileName, strlen(jsonFileName));
		strncat(newFileName, &dot, 1);
		strncat(newFileName, date, strlen(date));
		rename(dataFileName, newFileName);
	}	
}

void sig_handler(int signal){
	//TODO: Threads to join before exit? Or just a grace sleep...
	//      closemetricsfile...
    	switch (signal) {
        	case SIGINT:
			writeLog("Caught SIGINT, exiting program.", 0);
			closejsonfile();
			fclose(fptr);
			free(declarations);
			free(outputs);
			printf("Exiting application...");
            		exit(0);
		case SIGKILL:
			writeLog("Caught SIGKILL, exiting progam.", 0);
			closejsonfile();
			fclose(fptr);
			free(declarations);
			free(outputs);
			printf("Exiting application...");
			exit(0);
		case SIGTERM:
			printf("Caught signal to quit program.\n");
			closejsonfile();
                        writeLog("Caught signal to terminate program.", 0);
			free(declarations);
			free(outputs);
			writeLog("HowRU says goodbye.", 0);
                        fclose(fptr);
			printf("Application is stopped.");
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
		//printf("hostname: %s\n", p->ai_canonname);
		size_t dest_size = 255;
                snprintf(ret, dest_size, "%s", p->ai_canonname);
	}
	freeaddrinfo(info);
	return ret;
}

int getConfigurationValues() {
	char* file_name;
	char* line;
	size_t len = 0;
	ssize_t read;
	FILE *fp;
	int index = 0;
	file_name = "/etc/almond/almond.conf";
        fp = fopen(file_name, "r");
	char confName[MAX_STRING_SIZE] = "";
        char confValue[MAX_STRING_SIZE] = "";

	if (fp == NULL)
   	{
      		perror("Error while opening the file.\n");
		writeLog("Error opening configuration file", 2);
      		exit(EXIT_FAILURE);
   	}

	while ((read = getline(&line, &len, fp)) != -1) {
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
	   if (strcmp(confName, "scheduler.confDir") == 0) {
		   if (directoryExists(confValue, 255) == 0) {
			   size_t dest_size = sizeof(confValue);
			   snprintf(confDir, dest_size, "%s", confValue);
			   confDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
                      writeLog("Exporting to both json and prometheus style metrics.\n", 0);
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
		   if (directoryExists(confValue, 255) == 0) {
			   size_t dest_size = sizeof(confValue);
			   snprintf(dataDir, dest_size, "%s", confValue);
			   dataDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
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
                   if (directoryExists(confValue, 255) == 0) {
                           size_t dest_size = sizeof(confValue);
                           snprintf(storeDir, dest_size, "%s", confValue);
                           storeDirSet = 1;
                   }
                   else {
                           int status = mkdir(trim(confValue), 0755);
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
	   if (strcmp(confName, "scheduler.templateDir") == 0) {
                   if (directoryExists(confValue, 255) == 0) {
                           size_t dest_size = sizeof(confValue);
                           snprintf(templateDir, dest_size, "%s", confValue);
                           templateDirSet = 1;
                   }
                   else {
                           int status = mkdir(trim(confValue), 0755);
                           if (status != 0 && errno != EEXIST) {
                                   printf("Failed to create directory. Errno: %d\n", errno);
                                   writeLog("Error creating HowRU template directory.", 2);
                           }
                           else {
                                   strncpy(templateDir, trim(confValue), strlen(confValue));
                                   templateDirSet = 1;
                           }
                   }
           }
	   if (strcmp(confName, "scheduler.templateName") == 0) {
		   char info[150];
                   strncpy(templateName, trim(confValue), strlen(confValue));
                   //snprintf(info, 100, "Will use %s template for metrics.", templateName);
		   snprintf(info, 150, "Can not use %s as template for metrics.\nOnly built in template can be used in this 0.3.0.", templateName);
                   writeLog(trim(info), 1);
	   }

	   if (strcmp(confName, "scheduler.logToStdout") == 0) {
 		   printf("Found logToStdout\n");
 		   dockerLog = atoi(confValue);
 	   }
	   if (strcmp(confName, "scheduler.logDir") == 0) {
		   if (directoryExists(confValue, 255) == 0) {
			   size_t dest_size = sizeof(confValue);
			   snprintf(logDir, dest_size, "%s", confValue);
			   logDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
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
		   if (directoryExists(confValue, 255) == 0) {
			   size_t dest_size = sizeof(confValue);
			   snprintf(pluginDir, dest_size, "%s", confValue);
			   pluginDirSet = 1;
		   }
		   else {
			   int status = mkdir(trim(confValue), 0755);
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
	   if (strcmp(confName, "plugins.declaration") == 0) {
		   if (access(trim(confValue), F_OK) == 0){
			   strncpy(pluginDeclarationFile, trim(confValue), strlen(confValue));
		   }
		   else {
			   printf("ERROR: Plugin declaration file does not exist.");
			   writeLog("Plugin declaration file does not exist.", 2);
			   return 1;
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
   	if (line)
        	free(line);
        
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

	strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);
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

void collectMetrics(int decLen, int style) {
        char ch = '/';
	char* pluginName;
	char* serviceName;
        char fileName[100];
	char storeName[100];
	FILE *fp;
	FILE *mf;
        clock_t t;
        char info[225];
	char *p;

	strcpy(fileName, templateDir);
        strncat(fileName, &ch, 1);
        strcat(fileName, templateName);
        snprintf(info, 225, "Using template: %s", fileName);
        writeLog(trim(info), 0);
        t = clock();
	if (access(fileName, F_OK) == 0) {
        	fp = fopen(fileName, "r");
	}
        else {
		snprintf(info, 225, "Can not find template file: %s", fileName);
		writeLog(trim(info), 2);
		writeLog("Exiting application.", 2);
		flushLog();
		fclose(fp);
		exit(2);
	}
	while (fgets(template, sizeof template, fp) != NULL) {
		//fprintf(stdout, "%s", template);
	}
	fclose(fp);
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
                       		fprintf(mf, "almond_%s{hostname=\"%s\",%s_result=\"%s\"} %d\n", pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
			else { 
				// Get service name	
				serviceName = malloc(strlen(declarations[i].description)+1);
				strcpy(serviceName, declarations[i].description);
				fprintf(mf, "almond_%s{hostname=\"%s\", service=\"%s\", value=\"%s\"} %d\n", pluginName, hostName, serviceName, trim(outputs[i].retString), outputs[i].retCode);
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
				fprintf(mf, "almond_%s{hostname=\"%s\", %s_result=\"%s\"} %d\n", pluginName, hostName, pluginName, trim(outputs[i].retString), outputs[i].retCode);
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
					fprintf(mf, "almond_%s_%s{hostname=\"%s\", service=\"%s\", key=\"%s\"} %s\n", pluginName, metricsName, hostName, serviceName, metricsName, metricsValue);
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

void runPlugin(int storeIndex)
{
	FILE *fp;
	char command[100];
	char retString[2280];
	char ch = '/';
	struct PluginOutput output;
	clock_t t;
	char currTime[20];
	char info[295];
	int rc = 0;

	t = clock();
	strcpy(command, pluginDir);
	strncat(command, &ch, 1);
	strcat(command, declarations[storeIndex].command);
	snprintf(info, 295, "Running: %s.", declarations[storeIndex].command);
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
	strcpy(output.retString, retString);
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
                strcpy(declarations[storeIndex].lastRunTimestamp, currTime);
                time_t nextTime = t + (declarations[storeIndex].interval * 60);
                struct tm tNextTime;
                memset(&tNextTime, '\0', sizeof(struct tm));
                localtime_r(&nextTime, &tNextTime);
                snprintf(declarations[storeIndex].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
		declarations[storeIndex].nextRun = nextTime;
                output.prevRetCode = output.retCode;
	}
	else {
	        output.prevRetCode = 0; 
	}
	outputs[storeIndex] = output;
	t = clock() -t;
	snprintf(info, 295, "%s executed. Execution took %.0f milliseconds.\n", declarations[storeIndex].name, (double)t);
        writeLog(trim(info), 0);
	if (logPluginOutput == 1) {
		char o_info[2395];
		snprintf(o_info, 2395, "%s : %s", declarations[storeIndex].name, retString);
		writeLog(trim(o_info), 0);
	}
	if (pluginResultToFile == 1) {
		char fileName[100];
		char checkName[20];
		char timestr[35];
		char ch = '/';
		char csv = ',';
		FILE *fpt;

		strcpy(checkName, declarations[storeIndex].name);
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
		fprintf(fp, "%s, %s, %s", timestr, declarations[storeIndex].name, retString);
		fclose(fp);
	}
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

void* pluginExeThread(void* data) {
	long storeIndex = (long)data;
	pthread_detach(pthread_self());
	// VERBOSE printf("Executing %s in pthread %lu\n", declarations[storeIndex].description, pthread_self());
	runPlugin(storeIndex);
	pthread_exit(NULL);
}

void* gardenerExeThread(void* data) {
	pthread_detach(pthread_self());
	runGardener();
	pthread_exit(NULL);
}

int countDeclarations(char *file_name) {
	FILE *fp;
	int i = 0;
	char c;

        fp = fopen(file_name, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file.\n");
		writeLog("Error opening and counting declarations file.", 2);
                exit(EXIT_FAILURE);
        }
	for (c = getc(fp); c != EOF; c = getc(fp)){
		if (c == '\n')
			i++;
	}
	fclose(fp);
	return i-1;
}

int loadPluginDeclarations(char *pluginDeclarationsFile) {
	int hasFaults = 0;
	int counter = 0;
	int i;
	int index = 0;
        char* line;
	char *token;
	char *name;
	char *description;
	char loginfo[60];
        size_t len = 0;
        ssize_t read;
        FILE *fp;
	char *saveptr;
	struct PluginItem item;

        fp = fopen(pluginDeclarationsFile, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file.\n");
		writeLog("Error opening the plugin declarations file.", 2);
                exit(EXIT_FAILURE);
        }
	while ((read = getline(&line, &len, fp)) != -1) {
		index++;
		if (strchr(line, '#') == NULL){
		        for (i = 1, line;; i++, line=NULL) {
		               token = strtok_r(line, ";", &saveptr);
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
		       declarations[counter] = item;
                       counter++;
		}
	}
        fclose(fp);
        if (line)
                free(line);
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
	char loginfo[400];
	size_t len = 0;
	ssize_t read;
	struct PluginItem item;

	int newCount = countDeclarations(pluginDeclarationFile);
	if (newCount > decCount) {
		// Reload needed
		writeLog("You need to reload almond to update declarations.", 1);
		flushLog();
		return 1;
	}
	else {
		// Read plugin declarations file and update declarations	
		fp = fopen(pluginDeclarationFile, "r");
       	 	if (fp == NULL)
        	{
                	perror("Error while opening the file.\n");
                	writeLog("Error opening the plugin declarations file.", 2);
               		exit(EXIT_FAILURE);
        	}
        	while ((read = getline(&line, &len, fp)) != -1) {
                	index++;
                	if (strchr(line, '#') == NULL){
                        	for (i = 1, line;; i++, line=NULL) {
                               		token = strtok_r(line, ";", &saveptr);
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
       		if (line)
       		         free(line);
		return 0;
	}
}

void initScheduler(int numOfP, int msSleep) {
	char currTime[20];
	char logInfo[100];
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
			runPlugin(i);
			size_t dest_size = 20;
			time_t t = time(NULL);
  			struct tm tm = *localtime(&t);
			snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			strcpy(declarations[i].lastRunTimestamp, currTime);
			strcpy(declarations[i].lastChangeTimestamp, currTime);
			time_t nextTime = t + (declarations[i].interval *60);
			struct tm tNextTime;
			memset(&tNextTime, '\0', sizeof(struct tm));
			localtime_r(&nextTime, &tNextTime);
			snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			declarations[i].nextRun = nextTime;
			//printf("Sleep time is: %.3f\n", sleepTime);
			sleep(sleepTime);
		}
		else
		{
			snprintf(logInfo, 100, "%s is not active. Id: %d\n", declarations[i].name, declarations[i].id);
			writeLog(trim(logInfo), 0);
		}
		flushLog();
	}
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
        tnextGardener = time(0) + gardenerInterval;	
}

void runPluginThreads(int loopVal){
	char currTime[20];
	char logInfo[200];
	pthread_t thread_id;
        int rc;
        int i;
	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
	size_t dest_size = 20;

	snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        for (i = 0; i < loopVal; i++) {
           long j = i;
	   if (declarations[i].active == 1) {
		//snprintf(logInfo, 200, "Current time is: %s\n", currTime);
		//writeLog(logInfo, 0);
		//snprintf(logInfo, 200, "Next run time schedulation is: %s\ns", declarations[i].nextRunTimestamp);
		//writeLog(logInfo, 0);
		if (t > declarations[i].nextRun)
		{
			rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)j);
           		if(rc) {
                		snprintf(logInfo, 200, "Error: return code from phtread_create is %d\n", rc);
				writeLog(trim(logInfo), 2);
           		}
           		else {
                   		snprintf(logInfo, 200, "Created new thread (%lu) for plugin %s\n", thread_id, declarations[i].name);
				writeLog(trim(logInfo), 0);
           		}
		}
            }
	}
        //pthread_exit(NULL);
}

void executeGardener() {
	pthread_t thread_id;
	int rc;
	char logInfo[200];

	rc = pthread_create(&thread_id, NULL, gardenerExeThread, "gardener 1");
	if(rc) {
		snprintf(logInfo, 200, "Error: return code from phtread_create is %d\n", rc);
               	writeLog(trim(logInfo), 2);
        }
        else {
        	snprintf(logInfo, 200, "Created new thread (%lu) truncating metrics logs (gardener) \n", thread_id);
        	writeLog(trim(logInfo), 0);
       }

}

void scheduleChecks(int numOfT){
	char logInfo[100];
	float sleepTime = schedulerSleep/1000;
        const int i = 1;

	writeLog("Start timer...", 0);
	snprintf(logInfo, 100, "Sleep time is: %.3f\n", sleepTime);
	writeLog(trim(logInfo), 0);
	// Timer is an eternal loop :P
	while (i > 0) {
		runPluginThreads(numOfT);
		snprintf(logInfo, 100, "Sleeping for  %.3f seconds.\n", sleepTime);
		writeLog(trim(logInfo), 0);
		sleep(sleepTime);
		switch (output_type) {
                	case JSON_OUTPUT:
                        	collectJsonData(numOfT);
                        	break;
                	case METRICS_OUTPUT:
                        	collectMetrics(numOfT, 0);
                        	break;
                	case JSON_AND_METRICS_OUTPUT:
                       		collectJsonData(numOfT);
				collectMetrics(numOfT, 0);
                       		break;
			case PROMETHEUS_OUTPUT:
				collectMetrics(numOfT, 1);
				break;
			case JSON_AND_PROMETHEUS_OUTPUT:
				collectJsonData(numOfT);
                                collectMetrics(numOfT, 1);
				break;
                	default:
                        	collectJsonData(numOfT);
        	}
		// Set this to timestamp
		updatePluginDeclarations();
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

int main()
{
	fptr = fopen("/var/log/almond/almond.log", "a");
	fprintf(fptr, "\n");
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		fputs("An error occurred while setting a signal handler\n", stderr);
		writeLog("An error occurred while setting the SIGINT signal handler.", 2);
		fclose(fptr);
		return EXIT_FAILURE;
	}
        if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		fputs("An error occured while setting sigterm signal handler\n", stderr);
		writeLog("An error occured while setting sigterm signal handler.", 2);
		fclose(fptr);
		return EXIT_FAILURE;
	}
        writeLog("Starting almond...", 0);
        printf("Starting almond...\n");	
	int retVal = getConfigurationValues();	
	if (retVal == 0) {
		printf("Configuration read ok.\n");
		writeLog("Configuration read ok.", 0);
	}
	else {
		printf("ERROR: Configuration is not valid.\n");
		writeLog("Configuration is not valid", 1);
		return 1;
	}
	if (strcmp(hostName, "None") == 0) { 
		strncpy(hostName, getHostName(), 255);
	}
	decCount = countDeclarations(pluginDeclarationFile);
	declarations = malloc(sizeof(struct PluginItem) * decCount);
	if (!declarations) {
		perror ("Error allocating memory");
                writeLog("Error allocating memory - struct PluginItem.", 2);
		abort();
	}
	memset(declarations, 0, sizeof(struct PluginItem)*decCount);
	outputs = malloc(sizeof(struct PluginOutput)*decCount);
	if (!outputs){
		perror("Error allocating memory");
		writeLog("Error allocating memory - struct PluginOutput.", 2);
		abort();
	}
	memset(outputs, 0, sizeof(struct PluginOutput)*decCount);
	int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile);
	if (pluginDeclarationResult != 0){
		printf("ERROR: Problem reading plugin declaration file.\n");
		writeLog("Problem reading from plugin declaration file.", 1);
	}
	else {
		printf("Declarations read.\n");
		writeLog("Plugin declarations file loaded.", 0);
	}
	flushLog();
        initScheduler(decCount, initSleep);	
        writeLog("Initiating scheduler to run checks att given intervals.", 0);
	printf("Scheduler started.\n");
	flushLog();
        scheduleChecks(decCount);

   	return 0;
}
