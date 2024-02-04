#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char *argv[]) {

    bool batch_mode = false;
    char *command_line = NULL;
    size_t nsize = 100;

    if (argc > 1) batch_mode = true;

    while (!batch_mode) {
        printf("wish> ");

        if (getline(&command_line, &nsize, stdin) == -1) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
        }

        printf("%s", command_line);
        free(command_line);
    }

    return 0;
}