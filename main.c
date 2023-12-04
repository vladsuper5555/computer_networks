#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "./webcontent.h"
#include "./executeCommand.h"
#include <stdio.h>
#include <sys/stat.h>

int startsWith(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}

int isLocalhost(const char* url) {
    // Define the localhost and loopback strings
    const char* localhost = "localhost";
    const char* loopback = "127.0.0.1";
    const char* localNetworkPrefix = "192.168.";

    // Check if the URL starts with http:// or https://
    if (startsWith(url, "http://")) {
        url += 7; // Skip the http:// part
    } else if (startsWith(url, "https://")) {
        url += 8; // Skip the https:// part
    }

    // Check if the next part of the URL is localhost, 127.0.0.1, or starts with 192.168.
    if (startsWith(url, localhost) || startsWith(url, loopback) || startsWith(url, localNetworkPrefix)) {
        return 1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    
    const char* testUrls[] = {
        "http://localhost",
        "https://localhost",
        "http://localhost:8080",
        "https://127.0.0.1",
        "http://192.168.1.1", // This is a common local network IP
        "http://192.168.0.105",
        "http://example.com"
    };

    for (int i = 0; i < sizeof(testUrls)/sizeof(testUrls[0]); i++) {
        if (isLocalhost(testUrls[i])) {
            printf("URL '%s' is for localhost.\n", testUrls[i]);
        } else {
            printf("URL '%s' is not for localhost.\n", testUrls[i]);
        }
    }

    return 0;
    // char result[10000];
    // for (int i = 0; i < 10000; ++i)
    //     result[i] = '\0';
    // // check the args first time
    // returnFilesContent(argv[1], result);

    // const char *root = "root";
    // executeCommand(argv[0], "ls -l");
    // printf("the final lenfth is %d \n", strlen(result));
    // printf("%s\n", result);
    return 0;
}
