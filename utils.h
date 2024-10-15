#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <unistd.h>

// Function to clear console for the client
void clear_console(int connfd);

// Function to extract key from KEY:VALUE string
char* get_string_key(const char *string);

// Function to extract value from KEY:VALUE string
char* get_string_value(char *string);

#endif // UTILS_H
