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
    for (int i = 0; i < 10000; ++i)
        result[i] = '\0';
    // check the args first time
    returnFilesContent(argv[1], result);

    const char *root = "root";
    printf("the final lenfth is %d \n", strlen(result));
    printf("%s\n", result);
    return 0;
}
