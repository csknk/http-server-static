#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int serve(uint16_t port)
{
	char httpHeader[] = "HTTP/1.1 200 OK\nContent-Type:text/html\nServer: David's C HTTP Server\nConnection: close\r\n\n";
	char response[8000]; // This should be dynamically allocated

	if (fork()) {
		return 0;
	}
//	(void)signal(SIGCLD, SIG_IGN);
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
	int clientSocket, pid;
	char recvBuffer[9999];
	int counter = 0;

	while(1) {
		// accept()
		// ---------------------------------------------------------------------
		clientSocket = accept(serverSocket, NULL, NULL);

		// Fork the process, for basic multi-client functionality.
		if ((pid = fork()) == -1) {
			// Error forking the process
			close(clientSocket);
			continue;
		} else if (pid > 0) {
			// pid > 0 denotes that this is the parent, which should be closed
			close(clientSocket);
			continue;
		}
		
		printf("Number of forked processes: %d\n", ++counter);
		int received = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (received < 0) {
			puts("No request received");
		}
		printf("\nReceived:\n%s", recvBuffer);
		if (setResponse(httpHeader, response) != 0) {
			printf("Error setting the response.\n");
			close(clientSocket);
			return 1;
		}
		send(clientSocket, response, sizeof(response), 0);
		close(clientSocket);
	}
	return 0;
}

int setResponse(char httpHeader[], char response[])
{
	response[0] = 0;
	char *body = "";
	char filename[] = "index.html";
	if(setBody(&body, filename) != 0) {
		printf("Can't load data from %s\n", filename);
		strcpy(response, "HTTP/1.1 500 Internal Server Error\nContent-Type:text/html\nServer: David's C HTTP Server\nConnection: close\r\n\n");
		strcat(response, "<html><body><h1>+++ Out of Cheese Error +++</h1><h2>Redo from Start</h2>");
		strcat(response, "<img src='https://cdn.pixabay.com/photo/2016/10/01/20/54/mouse-1708347_960_720.jpg'></body></html>");
	} else {
		strcpy(response, httpHeader);
		strcat(response, body);
		free(body);
	}
	return 0;
}

int setBody(char **body, char filename[])
{
	FILE *fp = fopen(filename, "r");
	int errnum;
	if (fp == NULL) {
		errnum = errno;
		fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
		return 1;
	} else if (fp != NULL) {
		// Set the file position indicator for the stream pointed to by `fp`
		if (fseek(fp, 0L, SEEK_END) == 0) {
			// Get size of file: seek to the end of the file and then get the position.
			// ftell(fp) returns the current value of the file position indicator.
			long bufsize = ftell(fp);
			if (bufsize == -1) {
				errnum = errno;
				fprintf(stderr, "Can't get the size of file %s: %s\n", filename, strerror(errnum));
				return 1;
			}
			// Allocate an appropriately sized buffer to *body
			*body = malloc(sizeof(char) * (bufsize + 1));
			// Set file position indicator for stream pointed to by `fp` to the
			// beginning of the file. The `rewind()` function is an option here,
			// but that function returns void so can't be used as a check.
			if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

			// Read file into memory
			fread(*body, sizeof(char), bufsize, fp);
			if (ferror(fp) != 0) {
				fputs("Error reading file", stderr);
			} else {
				// Add a terminal null character.
				// Note: Can't do it like this: `body[newLen++] = '\0';`
				strcat(*body, "\0");
			}
		}
		fclose(fp);
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

// References
// -----------
// https://www.linuxquestions.org/questions/programming-9/fgets-read-a-whole-file-355504/#post1812050
// https://beej.us/guide/bgnet/html/multi/inet_ntopman.html
// http://www.zentut.com/c-tutorial/c-read-text-file/
// https://www.tutorialspoint.com/cprogramming/c_error_handling.htm
