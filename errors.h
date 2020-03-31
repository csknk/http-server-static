#ifndef ERRORS_H
#define ERRORS_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "string-utilities.h"

enum type {
	ERROR =	42,
	LOG = 44,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
};

void errorHandler(int type, char *msg, char *input, int socketFd);
void dieWithUserMessage(const char *msg, const char *detail);
void dieWithSystemMessage(const char *msg);

#endif
