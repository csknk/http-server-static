#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "string-utilities.h"

int serve(uint16_t port)
{
	char httpHeader[] = "HTTP/1.1 200 OK\nContent-Type:text/html\nServer: David's C HTTP Server\nConnection: close\r\n\n";
//	char response[8000]; // This should be dynamically allocated

	(void)signal(SIGCHLD, SIG_IGN);
//	(void)signal(SIGHUP, SIG_IGN);
//	for (int i = 0; i < 32; i++) {
//		(void)close(i);
//	}
	(void)setpgrp();

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
	int pid;
	char recvBuffer[INPUT_BUFFER_SIZE];
	memset(recvBuffer, 0, sizeof(char) * INPUT_BUFFER_SIZE);
	size_t childProcessCount = 0;

	// Wait for a connection from a client
	// --------------------------------------------------------------------------
	while(1) {
		clientSocket = acceptTCPConnection(serverSocket);

		// Fork the process, for basic multi-client functionality.
		if ((pid = fork()) == -1) {
			// Error forking the process
			close(clientSocket);
			continue;
		} else if (pid > 0) {
			// pid > 0 denotes that this is the parent, which should be closed.
			// In this case, the parent loop continues and waits for a new connection,
			// while the forked process handles the current client socket.
			close(clientSocket);
			continue;
		}

		// Child process - parent never makes it this far.
		printf("Number of forked processes: %lu\n", ++childProcessCount);
		
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
		if (setResponse(filename, httpHeader, &response, clientSocket) != 0) {
			printf("Error setting the response.\n");
			close(clientSocket);
			return 1;
		}
		send(clientSocket, response, strlen(response) + 1, 0);
		free(response);
		free(filename);
		close(clientSocket);
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

	size_t filenameLen = strlen(request) - 5;
	*filename = calloc(filenameLen + 1, sizeof(**filename));
	for (size_t i = 5, j = 0; j < filenameLen; i++, j++) {
		(*filename)[j] = request[i];
	}

	printf("request: %s\n", request);
	printf("request: %s\n", *filename);
	return 0;
}


int setResponse(char *filename, char httpHeader[], char **response, int clientSocket)
{
	char *body = NULL;
//	char filename[] = "index.html";
	if(setBody(&body, filename) != 0) {
		errorHandler(NOT_FOUND, "Not found", "", clientSocket);
	} else {
		*response = calloc(strlen(httpHeader) + strlen(body), sizeof(**response));
		strcpy(*response, httpHeader);
		strcat(*response, body);
		free(body);
	}
	return 0;
}

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

