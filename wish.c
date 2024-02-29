#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

#define MAX_PATHS 32
#define MAX_LENGTH 100
#define MAX_PROCESSES 10

// GLOBAL VARIABLES
char *arglist[MAX_PROCESSES][MAX_LENGTH];

void EXIT_COMMAND(char *command_line, char *pathnames[], int num_paths_max) {
    if (command_line) free(command_line);
    // Free exec path strings
    for (int i = 0; i < num_paths_max; i++) {
        free(pathnames[i]);
    }
    // Free arglist
    for (int i = 0; i < MAX_PROCESSES; i++) {
        for (int j = 0; j < MAX_LENGTH; j++) {
            if (arglist[i][j] != NULL) free(arglist[i][j]);
        }
    }
                    
    exit(0);
}

int main (int argc, char *argv[]) {
    // general variables
    bool batch_mode = false;
    bool out_redirect[MAX_PROCESSES];
    for (int i = 0; i < MAX_PROCESSES; i++) {
        out_redirect[i] = false;
    }
    bool in_built_cmd = false;
    char exec_path[MAX_PROCESSES][MAX_LENGTH];
    char cwd[MAX_LENGTH];
    size_t nsize = MAX_LENGTH;
    char *newcwd;

    // variables for parallel commands
    int process_num = 0;
    int max_process_num = 0;


    // variables used in creating execv args
    bool first_arg = true;
    char *command_line = NULL;
    char *arg;
    int counter = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        for (int j = 0; j < MAX_LENGTH; j++) {
            arglist[i][j] = NULL;
        }
    }
    char *newout[MAX_PROCESSES];

    // variables to track exec paths & initialise exec paths
    char *pathnames[MAX_PATHS];
    pathnames[0] = malloc(sizeof(char) * (strlen("/bin") + 1));
    pathnames[0] = strcpy(pathnames[0], "/bin");
    int num_paths = 1;
    int num_paths_max = 1;
    bool valid_path = false;
    //for (int i = 0; i < num_paths; i++) printf("%s\t", pathnames[i]);

    if (argc > 2) {
        printf("Usage: ./wish [optional: input file]\n");
        exit(1);
    } else if (argc == 2) {
        if (freopen(argv[1], "r", stdin) == NULL) {
            printf("Error reading input file\n");
            exit(1);
        }
        batch_mode = true;
    }

    while (true) {
        // Reset variables
        for (int i = 0; i <= max_process_num; i++) {
            exec_path[i][0] = '\0';
            out_redirect[i] = false;
        }
        counter = 0;
        process_num = 0;
        first_arg = true;
        valid_path = true;
        in_built_cmd = false;
        getcwd(cwd, MAX_LENGTH);
    
        if (!batch_mode) printf("wish ~%s> ", cwd);

        // Try to read command
        if (getline(&command_line, &nsize, stdin) == -1) {
            char error_message[30] = "EOF detected, exiting program\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        } else {
            command_line[strcspn(command_line, "\n")] = '\0';
        }
        
        // Separate command into args and parse
        while ((arg = strsep(&command_line, " ")) != NULL) {

            // Detect stdout redirection
            if (!strcmp(arg, ">")) {
                out_redirect[process_num] = true;
                // Check for invalid redirection syntax
                if ((newout[process_num] = strsep(&command_line, " ")) == NULL) {
                    exit(1);
                }
                continue;
            }

            // Detect parallel commands
            if (!strcmp(arg, "&")) {
                arglist[process_num][counter] = NULL;
                counter = 0;
                first_arg = true;
                process_num++;

                if (process_num > max_process_num) max_process_num = process_num;
                continue;
            }

            if (first_arg) {
                // In-built Exit command

                //printf("strcmp = %d\n",strcmp(arg, "exit"));
                //char *second_arg = strsep(&command_line, " ");
                //printf("second_arg = %s\n", second_arg);

                if (!strcmp(arg, "exit") && (strsep(&command_line, " ") == NULL)) {
                    EXIT_COMMAND(command_line, pathnames, num_paths_max);
                }

                // In-built cd command

                if  (!strcmp(arg, "cd") && 
                    (newcwd = strsep(&command_line, " ")) != NULL && 
                    (strsep(&command_line, " ") == NULL)) 
                {
                    assert(chdir(newcwd) == 0); 
                    in_built_cmd = true;
                    break;
                }

                // In-built path command
                if (!strcmp(arg, "path")) {
                    num_paths = 0;

                    while ((arg = strsep(&command_line, " ")) != NULL) {
                        if (num_paths >= num_paths_max) {
                            pathnames[num_paths] = malloc(sizeof(char) * (strlen(arg) + 1));
                        } else {
                            pathnames[num_paths] = realloc(pathnames[num_paths], sizeof(char) * (strlen(arg) + 1));
                        }
                        pathnames[num_paths] = strcpy(pathnames[num_paths], arg);
                        num_paths++;

                        if (num_paths > num_paths_max) num_paths_max = num_paths;
                    }
                
                    in_built_cmd = true;
                    break;
                }
                
                // Attempt to find executable filepath
                first_arg = false;
                for (int i = 0; i < num_paths; i++) {
                    strcpy(exec_path[process_num], pathnames[i]);
                    strcat(exec_path[process_num], "/");
                    strcat(exec_path[process_num], arg);

                    if (access(exec_path[process_num], F_OK) != 0) {
                        valid_path = false;
                        break;
                    } 
                }
            }

            arglist[process_num][counter++] = strdup(arg);
            
        }
        arglist[process_num][counter] = NULL;

        //  USED TO DEBUG execv args
        //printf("exec_path = %s\t", exec_path);
        //printf("Arguments are:\t");
        //int j = 0;
        //while (arglist[j] != NULL) {
        //    printf("%s ",arglist[j++]);
        //}
        //printf("\n");

        if (!valid_path) continue;
        if (in_built_cmd) continue;

        // Proceed to execute command(s)

        for (int i = 0; i <= process_num; i++) {
            if (fork() == 0) {
                // Handle out redirection
                if (out_redirect[i]) {
                    freopen(newout[i], "w", stdout);
                }

                execv(exec_path[i], arglist[i]);     
            }
        }

        while (wait(NULL) > 0);

        //int rc = fork();

        //if (rc < 0) {
        //    char error_message[30] = "An error has occurred\n";
        //    write(STDERR_FILENO, error_message, strlen(error_message));
        //    exit(1);
        //} else if (rc == 0) {

            // Handle out redirection
        //    if (out_redirect) {
                
                // Check for invalid redirection syntax
        //        if ((newout = strsep(&command_line, " ")) == NULL ||
        //        (arg = strsep(&command_line, " ")) != NULL) exit(1);

        //        freopen(newout, "w", stdout);
        //    }

        //    execv(exec_path[process_num], arglist[process_num]);
        //} else {
        //    wait(NULL);
        //}

    }

    return 0;
}