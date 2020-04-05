#include "errors.h"

void errorHandler(int type, char *msg, char *input, int socketFd)
{
	switch (type) {
		case ERROR:
			fprintf(stderr, "%s: %s exiting pid = %d\n", msg, input, getpid());
			dieWithSystemMessage(msg);
			break;
		case FORBIDDEN:
			sendErrorResponse("responses/403.html", FORBIDDEN, socketFd);
			dieWithUserMessage("FORBIDDEN", msg);
			break;
		case NOT_FOUND:
			sendErrorResponse("responses/404.html", NOT_FOUND, socketFd); 
			dieWithUserMessage("NOT_FOUND", msg);
			break;
	}
}

/**
 * Send a response to the client socket indicating an error status.
 *
 * Don't reuse the existing response building function `setResponse()` as there is a potential to generate
 * errors there, calling the errorHandler, then calling the response builder, in an infinite loop.
 * */
int sendErrorResponse(char* filename, int status, int socketFd)
{
	char *body = NULL;
	if (stringFromFile(filename, &body) != 0) {
		free(body);
		dieWithSystemMessage(filename);
	}
	size_t bodyLen = strlen(body);
	char *header = NULL;
	setHeader(&header, status, 9, bodyLen); 
	
	char *response = calloc(strlen(header) + bodyLen + 1, sizeof(*response));
	strcpy(response, header);
	strcat(response, body);
	
	write(socketFd, response, strlen(response) + 1);

	free(response);
	free(body);
	return 0;
}

void dieWithUserMessage(const char *msg, const char *detail)
{
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

/*
 * This function prints an error message and dies in the event of a system error.
 *
 * When a library function stores a non-zero value in `errno`, `perror()` can be
 * used to print the argument (in this case `msg`) along with an error message determined
 * by the value of errno.
 * */
void dieWithSystemMessage(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}


