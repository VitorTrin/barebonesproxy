// A multiprocess echo server
 
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
  for(;;) {
    /* Receive the request */
    if (ReadLine(cliSock, str, 99) < 0) 
      { ExitWithError("ReadLine() failed"); 
    } else printf("%s",str);  
    if (strncmp(str, "quit", 4) == 0) break; 
 
    /*... do something ... */
  
    /* Send the response */
    if (WriteN(cliSock, str, strlen(str)) <= 0)  
      { ExitWithError("WriteN() failed"); }  
  }
  close(cliSock);
  return 1;
}

int main(int argc, char *argv[]) {
  TSocket srvSock, cliSock;        /* server and client sockets */
  struct TArgs *args;              /* argument structure for thread */

  int pid;

  if (argc == 1) {
    ExitWithError("Usage: server <local port>");    
  }

  /* Create a passive-mode listener endpoint */  
  srvSock = CreateServer(atoi(argv[1]));

  /* Spawn off separate thread for each client */
  for (;;) { /* run forever */
    cliSock = AcceptConnection(srvSock);

    /* Create separate memory for client argument */
    if ((args = (struct TArgs *) malloc(sizeof(struct TArgs))) == NULL)
      { ExitWithError("malloc() failed"); }
    args->cliSock = cliSock;
  
    /* Create a child process to handle the request */
    if (( pid = fork()) < 0) { ExitWithError("fork() failed"); }
    else if (pid == 0)   {    /* child process */
      close(srvSock);	      /* close server socket */
      if (HandleRequest((void *) args) != 1) // processa a solicitacao
        { ExitWithError("HandleRequest() failed"); }
      exit(1);
    }
    else {   /* father process */
      close(cliSock);            /* close client socket */
      free(args);  /* deallocate memory for argument */
    }
  }
}
