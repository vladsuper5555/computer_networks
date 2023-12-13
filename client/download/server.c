HTTP/1.1 200 OK
Content-Type: application/octet-stream

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../webcontent.h"

#define PORT 8080
#define BUFFER_SIZE 104857600

const char *get_file_extension(const char *file_name)
{
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name)
    {
        return "";
    }
    return dot + 1;
}

const char *get_mime_type(const char *file_ext)
{
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0)
        return "text/html";
    else if (strcasecmp(file_ext, "txt") == 0)
        return "text/plain";
    else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0)
        return "image/jpeg";
    else if (strcasecmp(file_ext, "png") == 0)
        return "image/png";
    else
        return "application/octet-stream";
}

bool case_insensitive_compare(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2))
        {
            return false;
        }
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

char *get_file_case_insensitive(const char *file_name)
{
    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        perror("opendir");
        return NULL;
    }

    struct dirent *entry;
    char *found_file_name = NULL;
    while ((entry = readdir(dir)) != NULL)
    {
        if (case_insensitive_compare(entry->d_name, file_name))
        {
            found_file_name = entry->d_name;
            break;
        }
    }

    closedir(dir);
    return found_file_name;
}

char *url_decode(const char *src)
{
    size_t src_len = strlen(src);
    char *decoded = (char *)malloc(src_len + 1);
    size_t decoded_len = 0;

    for (size_t i = 0; i < src_len; i++)
    {
        if (src[i] == '%' && i + 2 < src_len)
        {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        }
        else
            decoded[decoded_len++] = src[i];
    }

    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name,
                         const char *file_ext,
                         char *response,
                         size_t *response_len)
{
    // build HTTP header
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    // if file not exist, response is 404 Not Found
    printf("the name of the file we try and open is %s", file_name);
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        snprintf(response, BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    // get file size for Content-Length
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    // copy header to response buffer
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    // copy file to response buffer
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
                              response + *response_len,
                              BUFFER_SIZE - *response_len)) > 0)
    {
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

bool is_remote_request(const char *file_name)
{
    return (strncmp(file_name, "http://", 7) == 0) || (strncmp(file_name, "https://", 8) == 0);
}

bool is_directory_request(const char *file_name)
{
    struct stat statbuf;
    if (stat(file_name, &statbuf) != 0)
    {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

char *list_directory_contents(const char *directory)
{
    DIR *dir;
    struct dirent *entry;
    char *directory_listing = (char *)malloc(BUFFER_SIZE * sizeof(char));
    directory_listing[0] = '\0';

    if ((dir = opendir(directory)) == NULL)
    {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            // Directory
            strcat(directory_listing, entry->d_name);
            strcat(directory_listing, " - 0\n");
        }
        else
        {
            // File
            strcat(directory_listing, entry->d_name);
            strcat(directory_listing, " - 1\n");
        }
    }

    closedir(dir);
    return directory_listing;
}

void *handle_client(void *arg)
{
    printf("created a new thread for the executiong of this thing");
    int client_fd = *((int *)arg);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    // receive request data from client and store into buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    printf("the number of bytes is %d \n and the string is \n %s \n", bytes_received, buffer);
    if (bytes_received > 0)
    {
        // check if request is GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1.1", REG_EXTENDED);
        regmatch_t matches[2];

        if (regexec(&regex, buffer, 2, matches, 0) == 0 || true)
        {
            // extract filename from request and decode URL
            buffer[matches[1].rm_eo] = '\0';
            const char *url_encoded_file_name = buffer + matches[1].rm_so;
            char *file_name = url_decode(url_encoded_file_name);
            printf("the name of the file first read is %s", file_name);
            // Determine if the request is local or remote
            if (is_remote_request(file_name))
            {
                printf("we received a request for a remote file\n");
                char remote_content[100001];
                returnFilesContent(file_name, remote_content);

                // send remote_content as response
                send(client_fd, remote_content, strlen(remote_content), 0);
                // free(remote_content);
            }
            else if (is_directory_request(file_name))
            {
                printf("we received a request for a directory\n");
                char *directory_listing = list_directory_contents(file_name);

                // send directory_listing as response
                send(client_fd, directory_listing, strlen(directory_listing), 0);
                free(directory_listing);
            }
            else
            {
                printf("we received a request for a local file\n");
                // handle local file request
                char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
                size_t response_len;
                build_http_response(file_name, get_file_extension(file_name), response, &response_len);
                send(client_fd, response, response_len, 0);
                free(response);
            }
            printf("done responding\n");

            free(file_name);
        }
        regfree(&regex);
    }

    close(client_fd);
    free(arg);
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
    server_addr.sin_port = htons(PORT);

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

    printf("Server listening on port %d\n", PORT);
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

        // create a new thread to handle client request
        pthread_t thread_id;
        printf("the thread id is %d", thread_id);
        pthread_create(&thread_id, NULL, handle_client, (void *)client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}