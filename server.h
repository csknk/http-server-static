#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h> // for getnameinfo()
#include <signal.h>

// Usual socket headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 100  // Passed to listen()

int serve(uint16_t port);
void report(struct sockaddr_in *serverAddress);
int setBody(char **body, char filename[]);
int setResponse(char httpHeader[], char response[]);

#endif
