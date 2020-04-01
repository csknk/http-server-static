#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "string-utilities.h"

int serve(uint16_t port)
{

	// socket()
	// -------------------------------------------------------------------------
	int serverSocket = socket(
			AF_INET,
			SOCK_STREAM,
			0
			);

	// Define `sockaddr_in`
	// -------------------------------------------------------------------------
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	// bind()
	// -------------------------------------------------------------------------
	int bound = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (bound != 0) {
		printf("\nSocket not bound.\n");
		return 1;
	}

	// listen()
	// -------------------------------------------------------------------------
	int listening = listen(serverSocket, BACKLOG);
	if (listening < 0) {
		printf("Error: The server is not listening.\n");
		return 1;
	}
	report(&serverAddress);
	int clientSocket;
	size_t childProcessCount = 0;
	
	// Prevent forked child processes from becoming zombies when they exit
//	signal(SIGCHLD, SIG_IGN);

	// Wait for a connection from a client
	// --------------------------------------------------------------------------
	while(1) {
		clientSocket = acceptTCPConnection(serverSocket);

		// Fork the process, for basic multi-client functionality.
		pid_t pid = fork();
		if (pid < 0) {
			// Error forking the process - should we die here?
			close(clientSocket);
			continue;
		} else if (pid == 0) {
			// This is a child process.
			close(serverSocket);
			handleHTTPClient(clientSocket);
			exit(EXIT_SUCCESS);
		}

		printf("Spawned child process %d\n", pid);
		childProcessCount++;
		printf("childProcessCount: %lu\n", childProcessCount);
		close(clientSocket);

		// Clean up zombies
		while (childProcessCount) {
			// NOHANG option with waitpid will result in a single zombie
			// that is reaped when the parent closes.
			pid = waitpid((pid_t) -1, NULL, WNOHANG);
//			pid = waitpid((pid_t) -1, NULL, 0);
			if (pid < 0) {
				dieWithSystemMessage("waitpid() failed.");
			} else if (pid == 0) {
				break;
			} else {
				printf("Reaped pid %d\n", pid);
				childProcessCount--;
			}
		}
	}
	return 0;
}

/**
 * Returns a non-negative file descriptor of an accepted client socket.
 *
 * The accept() function extracts the first connection on the queue of pending connections,
 * creating a new socket with the same socket type protocol and address family as the specified socket,
 * and allocating a new file descriptor for that socket.
 * */
int acceptTCPConnection(int serverSocket) {
	struct sockaddr_storage clientAddress;
	socklen_t clientAddressLength = sizeof(clientAddress);
	memset(&clientAddress, 0, clientAddressLength);

	int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
	if (clientSocket < 0) {
		dieWithSystemMessage("accept() failed");	
	}
	// @TODO Print connection data here <----------------------------------------------------------------
	
	return clientSocket;
}

void handleHTTPClient(int clientSocket)
{
	char recvBuffer[INPUT_BUFFER_SIZE];
	memset(recvBuffer, 0, sizeof(char) * INPUT_BUFFER_SIZE);
	
	int nBytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
	if (nBytesReceived < 0) {
		errorHandler(FORBIDDEN, "Failed to read request.", "", clientSocket);
	}
	// Terminate the recvBuffer after the received bytes
	if (nBytesReceived < INPUT_BUFFER_SIZE) {
		recvBuffer[nBytesReceived] = 0;
	} 

	char *response = NULL;
	char *filename = NULL;
	router(recvBuffer, clientSocket, &filename);

	if (setResponse(filename, &response, clientSocket) != 0) {
		printf("Error setting the response.\n");
		close(clientSocket);
	}
	send(clientSocket, response, strlen(response) + 1, 0);
	free(response);
	free(filename);
	close(clientSocket);
}

int router(char *request, int clientSocket, char **filename)
{
	printf("request: %s\n", request);
	if(strncmp(request, "GET ", 4) && strncmp(request, "get ", 4)) {
		errorHandler(FORBIDDEN, "Only simple GET operation supported", request, clientSocket);
	}
	
	// NUL terminate after the second space to simplify the request.
	size_t requestLen = strlen(request);
	for (size_t i = 4; i < requestLen; i++) {
		if (request[i] == ' ') {
			request[i] = 0;
			break;
		}
	}

	// Check for illegal access to parent directory "../"
	// Difficult to test as most clients don't allow requests like this.
	if (strstr(request, "..")) {
		errorHandler(FORBIDDEN, "Parent directory (..) path names are forbidden.", request, clientSocket);
	}

	// Convert / to index.html
	if(!strncmp(&request[0], "GET /\0", 6) || !strncmp(&request[0], "get /\0", 6)) {
		(void)strcpy(request, "GET /index.html");
	}

	// Set filename - the content that has been requested.
	size_t filenameLen = strlen(request) - 5;
	*filename = calloc(filenameLen + 1, sizeof(**filename));
	for (size_t i = 5, j = 0; j < filenameLen; i++, j++) {
		(*filename)[j] = request[i];
	}

	return 0;
}


int setResponse(char *filename, char **response, int clientSocket)
{
	char *body = NULL;
	if(setBody(&body, filename) != 0) {
		errorHandler(NOT_FOUND, "Not found", "", clientSocket);
	}

	size_t bodyLen = strlen(body);
	printf("bodyLen: %lu\n", bodyLen);

	// Build HTTP header, with size in bytes
//	char *header = NULL;
//	setHeader(&header, OK, bodyLen); 
	char httpHeader[] = "HTTP/1.1 200 OK\nContent-Type:text/html\nServer: David's C HTTP Server\nConnection: close\r\n\n";
	
	*response = calloc(strlen(httpHeader) + bodyLen, sizeof(**response));
	strcpy(*response, httpHeader);
	strcat(*response, body);
	free(body);
	return 0;
}

//int setHeader(char **header, int status, size_t bodyLen)
//{
//	return 0;
//}

int setBody(char **body, char filename[])
{
	return stringFromFile(filename, body); 
}

void report(struct sockaddr_in *serverAddress)
{
	char hostBuffer[INET6_ADDRSTRLEN];
	char serviceBuffer[NI_MAXSERV]; // defined in `<netdb.h>`
	socklen_t addr_len = sizeof(*serverAddress);
	int err = getnameinfo(
			(struct sockaddr *) serverAddress,
			addr_len,
			hostBuffer,
			sizeof(hostBuffer),
			serviceBuffer,
			sizeof(serviceBuffer),
			NI_NUMERICHOST
			);
	if (err != 0) {
		printf("It's not working!!\n");
	}
	printf("Server listening on http://%s:%s\n", hostBuffer, serviceBuffer);
}

