#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "errors.h"

int stringFromFile(char *file, char **buffer);

#endif
