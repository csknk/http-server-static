#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "errors.h"

#define DEFAULT_PORT 8001

int main(int argc, char **argv)
{
	uint16_t port = DEFAULT_PORT;
	if (argc > 1) {
		char *end;
		port = strtol(argv[1], &end, 10);
		if (port < 1024 || port > 49151) {
			errorHandler(ERROR, "Invalid port number (use 1024 - 49151)", argv[1], 0);
		}
	}
	serve(port);
	return EXIT_SUCCESS;
}
