#include "webcontent.h"

void extractProtocol(char *url, char *protocol)
{
    char *protoEnd = strstr(url, "://");
    if (protoEnd == NULL)
        strcpy(protocol, "http"); 
    else
    {
        strncpy(protocol, url, protoEnd - url);
        protocol[protoEnd - url] = '\0';
    }
}

void extractDomain(char *url, char *domain)
{
    char *start = strstr(url, "://");
    if (start == NULL)
        start = url;
    else
        start += 3;

    char *end = strchr(start, '/');
    char *port = strchr(start, ':');
    if (port != NULL && (end == NULL || port < end))
        end = port;

    if (end == NULL)
        strcpy(domain, start);
    else
    {
        strncpy(domain, start, end - start);
        domain[end - start] = '\0';
    }
}

void extractPath(char *url, char *path)
{
    char *start = strstr(url, "://");
    if (start == NULL)
        start = url;
    else
        start += 3;

    char *pathStart = strchr(start, '/');
    if (pathStart == NULL)
        strcpy(path, "/");
    else
        strcpy(path, pathStart);
}

void extractPort(char *url, char *protocol, char *port)
{
    char *start = strstr(url, "://");
    if (start == NULL)
        start = url;
    else
        start += 3;

    char *portStart = strchr(start, ':');
    if (portStart != NULL)
    {
        portStart++;
        char *portEnd = strchr(portStart, '/');
        if (portEnd == NULL)
            strcpy(port, portStart);
        else
        {
            strncpy(port, portStart, portEnd - portStart);
            port[portEnd - portStart] = '\0';
        }
    }
    else
        strcpy(port, strcmp(protocol, "https") == 0 ? "443" : "80");
}

int isspace(int ch)
{
    return (ch == ' ') || (ch == '\f') || (ch == '\n') ||
           (ch == '\r') || (ch == '\t') || (ch == '\v');
}

void stripTrailingWhiteSpace(char *str)
{
    if (str == NULL)
        return;

    int len = strlen(str);

    // Start from the end of the string and move backwards
    while (len > 0 && isspace((unsigned char)str[len - 1]))
        len--;

    // Null-terminate the string at the new end
    str[len] = '\0';
}

void returnFilesContent(char *url, char *result, char *header_response)
{
    struct addrinfo hints, *res;
    int sockfd;
    char buf[100056];
    char protocol[10];
    char domain[2056];
    char path[2056];
    char port[6];

    extractProtocol(url, protocol);
    extractDomain(url, domain);
    extractPath(url, path);
    extractPort(url, protocol, port);
    stripTrailingWhiteSpace(domain);
    stripTrailingWhiteSpace(path);
    // const char *port = strcmp(protocol, "https") == 0 ? "443" : "80";

    // printf("the protocol is %s \n", protocol);
    // printf("the domain is %s \n", domain);
    // printf("the path is %s \n", path);
    // printf("the port is %s \n", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(domain, port, &hints, &res) != 0)
    {
        perror("getaddrinfo failed");
        return;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        freeaddrinfo(res);
        return;
    }

    char header[5128];
    sprintf(header, "GET %s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", path, domain, port);
    printf("the request that we made is %s", header);
    send(sockfd, header, strlen(header), 0);

    int headers_finished = 0;
    while (1)
    {
        int byte_count = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (byte_count <= 0)
            break;

        buf[byte_count] = '\0';
        // printf("the bytes we read in total : %d\n", byte_count);

        if (!headers_finished)
        {
            char *header_end = strstr(buf, "\r\n\r\n");
            if (header_end)
            {
                headers_finished = 1;
                int header_length = header_end - buf + 4;
                strncat(header_response, buf, header_length);
                if (byte_count > header_length)
                    strncat(result, buf + header_length, byte_count - header_length);
            }
            else
                strcat(header_response, buf);
        }
        else
            strcat(result, buf);
    }

    close(sockfd);
    freeaddrinfo(res);
}
