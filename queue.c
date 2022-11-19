#include "queue.h"

/* -----------------------------------------------------------------------------
 * Creates a new process_info structure. 
 * Structure should be freed using free_prcess.
 *
 * PARAMETERS
 * name         process_info name (char*)
 * pid          process_info id (pid_t)
 * exit_code    process_info exit code (int)
 *
 * RETURN
 * p            a pointer to the new process_info structure (process_info*)
 * ---------------------------------------------------------------------------*/
process_info* new_process_info(char const * const name, const pid_t pid,
                                    const int exit_code)
{
    process_info* p = (process_info*)malloc(sizeof(process_info));
    if(p == NULL)
        goto mem_err1;

    p->name = (char*)malloc((strlen(name) + 1) * sizeof(char));
    if(p->name == NULL)
        goto mem_err2;

    strcpy(p->name, name);
    p->pid = pid;
    p->exit_code = exit_code;
    
    return p;

mem_err2:
    free(p); // Useless since we exit
mem_err1:
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
}

/* -----------------------------------------------------------------------------
 * Frees process_info structure
 *
 * PARAMETERS
 * p            pointer to the structure to be freed (process_info*)
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void free_process_info(process_info * const p)
{
    if(p != NULL)
    {
        free(p->name);
        free(p);
    }
    return;
}

/* -----------------------------------------------------------------------------
 * Creates a new queue structure. Structure should be freed using free_queue.
 *
 * PARAMETERS
 * /           
 *
 * RETURN
 * head         a pointer to the new queue structure (queue*)
 * ---------------------------------------------------------------------------*/
queue* new_queue(void)
{
    queue* head = (queue*)malloc(sizeof(queue));
    if(head == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    STAILQ_INIT(head);
    return head;
}

/* -----------------------------------------------------------------------------
 * Empties queue structure by freeing all its elements
 *
 * PARAMETERS
 * head         a pointer to the structure to be emptied (queue*)
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void empty_queue(queue * const head)
{
    if(head == NULL)
        return;
    
    while (!STAILQ_EMPTY(head))
    {
        process_info* p;
        p = STAILQ_FIRST(head);
        STAILQ_REMOVE_HEAD(head, next);
        free_process_info(p);
    }
    return;
}

/* -----------------------------------------------------------------------------
 * Frees a queue structure
 *
 * PARAMETERS
 * head         a pointer to the structure to be freed (queue*)
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void free_queue(queue * const head)
{
    empty_queue(head);
    free(head);
}
