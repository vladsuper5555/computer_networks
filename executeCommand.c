#include "executeCommand.h"
#include <sys/wait.h>  // Include header for waitpid

void executeCommand(char *argv0, char *command) {
    char array[20][100];
    char *queryArray[20];

    char commandCopy[100001];
    strcpy(commandCopy, command);

    char *p = strtok(commandCopy, " \n");
    int count = 0;
    while (p != NULL)
    {
        strcpy(array[count], p);
        queryArray[count] = array[count];
        count++;
        p = strtok(NULL, " ");
    }
    queryArray[count] = NULL;
    
    int pid = fork();
    if (pid == 0) {
        // Child process
        execvp(argv0, queryArray); // Run the command
        exit(EXIT_FAILURE);  // If execvp returns, there was an error
    }
    else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to finish
    }
    else {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    }
}
    