#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h> // for getnameinfo()
#include <signal.h>
#include <wait.h>

// Usual socket headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "errors.h"

#define BACKLOG 100		// Passed to listen()
#define INPUT_BUFFER_SIZE 16192	// Max size of input buffer for the request received.

int serve(uint16_t port);
int acceptTCPConnection(int serverSocket);
void handleHTTPClient(int clientSocket);
int router(char *request, int clientSocket, char **filename);
void report(struct sockaddr_in *serverAddress);
int setHeader(char **header, int status, size_t bodyLen);
int setBody(char **body, char filename[]);
int setResponse(char *filename, char **response, int clientSocket);

#endif
