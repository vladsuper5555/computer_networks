#ifndef WEBCONTENT_H
#define WEBCONTENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Function to extract content from a given HTTP URL
void returnFilesContent(const char *url, char *result);

#endif
