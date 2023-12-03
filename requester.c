#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void extractDomain(char *url, char *domain) {
    char *start = strstr(url, "://");
    if (start == NULL) {
        start = url;
    } else {
        start += 3;
    }

    char *end = strchr(start, '/');
    if (end == NULL) {
        strcpy(domain, start);
    } else {
        strncpy(domain, start, end - start);
        domain[end - start] = '\0';
    }
}

void extractPath(char *url, char *path) {
    char *start = strstr(url, "://");
    if (start == NULL) {
        start = url;
    } else {
        start += 3;
    }

    char *pathStart = strchr(start, '/');
    if (pathStart == NULL) {
        strcpy(path, "/");
    } else {
        strcpy(path, pathStart);
    }
}

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res;
    int sockfd;

    char buf[2056];
    char url[2056];
    strcpy(url, argv[0]); // Using argv[1] as argv[0] is the program name
    int byte_count;

    char domain[2056];
    char path[2056];
    extractDomain(url, domain);
    extractPath(url, path);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(domain, "80", &hints, &res);
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    printf("Connecting...\n");
    connect(sockfd, res->ai_addr, res->ai_addrlen);
    printf("Connected!\n");

    char header[2056];
    sprintf(header, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
    send(sockfd, header, strlen(header), 0);
    printf("GET Sent...\n");

    byte_count = recv(sockfd, buf, sizeof buf, 0);
    printf("recv()'d %d bytes of data in buf\n", byte_count);
    printf("%s", buf);

    // Close the socket
    close(sockfd);
    freeaddrinfo(res);

    return 0;
}
