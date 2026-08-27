#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>
#include <signal.h> 

extern FILE *fptr;
extern char *logmessage;
extern char* logfile;
extern char* logDir;
extern int is_file_open;
extern int shutdown_phase;
extern pthread_cond_t file_opened;
extern size_t logmessage_size;
extern size_t logfile_size;
extern volatile sig_atomic_t is_stopping;

void free_constants(void);
void free_plugin_declarations(void);
void destroy_mutexes(void);

#endif // MAIN_H
