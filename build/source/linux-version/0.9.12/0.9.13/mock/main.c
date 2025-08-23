#include <stdio.h>
#include <unistd.h>   
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <errno.h>  
#include "data.h"
#include "plugins.h"

char* pluginDeclarationFile = NULL;
int decCount;
time_t tPluginFile;

static int cmp_plugin_by_id(const void *pa, const void *pb) {
    const PluginItem * const *p1 = pa;
    const PluginItem * const *p2 = pb;
    return ((*p1)->id - (*p2)->id);
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


int countDeclarations(char *file_name) {
        FILE *fp = NULL;
        int i = 0;
        int ch;

        if (file_name == NULL || strlen(file_name) == 0) {
                //writeLog("Filename is not initialized or is empty.", 2, 0);
                fprintf(stderr, "Filename is uninitialized or empty.\n");
        }
        fp = fopen(file_name, "r");
        if (fp == NULL)
        {
                perror("Error while opening the file[countDeclarations].\n");
                //writeLog("Error opening and counting declarations file.", 2, 0);
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

void show_commands_sorted() {
    size_t total = getPluginCount();
    /* allocate max pointers */
    PluginItem **list = malloc(total * sizeof *list);
    if (!list) return;
    
    /* collect only active plugins */
    size_t n = 0;
    for (size_t i = 0; i < total; i++) {
        PluginItem *pi = getPluginItem(i);
        if (pi && pi->active) {
            list[n++] = pi;
        }
    }

    for (size_t i = 0; i < n; i++) {
         printf("Pre-sort: id=%d name=%s\n", list[i]->id, list[i]->name);
    } 

    /* sort by id */
    qsort(list, n, sizeof *list, cmp_plugin_by_id);

    /* print in id order */
    for (size_t i = 0; i < n; i++) {
        PluginItem *pi = list[i];
        printf("# %d %s: %s\n", pi->id, pi->name, pi->description);
        printf("%s\n\n", pi->command);
    }

    free(list);
}

void show_commands() {
	size_t count = getPluginCount();
	for (size_t i = 0; i < count; i++) { 
		PluginItem *pi = getPluginItem(i);
		if (!pi) continue;
		if (!pi->active) continue;
		printf("# %s: %s\n", pi->name, pi->description);
        	printf("%s\n\n", pi->command);
	}
}

int main(void) {
    pluginDeclarationFile = malloc(20);
    snprintf(pluginDeclarationFile, 20, "%s", "plugins.conf"); 
    decCount = countDeclarations(pluginDeclarationFile);
    size_t plugin_count = (size_t)decCount;
    checkPluginFileStat(pluginDeclarationFile, tPluginFile, 0);
    init_plugins(pluginDeclarationFile, &plugin_count);
    show_commands_sorted();
    
    while(1) {
	if (checkPluginFileStat(pluginDeclarationFile, tPluginFile, 0)) {
        	//writeLog("Detected change of plugins file.", 0, 0);
              	//flushLog();
		printf("Detected change of plugins file.");
            	updatePluginDeclarations();
		printf("Plugins updated. Total live plugins: %zu\n", g_plugin_count);
        }
	show_commands_sorted();
	printf("Sleeping...\n");
	printf("*****************************************************************\n");
	sleep(5);
    }
    free(pluginDeclarationFile);
    return 0;
}
