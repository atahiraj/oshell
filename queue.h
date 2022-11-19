#ifndef queue_h
#define queue_h

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>  
#include <sys/queue.h>
#include <sys/stat.h>

/* process_info structure definition */
typedef struct process_info
{
	char* name;
	pid_t pid;
	int exit_code;
	STAILQ_ENTRY(process_info) next;
} process_info;
process_info* new_process_info(char const * const name, const pid_t pid, 
									const int exit_code);
void free_process_info(process_info * const p);

/* queue strcture definition */
typedef STAILQ_HEAD(queue, process_info) queue;
queue* new_queue(void);
void empty_queue(queue * const head);
void free_queue(queue * const head);

#endif
