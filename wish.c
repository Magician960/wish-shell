#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_PATHS 32
#define MAX_LENGTH 100

int main (int argc, char *argv[]) {

    bool batch_mode = false;
    bool valid_path = false;
    char *command_line;
    char exec_path[MAX_LENGTH];
    size_t nsize = 100;

    char *pathnames[MAX_PATHS];
    pathnames[0] = "/usr/bin";
    int num_paths = 1;
    //for (int i = 0; i < num_paths; i++) printf("%s\t", pathnames[i]);

    if (argc > 1) batch_mode = true;

    while (!batch_mode) {
        valid_path = false;
        printf("wish> ");

        if (getline(&command_line, &nsize, stdin) == -1) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
        } else {
            command_line[strcspn(command_line, "\n")] = '\0';
        }

        for (int i = 0; i < num_paths; i++) {
            strcpy(exec_path, pathnames[i]);
            strcat(exec_path, "/");
            strcat(exec_path, command_line);

            if (access(exec_path, F_OK) == 0) {
                if ("file exists\n");
                valid_path = true;
                break;
            } 
        }

        //if (!valid_path) continue;

        int rc = fork();

        if (rc < 0) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        } else if (rc == 0) {
            execv(exec_path, NULL);
        } else {
            wait(NULL);
        }

    }

    free(command_line);
    return 0;
}