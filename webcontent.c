#include "webcontent.h"

void extractProtocol(char *url, char *protocol) {
    char *protoEnd = strstr(url, "://");
    if (protoEnd == NULL) {
        strcpy(protocol, "http"); // Default to HTTP if no protocol specified
    } else {
        strncpy(protocol, url, protoEnd - url);
        protocol[protoEnd - url] = '\0';
    }
}

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


void returnFilesContent(char *url, char *result) {
    struct addrinfo hints, *res;
    int sockfd;
    char buf[10056];
    char protocol[10];
    char domain[2056];
    char path[2056];

    extractProtocol(url, protocol);
    extractDomain(url, domain);
    extractPath(url, path);
    const char *port = strcmp(protocol, "https") == 0 ? "443" : "80";
    
    printf("the protocol is %s \n", protocol);
    printf("the domain is %s \n", domain);
    printf("the path is %s \n", path);
    printf("the port is %s \n", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    
    if (getaddrinfo(domain, port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        return;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connection failed");
        close(sockfd);
        freeaddrinfo(res);
        return;
    }

    char header[5128];
    sprintf(header, "GET %s \r\nHost: %s\r\n\r\n", path, domain);
    send(sockfd, header, strlen(header), 0);

    while (1) {
        int byte_count = recv(sockfd, buf, 128, 0);
        printf("bitcount is : %d\n", byte_count);
        if (byte_count <= 0) {
            break;
        }
        strcat(result, buf);
    }

    close(sockfd);
    freeaddrinfo(res);
}
