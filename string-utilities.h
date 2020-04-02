#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

enum type {
	ERROR =	42,
	LOG = 44,
	OK = 200,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
};

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "errors.h"

int stringFromFile(char *file, char **buffer);

#endif
