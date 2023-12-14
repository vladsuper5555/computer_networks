#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fileFetcher.h"

#define PORT 8080
#define BUFFER_SIZE 100 * 1024


// int doesURlPointToFolder(const char *url) {
//     int length = strlen(url);

//     // URL ending with / is likely a directory
//     if (url[length - 1] == '/') {
//         return 1;
//     }

//     // Check for a period in the last part of the URL
//     for (int i = length - 1; i >= 0; i--) {
//         if (url[i] == '/') {
//             break;
//         }
//         if (url[i] == '.') {
//             return 0;
//         }
//     }

//     // If no period found, it's likely a directory
//     return 1;
// }

int doesURlPointToFolder(const char *url) {
    // Check if URL ends with '/'
    int len = strlen(url);
    if (url[len - 1] == '/') {
        return 1;
    }

    // Check for presence of a file extension
    // If the last segment after the last '/' contains a '.', it's likely a file
    const char *lastSlash = strrchr(url + 7, '/'); // skipping http://
    if (lastSlash != NULL) {
        if (strchr(lastSlash, '.') != NULL) {
            return 0;
        }
    } else {
        // If no slashes after 'http://', it's likely a folder
        if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
            return 1;
        }
    }

    return 0;
}

void parseURL(const char *url, char *host, char *path, int *port) {
    *port = 80; // Default HTTP port
    // Assuming the URL format is http://[host]:[port]/[path]
    sscanf(url, "http://%99[^:/]:%i/%199[^\n]", host, port, path);
    if (strlen(path) == 0) {
        strcpy(path, "");
    }
}

void fileFetcher(struct enod* ENOD) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    char host[100] = {0};
    char path[200] = {0};
    int port;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        // return -1;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        // return -1;
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        // return -1;
        return;
    }
    // localhost:8080 is really our server we want to fetch from    
    // only if the host in the get request is the localhost:8080
    // then we treat it separately

    parseURL(ENOD->url, host, path, &port);
    // printf("domain : %s path : %s port : %d \n", host, path, port);
    while (1) {
        // if the port is 80 dont put it int he url
        // if (port != 80)
            sprintf(buffer, "GET /%s HTTP/1.1\r\nHost: %s:%d\r\n\r\n", path, host, port);
        // else
            // sprintf(buffer, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, host);
        // fgets(buffer, BUFFER_SIZE, stdin);
        printf("the buffer is %s\n", buffer);
        // Sending a message to the server
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer

        // Receiving a response from the server
        if (read(sock, buffer, BUFFER_SIZE) == 0) {
            // printf("Server closed the connection\n");
            break;
        }
        // printf("Server response: %s\n", buffer);
        if (strlen(buffer) == 0)
            break;
        strcpy(ENOD->fileContent, buffer);
        // need to change the way we check if it is ok not a folder
        // probably y adding something in the response
        if (strstr(buffer, ". - 0") != NULL) //  means it is a foldere;
            ENOD->isFile = 0;
        else
            ENOD->isFile = 1;
        // if it is a file but does not end with a file and is not localhost:8080 then add index.html at
        // the end of the string
        if (strstr(ENOD->url, "localhost:8080") == NULL && doesURlPointToFolder(ENOD->url) == 1)
        {
            int len = strlen(ENOD->url);
            if (ENOD->url[len - 1] == '/')
                strcat(ENOD->url, "index.html");
            else
                strcat(ENOD->url, "/index.html");
        }

    }

    close(sock);

    return;
}
