#ifndef UTILS_H
#define UTILS_H 1

#include <sys/select.h>
#include "mysocket.h"

void ExitWithError(char*);
void WriteError(char*);

int isTrimmedChar(char);
void TrimMessage(char*);
char* FindChar(char*, char);

int prepareAndEvokeSelect(fd_set*, TSocket);
#endif
