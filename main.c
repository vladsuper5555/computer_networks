#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./webcontent.h"
#include <stdio.h>
#include <sys/stat.h>


int main(int argc, char *argv[]) {
    char result[10000];
    // check the args first time
    returnFilesContent(argv[1], result);

    const char *root = "root";
    printf("%s\n", result);
    return 0;
}
