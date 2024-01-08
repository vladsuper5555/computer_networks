#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "webcontent.h"

void decomposeHTTPRequest(const char *request, char *url, char *domain, int *port, char *path)
{
    *port = 80;

    char method[10], httpVersion[10];
    sscanf(request, "%s %s %s", method, url, httpVersion);

    char *urlCopy = strdup(url);
    char *domainStart = strstr(urlCopy, "://");
    if (domainStart)
    {
        domainStart += 3;
    }
    else
    {
        domainStart = urlCopy; // No http://
    }

    char *pathStart = strchr(domainStart, '/');
    if (pathStart)
    {
        strcpy(path, pathStart);
        *pathStart = '\0';
    }
    else
    {
        strcpy(path, "/");
    }

    char *portStart = strchr(domainStart, ':');
    if (portStart)
    {
        *portStart = '\0';
        *port = atoi(portStart + 1); // conv to int
    }

    strcpy(domain, domainStart);
    free(urlCopy);
}

void translate_path_into_localpath(char *path, char *local_path)
{
    sprintf(local_path, "./resources%s", path); // just by adding the . it becaomes a local path
}

int is_local_path_existing(const char *path)
{
    struct stat buffer;
    return (stat(path, &buffer) == 0) ? 1 : 0;
}

int is_local_path_a_file(const char *path)
{
    struct stat path_stat;
    if (stat(path, &path_stat) != 0)
    {
        return 0;
    }

    return S_ISREG(path_stat.st_mode); // check if is a noraml file or not
}

void readFileInBuffer(char *local_path, char *buffer)
{
    FILE *file = fopen(local_path, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    char ch;
    int index = 0;
    while ((ch = fgetc(file)) != EOF)
    {
        buffer[index++] = ch;
    }
    buffer[index] = '\0';

    fclose(file);
}

void list_directory_contents(const char *directory, char *directory_listing)
{
    DIR *dir;
    struct dirent *entry;
    directory_listing[0] = '\0';

    if ((dir = opendir(directory)) == NULL)
    {
        perror("opendir");
        // return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }
        if (entry->d_type == DT_DIR)
        {
            // dir
            strcat(directory_listing, entry->d_name);
            strcat(directory_listing, " - 0\n");
        }
        else
        {
            // file
            strcat(directory_listing, entry->d_name);
            strcat(directory_listing, " - 1\n");
        }
    }

    closedir(dir);
    return;
}

int is_request_local(char *domain, int port)
{
    if (strstr(domain, "localhost") && port == 8080)
        return 1;
    return 0;
}

void *handle_client(void *clinet_fd_pointer)
{
    char *buffer = (char *)malloc(10000000 * sizeof(char));
    int client_fd = *((int *)clinet_fd_pointer);
    char request[256];
    char url[256];
    char domain[256];
    int port;
    char path[256];
    char header[256];
    strcpy(buffer, "");
    recv(client_fd, request, 256, 0);

    decomposeHTTPRequest(request, url, domain, &port, path);

    if (is_request_local(domain, port))
    {
        char local_path[256];
        translate_path_into_localpath(path, local_path);
        if (is_local_path_existing(local_path))
        {
            if (is_local_path_a_file(local_path))
            {
                strcpy(header, "HTTP/1.1 201 HTTP_LOCAL_FILE");
                // construct the header and the buffer for the request
                readFileInBuffer(local_path, buffer);
            }
            else
            {
                strcpy(header, "HTTP/1.1 202 HTTP_LOCAL_FOLDER");
                // the path is for a folder and we need to use our special type of request
                list_directory_contents(local_path, buffer);
            }
        }
        else
        {
            strcpy(header, "HTTP/1.1 404 Not Found");
            // here we return a not found thing
            strcpy(buffer, "HTTP/1.1 404 Not Found");
        }
    }
    else
    {
        // here is the only thing that remains
        printf("the url we are going to make the fetch from is %s", url);
        returnFilesContent(url, buffer, header);
    }
    int header_len = strlen(header);
    int buffer_len = strlen(buffer);
    send(client_fd, (&header_len), sizeof(int), 0);
    send(client_fd, header, strlen(header), 0);
    send(client_fd, (&buffer_len), sizeof(int), 0);
    send(client_fd, buffer, strlen(buffer), 0);
    close(client_fd);
    free(buffer);
    return NULL;
}

int main(int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", 8080);
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = (int *)malloc(sizeof(int));

        if ((*client_fd = accept(server_fd,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len)) < 0)
        {
            perror("accept failed");
            continue;
        }
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
