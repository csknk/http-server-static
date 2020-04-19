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
#define INPUT_BUFFER_SIZE 16192	// Max size of input buffer for the request received.

typedef struct r {
	char *method;
	char *route;
	
} Request;

//typedef struct l {
//	struct sockaddr *clientAddr;
////	char *id;	// RFC 1413 identity of the client (not reliable, hyphen as placeholder)
////	char *userid;	// userid, HTTP authentication
////	char *time;	// Time the request was received
////	char *req;	// The request line from the client in double quotes
//	int status;	// The returned status code
//	size_t size;	// Size of object returned, not including headers (body size in bytes)
//} LogData;

int serve(uint16_t port);
int acceptTCPConnection(int serverSocket, LogData *log);
void handleHTTPClient(int clientSocket, LogData *log);
int router(char *request, int clientSocket, char **filename);
void report(struct sockaddr_in *serverAddress);
void setHostServiceFromSocket(struct sockaddr_in *socketAddress, char *hostBuffer, char *serviceBuffer);
//void logConnection(struct sockaddr *genericClientAddress);
void logConnection(LogData *log);

#endif
