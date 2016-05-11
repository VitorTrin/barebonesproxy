// A multithread echo server 

#include "mysocket.h"  
#include <pthread.h>

#define BUFSIZE 100
#define NTHREADS 100

/* Structure of arguments to pass to client thread */
struct TArgs {
	TSocket cliSock;   /* socket descriptor for client */
};

int population;

/* Handle client request */
void * HandleRequest(void *args) {
	char str[BUFSIZE];
	char read[BUFSIZE];
	char secondread[BUFSIZE];
	int firstop;
	int secondop;
	int stop;
	TSocket cliSock;

	/* Extract socket file descriptor from argument */
	cliSock = ((struct TArgs *) args) -> cliSock;
	free(args);  /* deallocate memory for argument */

	for(;;) {
		/* Receive the request */
		if (ReadLine(cliSock, str, BUFSIZE-1) < 0) 
			{ ExitWithError("ReadLine() failed"); 
		} else printf("%s",str);  
		if (strncmp(str, "quit", 4) == 0) break; 

		if(strstr(str,"ADD")){ //Se tiver ADD na msg
//			printf("Entrou no strstr\n");
			stop = ReadUntil(str + 4,',',read); //Pega o primeiro operador, e guarda onde ele termina
//			printf("%s",read);
			firstop = atoi(read); 

			ReadUntil(str + 4 + stop + 1,'\n',secondread); // Pega o segundo operador
			secondop = atoi(secondread);

			memset(str,0,strlen(str)); //Limpa o str pra escrever

			sprintf(str,"%d + %d = %d\n",firstop,secondop,firstop+secondop);

			printf("%s\n",str);

			memset(read,0,strlen(read));//Flusha os operandos
			memset(secondread,0,strlen(secondread));
		}

		if(strstr(str,"SUB")){ //Se tiver ADD na msg
//			printf("Entrou no strstr\n");
			stop = ReadUntil(str + 4,',',read); //Pega o primeiro operador, e guarda onde ele termina
//			printf("%s",read);
			firstop = atoi(read); 

			ReadUntil(str + 4 + stop + 1,'\n',secondread); // Pega o segundo operador
			secondop = atoi(secondread);

			memset(str,0,strlen(str)); //Limpa o str pra escrever

			sprintf(str,"%d - %d = %d\n",firstop,secondop,firstop-secondop);

			printf("%s\n",str);

			memset(read,0,strlen(read));//Flusha os operandos
			memset(secondread,0,strlen(secondread));
		}

		if(strstr(str,"POP")){
			memset(str,0,strlen(str));
			sprintf(str, "Now connected: %d clients, including you\n",population);
		}

		/* Send the response */
		if (WriteN(cliSock, str, strlen(str)) <= 0)  
			{ ExitWithError("WriteN() failed"); }  
	}
	population--;
	close(cliSock);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	TSocket srvSock, cliSock;        /* server and client sockets */
	struct TArgs *args;              /* argument structure for thread */
	pthread_t threads[NTHREADS];
	int tid = 0;
	fd_set set;  /* file description set */
	int ret, i;
	char str[BUFSIZE];

	if (argc == 1) { ExitWithError("Usage: server <local port>"); }

	/* Create a passive-mode listener endpoint */  
	srvSock = CreateServer(atoi(argv[1]));

	printf("Server read!\n");
	/* Run forever */
	for (;;) { 
		/* Initialize the file descriptor set */
		FD_ZERO(&set);
		/* Include stdin into the file descriptor set */
		FD_SET(STDIN_FILENO, &set);
		/* Include srvSock into the file descriptor set */
		FD_SET(srvSock, &set);

		/* Select returns 1 if input available, -1 if error */
		ret = select (FD_SETSIZE, &set, NULL, NULL, NULL);
		if (ret<0) {
			 WriteError("select() failed"); 
			 break;
		}

		/* Read from stdin */
		if (FD_ISSET(STDIN_FILENO, &set)) {
			scanf("%99[^\n]%*c", str);
			if (strncasecmp(str, "FIM", 3) == 0) break;

		}

		/* Read from srvSock */
		if (FD_ISSET(srvSock, &set)) {
			if (tid == NTHREADS) { 
				WriteError("number of threads is over"); 
				break; 
			}
			
			/* Spawn off separate thread for each client */
			cliSock = AcceptConnection(srvSock);
			population++;

			/* Create separate memory for client argument */
			if ((args = (struct TArgs *) malloc(sizeof(struct TArgs))) == NULL) { 
				WriteError("malloc() failed"); 
				break;
			}
			args->cliSock = cliSock;

			/* Create a new thread to handle the client requests */
			if (pthread_create(&threads[tid++], NULL, HandleRequest, (void *) args)) { 
				WriteError("pthread_create() failed"); 
				break;
			}
		}
	}
	
	printf("Server will wait for the active threads and terminate!\n");
	/* Wait for all threads to terminate */
	for(i=0; i<tid; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}
