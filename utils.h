#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <unistd.h>

// Function to clear console for the client
void clear_console(int connfd);

// Function to extract key from KEY:VALUE string
char* getStringKey(const char *string);

// Function to extract value from KEY:VALUE string
char* getStringValue(char *string);

#endif // UTILS_H
