#include "executeCommand.h"

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
        // we are on the child so lets run the command
        execvp(argv0, queryArray); // this should now run our command
    }
    else
        return;
}
