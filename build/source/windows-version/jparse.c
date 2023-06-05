#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "jparse.h"
#include "structures.h"
#include "main.h"

#define API_READ 10
#define API_RUN 15
#define API_EXECUTE_AND_READ 25
#define API_GET_METRICS 30
#define API_READ_ALL 100
#define API_FLAGS_NONE 0
#define API_FLAGS_VERBOSE 1
#define API_DRY_RUN 6

char *trm(char *s) {
    char *ptr;
    if (!s)
        return NULL;   // NULL string
    if (!*s)
        return s;      // empty string
    for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
    ptr[1] = '\0';
    return s;
}

void remChar(char *str, char garbage) {
        char *src, *dest;
        for (src = dest = str; *src != '\0'; src++){
                *dest = *src;
                if (*dest != garbage) dest++;
        }
        *dest ='\0';
}

int getIdFromName(char *plugin_name) {
	char* pluginName;
	int retVal = -1;

	for (int i = 0; i < decCount; i++) {
		pluginName = malloc(strlen(declarations[i].name)+1);
        strcpy(pluginName, declarations[i].name);
		remChar(pluginName, '[');
		remChar(pluginName, ']');
		if (strcmp(trm(plugin_name), pluginName) == 0) {
			retVal = declarations[i].id;
			break;
		}
		free(pluginName);
	}
	return retVal;
}

int getStringId(char string[]){
    char subbuf[4];
    printf("DEBUG: [GetStringID] instring = %s\n", string);
    memcpy(subbuf, &string[7], 3);
    for (int i = 0; i < 4; i++) {
        if (subbuf[i] == '"') {
            subbuf[i] = '\0';
            break;
        }
    }
    printf("DEBUG: GetStringID = %s\n", subbuf);
    return atoi(subbuf);
}

int parseFlagOptions(char opts[]) {
    printf("DEBUG: [parseFlagOptions] instring = %s\n", opts);
    char subbuf[8];
    if (strlen(opts) >= 15) {
        memcpy(subbuf, &opts[10], 7);
    }
    else
        memcpy(subbuf, &opts[10], strlen(opts)-1);
    for (int i = 0; i < 7; i++) {
        if (subbuf[i] == '"') {
            subbuf[i] = '\0';
            break;
        }
    }
    printf("DEBUG: [parseFlagOptions] subbuf = %s\n", subbuf);
    if (strcmp(subbuf, "verbose") == 0) {
        return API_FLAGS_VERBOSE;
    }
    else if (strcmp(subbuf, "dry") == 0) {
        //api_action = API_DRY_RUN;
        return API_DRY_RUN;
    }
    else if (strcmp(subbuf, "all") == 0) {
        return API_READ_ALL;
    }
    else return API_FLAGS_NONE;
}

int getStringNameId(char string[]){
    char subbuf[25];
    if (strlen(string) >= 32) {
        memcpy(subbuf, &string[7], 24);
    }
    else {
        memcpy(subbuf, &string[7], strlen(string)-1);
    }
    for (int i = 0; i < 24; i++){
        if (subbuf[i] == '"'){
            subbuf[i] = '\0';
            break;
        }
    }
    printf("Name = %s\n", subbuf);
    return getIdFromName(subbuf);
}

int parseMessage(const char * message, int arr[]) {
    int action = 0;
    int id = 0;
    int aflags = 0;

    printf("%s\n", message);
    char *pr = strstr(message, "read");
    char *pg = strstr(message, "get");
    char *pe = strstr(message, "execute");
    char *pn = strstr(message, "run");
    char *prr = strstr(message, "runread");
    char *per = strstr(message, "exread");
    char *pm = strstr(message, "metrics");
    char *pgm = strstr(message, "getm");
    char *fid = strstr(message, "id");
    char *fname = strstr(message, "name");
    char *flags = strstr(message, "flags");
    if ((pr) || (pg))  {
        printf("Found read");
        action += API_READ;
    }
    if ((pe) || (pn)) {
        printf("Found execute");
        action += API_RUN;
    }
    if ((prr) || (per))  {
        printf("Found run read");
        action += API_EXECUTE_AND_READ;
    }
    if ((pm) || (pgm)) {
        printf("Found metrics");
        action += API_GET_METRICS;
    }

    printf("DEBUG: fid = %s\n", fid);
    if (fid){
        printf("DEBUG: = fid is true\n");
        id = getStringId(fid);
        printf("DEBUG: id = %d\n", id);
    }
    else if (fname) {
        getStringNameId(fname);
    }
    else {
        printf("No id or name in json.\n");
    }
    printf("DEBUG: flags = %s\n", flags);
    if (flags){
        printf("DEBUG: flags are true\n");
        aflags = parseFlagOptions(flags);
    }
    else
        printf("DEBUG: flags are not set.\n");

    if (aflags == API_DRY_RUN) action = API_DRY_RUN;

    arr[0] = id;
    arr[1] = aflags;
    return action;
}
