#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <json.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <errno.h>
#include <netdb.h>
#include "util.h"
#include "group.h"
#include "server.h"

#define MAX_NAME_SIZE 50
#define MAX_VALUE_SIZE 500
#define BUFFER_SIZE 1000

char* result;
char logDir[50];
char groupFile[50];
char serverFile[50];
char alert_command[100];
time_t groupFileAge;
time_t serverFileAge;

int logDirSet = 0;
int servers_to_check = 0;
int groups_to_check = 0;
int t_intervall = 1000;
struct Group *groups;
struct Server *servers;

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

void replaceAll(char *str, const char *oldWord, const char *newWord)
{
    char *pos, temp[BUFFER_SIZE];
    int index = 0;
    int owlen;

    owlen = strlen(oldWord);

    if (!strcmp(oldWord, newWord)) {
        return;
    }

    while ((pos = strstr(str, oldWord)) != NULL)
    {
        strcpy(temp, str);
        index = pos - str;
        str[index] = '\0';
        strcat(str, newWord);
        strcat(str, temp + index + owlen);
    }
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

void flush_log(int open) {
        char logFile[100];
        char ch = '/';

        strcpy(logFile, logDir);
        strncat(logFile, &ch, 1);
        strcat(logFile, "alerting.log");
        fclose(fptr);
	if (open == 0) {
        	sleep(0.10);
        	fptr = fopen(logFile, "a");
	}
}

int count_rows_in_file(char *file_name) {
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

int file_is_modified(const char *path, time_t OldMTime) {
	struct stat file_stat;
	int err = stat(path, &file_stat);
	if (err != 0) {
		exit(errno);
	}
	return file_stat.st_mtime > OldMTime;
}

char *replace_words(char *sentence, char *find, char *replace) {
    char *dest = malloc (strlen(sentence)-strlen(find)+strlen(replace)+1);
    char *destptr = dest;

    *dest = 0;

    while (*sentence) {
    	if (!strncmp (sentence, find, strlen(find))) {
        	strcat (destptr, replace);
            	sentence += strlen(find);
            	destptr += strlen(replace);
        } 
	else{
        	*destptr = *sentence;
            	destptr++;
            	sentence++;
        }
    }
    *destptr = 0;
    return dest;
}

int loadConfig() {
	char* file_name;
        char* line;
        size_t len = 0;
        ssize_t read;
        FILE *fp;
        int index = 0;
        file_name = "/etc/howru/alerting.conf";
        fp = fopen(file_name, "r");
        char confName[MAX_NAME_SIZE] = "";
        char confValue[MAX_VALUE_SIZE] = "";

        if (fp == NULL) {
                perror("Error while opening the file.\n");
                writeLog("Error opening configuration file", 2);
                exit(EXIT_FAILURE);
        }

        while ((read = getline(&line, &len, fp)) != -1) {
        	char * token = strtok(line, "=");
           	while (token != NULL) {
                	if (index == 0) {
                           strcpy(confName, token);
                           //printf("%s\n", confName);
                   	}
                   	else {
                           strcpy(confValue, token);
                           //printf("%s\n", confValue);
                   	}
                   	token = strtok(NULL, "=");
                   	index++;
                   	if (index == 2) index = 0;
           	}
	   	if (strcmp(confName, "alerter.logDir") == 0) {
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
                           	strcat(fileName, "alerting.log");
                           	writeLog("Closing logfile...", 0);
                           	fclose(fptr);
                           	sleep(0.2);
                           	logFile = fopen("/var/log/howru/alering.log", "r");
                           	fptr = fopen(fileName, "a");
                           	if (fptr == NULL) {
                                	fclose(logFile);
                                   	fptr = fopen("/var/log/howru/alerting.log", "a");
                                   	writeLog("Could not create new logfile.", 1);
                                   	writeLog("Reopened logfile '/var/log/howru/alerting.log'.", 0);
                           	}
                           	else {
                                	while ( (ch = fgetc(logFile)) != EOF)
                                        	fputc(ch, fptr);
                                   	fclose(logFile);
                                   	writeLog("Created new logfile.", 0);
                           	}
                   	}
           	}
		if (strcmp(confName, "alerter.groupFile") == 0) {
			if (access(trim(confValue), F_OK) != 0) {
				printf("No group file found.\nQuitting.\n");
				writeLog("Group file is required and not found.", 2);
				return 2;
			}
			else {
				strcpy(groupFile, trim(confValue));
			}
		}
		if (strcmp(confName, "alerter.serverFile") == 0) {
			if (access(trim(confValue), F_OK) != 0) {
                                printf("No server file found.\nQuitting.\n");
                                writeLog("Server file is required and not found.", 2);
                                return 2;
                        }
			else {
				strcpy(serverFile, trim(confValue));
			}
		}
		if (strcmp(confName, "alerter.timeout") == 0) {
                	char mes[40];
                   	int i = strtol(trim(confValue), NULL, 0);
                   	if (i < 1000)
                        	i = 1000;
                   	snprintf(mes, 40, "Scheduler sleep time is %d ms.", i);
                   	writeLog(trim(mes), 0);
                   	t_intervall= i;
		}	
		if (strcmp(confName, "alerter.command") == 0) {
			char mes[120];
			strcpy(alert_command, trim(confValue));
			snprintf(mes, 120, "Alert command is: %s", alert_command);
			writeLog(trim(mes), 0);
		}

	}
	if (logDirSet == 0)
		return 1;
	else
		return 0;
}

int loadGroups() {
        char* line;
        size_t len = 0;
        ssize_t read;
        FILE *fp;
        int index = 0;
	struct Group group;
	int rows = 0;
	int counter = 0;
	char loginfo[100];
	struct stat file_stat;

	rows = count_rows_in_file(groupFile);
	groups = malloc(sizeof(struct Group) * rows);
        if (!groups) {
                perror ("Error allocating memory");
                writeLog("Error allocating memory - struct Group.", 2);
                abort();
		return 2;
        }
        memset(groups, 0, sizeof(struct Group)*rows);
	
	int err = stat(groupFile, &file_stat);
	if (err != 0) {
		writeLog("Could not get file age from group file.", 1);
	}
	groupFileAge = file_stat.st_mtime;

        fp = fopen(groupFile, "r");
	if (fp == NULL) {
                perror("Error while opening the file.\n");
                writeLog("Error opening configuration file", 2);
                exit(EXIT_FAILURE);
        }
        while ((read = getline(&line, &len, fp)) != -1) {
		if (line[0] != '#') {
			char * token = strtok(line, ";");
                	while (token != NULL) {
				switch (index) {
					case 0: printf("Name : %s\n", token); 
						strcpy(group.name, trim(token));
						break;
					case 1: printf("Active: %s\n", token);
						int active = atoi(trim(token));
					        group.active = active;	
						break;
					case 2: printf("Escalation: %s\n", token);
                                                int es = atoi(trim(token));
                                                group.escalation = es;
                                                break;
					case 3: printf("Email: %s\n", token); 
						strcpy(group.email, trim(token));
						break;
				}
                        	token = strtok(NULL, ";");
                        	index++;
                        	if (index == 4) index = 0;
                	}
			groups[counter] = group;
			snprintf(loginfo, 100, "Group with name '%s' created with id %d.\n",groups[counter].name, counter);
                        writeLog(trim(loginfo), 0);
			counter++;
		}
	}
        fclose(fp);
        if (line)
                free(line);
	groups_to_check = rows;
        return 0;
}

int loadServers() {
	char* line;
        size_t len = 0;
        ssize_t read;
        FILE *fp;
        int index = 0;
        struct Server server;
        int rows = 0;
        int counter = 0;
        char loginfo[150];
	struct stat file_stat;

        rows = count_rows_in_file(serverFile);
        servers = malloc(sizeof(struct Server) * rows);
        if (!servers) {
                perror ("Error allocating memory");
                writeLog("Error allocating memory - struct Server.", 2);
                abort();
                return 2;
        }
        memset(servers, 0, sizeof(struct Server)*rows);

	int err = stat(serverFile, &file_stat);
        if (err != 0) {
                writeLog("Could not get file age from server file.", 1);
        }
        serverFileAge = file_stat.st_mtime;

        fp = fopen(serverFile, "r");
        if (fp == NULL) {
                perror("Error while opening the file.\n");
                writeLog("Error opening configuration file", 2);
                exit(EXIT_FAILURE);
        }
        while ((read = getline(&line, &len, fp)) != -1) {
                if (line[0] != '#') {
                        char * token = strtok(line, ";");
                        while (token != NULL) {
                                switch (index) {
                                        case 0: printf("Name : %s\n", token);
                                                strcpy(server.name, trim(token));
                                                break;
                                        case 1: printf("Port: %s\n", token); 
                                                int port = atoi(trim(token));
                                                server.port = port;
						break;
					case 2: printf("Active: %s\n", token);
                                                int active = atoi(trim(token));
                                                server.active = active;
						break;
                                        case 3: printf("Group: %s\n", token);
						memmove(token, token+1, strlen(token));
                                                strcpy(server.group, trim(token));
                                                break;
                                }
                                token = strtok(NULL, ";");
                                index++;
                                if (index == 4) index = 0;
                        }
                        servers[counter] = server;
                        snprintf(loginfo, 150, "Server with name '%s' created with id %d.\n",servers[counter].name, counter);
                        writeLog(trim(loginfo), 0);
                        counter++;
                }
        }
        fclose(fp);
        if (line)
                free(line);
	servers_to_check = rows;
        return 0;
}

void setTimeout(int millisecs) {
	if (millisecs <= 0) {
		fprintf(stderr, "Timer value must be higher than 0");
		return;
	}
	int milisecs_since = clock() * 1000 / CLOCKS_PER_SEC;
	int end = milisecs_since + millisecs;
	do {
		milisecs_since = clock() * 1000 / CLOCKS_PER_SEC;
	} while (milisecs_since < end);
}

int check_result() {
	int multi_server = 0;
	int retVal = 0;
	int slen = 0;
	char json_blob[6000] = "";
	char criticals[3] = "";
	char warnings[3] = "";
	//char server_blob[1024] = "";
	struct json_object *jobj;
	json_object *server;
	enum json_tokener_error error;
	//printf("%s\n", result);
	strcpy(json_blob, result);
	//printf("json_blob:\n%s\n", json_blob);
	removeChar(json_blob, '[');
	removeChar(json_blob, ']');
	trim(json_blob);
	//printf("json_blob:\n%s\n", json_blob);
	jobj = json_tokener_parse_verbose(json_blob, &error);
	if (error != json_tokener_success){
		// This could be a multiserver
		jobj = json_tokener_parse_verbose(result, &error);
		if (error != json_tokener_success) {
			printf("Parse error. \n");
			return -1;
		}
		else {
			bool isFound = json_object_object_get_ex(jobj, "server", &server);
			if (isFound){
		                //printf("server: %s \n", json_object_get_string(server));
				multi_server = 1;
				strcpy(json_blob, json_object_get_string(server));
			}
		}
	}
	json_object *value;
	if (multi_server == 0) {
		bool isFound = json_object_object_get_ex(jobj, "monitor_results", &value);
		if (isFound)
		{
			//printf("answer: %s \n", json_object_get_string(value));
			json_object *unknown;
			bool unknownFound = json_object_object_get_ex(value, "unknown", &unknown);
			if (unknownFound) {
				int val = json_object_get_int(unknown);
				if (val > 0) {
					retVal = 3;
				}
			}
			json_object *warn;
                	bool warnFound = json_object_object_get_ex(value, "warn", &warn);
                	if (warnFound) {
                        	strcpy(warnings, json_object_get_string(warn));
                        	//printf("Warnings: %s", warnings);
                       	 	int val = json_object_get_int(warn);
                        	if (val > 0) {
                        		retVal = 1;        
                        	}
                	}
			json_object *crit;
			bool critFound = json_object_object_get_ex(value, "crit", &crit);
			if (critFound) {
				//printf("crit: %s \n", json_object_get_string(crit));
				strcpy(criticals, json_object_get_string(crit));
				//printf("Criticals : %s", criticals);
				int val = json_object_get_int(crit);
				if (val > 0) {
					retVal = 2;
				}	
			}
		}
	}
	else {
		//printf("%s\n\n", json_blob);
		trim(json_blob);
		//for (int i = 0; i < 3; i++)
		//	memmove(json_blob, json_blob+1, strlen(json_blob));
		//printf("%s\n\n", json_blob);
		removeChar(json_blob,'[' );
		removeChar(json_blob, ']');
		printf("%s\n\n", json_blob);
		/*jobj = json_tokener_parse_verbose(json_blob, &error);
		if (error != json_tokener_success){
			printf("Parse error.\n");
			return -1;
		}
		bool isFound = json_object_object_get_ex(jobj, "name", &value);
		if (isFound) {
			printf("%s", json_object_get_string(value));
			isFound = json_object_object_get_ex(jobj, "monitor_results", &value);
			if (isFound)
				printf("%s", json_object_get_string(value));
		}*/
		struct json_tokener *tok = json_tokener_new();
		do {
			slen = strlen(json_blob);
			jobj = json_tokener_parse_ex(tok, json_blob, slen);
			bool isFound = json_object_object_get_ex(jobj, "name", &value);
                	if (isFound) {
                        	printf("%s", json_object_get_string(value));
                        	isFound = json_object_object_get_ex(jobj, "monitor_results", &value);
                        	if (isFound)
                                	printf("%s", json_object_get_string(value));
                	}
		} while ((error = json_tokener_get_error(tok)) == json_tokener_continue);
		if (error != json_tokener_success) {
			fprintf(stderr, "Error: %s\n", json_tokener_error_desc(error));
		}
		if (tok->char_offset < slen) {
			//printf("Out rolling in the deep.\n");
		}
	}
	free(result);
	return retVal;
}

int check_escalate(int index) {
	for (int i =0; i < groups_to_check; i++) {
		if (strcmp(servers[index].group, groups[i].name) == 0) {
			return groups[i].escalation;
		}
	}
	return 0;
}

void send_alert(int index) {
	FILE * fPtr;
   	FILE * fTemp;
	char body[300];
	char subject[120];
	char url[200];
	char mail_to[500] = "localhost";
	char buffer[BUFFER_SIZE];

	for (int i =0; i < groups_to_check; i++) {
                if (strcmp(servers[index].group, groups[i].name) == 0) {
			strcpy(mail_to, groups[i].email);
			trim(mail_to);
                }
        }
	memmove(mail_to, mail_to+1, strlen(mail_to));
	snprintf(subject, 120, "%s - HowRU Escalation", servers[index].name);
	trim(subject);
	snprintf(url, 200, "Check this url: %s:%d%s", servers[index].name, servers[index].port, "/api/v1/howru/monitoring/criticals");
	strcpy(body, url);
        fPtr = fopen(trim(alert_command), "r");
	fTemp = fopen("/opt/howru/run.sh", "w");
	if (fPtr == NULL || fTemp == NULL) {
        	printf("\nUnable to open file.\n");
        	printf("Please check whether file exists and you have read/write privilege.\n");
        	exit(EXIT_SUCCESS);
    	}
	while ((fgets(buffer, BUFFER_SIZE, fPtr)) != NULL) {
        	replaceAll(buffer, "{subject}", subject);
		replaceAll(buffer, "{receiver}", mail_to);
		replaceAll(buffer, "{text}", body);
		fputs(buffer, fTemp);
    	}
	fclose(fPtr);
	fclose(fTemp);
	chmod("/opt/howru/run.sh", S_IRWXU);
	system("/opt/howru/run.sh");
	remove("/opt/howru/run.sh");
}

int check_server(char *server, int port, int index) {
	CURL *curl_handle;
	CURLcode res;
	char str_port[6] = ":";
	char command[150] = "";
	int retVal = 0;

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	size_t dest_size = sizeof(server);
	snprintf(command, dest_size, "%s", server);
        int length = snprintf(NULL, 0, "%d", port);
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "%d", port);
	strcat(str_port, str);
	free(str);
	strcat(command, str_port);
	strcat(command, "/api/v1/howru/monitoring/howareyou");

	curl_handle = curl_easy_init();
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, command);
    		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    		res = curl_easy_perform(curl_handle);

		if (res != CURLE_OK) {
			fprintf(stderr, "error %s\n", curl_easy_strerror(res));
			return 1;
		}
		else {
			//printf("Size: %lu\n", (unsigned long)chunk.size);
			//printf("Data: %s\n", chunk.memory);
			int length = snprintf(NULL, 0, "%s", chunk.memory);
			result = malloc(length + 1);
			snprintf(result, length + 1, "%s", chunk.memory);
		}
		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
		retVal = check_result();
		if (retVal != 0) {
			switch (retVal) {
				case 1:
					printf("WARNING! %s has result code %d\n", server, retVal);
					if (check_escalate(index) > 0) {
						printf("ESCALATE\n");
						send_alert(index);
					}
					break;
				case 2:
					printf("ALERT! %s has code %d\n", server, retVal);
					if (check_escalate(index) > 1) {
						printf("ESCALATE\n");
						send_alert(index);
					}
					break;
				case 3:
					printf("UNKNOWN! %s has unknown command results.\n", server);
					if (check_escalate(index) > 2) {
						printf("ESCALATE\n");
						send_alert(index);
					}
					break;
				default:
					printf("PARSE ERROR: Could not parse json object from %s\n", server);
			}
		}
	}
	return 0;
}

void do_run(){
	// TODO t_intervall should be minutes
	int index = 0;
        //char logInfo[100];
        const int i = 1;

        writeLog("Start checking for alerts...", 0);
        // Timer is an eternal loop :P
	while (i > 0) {
		if (servers[index].active == 1) {
			check_server(servers[index].name, servers[index].port, index);
                	flush_log(0);
			setTimeout(t_intervall);
		}
		index++;
		if (index == servers_to_check) {
			if (file_is_modified(serverFile, serverFileAge) != 0) {
				// reload server configs
				printf("Server file is modified...reload required\n");
				serverFileAge = time(0);
				int retVal = loadServers();
        			if (retVal == 0) {
                			printf("Servers reloaded from server configuration file.\n");
                			writeLog("Servers reloaded from server configuration file",0);
        			}
        			else {
                			printf("ERROR: Could not reload servers from configutraion file.\nQuitting.\n");
                			writeLog("Could not reload servers from configuration file.", 2);
                			flush_log(1);
               				exit(1);
        			}

			}
			if (file_is_modified(groupFile, groupFileAge) != 0) {
				// reload group configs
				printf("Group file is modified...reload required\n");
				groupFileAge = time(0);
				int retVal = loadGroups();
                                if (retVal == 0) {
                                        printf("Groups reloaded from groups configuration file.\n");
                                        writeLog("Groups reloaded from groups configuration file",0);
                                }
                                else {
                                        printf("ERROR: Could not reload groups from configutration file.\nQuitting.\n");
                                        writeLog("Could not reload groups from configuration file.", 2);
                                        flush_log(1);
                                        exit(1);
                                }

			}
			index = 0;
		}
        }
}

int main() {
	fptr = fopen("/var/log/howru/alerter.log", "a");
        fprintf(fptr, "\n");
	int retVal = loadConfig();
        if (retVal == 0) {
                printf("Configuration read ok.\n");
                writeLog("Configuration read ok.", 0);
        }
        else {
                printf("ERROR: Configuration is not valid.\n");
                writeLog("Configuration is not valid", 1);
		flush_log(1);
                return 2;
        }
	flush_log(0);
	retVal = loadGroups();
	if (retVal == 0) {
		printf("Groups loaded from group configuration file.\n");
                writeLog("Groups loaded from group configuration file", 0);

	}
	else {
		printf("ERROR: Could not load groups from configuration file.\n");
                writeLog("Could not load groups from configuration file", 1);
                flush_log(0);
	}
	retVal = loadServers();
	if (retVal == 0) {
		printf("Servers loaded from server configuration file.\n");
		writeLog("Servers loaded from server configuration file",0);
	}
	else {
		printf("ERROR: Could not load servers from configutraion file.\nQuitting.\n");
		writeLog("Could not load servers from configuration file.", 2);
		flush_log(1);
		return 2;
	}
	printf("-------\n");
	printf("Alert command is: %s", alert_command);
	printf("_------_\n");
        do_run();	
	return 0;
}
