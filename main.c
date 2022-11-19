#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>

#include "oshell.h"

/*
Important:
    1. DO NOT MODIFY the given skeleton. Nevertheless, if it is easier for you, 
you can (only) wrap existing code within conditions (if/else, switch).
    2. Since I'm the TA, I can use static allocation. But you are a student, so
you MUST use dynamic allocation for your data structures. It is an Operating 
Systems course after all...
*/

void sigint_handler(int mysignal);

int main() {
    
    signal(SIGINT, sigint_handler);

    char line[MAX_CMD_SIZE]; 
    char* arguments[MAX_ARGS];

    int copies;
    int parallel = false;
    
    init();
    
    do{
        printf("OShell> ");
        fgets(line, sizeof(line), stdin);
        
        if (line[0] == '\n')
            continue;
        
        parseCmdLine(line, arguments);
        
        // Add some stuff here ...

        /* Special cases */
        // cd case handling
        if(strcmp(arguments[0], "cd") == 0) 
        {
            // Building argument in order to avoid passing the whole array
            char* path;

            path = malloc((strlen(arguments[1]) + 1) * sizeof(char));
            if(path == NULL)
            {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            strcpy(path, arguments[1]);
            cd(path);
            free(path);
            continue;
        }
        else if(strcmp(arguments[0], "sys") == 0)
        {
            if(arguments[1])
            {
                if(strcmp(arguments[1], "netstats") == 0)
                    netstats();
                else if(strcmp(arguments[1], "devstats") == 0)
                    devstats();
                else if(strcmp(arguments[1], "stats") == 0)
                {
                    // Building argument in order to avoid passing the whole array
                    char* arg;

                    arg = malloc((strlen(arguments[2]) + 1) * sizeof(char));
                    if(arg == NULL)
                    {
                        fprintf(stderr, "Memory allocation failed\n");
                        exit(1);
                    }
                    strcpy(arg, arguments[2]);
                    sysstats(arg);
                    free(arg);
                }
                else
                    fprintf(stderr, "Invalid sys command\n");
            }
            continue;
        }

        else if(strcmp(arguments[0], "exit") == 0)
        {
            my_exit();
        }
        else if(strcmp(arguments[0], "loadmem") == 0)
        {
            loadmem();
            continue;
        }
        else if(strcmp(arguments[0], "memdump") == 0)
        {
            memdump();
            continue;
        }
        else if(strcmp(arguments[0], "showlist") == 0)
        {
            showlist();
            continue;
        }



        // Number of times to execute a specific command
        do {
            printf("\tCopies> ");
            copies = readCharInput() - '0';
        } while (copies <= 0 || copies > 9);
        
        // For multiple executions
        if (copies > 1){
            printf("\t[S]equential (default) or [P]arallelize> ");
            parallel = (toupper(readCharInput()) == 'P') ? true : false;
        }
        
        // Add another stuff here ...

        /* Building argument vector in order to avoid giving the whole
        arguments variable as argument to execvp */
        size_t argc = 0;
        char** argv;

        // Counting number of arguments
        while(arguments[argc] != NULL)
        {
            argc++;
        }
        
        // Building argument vector
        argv = (char**)malloc((argc + 1) * sizeof(char*));
        if(argv == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        argv[argc] = NULL;
        for(size_t i = 0; i < argc; ++i)
        {
            size_t str_size = strlen(arguments[i]) + 1;
            argv[i] = (char*)malloc(sizeof(char) * str_size);
            if(argv[i] == NULL)
            {
                // Useless freeing since we exit
                for(int j = (int)i - 1; j > -1; --j)
                    free(argv[j]);
                free(argv);
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            strcpy(argv[i], arguments[i]);
        }

        // Calling parallel or sequential execution
        if(parallel)
            paral(copies, argv);
        else
            sequent(copies, argv);
        
        // Freeing used stuff
        for(int i = (int)(argc) - 1; i > -1; --i)
            free(argv[i]);
        free(argv);
    } while(true);
}

/* -----------------------------------------------------------------------------
 * Simple SIGINT handler that exits the current process
 *
 * PARAMETERS
 * mysignal SIGINT
 *
 * RETURN
 * /
 * ---------------------------------------------------------------------------*/
void sigint_handler(int mysignal)
{
    signal(mysignal, SIG_IGN);
    exit(0);
}
