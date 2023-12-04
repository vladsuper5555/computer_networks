#include "webcontent.h"

void extractProtocol(char *url, char *protocol)
{
    char *protoEnd = strstr(url, "://");
    if (protoEnd == NULL)
    {
        strcpy(protocol, "http"); // Default to HTTP if no protocol specified
    }
    else
    {
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
    char *port = strchr(start, ':');
    if (port != NULL && (end == NULL || port < end)) {
        end = port;
    }

    if (end == NULL) {
        strcpy(domain, start);
    } else {
        strncpy(domain, start, end - start);
        domain[end - start] = '\0';
    }
}


void extractPath(char *url, char *path)
{
    char *start = strstr(url, "://");
    if (start == NULL)
    {
        start = url;
    }
    else
    {
        start += 3;
    }

    char *pathStart = strchr(start, '/');
    if (pathStart == NULL)
    {
        strcpy(path, "/");
    }
    else
    {
        strcpy(path, pathStart);
    }
}

void extractPort(char *url, char *protocol, char *port)
{
    char *start = strstr(url, "://");
    if (start == NULL)
    {
        start = url;
    }
    else
    {
        start += 3;
    }

    char *portStart = strchr(start, ':');
    if (portStart != NULL)
    {
        portStart++;
        char *portEnd = strchr(portStart, '/');
        if (portEnd == NULL)
        {
            strcpy(port, portStart);
        }
        else
        {
            strncpy(port, portStart, portEnd - portStart);
            port[portEnd - portStart] = '\0';
        }
    }
    else
    {
        // Default port for HTTP and HTTPS
        strcpy(port, strcmp(protocol, "https") == 0 ? "443" : "80");
    }
}

void returnFilesContent(char *url, char *result)
{
    struct addrinfo hints, *res;
    int sockfd;
    char buf[10056];
    char protocol[10];
    char domain[2056];
    char path[2056];
    char port[6];

    extractProtocol(url, protocol);
    extractDomain(url, domain);
    extractPath(url, path);
    extractPort(url, protocol, port);
    // const char *port = strcmp(protocol, "https") == 0 ? "443" : "80";

    printf("the protocol is %s \n", protocol);
    printf("the domain is %s \n", domain);
    printf("the path is %s \n", path);
    printf("the port is %s \n", port);

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

    // Send the request
    char header[5128];
    if (strcmp(port, "80") != 0 && strcmp(port, "443") != 0)
        sprintf(header, "GET %s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", path, domain, port); // if it is a special port
    else
        sprintf(header, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain); // here not
    // sprintf(header, "GET ls /"); // here not
    send(sockfd, header, strlen(header), 0);

    // Initialize a flag to indicate whether headers are still being read
    int headers_finished = 0;
    while (1)
    {
        int byte_count = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (byte_count <= 0)
        {
            break;
        }

        buf[byte_count] = '\0'; // Null terminate the buffer

        // Check if we are still reading headers
        if (!headers_finished)
        {
            char *header_end = strstr(buf, "\r\n\r\n");
            if (header_end)
            {
                // Found the end of headers
                headers_finished = 1;

                // Calculate the length of the headers
                int header_length = header_end - buf + 4;

                // Append headers to result
                strncat(result, buf, header_length);

                // Check if there's more data after headers
                if (byte_count > header_length)
                {
                    strncat(result, buf + header_length, byte_count - header_length);
                }
            }
            else
            {
                // Headers not finished, append all read data
                strcat(result, buf);
            }
        }
        else
        {
            // Headers are done, append body
            strcat(result, buf);
        }
    }

    close(sockfd);
    freeaddrinfo(res);
}
