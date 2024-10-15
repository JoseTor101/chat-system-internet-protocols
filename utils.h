#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <unistd.h>

void clear_console(int connfd);
char* get_string_key(const char *string);
char* get_string_value(char *string);

#endif // UTILS_H
