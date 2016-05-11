// A simple ack server 
#include "mysocket.h"  

/* Structure of arguments to pass to client thread */
struct TArgs {
  TSocket cliSock;   /* socket descriptor for client */
};

/* Handle client request */
int HandleRequest(void *args) {
  char str[100];
  TSocket cliSock;

  /* Extract socket file descriptor from argument */
  cliSock = ((struct TArgs *) args) -> cliSock;
  free(args);  /* deallocate memory for argument */

  /* Receive the request */
  if (ReadLine(cliSock, str, 99) < 0) 
    { ExitWithError("ReadLine() failed"); 
  } else printf("%s",str);  
  
  /*... do something ... */
  
  /* Send the response */
  if (WriteN(cliSock, "ack\n", 4) <= 0)  
    { ExitWithError("WriteN() failed"); }  

  close(cliSock);
  return 1;
}

int main(int argc, char *argv[]) {
  TSocket srvSock, cliSock;        /* server and client sockets */
  struct TArgs *args;              /* argument structure for thread */

  if (argc == 1) {
    ExitWithError("Usage: server <local port>");    
  }

  /* Create a passive-mode listener endpoint */  
  srvSock = CreateServer(atoi(argv[1]));

  for (;;) { /* run forever */
    cliSock = AcceptConnection(srvSock);

    /* Create a memory space for client argument */
    if ((args = (struct TArgs *) malloc(sizeof(struct TArgs))) == NULL)
      { ExitWithError("malloc() failed"); }
    args->cliSock = cliSock;

    /* Handle the client request */
    if (HandleRequest((void *) args) != 1)
      { ExitWithError("HandleRequest() failed"); }
  }
}
