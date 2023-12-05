#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fileFetcher.h"

#define PORT 8080
#define BUFFER_SIZE 10 * 1024

void parseURL(const char *url, char *host, char *path, int *port) {
    *port = 80; // Default HTTP port
    // Assuming the URL format is http://[host]:[port]/[path]
    sscanf(url, "http://%99[^:/]:%i/%199[^\n]", host, port, path);
    if (strlen(path) == 0) {
        strcpy(path, "/");
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
    printf("domain : %s path : %s port : %d \n", host, path, port);
    while (1) {
        sprintf(buffer, "GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n", path, host, port);
        // fgets(buffer, BUFFER_SIZE, stdin);
        printf("the buffer is %s\n", buffer);
        // Sending a message to the server
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer

        // Receiving a response from the server
        if (read(sock, buffer, BUFFER_SIZE) == 0) {
            printf("Server closed the connection\n");
            break;
        }
        printf("Server response: %s\n", buffer);
    }

    // Close the socket
    close(sock);

    // return 0;
    return;
}

// fere we have to make a routine fetch a single file but to fetch it correctly 
// and we need to return a structure with
// isFile // we may have folders that we fetch from our server
// name, fileExtension and relative path of the file 
// fileContent