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
	char retString[255];
};

char confDir[50];
char dataDir[50];
char storeDir[50];
char logDir[50];
char pluginDir[50];
char pluginDeclarationFile[75];
char hostName[255];
char outputFormat[10];
char jsonFileName[50] = "monitor_data_c.json";
struct PluginItem *declarations;
struct PluginOutput *outputs;
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
time_t tLastUpdate, tnextUpdate;

FILE *fptr;
//pthread_t thread_id;
//volatile sig_atomic_t stop;

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

void writeLog(const char *message, int level) {
	char timeStamp[20];
        char wmes[300] = "";
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
}

void flushLog() {
        char logFile[100];
	char ch = '/';

        strcpy(logFile, logDir);
        strncat(logFile, &ch, 1);
        strcat(logFile, "howruc.log");
	fclose(fptr);
	sleep(0.10);
	//fptr = fopen("/var/log/howru/howruc.log", "a");
	fptr = fopen(logFile, "a");

}

void sig_handler(int signal){
    	switch (signal) {
        	case SIGINT:
			writeLog("Caught SIGINT, exiting program.", 0);
			fclose(fptr);
            		exit(0);
		case SIGKILL:
			writeLog("Caught SIGKILL, exiting progam.", 0);
			fclose(fptr);
			exit(0);
		case SIGTERM:
			printf("Caught signal to quit program.\n");
                        writeLog("Caught signal to terminate program.", 0);
			writeLog("HowRU says goodbye.", 0);
                        fclose(fptr);
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
	file_name = "/etc/howru/howruc.conf";
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
              if (strcmp(trim(confValue), "json") != 0){
		      printf("%s is not a valid value. Only json supported at this moment.\n", confValue);
		      writeLog("Unsupported value in configuration scheduler.format", 1);
		      writeLog("Only json supported in this version of the program.", 0);
	      }
	      strcpy(outputFormat, "json");
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
		   if (strcmp(confValue, "/var/log/howru") != 0) {
			   char ch =  '/';
			   char fileName[100];
			   FILE *logFile;
			   strcpy(fileName, logDir);
        		   strncat(fileName, &ch, 1);
                           strcat(fileName, "howruc.log");
			   writeLog("Closing logfile...", 0);
			   fclose(fptr);
			   sleep(0.2);
                           logFile = fopen("/var/log/howru/howruc.log", "r");
			   fptr = fopen(fileName, "a");
			   if (fptr == NULL) {
				   fclose(logFile);
				   fptr = fopen("/var/log/howru/howruc.log", "a");
				   writeLog("Could not create new logfile.", 1);
				   writeLog("Reopened logfile '/var/log/howru/howruc.log'.", 0);
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
	   if (strcmp(confName, "data.jsonFile") == 0) {
		   char info[100];
		   strncpy(jsonFileName, trim(confValue), strlen(confValue));
		   snprintf(info, 100, "Json data will be collected in file: %s.", jsonFileName);
		   writeLog(trim(info), 0);
	   }
 	}

	updateInterval = 60;

   	fclose(fp);
   	if (line)
        	free(line);
        
   	return 0;
}

void collectData(int decLen){
	int retVal = 0;
	char ch = '/';
	//char jsonFile[18] = "monitor_datc.json";
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

void runPlugin(int storeIndex)
{
	FILE *fp;
	char command[100];
	char retString[1035];
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
		char o_info[1150];
		snprintf(o_info, 1150, "%s : %s", declarations[storeIndex].name, retString);
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

void* pluginExeThread(void* data) {
	long storeIndex = (long)data;
	pthread_detach(pthread_self());
	// VERBOSE printf("Executing %s in pthread %lu\n", declarations[storeIndex].description, pthread_self());
	runPlugin(storeIndex);
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
		writeLog("You need to reload howru-scheduler to update declarations.", 1);
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
        collectData(numOfP);
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
		collectData(numOfT);
		// Set this to timestamp
		updatePluginDeclarations();
		flushLog();
	}
}

int main()
{
	fptr = fopen("/var/log/howru/howruc.log", "a");
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
        writeLog("Starting howru-scheduler...", 0);
        printf("Starting howru-scheduler...\n");	
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
	strncpy(hostName, getHostName(), 255);
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
