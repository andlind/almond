#ifndef ALMOND_PLUGINS_H
#define ALMOND_PLUGINS_H

#include "data.h"

// global config
extern char* pluginDeclarationFile;
extern size_t g_plugin_count;

// main entry point
void init_plugins(const char *, size_t *);
void updatePluginDeclarations(void);
size_t getPluginCount();
PluginItem *getPluginItem(size_t index);

#endif // ALMOND_PLUGINS_H
