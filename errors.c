#include "errors.h"

void errorHandler(int type, char *msg, char *input, int socketFd)
{
	if (socketFd) {
		printf("Socket id: %d\n", socketFd);
	}
	switch (type) {
		case ERROR:
			fprintf(stderr, "%s: %s exiting pid = %d\n", msg, input, getpid());
			break;

	}
	if (type == ERROR || type == NOT_FOUND || type == FORBIDDEN) {
		exit(EXIT_FAILURE);
	}
}

