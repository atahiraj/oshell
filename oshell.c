#define _GNU_SOURCE
#include "oshell.h"

// Macro to check whether static variable head is NULL
#define NULL_HEAD do{if(head == NULL) return;} while(0)

void findStatus(char* path, char* new, int* active, int* suspended, int* unsupported);

static int signal_flag;
static queue* head = NULL;

/* -----------------------------------------------------------------------------
 * Parse a command line into arguments.
 * /!\ DO NOT MODIFY /!\
 *
 * PARAMETERS
 * line         represents the line as a single string (unparsed).
 * arguments    represents an array of string which contains the command ([0]) 
 *              and its arguments ([1], [2], ... [255]).
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void parseCmdLine(char* line, char** arguments) {
    int i = 0;
    
    line[strlen(line) - 1] = '\0';
    arguments[i] = strtok(line, " ");
    while (arguments[i++] && i < MAX_ARGS) {
        arguments[i] = strtok(NULL, " ");
    }
}

/* -----------------------------------------------------------------------------
 * Read a character from the user input and discard up to the newline.
 * /!\ DO NOT MODIFY /!\
 *
 * PARAMETERS
 * /
 *
 * RETURN
 * char     a single character of user input.
 * ---------------------------------------------------------------------------*/
char readCharInput(void) {
    char c = getchar();
    
    while(getchar() != '\n');
    return c;
}

/* My creepy stuff */

/* -----------------------------------------------------------------------------
 * Change the working directory of the process_info
 *
 * PARAMETERS
 * nwd      absolute path of the new working directory (char*)
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void cd(char const * const nwd)
{
    int ret;
    if(nwd == NULL || strcmp(nwd, "~") == 0)
        ret = chdir(getenv("HOME"));
    else
        ret = chdir(nwd);
    if (ret == -1)
    {
        fprintf(stderr, "Directory change failed: ");
        perror(NULL);
    }
        
    return;
}

/* -----------------------------------------------------------------------------
 * Exits the program cleanly
 *
 * PARAMETERS
 * /
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void my_exit()
{
    // Freeing here is useless since we exit
    free_queue(head);
    exit(0);
}

/* -----------------------------------------------------------------------------
 * Initializes the queue so that the shell may be used
 *
 * PARAMETERS
 * /
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void init()
{
    free_queue(head); // Just in case
    head = new_queue();
}

/* -----------------------------------------------------------------------------
 * Loads the content of memdeump.file
 *
 * PARAMETERS
 * /
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void loadmem()
{
    NULL_HEAD;

    int fd;
    if(access("memdump.bin", F_OK | R_OK))
    {
        empty_queue(head);
        return;
    } 

    fd = open("memdump.bin", O_RDONLY, 0);
    if(fd == -1)
    {
        fprintf(stderr, "Failed to open memdump.bin: ");
        perror(NULL);
        return;
    }

    /* Reading binary file and building new Queue */
    char* name;
    queue* loaded_head = new_queue();
    int ret;
    while(1)
    {
        process_info* p;
        size_t name_size = 1;
        pid_t pid;
        int exit_code;   
        name = NULL;

        // Retrieving data
        ret = read(fd, &name_size, sizeof(size_t));
        if(ret != (int)sizeof(size_t))
        {
            if(ret == 0) // End of non corrupted file
                break;
            goto read_error;
        }

        name = (char*)malloc(name_size * sizeof(char));
        if(name == NULL)
        {
            free_queue(head);
            free_queue(loaded_head);
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }

        ret = read(fd, name, name_size);
        if(ret != (int)(name_size * sizeof(char)))
            goto read_error;

        ret = read(fd, &pid, sizeof(pid_t));
        if(ret != (int)sizeof(pid_t))
            goto read_error;

        ret= read(fd, &exit_code, sizeof(int));
        if(ret != (int)sizeof(int))
            goto read_error;

        // Creating new process_info structure with retrieved data
        p = new_process_info(name, pid, exit_code);
        STAILQ_INSERT_TAIL(loaded_head, p, next);
        free(name);
    }
    close(fd);
    empty_queue(head);
    STAILQ_CONCAT(head, loaded_head);
    free(loaded_head);

    // Emptying memdump.bin
    fd = open("memdump.bin", O_TRUNC | O_WRONLY, 0);
    if(fd == -1)
    {
        fprintf(stderr, "Failed to open memdump.bin: ");
        perror(NULL);
        return;
    }
    close(fd);

    return;

read_error:
    close(fd);
    free_queue(head);
    free_queue(loaded_head);
    free(name);
    fprintf(stderr, "Error occurred when reading memdump.bin: ");
    perror(NULL);
    exit(1);
}

/* -----------------------------------------------------------------------------
 * Saves the content of the queue in a binary memdump.bin file
 *
 * PARAMETERS
 * /
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void memdump()
{
    NULL_HEAD;

    int fd;
    
    fd = open("memdump.bin", O_APPEND | O_CREAT | O_WRONLY, 0666);
    if(fd == -1)
    {
        fprintf(stderr, "Failed to open memdump.bin: ");
        perror(NULL);
        return;
    }

    process_info* p;
    while (!STAILQ_EMPTY(head))
    {
        int ret;
        size_t name_size;

        // Retrieving first element and its fields
        p = STAILQ_FIRST(head);
        STAILQ_REMOVE_HEAD(head, next);
        name_size = strlen(p->name) + 1;

        // Writing in the binary file and freeing
        ret = write(fd, &name_size, sizeof(size_t));
        if(ret != (int)sizeof(size_t))
            goto write_error;
        
        ret = write(fd, p->name, name_size);
        if(ret != (int)(name_size * sizeof(char)))
            goto write_error;

        ret = write(fd, &p->pid, sizeof(pid_t));
        if(ret != (int)sizeof(pid_t))
            goto write_error;

        ret = write(fd, &p->exit_code, sizeof(int));
        if(ret != (int)sizeof(int))
            goto write_error;
            
        free_process_info(p);
    }
    close(fd);
    return;

write_error:
    close(fd);
    free_process_info(p);
    free_queue(head);
    fprintf(stderr, "Error occurred when writing in memdump.bin: ");
    perror(NULL);
    exit(1);
}

/* -----------------------------------------------------------------------------
 * The the the specified program a certain number of times in parallel
 *
 * PARAMETERS
 * copies   number of process_info to be created (int)
 * argv     argument vector for the program, argv[0] MUST be the name of the
 *          program
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void paral(int copies, char * const * const argv)
{
    NULL_HEAD;

    // Creating children
    for(size_t i = 0; i < (size_t)copies; ++i)
    {
        pid_t pid;

        pid = fork();
        if(pid < 0)
        {
            fprintf(stderr, "fork fail: ");
            perror(NULL);
            exit(1);
        }
        if(pid == 0)
        {
            execvp(argv[0], argv);
            fprintf(stderr, "execvp fail: ");
            perror(NULL);
            exit(1);
        }
    }

    // Waiting for them
    for(size_t i = 0; i < (size_t)copies; ++i)
    {
        int status;
        pid_t pid;
        process_info* p;
        signal(SIGCHLD, sigchild_handler);
        pid = wait(&status);
        p = new_process_info(argv[0], pid, WEXITSTATUS(status));
        STAILQ_INSERT_TAIL(head, p, next);
    }
    return;
}

/* -----------------------------------------------------------------------------
 * The the the specified program a certain number of times sequentially
 *
 * PARAMETERS
 * copies   number of process_info to be created (int)
 * argv     argument vector for the program, argv[0] MUST be the name of the
 *          program
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void sequent(int copies, char * const * const argv)
{
    NULL_HEAD;
    
    for(size_t i = 0; i < (size_t)copies; ++i)
    {
        int status;
        pid_t pid;
        process_info* p;
        
        // Creating child
        pid = fork();
        if(pid < 0)
        {
            fprintf(stderr, "fork fail: ");
            perror(NULL);
            exit(1);
        }
        if(pid == 0)
        {
            execvp(argv[0], argv);
            fprintf(stderr, "execvp fail: ");
            perror(NULL);
            exit(1);
        }

        // Waiting for him (her? them? it?)
        signal(SIGALRM, sigalarm_handler);
        signal(SIGCHLD, sigchild_handler);
        alarm(5);
        pause();
        if(signal_flag == 1)
        {
        	kill(pid, SIGKILL);
        }
        else
        {
        	waitpid(pid, &status, WNOHANG);
	        p = new_process_info(argv[0], pid, WEXITSTATUS(status));
	        STAILQ_INSERT_TAIL(head, p, next);	
        } 
    }
    return;
}


/* -----------------------------------------------------------------------------
 * Shows all elements in the queue
 *
 * PARAMETERS
 * /
 * 
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void showlist()
{
    NULL_HEAD;

    process_info* p,* first_p;

    first_p = STAILQ_FIRST(head);
    if(first_p == NULL)
        return;
    
    STAILQ_FOREACH(p, head, next) 
    {
        if(p != first_p)
            printf("->");  
        printf("(%s;%d;%d)", p->name, p->pid, p->exit_code);
    }
    printf("\n");
    return;
}

/* -----------------------------------------------------------------------------
 * Simple SIGALRM handler that sets signal flag to 1
 *
 * PARAMETERS
 * mysignal SIGALRM
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void sigalarm_handler(int mysignal)
{
    signal(mysignal, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal_flag = 1;
}

/* -----------------------------------------------------------------------------
 * Simple SIGCHLD handler that sets signal flag to 0
 *
 * PARAMETERS
 * mysignal SIGCHLD
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void sigchild_handler(int mysignal)
{
	signal(mysignal, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
    signal_flag = 0;
}


/* -----------------------------------------------------------------------------
 * sys netstats handler
 *
 * PARAMETERS
 * /
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void netstats(void)
{
    FILE* fp = fopen("/proc/net/dev", "r");
    char name[50], pkts[50], err[50], drop[50];

    if(fp == NULL)
    {
        fprintf(stderr, "Failed to open dev file: ");
        perror(NULL);
        return;
    }

    // Skipping the first two lines
    fscanf(fp, "%*[^\n\r]%*[\t\n\r ]");
    fscanf(fp, "%*[^\n\r]%*[\t\n\r ]");

    while(!(feof(fp)))
    {
        fscanf(fp, " %s %s %s %s %s", name, pkts, pkts, err, drop);
        printf("[%s]: Rx (pkts: %s, err: %s%%, drop: %s%%) - ", strtok(name, ":"), pkts, err, drop);

        // Skipping irrelevant information
        for (int i = 0; i < 5; i++)
            fscanf(fp, " %*s");

        fscanf(fp, " %s %s %s %s", pkts, pkts, err, drop);
        printf("Tx (pkts: %s, err: %s%%, drop: %s%%)\n", pkts, err, drop);
        fscanf(fp, "%*[^\n\r]%*[\t\n\r ]");
    }
    return;
}

/* -----------------------------------------------------------------------------
 * sys devstats handler
 *
 * PARAMETERS
 * /
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void devstats(void)
{
    int active = 0, suspended = 0, unsupported =0;
    DIR *d;
    struct dirent *dir;
    d = opendir("/sys/devices");
    if(d == NULL)
    {
        fprintf(stderr, "Failed to open directory: ");
        perror(NULL);
        return;
    }

    // Looking for status file in each folder
    while ((dir = readdir(d)) != NULL)
        if(dir->d_type == DT_DIR && strcmp(dir->d_name, "..") && strcmp(dir->d_name, "."))
            findStatus("/sys/devices", dir->d_name, &active, &suspended, &unsupported);
    
    closedir(d);
    printf("Active: %d \nSuspended: %d \nUnsupported: %d \n", active, suspended, unsupported);
    return;
}

/* -----------------------------------------------------------------------------
 * sys devstats handler
 *
 * PARAMETERS
 * path         the path to the directory new
 * new          a new directory to be explored
 * active       a valid pointer to the current number of found active devices
 * suspended    a valid pointer to the current number of found suspended devices
 * unsupported  a valid pointer to the current number of found unsupported devices
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void findStatus(char* path, char* new, int* active, int* suspended, int* unsupported){
    char newPath[500];
    newPath[0] = '\0';
    strcpy(newPath, path);
    strcat(newPath, "/");
    strcat(newPath, new);

    // Looking for runtime_status file if we are in power
    if(!(strcmp(new, "power"))){
        char statusPath[200];
        statusPath[0] = '\0';
        strcat(statusPath, newPath);
        strcat(statusPath, "/runtime_status");

        // If the file exists, we check its content
        if(!(access(statusPath, F_OK | R_OK)))
        {
            FILE* fp = fopen(statusPath, "r");
            if(fp == NULL)
            {
                fprintf(stderr, "Failed to open dev: %s ", statusPath);
                perror(NULL);
                return;
            }
            char state[30];
            fscanf(fp, " %s", state);
            if(!(strcmp(state, "active")))
                *active += 1;
            else if(!(strcmp(state, "suspended")))
                *suspended += 1;
            else if(!(strcmp(state, "unsupported")))
                *unsupported += 1;
            else
                printf("wtf");
            fclose(fp);
            return;
        }
    }

    // If the runtime_status file was not found or we were not in the power folder, we check the remaining folders
    DIR *d;
    struct dirent *dir;
    d = opendir(newPath);
    if(d == NULL)
    {
        fprintf(stderr, "Failed to open directory /sys/devices %s", newPath);
        perror(NULL);
        return;
    }
    while ((dir = readdir(d)) != NULL)
        if(dir->d_type == DT_DIR && strcmp(dir->d_name, "..") && strcmp(dir->d_name, "."))
            findStatus(newPath, dir->d_name, active, suspended, unsupported);

    closedir(d);
}    



/* -----------------------------------------------------------------------------
 * Sys stats command implementation
 *
 * PARAMETERS
 * pid      process id
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void sysstats(char const * const pid)
{
    char* path, *buf;
    FILE* f;

    /* Building base path */
    path = (char*)malloc((14 + strlen(pid)) * sizeof(char));
    strcpy(path, "/proc/");
    strcat(path, pid);
    strcat(path, "/status");

    /* Opening file */
    f = fopen(path, "r");
    if(f == NULL)
    {
        fprintf(stderr, "Invalid pid: %s\n", pid);
        free(path);
        return;
    }


    /* Reading file */
    // Should be large enough for what needs to be read
    // (e.g. maximimum process name size is 16 bytes)
    buf = (char*) malloc(50 * sizeof(char));
    do
    {
        fscanf(f, "%s", buf);
    } while(strcmp(buf, "Name:") != 0);

    fscanf(f, "%s", buf);
    printf("Process name: %s\n", buf);

    do
    {
        fscanf(f, "%s", buf);
    } while(strcmp(buf, "State:") != 0);

    fscanf(f, "%s", buf);
    printf("Process state: %s ", buf);
    fscanf(f, "%s", buf);
    printf("%s\n", buf);

    do
    {
        fscanf(f, "%s", buf);
    } while(strcmp(buf, "Threads:") != 0);

    fscanf(f, "%s", buf);
    printf("Process threads: %s\n", buf);
    fclose(f);
    free(buf);
    free(path);
    return;
}
