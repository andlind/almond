#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>

extern FILE *fptr;
extern char *logmessage;
extern char* logfile;
extern char* logDir;
extern int is_file_open;
extern pthread_cond_t file_opened;
extern size_t logmessage_size;
extern size_t logfile_size;

#endif // MAIN_H
