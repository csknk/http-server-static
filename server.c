#include <stdio.h>
#include <stdlib.h>
#include "server.h"

#define MAX_PORT_DIGITS 6 

/*
 * Function that manages the server functionality.
 *
 * Sets up server socket and runs an infinite loop to accept and handle incoming connections.
 *
 * Each connection is forked into a child process. To prevent forked child processes from becoming zombies
 * when they exit, we loop over child processes and call `waitpid()`. The NOHANG option for waitpid results
 * in a single zombie, as each finished child process is reaped and a new one forked. The final zombie is
 * reaped when the parent process closes.
 * */
int serve(uint16_t port)
{
	// Create a socket and set up a structure to hold the server address.
	int serverSocket = socket(AF_INET, SOCK_STREAM,	0);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int bound = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (bound != 0) {
		char portString[5 + MAX_PORT_DIGITS];
		sprintf(portString, "port %d", port);
		errorHandler(ERROR, "Socket not bound", portString, 0);
	}

	int listening = listen(serverSocket, BACKLOG);
	if (listening < 0) {
		printf("Error: The server is not listening.\n");
		return 1;
	}
	report(&serverAddress);
	int clientSocket;
	size_t childProcessCount = 0;
	
	while(1) {
		clientSocket = acceptTCPConnection(serverSocket);
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

		printf("Child process %d\n", pid);
		childProcessCount++;
		close(clientSocket);

		while (childProcessCount) {
			pid = waitpid((pid_t) -1, NULL, WNOHANG);
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
	struct sockaddr *genericClientAddressData = (struct sockaddr *)&clientAddress;

	int clientSocket = accept(serverSocket, genericClientAddressData, &clientAddressLength);

	if (clientSocket < 0) {
		dieWithSystemMessage("accept() failed");	
	}
	
	logConnection(genericClientAddressData);
	return clientSocket;
}

void handleHTTPClient(int clientSocket)
{
	// Size of recvBuffer?????????????????????
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
	char *mimeType = NULL;
	router(recvBuffer, clientSocket, &filename);

	ssize_t mimeTypeIndex = -1;
	if ((mimeTypeIndex = fileTypeAllowed(filename)) == -1) {
		free(filename);
		errorHandler(FORBIDDEN, "mime type", "Requested file type not allowed.", clientSocket);
	}

	// @TODO ------------
	// What kind of status code? This should check the return from router()...
	if (setResponse(filename, &response, OK, mimeTypeIndex, clientSocket) != 0) {
		errorHandler(ERROR, "Error setting the response.", "", clientSocket); 
	}
	send(clientSocket, response, strlen(response) + 1, 0);
	free(response);
	free(filename);
	free(mimeType);
	close(clientSocket);
}

/**
 *
 * @TODO We should check for 404s here...
 * */
int router(char *request, int clientSocket, char **filename) {
	printf("request: %s\n", request);
	if(strncmp(request, "GET ", 4) && strncmp(request, "get ", 4)) {
		errorHandler(FORBIDDEN, "Only simple GET operation supported", request, clientSocket);
	}
	
	// Limit the request. Include the resource requested only.
	// NUL terminate after the second space to end the request string. Because we've ruled out anything
	// but GET requests, we know that the resource being requested starts after index 4 of the
	// request e.g. `GET / HTTP/1.1`
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

	// Convert / to index.html, HTTP request verb to uppercase.
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

/**
 * Use Common Log Format:
 * - IP address of client
 * - RFC 1413 identity of the client (not reliable, hyphen as placeholder)
 * - userid, HTTP authentication (TODO, hyphen as placeholder)
 * - Time the request was received
 * - The request line from the client in double quotes
 * - The returned status code
 * - Size of object returned, not including headers (body size in bytes)
 *
 * */
void logConnection(struct sockaddr *genericClientAddressData)
{
	char *IPString = NULL;
	
	switch(genericClientAddressData->sa_family) {
	    case AF_INET: {
		struct sockaddr_in *addr_in = (struct sockaddr_in *)genericClientAddressData;
		IPString = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(addr_in->sin_addr), IPString, INET_ADDRSTRLEN);
		break;
	    }
	    case AF_INET6: {
		struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)genericClientAddressData;
		IPString = malloc(INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &(addr_in6->sin6_addr), IPString, INET6_ADDRSTRLEN);
		break;
	    }
	    default:
		break;
	}
	char *timeString = NULL;
	timestamp(&timeString);
	printf("%s - - [%s] \n", IPString, timeString);
	free(timeString);
	free(IPString);

}
