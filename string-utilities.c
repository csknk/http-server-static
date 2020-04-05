#include "string-utilities.h"

static const struct {
	char *ext;
	char *filetype;
} extensions[] = {			// Index
	{ "css", "text/css" },		// 0	
	{ "gif", "image/gif" },		// 1 
	{ "jpg", "image/jpg" },		// 2 
	{ "png", "image/png" },		// 3 
	{ "ico", "image/ico" },		// 4 
	{ "zip", "image/zip" },		// 5 
	{ "gz",  "image/gz"  },		// 6 
	{ "tar", "image/tar" },		// 7 
	{ "htm", "text/html" },		// 8 
	{ "html", "text/html" },	// 9 
	{ "jpeg", "image/jpeg" },	// 10
	{ "js", "text/javascript" },	// 11
	{ 0, 0 }
};

/**
 * Open file from the provided filename, put the contents in the referenced buffer.
 *
 * */
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

int setResponse(char *filename, char **response, int status, ssize_t mimeTypeIndex, int clientSocket)
{
	char *body = NULL;
	if(setBody(&body, filename) != 0) {
		free(body);
		free(filename);
		errorHandler(NOT_FOUND, "Not found", "", clientSocket);
	}


	// @TODO Need to handle a response with no body - i.e. error status return.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	size_t bodyLen = strlen(body);
	char *header = NULL;
	setHeader(&header, status, mimeTypeIndex, bodyLen); 
	*response = calloc(strlen(header) + bodyLen + 1, sizeof(**response));
	strcpy(*response, header);
	strcat(*response, body);
	free(body);
	free(header);
	return 0;
}

/**
 * Set a HTTP header.
 *
 * Includes the response code and body length.
 * */
int setHeader(char **header, int status, ssize_t mimeTypeIndex, size_t bodyLength)
{
	char *template =
		"HTTP/1.1 %d %s\n"
		"Content-Type:%s\n"
		"Content-Length: %lu\n"
		"Server: David's C HTTP Server\n"
		"Connection: close\r\n\n";

	char *statusString = NULL;
	setStatusString(&statusString, status);
	char *fileType = extensions[mimeTypeIndex].filetype;

	size_t headerLength = snprintf(NULL, 0, template, status, statusString, fileType, bodyLength);
	*header = calloc(headerLength + 1, sizeof(**header));
	sprintf(*header, template, status, statusString, fileType, bodyLength);
	
	free(statusString);
	return 0;
}

int setBody(char **body, char filename[])
{
	return stringFromFile(filename, body); 
}

/**
 * Set a dynamically allocated string that represents the HTTP return code.
 *
 * The status codes are defined as an enum - because of their values, they can't
 * be effectively used to dereference an array of character strings, hence this function.
 *
 * This is not good for maintenance - codes are defined separately from the return strings - this
 * is a compromise. An alternative would be a hash map/associative array of integers indexed by char* value,
 * but this is a small project and doesn't warrant this.
 *
 * The approach taken by Apache httpd is to `#define` define codes (e.g. `#define HTTP_CONTINUE 100`), and using
 * these values to dereference a large array of char * strings, using an `index_of_response()` function to compute
 * the correct offsets from the x00 code of each level, such that a supplied index of 100 (the lowest possible
 * return code value) returns the first element of the array (the correct string denoting a 100 return code).
 *
 * A supplied index of 200 returns the element at index 3, because there are only 3 1xx codes.
 *
 * For more info see:
 * - https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L73
 * - https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L795
 * - https://github.com/apache/httpd/blob/trunk/include/httpd.h#L479
 *
 * */
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

/**
 * Determine if the filetype is in our array of allowed types, based on the filename extension.
 * If it is, return it's index in the array so that it can be easily accessed elsewhere.
 *
 * */
ssize_t fileTypeAllowed(char *filename)
{
	size_t extLen = 0;
	ssize_t index = -1;
	const char *filenameExtension = strrchr(filename, '.');
	if (!filenameExtension) {
		return -1;
	}

	for(size_t i = 0; extensions[i].ext != 0; i++) {
		extLen = strlen(extensions[i].ext);
		if(!strncmp(filenameExtension + 1, extensions[i].ext, extLen)) {
			index = i;
			break;
		}
	}
	printf("\nmimeTypeIndex: %ld\n--------------------\n", index);
	return index;
}
