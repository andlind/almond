#ifndef ALMOND_DATA_STRUCTURES_HEADER
#define ALMOND_DATA_STRUCTURES_HEADER

#include <time.h>
#include <stdbool.h>

typedef struct PluginItem {
    char   *name;
    char   *description;
    char   *command;                // use "command" consistently
    char    lastRunTimestamp[20];
    char    nextRunTimestamp[20];
    char    lastChangeTimestamp[20];
    int     active;
    int     interval;
    int     id;
    time_t  nextRun;
} PluginItem;

#endif // ALMOND_DATA_STRUCTURES_HEADER
