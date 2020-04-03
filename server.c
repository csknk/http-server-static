#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "string-utilities.h"

int serve(uint16_t port)
{

	int serverSocket = socket(
			AF_INET,
			SOCK_STREAM,
			0
			);

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int bound = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (bound != 0) {
		printf("\nSocket not bound.\n");
		return 1;
	}

	int listening = listen(serverSocket, BACKLOG);
	if (listening < 0) {
		printf("Error: The server is not listening.\n");
		return 1;
	}
	report(&serverAddress);
	int clientSocket;
	size_t childProcessCount = 0;
	
	// Prevent forked child processes from becoming zombies when they exit
	// signal(SIGCHLD, SIG_IGN);

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
	struct sockaddr *res = (struct sockaddr *)&clientAddress;

	int clientSocket = accept(serverSocket, res, &clientAddressLength);
	if (clientSocket < 0) {
		dieWithSystemMessage("accept() failed");	
	}
	// @TODO Print connection data here <----------------------------------------------------------------
	// SAMPLE CODE ++++++++++++++++++++++++++++++++++++++++++++
	char *s = NULL;
	switch(res->sa_family) {
	    case AF_INET: {
		struct sockaddr_in *addr_in = (struct sockaddr_in *)res;
		s = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
		break;
	    }
	    case AF_INET6: {
		struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)res;
		s = malloc(INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
		break;
	    }
	    default:
		break;
	}
	printf("IP address: %s\n", s);
	free(s);
	
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
	router(recvBuffer, clientSocket, &filename);

	// @TODO ------------
	// What kind of status code? This should check the return from router()...
	if (setResponse(filename, &response, OK, clientSocket) != 0) {
		errorHandler(ERROR, "Error setting the response.", "", clientSocket); 
	}
	send(clientSocket, response, strlen(response) + 1, 0);
	free(response);
	free(filename);
	close(clientSocket);
}

int router(char *request, int clientSocket, char **filename) {
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

