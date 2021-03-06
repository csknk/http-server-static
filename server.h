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
#include "string-utilities.h"

#define BACKLOG 100		// Passed to listen()
#define INPUT_BUFFER_SIZE 8096	// Max size of input buffer for the request received.

typedef struct r {
	char *method;
	char *route;
} Request;

int serve(uint16_t port);
int acceptTCPConnection(int serverSocket, LogData *log);
void handleHTTPClient(int clientSocket, LogData *log);
int router(char *request, int clientSocket, char **filename);
void report(struct sockaddr_in *serverAddress);
void setHostServiceFromSocket(struct sockaddr_in *socketAddress, char *hostBuffer, char *serviceBuffer);
void logConnection(LogData *log);
void freeLog(LogData *log);

#endif
