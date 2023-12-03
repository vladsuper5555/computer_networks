#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./webcontent.h"


int main(int argc, char *argv[]) {
    char result[100000];
    // check the args first time
    returnFilesContent(argv[1], result);

    printf("%s\n", result);
    return 0;
}
