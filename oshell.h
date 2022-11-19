#ifndef oshell_h
#define oshell_h

#define _POSIX_SOURCE
// You can add new header(s), prototype(s) and constant(s)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h> 
#include <linux/limits.h>
#include "queue.h"

static const int MAX_CMD_SIZE = 256;                // DO NOT MODIFY
static const int MAX_ARGS = 256;                    // DO NOT MODIFY

char readCharInput(void);                           // DO NOT MODIFY
void parseCmdLine(char* line, char** arguments);    // DO NOT MODIFY

void my_exit(void);
void cd(char const * const nwd);
void init(void);
void loadmem(void);
void memdump(void);
void paral(int copies, char * const * const argv);
void sequent(int copies, char * const * const argv);
void showlist(void);
void sigalarm_handler(int mysignal);
void sigchild_handler(int mysignal);
void devstats(void);
void netstats(void);
void sysstats(char const * const pid);

#endif /* oshell_h */
