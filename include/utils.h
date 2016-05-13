#ifndef UTILS_H
#define UTILS_H 1

#include <sys/select.h>
#include "mysocket.h"

#define DEFAULT_PEER_PORT 2017
#define DEFAULT_SERVER_PORT 2016

#define BUFSIZE 100
#define NTHREADS 100
#define MAX_NAME_LEN 20
#define MAX_USER_COUNT 1000

void ExitWithError(char*);
void WriteError(char*);

int isTrimmedChar(char);
void TrimMessage(char*);
char* FindChar(char*, char);

int prepareAndEvokeSelect(fd_set*, TSocket);
#endif
