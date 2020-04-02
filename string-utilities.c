#include "string-utilities.h"

int stringFromFile(char *filename, char **buffer)
{
	int errnum;
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		errnum = errno;
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errnum));
		return 1;
	}
	if (fseek(fp, 0L, SEEK_END) != 0) {
		fprintf(stderr, "Can't get file size\n");
		return 1;
	}
	long bufferSize = ftell(fp);
	if (fseek(fp, 0L, SEEK_SET) != 0) {
		fprintf(stderr, "Can't reset file to beginning.\n");
		return 1;
	}

	*buffer = calloc(bufferSize + 1, sizeof(**buffer));
	if (*buffer == NULL) {
		fprintf(stderr, "Error allocating memory.\n");
		return 1;
	}

	fread(*buffer, sizeof(**buffer), bufferSize, fp);
	fclose(fp);	
	return 0;
}

int setHeader(char **header, int status, size_t bodyLength)
{
	char *template = "HTTP/1.1 %d %s\nContent-Type:text/html\nContent-Length: %lu\nServer: David's C HTTP Server\nConnection: close\r\n\n";
	char *statusString = NULL;
	setStatusString(&statusString, status);
	size_t headerLength = snprintf(NULL, 0, template, status, statusString, bodyLength);
	*header = calloc(headerLength + 1, sizeof(**header));
	sprintf(*header, template, status, statusString, bodyLength);
	free(statusString);
	return 0;
}

int setBody(char **body, char filename[])
{
	return stringFromFile(filename, body); 
}

int setStatusString(char **statusString, enum statusCode s)
{
	char *tmp;
	size_t nChars;
	switch(s) {
		case ERROR:
			tmp = "ERROR";
			break;
		case OK:
			tmp = "OK";
			break;
		case FORBIDDEN:
			tmp = "FORBIDDEN";
			break;
		case NOT_FOUND:
		       	tmp = "NOT FOUND";
			break;
		default:
			tmp = "UNKNOWN";
	}
	nChars = strlen(tmp);
	*statusString = calloc(nChars + 1, sizeof(**statusString));
	strcpy(*statusString, tmp);
	return 0;
}

