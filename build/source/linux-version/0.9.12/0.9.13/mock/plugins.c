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

static void removePlugin(const char *name) {
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
}

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

/*void updatePluginDeclarations(void) {
    // 1) Snapshot the original count
    size_t orig_count = g_plugin_count;

    // 2) Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // 3) Allocate matched[] against the OLD count
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // 4) Loop over new decls
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        // Pre-extract once per new
        char *new_base_cmd = extract_base_command(new->command);

        // Compare only against the original set
        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];
            char *old_base_cmd = extract_base_command(old->command);

            if (strcmp(old->name, new->name) == 0) {
                found = true;

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Rule c: base command changed
                    removePlugin(old->name);
                    addPlugin(new);

                    // Newly added plugin lives at the *end*, so its index is g_plugin_count-1
                    matched[j] = true;  
                }
                else {
                    // Rule d: soft update in place
                    matched[j] = true;
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }
            }

            free(old_base_cmd);
        }

        free(new_base_cmd);

        if (!found) {
            // Rule b: brand-new plugin
            addPlugin(new);

            // Mark the slot in the *new* array that corresponds to this add
            size_t added_idx = g_plugin_count - 1;
            if (added_idx < orig_count)  // sanity check
                matched[added_idx] = true;
        }
    }

       // 5) Compact out the “false” slots in matched[], remove as you go
    size_t write = 0;
    for (size_t read = 0; read < orig_count; ++read) {
        if (matched[read]) {
            // keep this one – if read!=write, slide it down
            if (write != read) {
                g_plugins[write] = g_plugins[read];
            }
            ++write;
        }
        else {
            // drop this one
            removePlugin(g_plugins[read].name);

            // if removePlugin() doesn’t free the strings for you:
            // free(g_plugins[read].name);
            // free(g_plugins[read].command);
            // free(g_plugins[read].description);
        }
    }

    // now “write” is the new count
    g_plugin_count = write;

    // 6) Cleanup
    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;
    PluginItem *new_decls = NULL;
    bool *matched = NULL;
    
    // Step 1: Load new declarations
    new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) {
        goto cleanup;
    }

    // Step 2: Allocate matched array
    matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        goto cleanup;
    }

    // ... rest of the function remains the same ...

cleanup:
    if (matched) free(matched);
    if (new_decls) free_declarations(new_decls, new_count);
    return;
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        char *new_base_cmd = extract_base_command(new->command);

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];
            char *old_base_cmd = extract_base_command(old->command);

            if (strcmp(old->name, new->name) == 0) {
                found = true;
                matched[j] = true;

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    removePlugin(old->name);
                    addPlugin(new);
                } else {
                    old->id = new->id;
                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }
            }

            free(old_base_cmd);
        }

        free(new_base_cmd);

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
	    memset(&g_plugins[i], 0, sizeof(PluginItem));
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    //for (size_t i = 0; i < g_plugin_count; i++) {
    //    PluginItem *p = &g_plugins[i];
    //    if (p->name && strlen(p->name) > 0) {
    //        if (new_total != i) {
    //            g_plugins[new_total] = g_plugins[i];
    //        }
    //        new_total++;
    //    }
    //}

    for (size_t i = 0; i < g_plugin_count; i++) {
    	if (g_plugins[i].name && strlen(g_plugins[i].name) > 0) {
        	if (new_total != i)
            		g_plugins[new_total] = g_plugins[i];
        	new_total++;
    	}
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        char *new_base_cmd = extract_base_command(new->command);

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];
            char *old_base_cmd = extract_base_command(old->command);

            if (strcmp(old->name, new->name) == 0) {
                found = true;

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    removePlugin(old->name);
                    addPlugin(new);
                    memset(&g_plugins[j], 0, sizeof(PluginItem)); // Clear removed slot
                } else {
                    matched[j] = true; // Only mark as matched if we keep the original
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }
            }

            free(old_base_cmd);
        }

        free(new_base_cmd);

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem)); // Clear removed slot
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name != NULL && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        char *new_base_cmd = extract_base_command(new->command);

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];
            char *old_base_cmd = extract_base_command(old->command);

            if (strcmp(old->name, new->name) == 0) {
                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Base command changed — treat as replacement
                    removePlugin(old->name);
                    addPlugin(new);
                    memset(&g_plugins[j], 0, sizeof(PluginItem)); // Clear removed slot
                    // Do NOT mark as matched or found
                } else {
                    // Base command matches — update in place
                    matched[j] = true;
                    found = true;
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }
            }

            free(old_base_cmd);
        }

        free(new_base_cmd);

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem)); // Clear removed slot
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name != NULL && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        char *new_base_cmd = extract_base_command(new->command);

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];

            if (strcmp(old->name, new->name) == 0) {
                char *old_base_cmd = extract_base_command(old->command);

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Base command changed — replace plugin
                    removePlugin(old->name);
                    addPlugin(new);
                    memset(&g_plugins[j], 0, sizeof(PluginItem)); // Clear removed slot
                    // Don't mark as matched or found
                } else {
                    // Base command matches — update in place
                    matched[j] = true;
                    found = true;
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }

                free(old_base_cmd);
                break; // Stop after matching by name
            }
        }

        free(new_base_cmd);

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem)); // Clear removed slot
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name != NULL && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];

            if (strcmp(old->name, new->name) == 0) {
                found = true;

                char *old_base_cmd = extract_base_command(old->command);
                char *new_base_cmd = extract_base_command(new->command);

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Base command changed — replace plugin
                    removePlugin(old->name);
                    addPlugin(new);
                    memset(&g_plugins[j], 0, sizeof(PluginItem)); // Clear removed slot
                    // Don't mark as matched
                } else {
                    // Base command matches — update in place
                    matched[j] = true;
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }

                free(old_base_cmd);
                free(new_base_cmd);
                break; // Stop after matching by name
            }
        }

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem)); // Clear removed slot
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name != NULL && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

/*void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool found = false;

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];

            if (strcmp(old->name, new->name) == 0) {
                found = true;

                char *old_base_cmd = extract_base_command(old->command);
                char *new_base_cmd = extract_base_command(new->command);

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Base command changed — replace plugin
                    removePlugin(old->name);
                    memset(&g_plugins[j], 0, sizeof(PluginItem)); // Clear removed slot
                    addPlugin(new); // Add new plugin
                } else {
                    // Base command matches — update in place
                    matched[j] = true;
                    old->id = new->id;

                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }

                free(old_base_cmd);
                free(new_base_cmd);
                break; // Stop after matching by name
            }
        }

        if (!found) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem)); // Clear removed slot
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name != NULL && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}*/

void updatePluginDeclarations(void) {
    size_t orig_count = g_plugin_count;

    // Load new declarations
    size_t new_count;
    PluginItem *new_decls = load_declarations(pluginDeclarationFile, &new_count);
    if (!new_decls) return;

    // Track which original plugins were matched
    bool *matched = calloc(orig_count, sizeof(bool));
    if (!matched) {
        free_declarations(new_decls, new_count);
        return;
    }

    // Phase 1: Match and update
    for (size_t i = 0; i < new_count; i++) {
        PluginItem *new = &new_decls[i];
        new->id = i + 1;
        bool matched_existing = false;

        for (size_t j = 0; j < orig_count; j++) {
            PluginItem *old = &g_plugins[j];

            if (strcmp(old->name, new->name) == 0) {
                matched[j] = true;
                matched_existing = true;

                char *old_base_cmd = extract_base_command(old->command);
                char *new_base_cmd = extract_base_command(new->command);

                if (strcmp(old_base_cmd, new_base_cmd) != 0) {
                    // Command changed — replace plugin
                    removePlugin(old->name);
                    memset(&g_plugins[j], 0, sizeof(PluginItem));
                    addPlugin(new);
                } else {
                    // Command same — update if needed
                    old->id = new->id;
                    if (strcmp(old->command,     new->command)     != 0 ||
                        strcmp(old->description, new->description) != 0 ||
                        old->active             != new->active     ||
                        old->interval           != new->interval) {
                        updatePlugin(old, new);
                    }
                }

                free(old_base_cmd);
                free(new_base_cmd);
                break;
            }
        }

        if (!matched_existing) {
            // Brand-new plugin
            addPlugin(new);
        }
    }

    // Phase 2: Remove unmatched original plugins
    for (size_t i = 0; i < orig_count; i++) {
        if (!matched[i]) {
            removePlugin(g_plugins[i].name);
            memset(&g_plugins[i], 0, sizeof(PluginItem));
        }
    }

    // Phase 3: Rebuild g_plugins to contain only current plugins
    size_t new_total = 0;
    for (size_t i = 0; i < g_plugin_count; i++) {
        if (g_plugins[i].name && g_plugins[i].name[0] != '\0') {
            if (new_total != i)
                g_plugins[new_total] = g_plugins[i];
            new_total++;
        }
    }

    g_plugin_count = new_total;

    free(matched);
    free_declarations(new_decls, new_count);
}

// Return pointer to the plugin at [index], or NULL if OOB
PluginItem *getPluginItem(size_t index) {
    if (index < g_plugin_count)
        return &g_plugins[index];
    return NULL;
}
