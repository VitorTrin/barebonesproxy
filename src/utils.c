#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

#include "mysocket.h"
#include "utils.h"

/* Error handling */
void ExitWithError(char *errorMsg) {
  printf("-- EXIT: %s\n", errorMsg);
  exit(1);
}

/* Write an error message */
void WriteError(char *errorMsg) {
  printf("-- ERROR: %s\n", errorMsg);
}

int isTrimmedChar(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void TrimMessage(char* msg) {
    char* msg_ptr = msg;

    while(*msg_ptr != '\0') {
        if(!isTrimmedChar(*msg_ptr)) {
            break;
        }
        ++msg_ptr;
    }

    while(*msg_ptr != '\0') {
        *msg = *msg_ptr;
        ++msg_ptr;
        ++msg;
    }

    do {
        --msg;
    } while(isTrimmedChar(*msg));

    *++msg = '\0';
}

// NOTE(erick): Returns a pointer to the first sub-string of 'haystack'
// starting with 'needle' or NULL otherwise.
char* FindChar(char* haystack, char needle) {
    while(*haystack != '\0') {
        if(*haystack == needle) {
            return haystack;
        }
        haystack++;
    }

    return NULL;
}

int prepareAndEvokeSelect(fd_set *set, TSocket sock) {
    /* Initialize the file descriptor set */
    FD_ZERO(set);
    /* Include stdin into the file descriptor set */
    FD_SET(STDIN_FILENO, set);
    /* Include srvSock into the file descriptor set */
    FD_SET(sock, set);
    /* Select returns 1 if input available, -1 if error */
    int ret = select (FD_SETSIZE, set, NULL, NULL, NULL);

    if (ret < 0) {
        WriteError("select() failed");
        return 0;
    }
    return 1;
}
