#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
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
int isEnodAFolder (struct enod* e) {
    return !e->isFile;
}
int isEnodAHTMLFile (struct enod* e) {
    return 0;
}

int isFileNotFound(struct enod* ENOD) 
{
    if (strstr(ENOD->fileContent, "HTTP/1.1 404 Not Found") != NULL)
        return 1;
    return 0;
}

void createFileOrFolder(struct enod *node) {
    if (node == NULL) return;

    // Creating the base path for files and folders
    char basePath[1024] = "./download";

    // Constructing the relative path
    char relativePath[1024];
    strcpy(relativePath, node->url + strlen(node->baseUrl));

    // Full path for the file or folder
    char fullPath[2048];
    sprintf(fullPath, "%s%s", basePath, relativePath);

    // Check if it's a file or folder
    if (node->isFile) {
        // Create file and write content
        FILE *file = fopen(fullPath, "w");
        if (file) {
            fputs(node->fileContent, file);
            fclose(file);
        } else {
            printf("Failed to create file: %s\n", fullPath);
        }
    } else {
        // Create folder
        mkdir(fullPath, 0777);
    }
}


void fetchFileAndContinueDown (struct enod* ENOD, int maxDepth) 
{
    //enod is the file we have to fetch now 
    if (ENOD->depth > maxDepth)
        return;

    fileFetcher(ENOD);
    printf("the fetched file url is %s is a %d and the content is: \n %s \n", ENOD->url, ENOD->isFile, ENOD->fileContent);
    if (isFileNotFound(ENOD)) // no cotent to use further
        return;

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
            for (int index = 1; index < strlen(nextNode.url); ++index)// optimize this
                if (nextNode.url[index] == ' ' && nextNode.url[index - 1] != '\\')
                    nextNode.url[index] = '\0'; 
            fetchFileAndContinueDown(&nextNode, maxDepth);
            childFolder = strtok_r(NULL, "\n", &savePtr);
            // printf("now we use a different folder\n %s \n", childFolder);
        }
    }
    else if (isEnodAHTMLFile(ENOD)) 
    {
        // here we need to extract the
        return;
    }
    else {
        //this is a normal file
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
    /// now we are sure this is cleaned up
    struct enod rootENOD;
    rootENOD.id = globalENODId++;
    rootENOD.depth = 1;
    strcpy(rootENOD.localPath, "./download");
    strcpy(rootENOD.url, argv[1]);
    strcpy(rootENOD.baseUrl, argv[1]);
    fetchFileAndContinueDown(&rootENOD, depth);

    // now we need to make the first 
}
