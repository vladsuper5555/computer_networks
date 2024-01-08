#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fileFetcher.h"

#define PORT 8080
#define BUFFER_SIZE 1000 * 1024

int doesURlPointToFolder(const char *url)
{
    // printf("the user are tring is %s", url);
    int len = strlen(url);
    if (url[len - 1] == '/')
        return 1;

    const char *lastSlash = strrchr(url + 7, '/'); // skipping http://
    if (lastSlash != NULL)
    {
        if (strchr(lastSlash, '.') != NULL)
            return 0;
        return 1;
    }
    else
        return 1;
}

void fileFetcher(struct enod *ENOD)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char header[BUFFER_SIZE / 1000] = {0};
    strcpy(ENOD->fileContent, "");

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        // return -1;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        // return -1;
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        // return -1;
        return;
    }

    sprintf(buffer, "GET %s HTTP/1.1\r\n\r\n", ENOD->url);
    send(sock, buffer, strlen(buffer), 0);
    int isHeaderRead = 0;
    int firstTimeWritingInBuffer = 1;
    int buffer_size = 0;
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
        if (isHeaderRead == 0)
        {
            int header_size;
            read(sock, &header_size, sizeof(int));
            if (read(sock, ENOD->header, header_size) == 0)
                break;
            isHeaderRead = 1;
        }
        if (buffer_size == 0)
            read(sock, &buffer_size, sizeof(int));
        // printf("the buffer size is %d\n", buffer_size);
        if (read(sock, buffer, buffer_size) == 0)
            break;
        // printf("the size of the fetched content is %d\n", strlen(buffer));
        if (firstTimeWritingInBuffer == 1)
        {
            strcpy(ENOD->fileContent, buffer);
            firstTimeWritingInBuffer = 0;
        }
        else strcat(ENOD->fileContent, buffer);
    }
    // change the chekc to the host check
    if (strstr(ENOD->url, "localhost:8080") == NULL && doesURlPointToFolder(ENOD->url) == 1)
    {
        if (ENOD->url[strlen(ENOD->url) - 1] == '/')
            strcat(ENOD->url, "index.html");
        else
            strcat(ENOD->url, "/index.html");
    }
    if (strstr(ENOD->url, "localhost:8080") != NULL && strstr(ENOD->header, "202") != NULL)
        ENOD->isFile = 0;
    else
        ENOD->isFile = 1;

    close(sock);

    return;
}
