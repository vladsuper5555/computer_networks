#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../executeCommand.h"
#include "fileFetcher.h"

// isFile // we may have folders that we fetch from our server
// name, fileExtension and relative path of the file 
// fileContent

// also we need to handle the case when file is too big
// lets say if the file is more than 10 kb // to be handled in the server 
// we print a message regarding that
int globalENODId = 0;
int isEnodAFolder (struct enod* e) {
    return e->isFile;
}
int isEnodAHTMLFIle (struct enod* e) {

}
/*
void downloadFiles(Link link, int maxDepth) {
    if (link.depth > maxDepth) {
        createFoldersForUrl(link.url);
        return;
    }

    char* htmlContent = downloadHtmlContent(link.url);
    Link* links = parseHtmlForLinks(htmlContent, link.depth);

    // Iterate over extracted links and call downloadFiles recursively
    for (int i = 0; i < /* number of links ; i++) {
    //     downloadFiles(links[i], maxDepth);
    // }

    // // Free resources
    // free(htmlContent);
    // free(links);
// }

void createFoldersForUrl(const char* url, int maxDepth) {
    char* urlCopy = strdup(url); // Duplicate the URL as strtok modifies the string
    const char* delim = "/";     // Delimiter for splitting the URL

    // Tokenize the URL to extract path components
    char* token = strtok(urlCopy, delim);
    char path[1024] = {0}; // Buffer to hold the constructed path
    int depth = 0;

    while (token != NULL && depth < maxDepth) {
        strcat(path, token);
        strcat(path, "/");

        // Create directory if it doesn't exist
        mkdir(path, 0777); // You might need to adjust permissions

        token = strtok(NULL, delim);
        depth++;
    }

    free(urlCopy); // Free the duplicated URL
}
/*
void fetchFileAndContinueDown (struct enod* ENOD, int maxDepth) 
{
    //enod is the file we have to fetch now 
    if (ENOD->depth > maxDepth)
        return;

    fileFetcher(ENOD);
    // now we have the following cases
    // if it is a folder create a folder and launche a file fetch for each of the files inside (we know them fromt he file content)
    // if it is a html document fetched find the files in it and launch a fetch for each of them
    // 
    if (isEnodAFolder(ENOD)) 
    {
        // if it is a folder check if the files in the localpath are existing 
        
        // implement the logic to create a enod for a each file in the ENOD

        createFoldersForUrl(ENOD->url, maxDepth);
        fetchFileAndContinueDown(childEnod, maxDepth);
    }
    else if (isEnodAHTMLFile(ENOD)) 
    {

    }
    else {
        //this is a normal file
    }
    
    // now we fetch the file
}
*/
int main(int argc, char *argv[])
{
    // we get the file that we need to evaluate from the args
    // if the argv is < 2 error
    printf("What we are going to fetch is %s \n", argv[1]);
    int depth = atoi(argv[2]);
    printf("The depth we are going to fetch to is %d", depth);
    // clear the folder for the files to come
    // executeCommand("rm", "rm -R download");
    // executeCommand("mkdir", "mkdir download");
    /// now we are sure this is cleaned up
    struct enod rootENOD;
    rootENOD.id = globalENODId++;
    rootENOD.depth = 1;
    strcpy(rootENOD.localPath, "./download");
    strcpy(rootENOD.url, argv[1]);
    fileFetcher(&rootENOD);

    // now we need to make the first 
}
