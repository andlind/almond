#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>
#include "data.h"
#include "plugins.h"

size_t      g_plugin_count        = 0;

// if strdup is missing, declare it
extern char *strdup(const char *);

static PluginItem *g_plugins   = NULL;
static size_t      g_plugin_cap = 0;
static int         g_next_id    = 1;

// simple non-reentrant timestamp fill
static void fill_timestamp(char buf[20]) {
    time_t now = time(NULL);
    struct tm tm;
    struct tm *tmp = localtime(&now);
    tm = *tmp;
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &tm);
}

static char *extract_base_command(const char *cmd) {
    char *space = strchr(cmd, ' ');
    size_t len = space ? (size_t)(space - cmd) : strlen(cmd);
    char *base = malloc(len + 1);
    strncpy(base, cmd, len);
    base[len] = '\0';
    return base;
}

// find by name in our live array
static PluginItem *find_plugin(const char *name) {
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, name) == 0)
            return &g_plugins[i];
    }
    return NULL;
}

static void addPlugin(const PluginItem *d) {
    if (g_plugin_count + 1 > g_plugin_cap) {
        g_plugin_cap = g_plugin_cap ? g_plugin_cap * 2 : 8;
        g_plugins    = realloc(g_plugins, g_plugin_cap * sizeof *g_plugins);
    }
    PluginItem *pi = &g_plugins[g_plugin_count++];
    memset(pi, 0, sizeof *pi);

    if (d->id > 0) {
        pi->id = d->id;
    }
    else {
        pi->id = g_next_id++;
    }
    pi->name     = strdup(d->name);
    pi->description = strdup(d->description);
    pi->command  = strdup(d->command);
    pi->active   = d->active;
    pi->interval = d->interval;
    fill_timestamp(pi->lastChangeTimestamp);
    fill_timestamp(pi->lastRunTimestamp);
    pi->nextRun  = time(NULL) + d->interval;
    strftime(pi->nextRunTimestamp, 20, "%Y-%m-%d %H:%M:%S",
             localtime(&pi->nextRun));

    //writeLog("Added plugin %s (id=%d)", 0, 0, pi->name, pi->id);
    printf("Added plugin %s (id=%d)\n", pi->name, pi->id);
}

static void removePlugin(PluginItem *plugin) {
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

/*static void removePlugin(const char *name) {
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name && strcmp(g_plugins[i].name, name) == 0) {
            printf("Removing plugin %s (id=%d)\n", g_plugins[i].name, g_plugins[i].id);

            // Free and null out pointers
            free(g_plugins[i].name);
            g_plugins[i].name = NULL;

            free(g_plugins[i].description);
            g_plugins[i].description = NULL;

            free(g_plugins[i].command);
            g_plugins[i].command = NULL;

            // Shift remaining plugins
            if (i < g_plugin_count - 1) {
                memmove(&g_plugins[i],
                        &g_plugins[i + 1],
                        (g_plugin_count - i - 1) * sizeof *g_plugins);
            }

            g_plugin_count--;

            // Optional: zero out last slot to avoid garbage
            memset(&g_plugins[g_plugin_count], 0, sizeof *g_plugins);

            return;
        }
    }
}*/

void updatePlugin(PluginItem *old, const PluginItem *new) {
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
        fill_timestamp(old->lastChangeTimestamp);
        //writeLog("Updated plugin %s (id=%d)", 0, 0, old->name, old->id);
	printf("Updated plugin %s (id=%d)\n", old->name, old->id);
    }
}


// load from file, return malloc()’d array and set *out_count
static PluginItem *load_declarations(const char *path, size_t *out_count) {
    FILE       *f = fopen(path, "r");
    char        buf[1024];
    PluginItem *arr = NULL;
    size_t      cap = 0, n = 0;

    if (!f) return NULL;
    while (fgets(buf, sizeof(buf), f)) {
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

        char *rb = strchr(tokens[0],']');
	if (!rb) continue;
        *rb = '\0';
        arr[n].id = n + 1;
        arr[n].name        = strdup(tokens[0]+1);
        arr[n].description = strdup(rb+1 + (* (rb+1)==' '));
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
}

static int decl_cmp(const void *a, const void *b) {
    return strcmp(((PluginItem*)a)->name, ((PluginItem*)b)->name);
}

void mock_runCommand(int index, char* cmd) {
	printf("Run: %s\n", cmd);
        fill_timestamp(g_plugins[index].lastRunTimestamp);
	fill_timestamp(g_plugins[index].nextRunTimestamp);
}

void init_plugins(const char *path, size_t *cnt) {
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
		//mock_runCommand(i, g_plugins[i].command);
    }
    
}

int findPluginIndexByName(const char *name) {
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (strcmp(g_plugins[i].name, name) == 0) return i;
    }
    return -1;
}

size_t getPluginCount(void) {
    return g_plugin_count;
}

void updatePluginDeclarations(void) {
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
        printf("id=%zu name=%s ", g_plugins[i].id, g_plugins[i].name);
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
        printf("id=%zu name=%s matched=%d ", g_plugins[i].id, 
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
            printf("id=%zu name=%s ", g_plugins[i].id, g_plugins[i].name);
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
        printf("id=%zu name=%s ", temp_array[i].id, temp_array[i].name);
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
}

PluginItem *getPluginItem(size_t index) {
    if (index < g_plugin_count)
        return &g_plugins[index];
    return NULL;
}
