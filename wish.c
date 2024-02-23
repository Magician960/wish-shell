#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

#define MAX_PATHS 32
#define MAX_LENGTH 100

int main (int argc, char *argv[]) {
    // general variables
    bool batch_mode = false;
    bool out_redirect = false;
    bool in_built_cmd = false;
    char exec_path[MAX_LENGTH];
    char cwd[MAX_LENGTH];
    size_t nsize = MAX_LENGTH;
    char *newcwd;


    // variables used in creating execv args
    bool first_arg = true;
    char *command_line = NULL;
    char *arg;
    int counter = 0;
    char *arglist[MAX_LENGTH];
    char *newout;

    // variables to track exec paths & initialise exec paths
    char *pathnames[MAX_PATHS];
    pathnames[0] = malloc(sizeof(char) * (strlen("/bin") + 1));
    pathnames[0] = strcpy(pathnames[0], "/bin");
    int num_paths = 1;
    int num_paths_max = 1;
    bool valid_path = false;
    //for (int i = 0; i < num_paths; i++) printf("%s\t", pathnames[i]);

    if (argc > 1) batch_mode = true;

    while (!batch_mode) {
        // Reset variables
        exec_path[0] = '\0';
        counter = 0;
        first_arg = true;
        valid_path = false;
        out_redirect = false;
        in_built_cmd = false;
        getcwd(cwd, MAX_LENGTH);
    
        printf("wish ~%s> ", cwd);

        // Try to read command
        if (getline(&command_line, &nsize, stdin) == -1) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
        } else {
            command_line[strcspn(command_line, "\n")] = '\0';
        }
        
        // Separate command into args
        while ((arg = strsep(&command_line, " ")) != NULL) {

            // Detect stdout redirection
            if (!strcmp(arg, ">")) {
                out_redirect = true;
                break;
            }

            if (first_arg) {
                // In-built Exit command

                //printf("strcmp = %d\n",strcmp(arg, "exit"));
                //char *second_arg = strsep(&command_line, " ");
                //printf("second_arg = %s\n", second_arg);

                if (!strcmp(arg, "exit") && (strsep(&command_line, " ") == NULL)) {
                    if (command_line) free(command_line);
                    // Free exec path strings
                    for (int i = 0; i < num_paths_max; i++) {
                        free(pathnames[i]);
                    }
                    exit(0);
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
                    strcpy(exec_path, pathnames[i]);
                    strcat(exec_path, "/");
                    strcat(exec_path, arg);

                    if (access(exec_path, F_OK) == 0) {
                        valid_path = true;
                        break;
                    } 
                }
            }

            arglist[counter++] = strdup(arg);
            
        }
        arglist[counter] = NULL;

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

        // Proceed to execute command
        int rc = fork();

        if (rc < 0) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        } else if (rc == 0) {

            // Handle out redirection
            if (out_redirect) {
                
                // Check for invalid redirection syntax
                if ((newout = strsep(&command_line, " ")) == NULL ||
                (arg = strsep(&command_line, " ")) != NULL) exit(1);

                freopen(newout, "w", stdout);
            }

            execv(exec_path, arglist);
        } else {
            wait(NULL);
        }

    }

    return 0;
}