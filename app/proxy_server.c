// A multithread echo server

#include "mysocket.h"
#include "utils.h"
#include <pthread.h>

#define LINE_LEN 512
#define HEADER_LEN 1024
#define BUFFER_SIZE 1024
#define NTHREADS 100

/* Structure of arguments to pass to client thread */
struct TArgs {
	TSocket cliSock;   /* socket descriptor for client */
};

typedef struct {
	char* bodyData;
	int bodySize;
	char firstLine[LINE_LEN];
	char header[HEADER_LEN];
} HttpPacket;

void PrintHttpPacket(HttpPacket* packet) {
	printf("FIRST LINE:\n");
	printf("%s\n", packet->firstLine);

	printf("HEADER:\n");
	printf("%s", packet->header);

	printf("BODY SIZE:\n"
		   "%d\n", packet->bodySize);

	printf("BODY:\n");
	if(packet->bodyData && packet->bodySize > 0) {
		printf("%s", packet->bodyData);
	}
	// TODO(erick): Print body
}

int ReadRequestFromSocket(HttpPacket* request, TSocket socket) {
	char buffer[LINE_LEN];

	if(ReadLine(socket, request->firstLine, LINE_LEN) <= 0) {
		WriteError("ReadRequestFromSocket: Could not establish connection with client");
		return 0;
	}
	TrimMessage(request->firstLine);

#if DEBUG_BUILD
	printf("Message is: %s - size: %ld\n", request->firstLine, strlen(request->firstLine));
#endif

	char* write_ptr = request->header;
	int headerCharsLeft = HEADER_LEN;
	int readingHeader = 1;
	while(readingHeader) {
		int readResult = ReadLine(socket, buffer, LINE_LEN);
		if(readResult < 0) {
			WriteError("ReadRequestFromSocket: Communication with client has failed");
			return 0;
		}
		if(readResult == 0) {
			WriteError("ReadRequestFromSocket: HTTP packet ended abruptly");
			return 0;
		}

		TrimMessage(buffer);

#if DEBUG_BUILD
		printf("Header line: %s\n", buffer);
#endif

		int headerLineLen = strlen(buffer) + 2;
		headerCharsLeft -= headerLineLen;

		if(headerCharsLeft < 0) {
			WriteError("ReadRequestFromSocket: The 'request' header size was exceeded");
			return 0;
		}

		sprintf(write_ptr, "%s\r\n", buffer);
		write_ptr += headerLineLen;

		// NOTE(erick): The header ends when we read an empty line
		if(strlen(buffer) == 0) {
			readingHeader = 0;
		}
	}

#if DEBUG_BUILD
	printf("Request was:\n");
	PrintHttpPacket(request);
#endif

	return 1;
}

typedef struct {
	char protocol[7];
	char host[128];
	char file[256];
} Url;

int HandleGetRequest(HttpPacket* request, Url* url) {
	char* firstSpace = FindChar(request->firstLine, ' ');

	if(!firstSpace) {
		WriteError("HandleGetRequest: Invalid first line (first space not found).");
		return 0;
	}
	if(strncmp(request->firstLine, "GET", firstSpace - request->firstLine) != 0) {
		WriteError("HandleGetRequest: Request is not a GET request.");
		return 0;
	}

	TrimMessage(firstSpace);
	char* secondSpace = FindChar(firstSpace, ' ');

	if(!secondSpace) {
		WriteError("HandleGetRequest: Invalid first line (second space not found).");
		return 0;
	}

	sscanf(firstSpace, "%7[^:]://%127[^/ ]/%255[^ ]", url->protocol, url->host, url->file);

#if DEBUG_BUILD
	printf("Proto: %s - Host: %s - File: %s\n", url->protocol, url->host, url->file);
#endif

	return 1;
}

void WriteHttpRequestToSocket(HttpPacket* packet, Url* url, TSocket socket) {
	char buffer[BUFFER_SIZE];

	sprintf(buffer, "GET /%s HTTP/1.1\r\n", url->file);
	WriteN(socket, buffer, strlen(buffer));

	sprintf(buffer, "Host: %s\r\n", url->host);
	WriteN(socket, buffer, strlen(buffer));

	// NOTE(erick): Send packet header
	WriteN(socket, packet->header, strlen(packet->header));

	// NOTE(erick): Finishes the request
	sprintf(buffer, "\r\n");
	WriteN(socket, buffer, strlen(buffer));
}

int ReadResponseFromServerSocket(TSocket hostSocket, HttpPacket* response) {
	if(ReadLine(hostSocket, response->firstLine, LINE_LEN) <= 0) {
		WriteError("ReadRequestFromSocket: Could not establish connection with client");
		return 0;
	}

	int reading = 1;
	int headerCharsLeft = HEADER_LEN;
	char buffer[LINE_LEN];
	char* write_ptr = response->header;
	while(reading) {
		int readResult = ReadLine(hostSocket, buffer, LINE_LEN);

		if(readResult < 0) {
			WriteError("ReadResponseFromServerSocket: Communication with server has failed");
			return 0;
		}
		if(readResult == 0) {
			WriteError("ReadResponseFromServerSocket: HTTP packet ended abruptly");
			return 0;
		}

		TrimMessage(buffer);

		int headerLineLen = strlen(buffer) + 2;
		headerCharsLeft -= headerLineLen;

#if DEBUG_BUILD
		printf("Response line: %s\n", buffer);
#endif

		if(headerCharsLeft < 0) {
			WriteError("ReadResponseFromServerSocket: The 'request' header size was exceeded");
			return 0;
		}

		sprintf(write_ptr, "%s\r\n", buffer);
		write_ptr += headerLineLen;

		// TODO(erick): Check whether this line contains a
		// 'Content-Length:' parameter and set the response
		// 'bodySize' to its value.
		if(strlen(buffer) == 0) {
			reading = 0;
		}
	}

	return 1;
}

#define EndSocketRequest(socket) \
	close(socket); \
	pthread_exit(NULL);

/* Handle client request */
void * HandleRequest(void *args) {
	TSocket clientSocket = 0;
	HttpPacket request = {0};
	HttpPacket response = {0};
	Url requestedUrl = {0};

	/* Extract socket file descriptor from argument */
	clientSocket = ((struct TArgs *) args) -> cliSock;
	free(args);  /* deallocate memory for argument */

	if(!ReadRequestFromSocket(&request, clientSocket)) {
		EndSocketRequest(clientSocket);
	}

	if(!HandleGetRequest(&request, &requestedUrl)) {
		EndSocketRequest(clientSocket);
	}

	// NOTE(erick): RedirectGetRequest
	TSocket hostSocket = ConnectToHostByName(requestedUrl.host);
	WriteHttpRequestToSocket(&request, &requestedUrl, hostSocket);


	if(!ReadResponseFromServerSocket(hostSocket, &response)) {
		EndSocketRequest(clientSocket);
	}

#if DEBUG_BUILD
	printf("Response Packet:\n");
	PrintHttpPacket(&response);
#endif

	EndSocketRequest(clientSocket);
}

int main(int argc, char *argv[]) {
	TSocket srvSock, cliSock;        /* server and client sockets */
	struct TArgs *args;              /* argument structure for thread */
	pthread_t threads[NTHREADS];
	int tid = 0;
	fd_set set;  /* file description set */
	int ret, i;
	char str[BUFFER_SIZE];

	if (argc == 1) { ExitWithError("Usage: server <local port>"); }

	/* Create a passive-mode listener endpoint */
	srvSock = CreateServer(atoi(argv[1]));

	printf("Server ready!\n");
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
