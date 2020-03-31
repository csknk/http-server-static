#include "errors.h"

void errorHandler(int type, char *msg, char *input, int socketFd)
{
	char *response = NULL;
	switch (type) {
		case ERROR:
			fprintf(stderr, "%s: %s exiting pid = %d\n", msg, input, getpid());
			dieWithSystemMessage(msg);
			break;
		case FORBIDDEN:
			// Send the response to the client, then exit this (forked!) process.
			stringFromFile("responses/403", &response);
			write(socketFd, response, strlen(response) + 1);
			dieWithUserMessage("FORBIDDEN", msg);
		case NOT_FOUND:
			stringFromFile("responses/404", &response);
			printf("NOT_FOUND case: %s\n", response);
			write(socketFd, response, strlen(response) + 1);
			dieWithUserMessage("NOT_FOUND", msg);
	}
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


