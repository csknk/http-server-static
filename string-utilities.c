#include "string-utilities.h"

#define MAX_TIME_CHARS 32

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
	{ 0, 0 }			// NULL for end
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

/**
 * Set a response.
 *
 * The response consists of a body and a header. The body is determined by the resource
 * that has been requested (a filename) and the header includes the size of the body.
 *
 * If there is a problem setting the body, the `errorHandler()` function takes care of returning
 * an appropriate response to the client socket.
 *
 * */
int setResponse(char *filename, char **response, int status, ssize_t mimeTypeIndex,
		int clientSocket,
		LogData *log)
{
	char *body = NULL;
	if(setBody(&body, filename) != 0) {
		free(body);
		free(filename);
		errorHandler(NOT_FOUND, "Not found", "", clientSocket);
	}

	size_t bodyLen = strlen(body);
	printf("BEFORE: %s\n", log->req);
	log->size = bodyLen;
	printf("AFTER: %s\n", log->req);
//	log->status = status;
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
	return index;
}

/**
 * Set a timestamp.
 * 
 * Used for logging events.
 * */
void timestamp(char **str)
{
	time_t now = time(0);
	struct tm *gtm = gmtime(&now);
	char timeBuffer[MAX_TIME_CHARS] = {0};
	size_t n = strftime(timeBuffer, MAX_TIME_CHARS, "%d/%b/%G:%T %Z", gtm);
	if (n == 0) {
		errorHandler(ERROR, "strftime() error", "", 0);	
	}
	*str = calloc(n + 1, sizeof(**str));
	strcpy(*str, timeBuffer);
}

/**
 * Get the first line of a string.
 *
 * Note that the strcspn search includes terminating null character - so if the target string does not
 * contain a newline, strcspn returns the number of characters in the target string (excluding
 * the terminating null character).
 *
 * Returns the number of characters in the found line.
 *
 * */
ssize_t firstLine(char *str, char **line)
{
	ssize_t n = -1;
	n = strcspn(str, "\n");
	*line = calloc(n + 1, sizeof(*line));
	if (*line == NULL) {
		fprintf(stderr, "calloc() failed.");
		return n;
	}
	// Explain the offset! was including '\n'
	strncpy(*line, str, n - 1);
	return n;
}

