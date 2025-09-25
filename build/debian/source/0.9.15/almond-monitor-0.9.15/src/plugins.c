#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <malloc.h>
#include <sys/types.h>
#include "data.h"
#include "plugins.h"

#define STATUS_SIZE 2

//size_t      g_plugin_count        = 0;

// if strdup is missing, declare it
extern char *strdup(const char *);

//static PluginItem *g_plugins   = NULL;
//static size_t      g_plugin_cap = 0;
//static int         g_next_id    = 1;

/*static void fill_timestamp(char *buffer, size_t bufsize) {
    assert(bufsize >= 20);  // sanity check
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info != NULL) {
        strftime(buffer, bufsize, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strncpy(buffer, "invalid time", bufsize - 1);
        buffer[bufsize - 1] = '\0';
    }
}*/

static char *extract_base_command(const char *cmd) {
    if (!cmd) return NULL;

    char *space = strchr(cmd, ' ');
    size_t len = space ? (size_t)(space - cmd) : strlen(cmd);
   
    char *base = malloc(len + 1);
    if (!base) return NULL;

    strncpy(base, cmd, len);
    base[len] = '\0';
    return base;
}

// find by name in our live array
/*static PluginItem *find_plugin(const char *name) {
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, name) == 0)
            return &g_plugins[i];
    }
    return NULL;
}*/

// THIS IS THE LATEST
/*static void addPlugin(const PluginItem *d) {
    // Early return for NULL pointer
    if (d == NULL) return;

    // Handle capacity increase
    if (g_plugin_count + 1 > g_plugin_cap) {
        g_plugin_cap = g_plugin_cap ? g_plugin_cap * 2 : 8;
        PluginItem *temp = realloc(g_plugins, g_plugin_cap * sizeof *g_plugins);
        if (!temp) {
            // Handle memory allocation failure
            //writeLog("Memory allocation failed in addPlugin", LOG_ERROR);
            printf("Memory allocation failed in addPlugin");
            return;
        }
        g_plugins = temp;
    }

    PluginItem *pi = &g_plugins[g_plugin_count++];
    memset(pi, 0, sizeof *pi);

    // Initialize ID
    pi->id = (d->id > 0) ? d->id : g_next_id++;

    // Copy strings with error checking
    pi->name = strdup(d->name);
    if (!pi->name) {
        //writeLog("Failed to allocate memory for plugin name", LOG_ERROR);
        printf("Failed to allocate memory for plugin name");
        return;
    }

    pi->description = strdup(d->description);
    if (!pi->description) {
        free(pi->name);
        //writeLog("Failed to allocate memory for plugin description", LOG_ERROR);
        printf("Failed to allocate memory for plugin description");
        return;
    }

    pi->command = strdup(d->command);
    if (!pi->command) {
        free(pi->name);
        free(pi->description);
        //writeLog("Failed to allocate memory for plugin command", LOG_ERROR);
        printf("Failed to allocate memory for plugin command.");
        return;
    }

    // Copy other fields
    pi->active = d->active;
    pi->interval = d->interval;

    // Initialize timestamps
    fill_timestamp(pi->lastChangeTimestamp, TIMESTAMP_SIZE);
    fill_timestamp(pi->lastRunTimestamp, TIMESTAMP_SIZE);

    // Set next run time
    pi->nextRun = time(NULL) + d->interval;
    strftime(pi->nextRunTimestamp, sizeof(pi->nextRunTimestamp),
             "%Y-%m-%d %H:%M:%S", localtime(&pi->nextRun));

    // Log plugin addition
    //writeLog("Added plugin %s (id=%d)", LOG_INFO, pi->name, pi->id);
    printf("Added plugin %s (id=%d)\n", pi->name, pi->id);
}*/

/*static void removePlugin(PluginItem *plugin) {
    if (plugin == NULL) return;

    // Free all dynamically allocated fields
    if (plugin->name != NULL) {
        free(plugin->name);
        plugin->name = NULL;
    }
    if (plugin->description != NULL) {
        free(plugin->description);
        plugin->description = NULL;
    }
    if (plugin->command != NULL) {
        free(plugin->command);
        plugin->command = NULL;
    }

    // Clear timestamps
    memset(plugin->lastRunTimestamp, 0, sizeof(plugin->lastRunTimestamp));
    memset(plugin->nextRunTimestamp, 0, sizeof(plugin->nextRunTimestamp));
    memset(plugin->lastChangeTimestamp, 0, sizeof(plugin->lastChangeTimestamp));

    // Reset other fields
    plugin->active = 0;
    plugin->interval = 0;
    plugin->id = 0;
    plugin->nextRun = 0;
}

void updatePlugin(PluginItem *old, const PluginItem *new) {
    printf("sizeof(PluginItem) = %zu\n", sizeof(PluginItem));
    bool changed = false;

    if (strcmp(old->description, new->description) != 0) {
        free(old->description);
        old->description = strdup(new->description);
        changed = true;
    }

    if (strcmp(old->command, new->command) != 0) {
        free(old->command);
        old->command = strdup(new->command);
        changed = true;
    }

    if (old->active != new->active) {
        old->active = new->active;
        changed = true;
    }

    if (old->interval != new->interval) {
        old->interval = new->interval;
        old->nextRun = time(NULL) + new->interval;
        strftime(old->nextRunTimestamp, 20, "%Y-%m-%d %H:%M:%S", localtime(&old->nextRun));
        changed = true;
    }

    if (changed) {
        fill_timestamp(old->lastChangeTimestamp, TIMESTAMP_SIZE);
        //writeLog("Updated plugin %s (id=%d)", 0, 0, old->name, old->id);
	printf("Updated plugin %s (id=%d)\n", old->name, old->id);
    }
}*/

static PluginItem **load_declarations(const char *path, int *out_count) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("Failed to open plugin declaration file");
        return NULL;
    }

    PluginItem **list = NULL;
    size_t cap = 0, n = 0;
    char   line[1024];

    while (fgets(line, sizeof line, f)) {
        // Skip comments and short lines
        if (line[0] == '#' || strlen(line) < 5) 
            continue;

        // Strip newline
        line[strcspn(line, "\n")] = '\0';

        // Find [name]…;…;…;…
        char *start = strchr(line, '[');
        char *end   = strchr(line, ']');
        if (!start || !end || end <= start) 
            continue;
        *end = '\0';
        char *name = start + 1;

        // Split remainder by ';'
        char *rest        = end + 1;
        while (*rest == ' ') rest++;
             char *description = strtok(rest, ";");
        char *command     = strtok(NULL, ";");
        char *active_str  = strtok(NULL, ";");
        char *interval_str= strtok(NULL, ";");
        if (!description || !command || !active_str || !interval_str) 
            continue;

        // Grow pointer array if needed
        if (n + 1 > cap) {
            cap = cap ? cap * 2 : 16;
            PluginItem **tmp = realloc(list, cap * sizeof *list);
            if (!tmp) {
                perror("realloc in load_declarations");
                // cleanup partial list
                for (size_t i = 0; i < n; ++i) {
                    free(list[i]->name);
                    free(list[i]->description);
                    free(list[i]->command);
                    free(list[i]);
                }
                free(list);
                fclose(f);
                return NULL;
            }
            list = tmp;
        }

        // Allocate and initialize one PluginItem
        PluginItem *item = calloc(1, sizeof *item);
        if (!item) {
            perror("calloc PluginItem");
            break;
        }
         // Fill config fields
        item->name        = strdup(name);
        item->description = strdup(description);
        item->command     = strdup(command);
        item->active      = atoi(active_str);
        item->interval    = atoi(interval_str);
        // id & runtime fields will be set later in update_plugins()
        item->touched     = false;

        list[n++] = item;
    }

    fclose(f);
    *out_count = (int)n;
    return list;
}


/*static PluginItem *load_declarations(const char *path, size_t *out_count) {
    FILE       *f = fopen(path, "r");
    char        buf[1024];
    PluginItem *arr = NULL;
    size_t      cap = 0, n = 0;

    if (!f) return NULL;
    while (fgets(buf, sizeof(buf), f)) {
        buf[sizeof(buf)-1] = '\0';
        char *p = buf;
        while (*p==' '||*p=='\t') p++;
        if (*p=='#'||*p=='\n'||*p=='\0') continue;

        char *tokens[4] = {0};
        int   i = 0;
        tokens[i++] = strtok(p, ";\n");
        while (i<4 && (tokens[i]=strtok(NULL,";\n"))) i++;
        if (i!=4) continue;

        if (n+1 > cap) {
            cap = cap ? cap*2 : 16;
	    PluginItem *tmpload = realloc(arr, cap * sizeof *arr);
	    if (!tmpload) {
		free(arr);
		printf("Error reallocating memory in load_declarations.");
		fclose(f);
		return NULL;
	    }
            arr = tmpload;
        }

        memset(&arr[n], 0, sizeof *arr);
        char *rb = strchr(tokens[0],']');
	if (!rb) continue;
        *rb = '\0';
        arr[n].id = n + 1;
        if (!tokens[0] || !tokens[1] || !tokens[2] || !tokens[3]) continue;
        arr[n].name        = strdup(tokens[0]+1);
        char *desc_start = rb + 1;
        if (*desc_start == ' ') desc_start++;
        if (*desc_start == '\0') continue; // avoid strdup on empty string
        //arr[n].description = strdup(rb+1 + (* (rb+1)==' '));
        arr[n].description = strdup(desc_start);
        arr[n].command     = strdup(tokens[1]);
        arr[n].active      = atoi(tokens[2]);
        arr[n].interval    = atoi(tokens[3]);
        n++;
    }
    fclose(f);
    *out_count = n;
    g_plugin_count = n;
    return arr;
}

static void free_declarations(PluginItem *arr, size_t n) {
    for (size_t i=0; i<n; i++) {
        free(arr[i].name);
        free(arr[i].description);
        free(arr[i].command);
    }
    free(arr);
}*/

/*static int decl_cmp(const void *a, const void *b) {
    return strcmp(((PluginItem*)a)->name, ((PluginItem*)b)->name);
}*/

/*void mock_runCommand(int index, char* cmd) {
	printf("Run: %s\n", cmd);
        fill_timestamp(g_plugins[index].lastRunTimestamp, TIMESTAMP_SIZE);
	fill_timestamp(g_plugins[index].nextRunTimestamp, TIMESTAMP_SIZE);
}*/

// THIS IS LATEST
/*void mock_runCommand(PluginItem *item) {
    printf("Run: %s\n", item->command);
    fill_timestamp(item->lastRunTimestamp, TIMESTAMP_SIZE);
    fill_timestamp(item->nextRunTimestamp, TIMESTAMP_SIZE);
}*/

/*void init_plugins(const char *path, size_t *cnt) {
	printf("Initiate plugins\n");
	g_plugins = load_declarations(path, cnt);
	if (!g_plugins) {
		fprintf(stderr, "Failed to load %s\n", path);
        	return;
    	}
	// DEBUG: inspect exactly what you got back
    	printf("DEBUG: loaded %zu plugins\n", *cnt);
    	for (size_t i = 0; i < *cnt; i++) {
        	printf("  [%zu] name=%s cmd=\"%s\"\n",
               		i,
               		g_plugins[i].name,
               		g_plugins[i].command);
		mock_runCommand(g_plugins[i]);
    }
    
}*/

/*void load_plugins() {
    FILE *fp = fopen(pluginDeclarationFile, "r");
    if (!fp) {
        perror("Failed to open config file");
        return;
    }

    g_plugins = calloc(decCount, sizeof(PluginItem));
    if (!g_plugins) {
        perror("Memory allocation failed");
        fclose(fp);
        return;
    }

    char line[1024];
    int index = 0;

    while (fgets(line, sizeof(line), fp) && index < decCount) {
        // Skip comments and empty lines
        if (line[0] == '#' || strlen(line) < 5) continue;

        // Remove newline
        line[strcspn(line, "\n")] = 0;

        // Parse line
        char *start = strchr(line, '[');
        char *end = strchr(line, ']');
        if (!start || !end || end <= start) continue;

        *end = '\0';
        char *name = start + 1;

        char *rest = end + 1;
        while (*rest == ' ') rest++; // skip spaces

        char *description = strtok(rest, ";");
        char *command     = strtok(NULL, ";");
        char *active_str  = strtok(NULL, ";");
        char *interval_str= strtok(NULL, ";");

        if (!description || !command || !active_str || !interval_str) continue;

        PluginItem *item = &g_plugins[index];
        item->name        = strdup(name);
        item->description = strdup(description);
        item->command     = strdup(command);
        item->active      = atoi(active_str);
        item->interval    = atoi(interval_str);
        item->id          = index;

        // Initialize timestamps and status
        strcpy(item->lastRunTimestamp, "");      // or use current time
        strcpy(item->nextRunTimestamp, "");
        strcpy(item->lastChangeTimestamp, "");
        strcpy(item->statusChanged, "0");
        item->nextRun = 0;

        index++;
    }

    fclose(fp);
}*/

void load_plugins() {
    FILE *fp = fopen(pluginDeclarationFile, "r");
    if (!fp) {
        perror("Failed to open config file");
        return;
    }

    /* Allocate array of pointers */
    PluginItem **new_list = calloc(decCount, sizeof *new_list);
    if (!new_list) {
        perror("Memory allocation failed for plugin list");
        fclose(fp);
        return;
    }

    char  line[1024];
    int   index = 0;
    //int lineno = 0;
    
    while (fgets(line, sizeof(line), fp) && index < decCount) {
        //printf("[DBG] line %3d: %s", ++lineno, line);
        /* Skip comments and very short lines */
        if (line[0] == '#' || strlen(line) < 5) 
            continue;

        /* Trim newline */
        line[strcspn(line, "\n")] = '\0';

        /* Parse [name]desc;cmd;active;interval */
        char *start = strchr(line, '[');
        char *end   = strchr(line, ']');
        if (!start || !end || end <= start) 
            continue;

        *end = '\0';
        char *name        = start + 1;
        char *rest        = end + 1;
        while (*rest == ' ') rest++;

        char *description = strtok(rest, ";");
        char *command     = strtok(NULL, ";");
        char *active_str  = strtok(NULL, ";");
        char *interval_str= strtok(NULL, ";");
        if (!description || !command || !active_str || !interval_str)
            continue;
       
         /* Allocate and initialize one PluginItem */
        PluginItem *item = calloc(1, sizeof *item);
        if (!item) {
            perror("calloc PluginItem");
            break;
        }

        /* Copy config fields */
        item->name        = strdup(name);
        item->description = strdup(description);
        item->command     = strdup(command);
        item->active      = atoi(active_str);
        item->interval    = atoi(interval_str);
        item->id          = index;

        /* Initialize all timestamps and status */
        item->lastRunTimestamp[0]    = '\0';
        item->nextRunTimestamp[0]    = '\0';
        item->lastChangeTimestamp[0] = '\0';
        strcpy(item->statusChanged, "0");
        item->nextRun = 0;

        /* Initialize output state */
        item->output.retCode     = 0;
        item->output.prevRetCode = 0;
        item->output.retString   = strdup("");  // or NULL if you prefer

        /* Mark as present (for future reload logic) */
        item->touched = true;
/* Add to hash map keyed by name */
        HASH_ADD_KEYPTR(hh, g_plugin_map, item->name, strlen(item->name), item);

        /* Append to ordered list */
        new_list[index++] = item;
    }

    fclose(fp);

    /* Replace globals */
    free(g_plugins);
    g_plugins      = new_list;
    g_plugin_count = index;
}

int init_plugins() {
	printf("Initiate plugins\n");
	load_plugins();
	if (!g_plugins) {
		return 1;
	}
        g_plugin_count = (size_t)decCount;
        printf("Loaded %d plugins\n", g_plugin_count);
        for (int i = 0; i < g_plugin_count; i++) {
        	//printf("  [%zu] name=%s cmd=\"%s\"\n",
                //        i,
                //        g_plugins[i].name,
                //        g_plugins[i].command);
                //mock_runCommand(&g_plugins[i]);
                //runPlugin(i);
    	}
	return 0;
}

/*int findPluginIndexByName(const char *name) {
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, name) == 0) return i;
    }
    return -1;
}

size_t getPluginCount(void) {
    return g_plugin_count;
}*/

void update_plugins(void) {
    // 1) Load new config declarations
    int newCount = 0;
    PluginItem **decls = load_declarations(pluginDeclarationFile, &newCount);
    if (!decls) {
        return;
    }
    if (newCount <= 0) {
        free(decls);
        return;
    }

    // 2) Steal the old array and count
    PluginItem **old_plugs = g_plugins;
    size_t       oldCount  = decCount;

    // 3) Allocate and immediately swap in the new g_plugins[]
    PluginItem **new_array = calloc((size_t)newCount, sizeof *new_array);
    if (!new_array) {
        perror("calloc new g_plugins");
        // roll back; g_plugins was untouched
        free(decls);
        return;
    }
    g_plugins   = new_array;
    decCount    = newCount;

    // 4) Mark all old items untouched
    for (size_t j = 0; j < oldCount; ++j) {
        if (old_plugs && old_plugs[j]) {
            old_plugs[j]->touched = false;
        }
    }

    // 5) Diff/merge in file order
    for (int i = 0; i < newCount; ++i) {
        PluginItem *cfg    = decls[i];
        cfg->id            = i;  // enforce file-order id
        bool reused        = false;
        char *new_bc       = extract_base_command(cfg->command);

        // Try to match an old plugin by name + base_command
        for (size_t j = 0; j < oldCount; ++j) {
            PluginItem *old = old_plugs ? old_plugs[j] : NULL;
            if (!old)
                continue;

            if (strcmp(old->name, cfg->name) == 0) {
                char *old_bc = extract_base_command(old->command);
                if (old_bc && new_bc && strcmp(old_bc, new_bc) == 0) {
                    // Reuse runtime fields + output
                    cfg->nextRun = old->nextRun;

                    // Copy timestamps (ensure termination)
                    strncpy(cfg->lastRunTimestamp,    old->lastRunTimestamp,    TIMESTAMP_SIZE);
                    cfg->lastRunTimestamp[TIMESTAMP_SIZE - 1] = '\0';
                    strncpy(cfg->nextRunTimestamp,    old->nextRunTimestamp,    TIMESTAMP_SIZE);
                    cfg->nextRunTimestamp[TIMESTAMP_SIZE - 1] = '\0';
                    strncpy(cfg->lastChangeTimestamp, old->lastChangeTimestamp, TIMESTAMP_SIZE);
                    cfg->lastChangeTimestamp[TIMESTAMP_SIZE - 1] = '\0';

                    // Copy statusChanged
                    memcpy(cfg->statusChanged, old->statusChanged, STATUS_SIZE);

                    // Transfer PluginOutput ownership
                    cfg->output = old->output;
                    old->output.retString = NULL; // disown to avoid double-free

                    cfg->touched = true;
                    reused       = true;

                    // Replace in hash: remove old before adding cfg
                    HASH_DEL(g_plugin_map, old);

                    // Free old struct (fields we did NOT transfer)
                    free(old->name);
                    free(old->description);
                    free(old->command);
                    free(old);

                    // Mark slot as consumed (Step 6 won't see it)
                    old_plugs[j] = NULL;
                }
                free(old_bc);
                break;
            }
        }

        // Brand-new plugin ⇒ initialize and run once
        if (!reused) {
            cfg->nextRun                 = 0;
            cfg->lastRunTimestamp[0]     = '\0';
            cfg->nextRunTimestamp[0]     = '\0';
            cfg->lastChangeTimestamp[0]  = '\0';
            strcpy(cfg->statusChanged, "0");

            run_plugin(cfg);
        }

        // Add the cfg (both new and reused) to the hash
        HASH_ADD_KEYPTR(hh, g_plugin_map, cfg->name, strlen(cfg->name), cfg);

        // Install in the new array
        g_plugins[i] = cfg;

        free(new_bc);
    }

    // 6) Teardown truly removed or renamed plugins
    if (old_plugs) {
        for (size_t j = 0; j < oldCount; ++j) {
            PluginItem *old = old_plugs[j];
            if (!old)
                continue;

            // Remove from hash first
            HASH_DEL(g_plugin_map, old);

            // Free fields (retString only if still owned)
            free(old->name);
            free(old->description);
            free(old->command);
            if (old->output.retString) {
                free(old->output.retString);
                old->output.retString = NULL;
            }

            free(old);
        }
        free(old_plugs);
    }

    // 7) Cleanup
    g_plugin_count = decCount;
    free(decls);       // decls holds pointers now owned by g_plugins (or freed above)
    malloc_trim(0);
}


/*void update_plugins(void) {
    // 1) Load new config declarations
    int newCount = 0;
    PluginItem **decls = load_declarations(pluginDeclarationFile, &newCount);
    if (!decls || newCount <= 0) {
        free(decls);
        return;
    }

    // 2) Steal the old array and count
    PluginItem **old_plugs = g_plugins;
    size_t       oldCount = decCount;

    // 3) Allocate and immediately swap in the new g_plugins[]
    g_plugins = calloc(newCount, sizeof *g_plugins);
    if (!g_plugins) {
        perror("calloc new g_plugins");
        g_plugins = old_plugs;  // roll back
        free(decls);
        return;
    }
    //g_plugins = decls;
    decCount = newCount;
    //decls = NULL;

    // 4) Mark all old items untouched
    for (size_t j = 0; j < oldCount; ++j) {
        if (old_plugs[j]) {
            old_plugs[j]->touched = false;
        }
    }

    // 5) Diff/merge in file order
   for (int i = 0; i < newCount; ++i) {
        PluginItem *cfg    = decls[i];
        cfg->id            = i;  // enforce rule a
        bool        reused = false;
        char       *new_bc = extract_base_command(cfg->command);

        // Try to match an old plugin by name + base_command
        for (size_t j = 0; j < oldCount; ++j) {
            PluginItem *old = old_plugs[j];
            if (!old) 
                continue;

            if (strcmp(old->name, cfg->name) == 0) {
                char *old_bc = extract_base_command(old->command);
                if (old_bc && new_bc && strcmp(old_bc, new_bc) == 0) {
                    // Reuse runtime fields + output
                    cfg->nextRun               = old->nextRun;
                    strncpy(cfg->lastRunTimestamp,
                            old->lastRunTimestamp,
                            TIMESTAMP_SIZE);
                    strncpy(cfg->nextRunTimestamp,
                            old->nextRunTimestamp,
                            TIMESTAMP_SIZE);
                    strncpy(cfg->lastChangeTimestamp,
                            old->lastChangeTimestamp,
                            TIMESTAMP_SIZE);
                    memcpy(cfg->statusChanged,
                           old->statusChanged,
                           STATUS_SIZE);

                    // Transfer the PluginOutput struct
                    cfg->output                = old->output;
                    old->output.retString      = NULL;  // avoid double-free

                    cfg->touched  = true;
                    reused        = true;

                    // Mark old slot as consumed
                    old_plugs[j] = NULL;
                }
                free(old_bc);
                break;
            }
        }

        // Brand-new plugin ⇒ initialize and run once (rule c)
        if (!reused) {
            cfg->nextRun               = 0;
            cfg->lastRunTimestamp[0]   = '\0';
            cfg->nextRunTimestamp[0]   = '\0';
            cfg->lastChangeTimestamp[0]= '\0';
            strcpy(cfg->statusChanged, "0");

            run_plugin(cfg);
        }

        free(new_bc);
        HASH_ADD_KEYPTR(hh, g_plugin_map, cfg->name, strlen(cfg->name), cfg);
        g_plugins[i] = cfg;
    }

    // 6) Teardown truly removed or renamed plugins (rule b, d, e)
    for (size_t j = 0; j < oldCount; ++j) {
        PluginItem *old = old_plugs[j];
        if (!old)
            continue;

        // No scheduled runs to cancel in this version
        free(old->name);
        free(old->description);
        free(old->command);

        // Only free retString if it wasn’t transferred
        if (old->output.retString) {
            free(old->output.retString);
        }
        HASH_DEL(g_plugin_map, old);
        free(old->name);
	free(old->description);
	free(old->command);
	if (old->output.retString) {
    		free(old->output.retString);
	}
        free(old);
    }

    // 7) Cleanup
    free(old_plugs);
    malloc_trim(0);
    free(decls);
    g_plugin_count = decCount;
}*/

/*void update_plugins(void) {
    // 1) Load new config‐only declarations
    int newCount = 0;
    PluginItem **decls = load_declarations(pluginDeclarationFile, &newCount);
    if (!decls || newCount <= 0) {
        free(decls);
        return;
    }

    // 2) Save old arrays for diff + teardown
    PluginItem  **old_plugs   = g_plugins;
    PluginOutput  *old_outs   = g_outputs;
    size_t         oldCount   = decCount;

    // 3) Allocate fresh arrays and swap in immediately
    g_plugins = calloc(newCount, sizeof *g_plugins);
    g_outputs = calloc(newCount, sizeof *g_outputs);
    if (!g_plugins || !g_outputs) {
        perror("calloc new plugins/outputs");
        // restore old pointers before exit
        g_plugins = old_plugs;
        g_outputs = old_outs;
        free(decls);
        return;
    }
    decCount = newCount;

    // 4) Mark all old entries as untouched
    for (size_t j = 0; j < oldCount; ++j) {
        if (old_plugs[j]) {
            old_plugs[j]->touched = false;
        }
    }
     // 5) Diff & merge in file order (IDs = new array index)
    for (int i = 0; i < newCount; ++i) {
        PluginItem *cfg    = decls[i];
        cfg->id            = i;                       // assign new ID
        char       *new_bc = extract_base_command(cfg->command);
        bool        reused = false;

        // Try finding matching old plugin by name + base_command
        for (size_t j = 0; j < oldCount; ++j) {
            PluginItem *old = old_plugs[j];
            if (!old) continue;

            if (strcmp(old->name, cfg->name) == 0) {
                char *old_bc = extract_base_command(old->command);
                if (old_bc && new_bc && strcmp(old_bc, new_bc) == 0) {
                    // reuse runtime state + output
                    cfg->nextRun               = old->nextRun;
                    strncpy(cfg->lastRunTimestamp,
                            old->lastRunTimestamp,
                            TIMESTAMP_SIZE);
                    strncpy(cfg->nextRunTimestamp,
                            old->nextRunTimestamp,
                            TIMESTAMP_SIZE);
                    strncpy(cfg->lastChangeTimestamp,
                            old->lastChangeTimestamp,
                            TIMESTAMP_SIZE);
                    memcpy(cfg->statusChanged,
                           old->statusChanged,
                           STATUS_SIZE);
 // transfer old output into new slot
                    g_outputs[i] = old_outs[j];

                    cfg->touched  = true;
                    reused        = true;

                    // consume this old entry
                    free(old_bc);
                    old_plugs[j] = NULL;
                }
                free(old_bc);
                break;
            }
        }

        // Brand‐new plugin ⇒ initialize + run once
        if (!reused) {
            // zero runtime fields
            cfg->nextRun               = 0;
            cfg->lastRunTimestamp[0]   = '\0';
            cfg->nextRunTimestamp[0]   = '\0';
            cfg->lastChangeTimestamp[0]= '\0';
            strcpy(cfg->statusChanged, "0");

            // use run_plugin() to populate g_outputs[i]
            run_plugin(cfg);
        }

        free(new_bc);
        g_plugins[i] = cfg;
    }

    // 6) Teardown any untouched old plugins (deleted or renamed)
    for (size_t j = 0; j < oldCount; ++j) {
        PluginItem *old = old_plugs[j];
        if (!old) 
            continue;
        cancel_scheduled_run(old);
        free(old->name);
        free(old->description);
        free(old->command);
        free(old);
        free(old_outs[j].retString);
    }

    // 7) Free old arrays & the decls container
    free(old_plugs);
    free(old_outs);
    free(decls);
}*/

/*void update_plugins() {
    int newCount = countDeclarations(pluginDeclarationFile);
    printf("Allocating %d PluginItem structs (%zu bytes)\n", newCount, newCount * sizeof(PluginItem));
    PluginItem *new_plugins = calloc(newCount, sizeof(PluginItem));
    PluginOutput *new_otpts = calloc(newCount, sizeof(PluginOutput));
    if (!new_plugins) {
        perror("Memory allocation failed");
        return;
    }
    if (!new_otpts) {
	perror("Memory allocation failed");
	return;
    }

    // Load new config into new_plugins
    FILE *fp = fopen(pluginDeclarationFile, "r");
    if (!fp) {
        perror("Failed to open config file");
        free(new_plugins);
        return;
    }

    char line[1024];
    int index = 0;

    while (fgets(line, sizeof(line), fp) && index < newCount) {
        if (line[0] == '#' || strlen(line) < 5) continue;
        line[strcspn(line, "\n")] = 0;

        char *start = strchr(line, '[');
        char *end = strchr(line, ']');
        if (!start || !end || end <= start) continue;
	 *end = '\0';
        char *name = start + 1;
        char *rest = end + 1;
        while (*rest == ' ') rest++;

        char *description = strtok(rest, ";");
        char *command     = strtok(NULL, ";");
        char *active_str  = strtok(NULL, ";");
        char *interval_str= strtok(NULL, ";");

        if (!description || !command || !active_str || !interval_str) continue;

        printf("Index: %d / newCount: %d\n", index, newCount);
        //assert(item != NULL);
	//assert((uintptr_t)item % alignof(PluginItem) == 0);
        PluginItem *item = &new_plugins[index];
        item->name        = strdup(name);
        item->description = strdup(description);
        item->command     = strdup(command);
        item->active      = atoi(active_str);
        item->interval    = atoi(interval_str);
        item->id          = index;

        // Compare with existing g_plugins
        int found = 0;
        for (int i = 0; i < decCount; ++i) {
            PluginItem *old = &g_plugins[i];
            //PluginOutput *old_out = &g_outputs[i];
            if (!old->name || !old->command) continue;

            if (strcmp(old->name, item->name) == 0) {
                char *old_base = extract_base_command(old->command);
                char *new_base = extract_base_command(item->command);

                if (old_base && new_base && strcmp(old_base, new_base) == 0) {
                    // Match found, copy runtime fields
                    //strcpy(item->lastRunTimestamp, old->lastRunTimestamp);
                    strncpy(item->lastRunTimestamp, old->lastRunTimestamp, TIMESTAMP_SIZE - 1);
		    item->lastRunTimestamp[TIMESTAMP_SIZE - 1] = '\0';
                    //strcpy(item->nextRunTimestamp, old->nextRunTimestamp);
                    strncpy(item->nextRunTimestamp, old->nextRunTimestamp, TIMESTAMP_SIZE - 1);
		    item->nextRunTimestamp[TIMESTAMP_SIZE - 1] = '\0';
                    //strcpy(item->lastChangeTimestamp, old->lastChangeTimestamp);
                    strncpy(item->lastChangeTimestamp, old->lastChangeTimestamp, TIMESTAMP_SIZE - 1);
		    item->lastChangeTimestamp[TIMESTAMP_SIZE - 1] = '\0';
                    strcpy(item->statusChanged, old->statusChanged);
                    item->nextRun = old->nextRun;
                    found = 1;
                }

                free(old_base);
                free(new_base);
                break;
            }
        }

	if (!found) {
            // New plugin, initialize runtime fields
            strcpy(item->lastRunTimestamp, "");
            strcpy(item->nextRunTimestamp, "");
            strcpy(item->lastChangeTimestamp, "");
            strcpy(item->statusChanged, "0");
            item->nextRun = 0;

            mock_runCommand(item);
        }

        index++;
    }

    fclose(fp);

    // Free old plugins
    for (int i = 0; i < decCount; ++i) {
        free(g_plugins[i].name);
        free(g_plugins[i].description);
        free(g_plugins[i].command);
        //free(g_outputs[i].retString);
    }
    free(g_plugins);
    //free(g_outputs);

    // Replace with new
    g_plugins = new_plugins;
    //g_outputs = new_otpts;
    decCount = newCount;
    g_plugin_count = (size_t)newCount;
}*/

/*void updatePluginDeclarationsOld(void) {
    size_t orig_count = g_plugin_count;
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Track which new declarations have been processed
    bool *processed_new = calloc(new_count, sizeof(bool));
    if (!processed_new) {
        free(matched);
        free_declarations(new_decls, new_count);
        return;
    }

    // Debug logging
    printf("Pre-sort: ");
    for (size_t i = 0; i < orig_count; i++) {
        printf("id=%d name=%s ", g_plugins[i].id, g_plugins[i].name);
    }
    printf("\n");

    // Phase 1: Match and update existing plugins
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool matched_existing = false;

        // Skip if this new declaration has already been processed
        if (processed_new[i]) {
            continue;
        }

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];

            // Extract base commands
            char *old_base_cmd = extract_base_command(old->command);
            char *new_base_cmd = extract_base_command(new->command);

            if (old_base_cmd == NULL || new_base_cmd == NULL) {
                free(old_base_cmd);
                free(new_base_cmd);
                continue;
            }

            if (strcmp(old_base_cmd, new_base_cmd) == 0 && !matched[j]) {
                matched[j] = true;
                processed_new[i] = true;
                matched_existing = true;

                // Free old strings
                if (old->name != NULL) {
                    free(old->name);
                }
                if (old->description != NULL) {
                    free(old->description);
                }
                if (old->command != NULL) {
                    free(old->command);
                }

                // Copy new values
                old->name = strdup(new->name);
                old->description = strdup(new->description);
                old->command = strdup(new->command);

                if (old->name == NULL || old->description == NULL || 
                    old->command == NULL) {
                    // Handle allocation failure
                    free(old_base_cmd);
                    free(new_base_cmd);
                    free_declarations(new_decls, new_count);
                    free(matched);
                    free(processed_new);
                    return;
                }

                old->id = new->id;
                free(old_base_cmd);
                free(new_base_cmd);
                break;
            }
        }
    }

    // Debug logging
    printf("After matching: ");
    for (size_t i = 0; i < orig_count; i++) {
        printf("id=%d name=%s matched=%d ", g_plugins[i].id, 
               g_plugins[i].name, matched[i]);
    }
    printf("\n");

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(&g_plugins[i]);
        }
    }

    // Debug logging
    printf("After removal: ");
    for (size_t i = 0; i < orig_count; i++) {
        if (g_plugins[i].name != NULL) {
            printf("id=%d name=%s ", g_plugins[i].id, g_plugins[i].name);
        }
    }
    printf("\n");

    // Phase 3: Rebuild g_plugins array
    PluginItem *temp_array = malloc(sizeof(PluginItem) * g_plugin_count);
    if (temp_array == NULL) {
        free(matched);
        free(processed_new);
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 4: Add new unmatched declarations
    for (size_t i = 0; i < new_count; i++) {
    	if (!processed_new[i]) {
        	addPlugin(&new_decls[i]);
    	}
    }

    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name && g_plugins[i].name[0] != '\0') {
            temp_array[new_total] = g_plugins[i];
            new_total++;
        }
    }

    // Debug logging
    printf("In temp array: ");
    for (size_t i = 0; i < new_total; i++) {
        printf("id=%d name=%s ", temp_array[i].id, temp_array[i].name);
    }
    printf("\n");

    // Copy back to original array
    for (size_t i = 0; i < new_total; i++) {
        g_plugins[i] = temp_array[i];
    }

    free(temp_array);
    g_plugin_count = new_total;

    free(matched);
    free(processed_new);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
	update_plugins();
}*/

PluginItem *getPluginItem(size_t index) {
    if (index < g_plugin_count)
         return g_plugins[index];
        //return &g_plugins[index];
    return NULL;
}
