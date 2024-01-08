#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <libgen.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "executeCommand.h"
#include "fileFetcher.h"
#include <sys/stat.h>
#include <unistd.h>
int globalENODId = 0;

void parseURL1(const char *url, char *host, char *path, int *port)
{
    *port = 80;
    // assuming the URL format is http://[host]:[port]/[path]
    sscanf(url, "http://%99[^:/]:%i/%199[^\n]", host, port, path);
    if (strlen(path) == 0)
    {
        strcpy(path, "/");
    }
}

int isEnodAFolder(struct enod *e)
{
    return !e->isFile;
}
int isEnodAHTMLFile(struct enod *e)
{
    if (strstr(e->url, ".html") != NULL) // also to check if it is the last 4 letters and is the file extension
        return 1;
    return 0;
}

int isFileNotFound(struct enod *ENOD)
{
    if (strstr(ENOD->header, "HTTP/1.1 404 Not Found") != NULL && strstr(ENOD->header, "HTTP/1.1 404 Not Found") == ENOD->header)
        return 1;
    return 0;
}

void createDirectoryIfNotExists(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
        mkdir(path, 0777);
}

void createFileOrFolder(struct enod *node)
{
    if (node == NULL)
        return;

    char basePath[1024] = "./download2";

    char relativePath[1024];
    strcpy(relativePath, node->url + strlen(node->baseUrl));

    char fullPath[2048];
    sprintf(fullPath, "%s%s", basePath, relativePath);
    printf("the fullpath is %s\n", fullPath);
    if (node->isFile)
    {
        char *dirPath = strdup(fullPath);
        char *parentDir = dirname(dirPath); // extracts the directory path

        char *p = NULL;
        for (p = parentDir + 1; *p; p++)
        {
            if (*p == '/')
            {
                *p = '\0'; // temporarily end the string
                createDirectoryIfNotExists(parentDir);
                *p = '/'; // put the slash back
            }
        }
        createDirectoryIfNotExists(parentDir);

        free(dirPath);

        FILE *file = fopen(fullPath, "w");
        if (file)
        {
            fputs(node->fileContent, file);
            fclose(file);
        }
        else
            printf("Failed to create file: %s\n", fullPath);
    }
    else
        createDirectoryIfNotExists(fullPath);
}

int isFile(const char *path)
{
    const char *dot = strrchr(path, '.');
    return dot != NULL && strchr(dot, '/') == NULL;
}

void buildNewUrl(char *baseUrl, char *path, char *resultUrl)
{
    printf("the base url is %s and the path is %s\n", baseUrl, path);
    char domain[1000];
    char fullPath[2000];
    char *token;
    char *state;

    strcpy(domain, baseUrl);

    char *lastSlash = strrchr(domain, '/');
    if (lastSlash != NULL && isFile(lastSlash + 1))
        *lastSlash = '\0';

    strcpy(fullPath, domain);

    char *inputPath = strdup(path); 
    for (token = strtok_r(inputPath, "/", &state); token != NULL; token = strtok_r(NULL, "/", &state))
    {
        if (strcmp(token, "..") == 0)
        {
            lastSlash = strrchr(fullPath, '/');
            if (lastSlash != NULL)
                *lastSlash = '\0'; 
        }
        else if (strcmp(token, ".") != 0)
        {
            strcat(fullPath, "/");
            strcat(fullPath, token);
        }
    }
    free(inputPath);

    strcpy(resultUrl, fullPath);
}

void fetchFileAndContinueDown(struct enod *ENOD, int maxDepth)
{
    // enod is the file we have to fetch now
    if (ENOD->depth > maxDepth)
        return;

    fileFetcher(ENOD);
    // printf("the fetched file url is %s is a %d and the content is: \n", ENOD->url, ENOD->isFile);
    // printf("the fetched file url is %s is a %d and the content is: \n %s \n", ENOD->url, ENOD->isFile, ENOD->fileContent);
    if (isFileNotFound(ENOD)) // no cotent to use further
    {
        printf("the file is not found\n");
        return;
    }
    createFileOrFolder(ENOD);
    if (isEnodAFolder(ENOD))
    {
        char *savePtr;
        char *childFolder = strtok_r(ENOD->fileContent, "\n", &savePtr);
        while (childFolder != NULL)
        {
            // printf("the child folder we want to use is %s\n", childFolder);
            struct enod nextNode;
            nextNode.id = globalENODId++;
            nextNode.depth = ENOD->depth + 1;
            strcpy(nextNode.url, "\0");
            strcat(nextNode.url, ENOD->url);
            strcat(nextNode.url, "/");
            strcat(nextNode.url, childFolder);
            strcpy(nextNode.baseUrl, ENOD->baseUrl);
            // cleaning the url of the file type receives
            for (int index = 1; index < strlen(nextNode.url); ++index) // optimize this
                if (nextNode.url[index] == ' ' && nextNode.url[index - 1] != '\\')
                    nextNode.url[index] = '\0';
            fetchFileAndContinueDown(&nextNode, maxDepth);
            childFolder = strtok_r(NULL, "\n", &savePtr);
            // printf("now we use a different folder\n %s \n", childFolder);
        }
    }
    else if (isEnodAHTMLFile(ENOD))
    {
        // here we need to extract the urls from it
        char *p; // this is the pointer to the last found src or href
        p = strstr(ENOD->fileContent, "src");
        while (p != NULL)
        {
            // use the string found
            // extract the url;
            char fileLocation[100];
            int i = 0;
            while (*p != '"')
                p++;
            p++;
            while (*p != ' ' && *p != '"') // to also add the case with "'" if it is safe for eval
                fileLocation[i++] = (*p), p++;
            fileLocation[i] = '\0';
            char resultUrl[100];
            // printf("the file localtion we have to get is %s\n", fileLocation);
            buildNewUrl(ENOD->baseUrl, fileLocation, resultUrl);
            struct enod nextNode;
            nextNode.id = globalENODId++;
            nextNode.depth = ENOD->depth + 1;
            // printf("the result url from the html resources is %s\n", resultUrl);
            strcpy(nextNode.url, resultUrl);
            strcpy(nextNode.baseUrl, ENOD->baseUrl);
            // now we need to implement the recursion depth limit check for max depth
            fetchFileAndContinueDown(&nextNode, maxDepth);
            p = strstr(p, "src");
        }
        return;
    }
}

void addPortIfMissingInURL(char *url)
{
    const char *httpPrefix = "http://";
    const char *httpsPrefix = "https://";
    const char portSuffix[] = ":80";
    char *newURL;
    int needsPort = 1;

    for (int i = 0; url[i] != '\0'; ++i)
    {
        if (url[i] == ':')
        {
            if (i > 5)
            {
                needsPort = 0;
                break;
            }
        }
    }

    if (needsPort)
    {
        newURL = (char *)malloc(strlen(url) + strlen(portSuffix) + 1);
        if (newURL == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }

        if (strncmp(url, httpPrefix, strlen(httpPrefix)) == 0 || strncmp(url, httpsPrefix, strlen(httpsPrefix)) == 0)
        {
            char *slashPtr = strchr(url + (strncmp(url, httpsPrefix, strlen(httpsPrefix)) == 0 ? 8 : 7), '/');
            if (slashPtr != NULL)
            {
                size_t offset = slashPtr - url;
                strncpy(newURL, url, offset);
                strcpy(newURL + offset, portSuffix);
                strcpy(newURL + offset + strlen(portSuffix), slashPtr);
            }
            else
            {
                strcpy(newURL, url);
                strcat(newURL, portSuffix);
            }
        }
        else
        {
            fprintf(stderr, "Invalid URL format\n");
            free(newURL);
            return;
        }

        printf("Modified URL: %s\n", newURL);
        strcpy(url, newURL);
        free(newURL);
    }
    else
        printf("URL already contains a port: %s\n", url);
}

void bringURLToStandardForm(char *url)
{
    addPortIfMissingInURL(url);
    // count the number of /
    // if it is less than 3 then
    int count = 0;
    int len = strlen(url);
    for (int i = 0; i < len; ++i)
        if (url[i] == '/')
            count++;

    if (count == 2)
        strcat(url, "/");
}

int main(int argc, char *argv[])
{
    // we get the file that we need to evaluate from the args
    // if the argv is < 2 error
    int depth = atoi(argv[2]);
    // clear the folder for the files to come
    executeCommand("rm", "rm -R download2");
    executeCommand("mkdir", "mkdir download2");

    /// now we are sure this is cleaned up
    struct enod rootENOD;
    rootENOD.id = globalENODId++;
    rootENOD.depth = 1;
    strcpy(rootENOD.url, argv[1]);
    // we want to bring the url to the standard form
    // the standard form is http://domain:port/[path] // the path can be or not but the / is of most imporntance

    bringURLToStandardForm(rootENOD.url);
    printf("the urls we being with is %s\n", rootENOD.url);
    char host[100] = {0};
    char path[200] = {0};
    int port;
    // we need to check if the url is not a localhost:8080 one then we add a index.html at the end of the url
    parseURL1(argv[1], host, path, &port);
    sprintf(rootENOD.baseUrl, "http://%s:%d", host, port);
    fetchFileAndContinueDown(&rootENOD, depth);
    // now we need to make the first
}
