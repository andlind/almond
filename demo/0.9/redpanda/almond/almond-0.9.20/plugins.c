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

// if strdup is missing, declare it
extern char *strdup(const char *);

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

PluginItem *getPluginItem(size_t index) {
    if (index < g_plugin_count)
         return g_plugins[index];
    return NULL;
}
