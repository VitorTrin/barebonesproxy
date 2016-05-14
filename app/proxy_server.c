#include "mysocket.h"
#include "utils.h"
#include <pthread.h>

#define LINE_LEN 512
#define HEADER_LEN 1024
#define BUFFER_SIZE 1024
#define NTHREADS 100000

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
	char protocol[8];
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

	// // NOTE(erick): Send packet header
	// WriteN(socket, packet->header, strlen(packet->header));

	// NOTE(erick): Finishes the request
	sprintf(buffer, "\r\n");
	WriteN(socket, buffer, strlen(buffer));
}

int ReadResponseHeadFromServerSocket(TSocket hostSocket, HttpPacket* response) {
	if(ReadLine(hostSocket, response->firstLine, LINE_LEN) <= 0) {
		WriteError("ReadRequestFromSocket: Could not establish connection with client");
		return 0;
	}

	TrimMessage(response->firstLine);

	int reading = 1;
	int headerCharsLeft = HEADER_LEN;
	char buffer[LINE_LEN];
	char* write_ptr = response->header;
	response->bodySize = -2;
	while(reading) {
		int readResult = ReadLine(hostSocket, buffer, LINE_LEN);

		if(readResult < 0) {
			WriteError("ReadResponseHeadFromServerSocket: Communication with server has failed");
			return 0;
		}
		if(readResult == 0) {
			WriteError("ReadResponseHeadFromServerSocket: HTTP packet ended abruptly");
			return 0;
		}

		TrimMessage(buffer);

		int headerLineLen = strlen(buffer) + 2;
		headerCharsLeft -= headerLineLen;

#if DEBUG_BUILD
		printf("Response line: %s\n", buffer);
#endif

		if(headerCharsLeft < 0) {
			WriteError("ReadResponseHeadFromServerSocket: The 'request' header size was exceeded");
			return 0;
		}

		sprintf(write_ptr, "%s\r\n", buffer);
		write_ptr += headerLineLen;

		if(strstr(buffer, "Content-Length: ") == buffer) {
			sscanf(buffer, "Content-Length: %d", &(response->bodySize));
		}

		if(strcasecmp(buffer, "Transfer-Encoding: Chunked") == 0) {
			response->bodySize = -1;
		}

		if(strlen(buffer) == 0) {
			reading = 0;
		}
	}

	return 1;
}

int ReadResponseBodyWithLengthFromServerSocket(TSocket hostSocket, HttpPacket *response) {

	response->bodyData = (char*) calloc(response->bodySize, sizeof(char));

	int bodyCharsLeft = response->bodySize;
	char buffer[LINE_LEN];
	char* write_ptr = response->bodyData;
	while(bodyCharsLeft > 0) {
		int readResult = ReadLine(hostSocket, buffer, LINE_LEN);

		if(readResult < 0) {
			WriteError("ReadResponseBodyWithLengthFromServerSocket: Communication with server has failed");
			return 0;
		}
		if(readResult == 0) {
			WriteError("ReadResponseBodyWithLengthFromServerSocket: HTTP packet ended abruptly");
			return 0;
		}

		int bodyLineLen = strlen(buffer);
		bodyCharsLeft -= bodyLineLen;

#if DEBUG_BUILD
		printf("Response line: %s", buffer);
#endif

		if(bodyCharsLeft < 0) {
			WriteError("ReadResponseBodyWithLengthFromServerSocket: The 'request' body size was exceeded");
			return 0;
		}

		sprintf(write_ptr, "%s", buffer);
		write_ptr += bodyLineLen;
	}

	return 1;
}

#define KILO (1024)
#define MEGA (KILO * KILO)

typedef enum {
	ReadChunkSize,
	ReadChunkData
} ReadChunkState;

int ReadResponseBodyChunkedFromServerSocket(TSocket hostSocket, HttpPacket *response) {
	int bufferSize = 5 * MEGA;
	char* buffer = (char*) calloc(bufferSize, sizeof(char));
	int charsReadToBuffer = 0;

	ReadChunkState readState = ReadChunkSize;
	int currentChunkSize;
	int currentChunkRead;

	int reading = 1;
	while(reading) {
		char* write_ptr = buffer + charsReadToBuffer;
		int charsRead = ReadLine(hostSocket, write_ptr, bufferSize - charsReadToBuffer);

		if(charsRead < 0) {
			WriteError("ReadResponseBodyChunkedFromServerSocket: Communication with server has failed");
			return 0;
		}
		if(charsRead == 0) {
			WriteError("ReadResponseBodyChunkedFromServerSocket: HTTP packet ended abruptly");
			return 0;
		}

		charsReadToBuffer += charsRead;

#if DEBUG_BUILD
		printf("Body line: %s", write_ptr);
#endif

		switch(readState) {
			case ReadChunkSize :
				sscanf(write_ptr, "%x", &currentChunkSize);
				currentChunkRead = 0;
				readState = ReadChunkData;
			break;
			case ReadChunkData :
				currentChunkRead += charsRead;
				if(currentChunkRead >= currentChunkSize) {
					readState = ReadChunkSize;
				}
				if(currentChunkSize == 0) {
					reading = 0;
				}
			break;
		}

		// NOTE(erick): We reallocate when the remaining space is less than 1 KiB
		if(bufferSize - charsReadToBuffer < 1 * KILO) {
			bufferSize *= 2;
			buffer = (char*) realloc(buffer, bufferSize);
		}
	}

	response->bodyData = buffer;
	response->bodySize = charsReadToBuffer;

	return 1;
}

int WriteHttpResponseToSocket(HttpPacket *response, TSocket socket) {
	char buffer[LINE_LEN + 2];
	// FIRST LINE
	sprintf(buffer, "%s\r\n", response->firstLine);
	if(WriteN(socket, buffer, strlen(buffer)) <= 0) {
		WriteError("WriteHttpResponseToSocket: Connection with client closed");
		return 0;
	}

	// HEADER
	if(WriteN(socket, response->header, strlen(response->header)) <= 0) {
		WriteError("WriteHttpResponseToSocket: Connection with client closed");
		return 0;
	}

	// BODY
	if(response->bodyData && response->bodySize > 0) {
		if(WriteN(socket, response->bodyData, response->bodySize) <= 0) {
			WriteError("WriteHttpResponseToSocket: Connection with client closed");
			return 0;
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

	if(strlen(requestedUrl.host) == 0) {
		WriteError("Invalid request");
		EndSocketRequest(clientSocket);
	}

	// NOTE(erick): RedirectGetRequest
	TSocket hostSocket = ConnectToHostByName(requestedUrl.host);
	WriteHttpRequestToSocket(&request, &requestedUrl, hostSocket);


	if(!ReadResponseHeadFromServerSocket(hostSocket, &response)) {
		EndSocketRequest(clientSocket);
	}

	if(response.bodySize > 0) {
		ReadResponseBodyWithLengthFromServerSocket(hostSocket, &response);
	}
	else if(response.bodySize == -1) {
		ReadResponseBodyChunkedFromServerSocket(hostSocket, &response);
	}
	else {
		printf("There is no packet bode\n");
	}

#if DEBUG_BUILD
	printf("Response Packet:\n");
	PrintHttpPacket(&response);
#endif

	WriteHttpResponseToSocket(&response, clientSocket);

	printf("Finish\n");

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
	while(1) {
		ret = prepareAndEvokeSelect(&set, srvSock);

		if (ret == 0) {
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
