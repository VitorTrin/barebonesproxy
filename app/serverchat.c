// Servidor de clientes

#include "mysocket.h"  
#include <pthread.h>

#define BUFSIZE 100
#define NTHREADS 100
#define MAXCHARACTERS 20
#define IPSIZE 

/* Structure of arguments to pass to client thread */
struct TArgs {
	TSocket cliSock;   /* socket descriptor for client */
};

typedef struct {

	char* nome;
	char* ip;

}user;

user *list;

int retcode;

pthread_mutex_t mutex_population;
pthread_mutex_t mutex_userList;

struct sockaddr_in cliAddr = {0}; //estrutura para guardar informacoes do cliente
//memset((void*)&cliAddr, 0, sizeof(cliAddr)); //zera a estrutura de dados
unsigned int cliLen = sizeof(cliAddr); //tamanho atual da estrutura de dados

int passivePopulation = 0; 

/* Handle client 1:request */
void * HandleRequest(void *args) {
	char str[BUFSIZE];
	TSocket cliSock;
	int i , j;
	char * newBegin;
	int success;

	/* Extract socket file descriptor from argument */
	cliSock = ((struct TArgs *) args) -> cliSock;
	free(args);  /* deallocate memory for argument */

	while(1) {
		/* Receive the request */
		if (ReadLine(cliSock, str, BUFSIZE-1) < 0) 
			{ ExitWithError("ReadLine() failed"); 
		} else printf("%s",str);  
		if (strncmp(str, "bye\n", 4) == 0) break; 

		printf("valor do strncasecmp : %d\n", strncasecmp(str,"2\n",2) );
		printf("O teste do logout tá dando %d\n",strncasecmp(str,"3 ",2) );

		if (strncasecmp(str,"1 ",2) == 0) {// se for login

			printf("new login of the user %s\n",str);

			//if (ReadLine(cliSock, str, BUFSIZE-1) < 0) 
			//{ ExitWithError("ReadLine() failed"); }
			printf("Antes do mutex\n");
			pthread_mutex_lock(&mutex_population);
			pthread_mutex_lock(&mutex_userList);

			printf("Depois do mutex\n");
			str[strlen(str)-1] = 0;

			strcpy(list[passivePopulation].nome , str +2);

			printf("O nome copiado foi %s\n", list[passivePopulation].nome);

			getpeername(cliSock, (struct sockaddr* ) &cliAddr, &cliLen);

			strcpy(list[passivePopulation].ip, inet_ntoa(cliAddr.sin_addr));

			printf("O Ip copiado foi %s\n", list[passivePopulation].ip);

			passivePopulation++;

			pthread_mutex_unlock(&mutex_userList);
			pthread_mutex_unlock(&mutex_population);

			strcpy(str, "1\n");

			if (WriteN(cliSock, str, strlen(str)) <= 0)  
				{ ExitWithError("WriteN() failed"); } 

		}

		if(strncasecmp(str,"2\n",2) == 0) // se for BUMP
		{// se for BUMP

			memset(str,0,strlen(str));

			printf("Bump!\n");

			pthread_mutex_lock(&mutex_population);

			printf("População depois do bump é de %d\n",passivePopulation);


			sprintf(str,"%d\n", passivePopulation);
			
			pthread_mutex_unlock(&mutex_population);


			if (WriteN(cliSock, str, strlen(str)) <= 0)  
				{ ExitWithError("WriteN() failed"); } 

			for (i = 0; i < passivePopulation; ++i) {

			//	strcpy(str,list[i].nome);
			//	strcat(str,"");
			//	strcat(str,list[i].ip);
				sprintf(str , "%s %s\n" , list[i].nome , list[i].ip);
			 	
				if (WriteN(cliSock, str, strlen(str)) <= 0)  
					{ ExitWithError("WriteN() failed"); }  

			 } 


		}

		if(strncasecmp(str,"3 ",2) == 0) {// se for logout


			printf("logout of the user %s\n",str);

			newBegin = strchr(str, ':');
			newBegin++;
			newBegin++; //começa no nome da pessoa, teoricamente *dedos cruzados*

			success = 0;

			pthread_mutex_lock(&mutex_population);
			pthread_mutex_lock(&mutex_userList);


			for (i = 0; i < passivePopulation; ++i) {

				if(strncasecmp(list[i].nome , newBegin , strlen(newBegin) == 0))
				{
					success = 1;
					//remover esse cara e acertar a list pq isso aqui é um array e não uma lista encadeada ;_;
					for (j = i; i < (passivePopulation - 1) ; ++j)//passivePopulation - 1 senão ele copia umas paradas não inicializadas
					{

						list[j].nome = list[j + 1].nome;
						list[j].ip = list [j + 1].ip;

					}
					passivePopulation--;
					break;
				}

			}


			pthread_mutex_unlock(&mutex_userList);
			pthread_mutex_unlock(&mutex_population);


			sprintf(str,"%d", success);


			if (WriteN(cliSock, str, strlen(str)) <= 0)  
				{ ExitWithError("WriteN() failed"); }



		}


	close(cliSock);
	pthread_exit(NULL);
}
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

	list = malloc(NTHREADS*sizeof(user));

	for (i = 0; i < NTHREADS; ++i)
	{

//		list[i].nome = (char * )malloc(MAXCHARACTERS*sizeof(char));
//		list[i].ip = (char * )malloc(MAXCHARACTERS*sizeof(char));

		list[i].nome = (char * )malloc(9*sizeof(char));
		list[i].ip = (char * )malloc(9*sizeof(char));

	}

	pthread_mutex_init(&mutex_population, NULL); 
	pthread_mutex_init(&mutex_userList, NULL); 


	/* Create a success-mNTHREADS
	ode listener endpoint */  
	srvSock = CreateServer(2016);

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
