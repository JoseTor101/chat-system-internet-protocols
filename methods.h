// request_handler.h
#ifndef METHODS_H
#define METHODS_H

char* getVersion(size_t start, char *msg);
char** createResponse(const char *version, int statusCode, const char *contentType, int contentLength);
void handleGET(char *rawMsg, size_t methodLength);
char* parseRequest(char *rawMsg);

#endif // METHODS_H
