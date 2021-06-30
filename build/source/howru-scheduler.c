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
char logDir[50];
char pluginDir[50];
char pluginDeclarationFile[75];
char hostName[255];
char outputFormat[10];
char jsonFileName[50] = "monitor_data_c.json";
struct PluginItem *declarations;
struct PluginOutput *outputs;
int initSleep;
int schedulerSleep = 5000;
int confDirSet = 0;
int dataDirSet = 0;
int logDirSet = 0;
int pluginDirSet = 0;
//pthread_t thread_id;
//volatile sig_atomic_t stop;

void sig_handler(int signal){
    	switch (signal) {
        	case SIGINT:
            		printf("Caught SIGINT, exiting now\n");
            		exit(0);
		case SIGKILL:
			printf("Hey, wait - I am getting killed.\n");
			exit(0);
		case SIGTERM:
			printf("Caught signal to quit program.\n");
            	        return;
    	}
//	stop = 1;
}

int directoryExists(const char *checkDir, size_t length) {
	printf("Checking directory...");
	printf("%s\n", checkDir);
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
		printf("hostname: %s\n", p->ai_canonname);
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
      		exit(EXIT_FAILURE);
   	}

	while ((read = getline(&line, &len, fp)) != -1) {
	   char * token = strtok(line, "=");
	   while (token != NULL)
	   {
		   if (index == 0)
		   {
			   strcpy(confName, token);
			   printf("%s\n", confName);
		   }
		   else
		   {
			   strcpy(confValue, token);
			   printf("%s\n", confValue);
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
                               printf("Failed to create directory");
			       printf("%d\n", errno);
                           }
			   else{
			       strncpy(confDir, trim(confValue), strlen(confValue));
			       confDirSet = 1;
			   }
		  }
		  printf("%s\n", confDir);
	   }
	   if (strcmp(confName, "scheduler.format") == 0) {
              if (strcmp(trim(confValue), "json") != 0){
		      printf("%s\n", confValue);
		      printf("Is not a valid value. Only json supported at this moment.");
	      }
	      strcpy(outputFormat, "json");
	   }
	   if (strcmp(confName, "plugins.initSleepMs") == 0) {
              int i = strtol(trim(confValue), NULL, 0);
	      if (i < 5000)
		      i = 7000;
	      initSleep = i;
	   }
	   if (strcmp(confName, "scheduler.sleepMs") == 0) {
		   int i = strtol(trim(confValue), NULL, 0);
		   if (i > 2000)
			   i = 2000;
		   printf("Scheduler Sleep time is %d\n", i);
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
				   printf("Failed to create directory");
				   printf("%d\n", errno);
			   }
			   else {
				   strncpy(dataDir, trim(confValue), strlen(confValue));
				   dataDirSet = 1;
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
			   if (status != 0) {
				   printf("Failed to create directory");
				   printf("%d\n", errno);
			   }
			   else {
				   strncpy(logDir, trim(confValue), strlen(confValue));
				   logDirSet = 1;
			   }
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
				   printf("Failed to create directory");
				   printf("%d\n", errno);
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
			   return 1;
		   }
	   }
	   if (strcmp(confName, "data.jsonFile") == 0) {
		   strncpy(jsonFileName, trim(confValue), strlen(confValue));
		   printf("Json data will be collected in file: %s\n", jsonFileName);
	   }
 	}

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

	strcpy(fileName, dataDir);
	strncat(fileName, &ch, 1);
	strcat(fileName, jsonFileName);
	printf("Collect data.\n");
	printf("Data file: %s\n", fileName);
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
		fprintf(fp, "         \"nextRun\":\"%s\",\n", declarations[i].nextRunTimestamp);
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
	printf("Data collection took %.0f miliseconds to execute.\n", (double)t);
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

	t = clock();
	strcpy(command, pluginDir);
	strncat(command, &ch, 1);
	strcat(command, declarations[storeIndex].command);
	printf("%s\n", command);
        printf("%d\n", storeIndex);
	printf("Running: %s\n", declarations[storeIndex].command);
	fp = popen(command, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
	}
	while (fgets(retString, sizeof(retString), fp) != NULL) {
		printf("%s", retString);
	}
	output.retCode = pclose(fp);
	strcpy(output.retString, retString);
	outputs[storeIndex] = output;
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
	}
	else {
	        output.prevRetCode = 0; 
	}
	outputs[storeIndex] = output;
	t = clock() -t;
	printf("%s executed. Execution took %.0f milliseconds.\n", declarations[storeIndex].name, (double)t);
}

void* pluginExeThread(void* data) {
	long storeIndex = (long)data;
	pthread_detach(pthread_self());
	printf("Executing %s in pthread %lu\n", declarations[storeIndex].description, pthread_self());
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
        size_t len = 0;
        ssize_t read;
        FILE *fp;
	char *saveptr;
	struct PluginItem item;

        fp = fopen(pluginDeclarationsFile, "r");
	if (fp == NULL)
        {
                perror("Error while opening the file.\n");
                exit(EXIT_FAILURE);
        }
	while ((read = getline(&line, &len, fp)) != -1) {
		index++;
		if (strchr(line, '#') == NULL){
		        for (i = 1, line;; i++, line=NULL) {
		               token = strtok_r(line, ";", &saveptr);
			       if (token == NULL)
				       break;
			       printf("%d: %s\n", i, token);
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
		       /*printf("%s", item.name);
		       printf("%d", item.id);
		       printf("%s", item.description);
		       printf("%s", item.command);
		       printf(" %d", item.active);
		       printf(" %d\n", item.interval);
		       printf("----\n");*/
		       printf("Declaration with index %d is created.\n", counter);
		       declarations[counter] = item;
                       counter++;
		}
	}
        fclose(fp);
        if (line)
                free(line);
	return 0;
}

void initScheduler(int numOfP, int msSleep) {
	char currTime[20];
	float sleepTime = msSleep/1000;
	printf("Initiating scheduler\n");
	for (int i = 0; i < numOfP; i++)
	{
		if (declarations[i].active == 1)
		{
			printf("%s is active. Id %d\n", declarations[i].name, declarations[i].id);
			outputs[i].prevRetCode = -1;
			strcpy(declarations[i].statusChanged, "0");
			runPlugin(i);
			size_t dest_size = 20;
			time_t t = time(NULL);
  			struct tm tm = *localtime(&t);
			snprintf(currTime, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon +1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			printf("%s\n",currTime);
			strcpy(declarations[i].lastRunTimestamp, currTime);
			strcpy(declarations[i].lastChangeTimestamp, currTime);
			time_t nextTime = t + (declarations[i].interval *60);
			struct tm tNextTime;
			memset(&tNextTime, '\0', sizeof(struct tm));
			localtime_r(&nextTime, &tNextTime);
			snprintf(declarations[i].nextRunTimestamp, dest_size, "%d-%02d-%02d %02d:%02d:%02d", tNextTime.tm_year + 1900, tNextTime.tm_mon +1, tNextTime.tm_mday, tNextTime.tm_hour, tNextTime.tm_min, tNextTime.tm_sec);
			declarations[i].nextRun = nextTime;
			printf("Sleep time is: %.3f\n", sleepTime);
			sleep(sleepTime);
		}
		else
		{
			printf("%s is not active. Id: %d\n", declarations[i].name, declarations[i].id);
		}
	}
        collectData(numOfP);
}

void runPluginThreads(int loopVal){
	char currTime[20];
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
		printf("Current time is: %s\n", currTime);
		printf("Next run time should be: %s\ns", declarations[i].nextRunTimestamp);
		if (t > declarations[i].nextRun)
		{
			rc = pthread_create(&thread_id, NULL, pluginExeThread, (void *)j);
           		if(rc) {
                		printf("\nError: return code from phtread_create is %d\n", rc);
           		}
           		else {
                   		printf("\nCreated new thread (%lu) for plugin %s\n", thread_id, declarations[i].name);
           		}
		}
            }
	}
        //pthread_exit(NULL);
}

void scheduleChecks(int numOfT){
	float sleepTime = schedulerSleep/1000;
        const int i = 1;
	printf("Start timer....");

	printf("Sleep time is: %.3f\n", sleepTime);
	// Timer is an eternal loop :P
	while (i > 0) {
		runPluginThreads(numOfT);
		printf("Sleeping for  %.3f seconds.\n", sleepTime);
		sleep(sleepTime);
		collectData(numOfT);
	}
}

int main()
{
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		fputs("An error occurred while setting a signal handler\n", stderr);
		return EXIT_FAILURE;
	}
        if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		fputs("An error occured while setting sigterm signal handler\n", stderr);
		return EXIT_FAILURE;
	}
        
	int retVal = getConfigurationValues();	
	if (retVal == 0) {
		printf("Confifuration read ok.");
	}
	else {
		printf("ERROR: Configuration is not valid.");
		return 1;
	}
	strncpy(hostName, getHostName(), 255);
	int decCount = countDeclarations(pluginDeclarationFile);
	printf("%d\n", decCount);
	declarations = malloc(sizeof(struct PluginItem) * decCount);
	if (!declarations) {
		perror ("Error allocating memory");
		abort();
	}
	memset(declarations, 0, sizeof(struct PluginItem)*decCount);
	outputs = malloc(sizeof(struct PluginOutput)*decCount);
	if (!outputs){
		perror("Error allocating memory");
		abort();
	}
	memset(outputs, 0, sizeof(struct PluginOutput)*decCount);
	int pluginDeclarationResult = loadPluginDeclarations(pluginDeclarationFile);
	if (pluginDeclarationResult != 0){
		printf("ERROR: Problem reading plugin declaration file.");
	}
	else {
		printf("Declarations read.\n");
	}
        initScheduler(decCount, initSleep);	
	for (int i = 0; i < decCount; i++)
	{
		printf("%s", declarations[i].name);
		printf("%s\n", declarations[i].command);
		printf("Plugin Ret Code: %d", outputs[i].retCode);
		printf("Plugin Output: %s\n", outputs[i].retString);
	}
        printf("Starting timer to run checks.\n");
        scheduleChecks(decCount);

   	return 0;
}
