// we need to remove all the logic for the https kind of functions

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <libgen.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../executeCommand.h"
#include "fileFetcher.h"
#include <sys/stat.h>
#include <unistd.h>

// isFile // we may have folders that we fetch from our server
// name, fileExtension and relative path of the file
// fileContent

// also we need to handle the case when file is too big
// lets say if the file is more than 10 kb // to be handled in the server
// we print a message regarding that
int globalENODId = 0;

void parseURL1(const char *url, char *host, char *path, int *port)
{
    *port = 80; // Default HTTP port
    // Assuming the URL format is http://[host]:[port]/[path]
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
    if (strstr(ENOD->fileContent, "HTTP/1.1 404 Not Found") != NULL && strstr(ENOD->fileContent, "HTTP/1.1 404 Not Found") == ENOD->fileContent)
        return 1;
    return 0;
}

void createDirectoryIfNotExists(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        mkdir(path, 0777);
    }
}

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

void createFileOrFolder(struct enod *node)
{
    if (node == NULL)
        return;

    char basePath[1024] = "./download";

    char relativePath[1024];
    strcpy(relativePath, node->url + strlen(node->baseUrl));

    char fullPath[2048];
    sprintf(fullPath, "%s%s", basePath, relativePath);
    printf("the fullpath is %s\n", fullPath);
    if (node->isFile)
    {
        char *dirPath = strdup(fullPath);
        char *parentDir = dirname(dirPath); // Extracts the directory path

        // Create all directories in the path
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
        createDirectoryIfNotExists(parentDir); // Ensure the parent directory is created

        free(dirPath);

        // Create the file
        FILE *file = fopen(fullPath, "w");
        if (file)
        {
            fputs(node->fileContent, file);
            fclose(file);
        }
        else
        {
            printf("Failed to create file: %s\n", fullPath);
        }
    }
    else
    {
        createDirectoryIfNotExists(fullPath);
    }
}

// void createFileOrFolder(struct enod *node) {
//     if (node == NULL) return;

//     // Creating the base path for files and folders
//     char basePath[1024] = "./download";

//     // Constructing the relative path
//     char relativePath[1024];
//     strcpy(relativePath, node->url + strlen(node->baseUrl));

//     // Full path for the file or folder
//     char fullPath[2048];
//     sprintf(fullPath, "%s%s", basePath, relativePath);

//     // Check if it's a file or folder
//     if (node->isFile) {
//         // Create file and write content
//         FILE *file = fopen(fullPath, "w");
//         if (file) {
//             fputs(node->fileContent, file);
//             fclose(file);
//         } else {
//             printf("Failed to create file: %s\n", fullPath);
//         }
//     } else {
//         // Create folder
//         mkdir(fullPath, 0777);
//     }
// }

int isFile(const char *path)
{
    const char *dot = strrchr(path, '.');
    return dot != NULL && strchr(dot, '/') == NULL;
}

void buildNewUrl(const char *baseUrl, const char *path, char *resultUrl)
{
    char domain[1000];
    char fullPath[2000];
    char *token;
    char *state;

    // Copy baseUrl to domain
    strcpy(domain, baseUrl);

    // Remove file part from domain if present
    char *lastSlash = strrchr(domain, '/');
    if (lastSlash != NULL && isFile(lastSlash + 1))
    {
        *lastSlash = '\0';
    }

    // Initialize fullPath with domain
    strcpy(fullPath, domain);

    // Tokenize the input path
    char *inputPath = strdup(path); // Duplicate path to avoid modifying the original
    for (token = strtok_r(inputPath, "/", &state); token != NULL; token = strtok_r(NULL, "/", &state))
    {
        if (strcmp(token, "..") == 0)
        {
            // Go back one directory
            lastSlash = strrchr(fullPath, '/');
            if (lastSlash != NULL)
            {
                *lastSlash = '\0'; // Remove the last segment
            }
        }
        else if (strcmp(token, ".") != 0)
        {
            // Append the token to fullPath
            strcat(fullPath, "/");
            strcat(fullPath, token);
        }
    }
    free(inputPath);

    // Copy the result to resultUrl
    strcpy(resultUrl, fullPath);
}

void fetchFileAndContinueDown(struct enod *ENOD, int maxDepth)
{
    // enod is the file we have to fetch now
    if (ENOD->depth > maxDepth)
        return;

    fileFetcher(ENOD);
    printf("the fetched file url is %s is a %d and the content is: \n %s \n", ENOD->url, ENOD->isFile, ENOD->fileContent);
    if (isFileNotFound(ENOD)) // no cotent to use further
    {
        printf("the file is not found\n");
        return;
    }
    createFileOrFolder(ENOD);
    // now we have the following cases
    // if it is a folder create a folder and launche a file fetch for each of the files inside (we know them fromt he file content)
    // if it is a html document fetched find the files in it and launch a fetch for each of them
    //
    if (isEnodAFolder(ENOD))
    {
        // if it is a folder check if the files in the localpath are existing

        // implement the logic to create a enod for a each file in the ENOD

        // createFoldersForUrl(ENOD->url, maxDepth);
        // this is not childfolder but rather childENOD
        char *savePtr;
        char *childFolder = strtok_r(ENOD->fileContent, "\n", &savePtr);
        // printf("THis is the first child %s \n", childFolder);
        childFolder = strtok_r(NULL, "\n", &savePtr); // skipping the . folder
        // printf("THis is the second child %s \n", childFolder);
        childFolder = strtok_r(NULL, "\n", &savePtr); // skipping the .. foder
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
        p = strstr(ENOD->fileContent, "src=");
        while (p != NULL)
        {
            // use the string found
            // extract the url;
            char fileLocation[100];
            int i = 0;
            // if (strstr(p, "src=") != NULL)
            p += 5; // 4 is src= and 1 is the " after that
            printf("the locatiion of p is %s \n", p);
            while (*p != ' ' && *p != '"') // to also add the case with "'" if it is safe for eval
                fileLocation[i++] = (*p), p++;
            fileLocation[i] = '\0';
            // use the file location to determine the rest of the files
            // ENOD->url is the current url, fileLocation is the href from the src tag,
            // compose the new file that we need to request
            char resultUrl[100];
            printf("the file localtion we have to get is %s\n", fileLocation);
            buildNewUrl(ENOD->url, fileLocation, resultUrl);
            struct enod nextNode;
            nextNode.id = globalENODId++;
            nextNode.depth = ENOD->depth + 1;
            printf("the result url from the html resources is %s\n", resultUrl);
            strcpy(nextNode.url, resultUrl);
            strcpy(nextNode.baseUrl, ENOD->baseUrl);
            // now we need to implement the recursion depth limit check for max depth
            fetchFileAndContinueDown(&nextNode, maxDepth);
            p = strstr(p, "src=");
        }
        return;
    }
    else
    {
        // this is a normal file
        //  since we already create the enod we dont have anything else to do
        return;
    }

    // now we fetch the file
}

int main(int argc, char *argv[])
{
    // we get the file that we need to evaluate from the args
    // if the argv is < 2 error
    printf("What we are going to fetch is %s \n", argv[1]);
    int depth = atoi(argv[2]);
    printf("The depth we are going to fetch to is %d\n", depth);
    // clear the folder for the files to come
    executeCommand("rm", "rm -R download");
    executeCommand("mkdir", "mkdir download");
    // if (strstr(argv[1], "localhost:8080") == NULL && doesURlPointToFolder(argv[1]) == 1)
    // {
    //     if (argv[1][strlen(argv[1]) - 1] == '/')
    //         strcat(argv[1], "index.html");
    //     else
    //         strcat(argv[1], "/index.html");

    // }

    /// now we are sure this is cleaned up
    struct enod rootENOD;
    rootENOD.id = globalENODId++;
    rootENOD.depth = 1;
    strcpy(rootENOD.localPath, "./download");
    strcpy(rootENOD.url, argv[1]);

    char host[100] = {0};
    char path[200] = {0};
    int port;
    // we need to check if the url is not a localhost:8080 one then we add a index.html at the end of the url
    printf("the url is %s\n", argv[1]);
    parseURL1(argv[1], host, path, &port);
    // if (port != 80)
        sprintf(rootENOD.baseUrl, "http://%s:%d", host, port);
    // else
        // sprintf(rootENOD.baseUrl, "http://%s", host);
    printf("%s\n", rootENOD.baseUrl);
    fetchFileAndContinueDown(&rootENOD, depth);

    // now we need to make the first
}
