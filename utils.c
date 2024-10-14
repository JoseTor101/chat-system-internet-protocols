#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

// Function to clear console for the client
void clear_console(int connfd) {
    const char *clear_console_msg = "\033[H\033[J"; // ANSI escape code for clearing console
    write(connfd, clear_console_msg, strlen(clear_console_msg)); // Send to client
}

// KEY:VALUE -> KEY
char* getStringKey(const char *string) {
    if (!string) return NULL;
    char *colon_pos = strchr(string, ':');
    if (!colon_pos) return NULL;

    size_t key_length = colon_pos - string;
    char *key = (char *)malloc(key_length + 1);
    if (key == NULL) {
        perror("Failed to allocate memory for key");
        return NULL;
    }
    
    strncpy(key, string, key_length);
    key[key_length] = '\0';

    return key;
}

// KEY:VALUE -> VALUE   
char* getStringValue(char *string) {
    if (!string) return NULL;
    char *colon_pos = strchr(string, ':');
    if (!colon_pos) return NULL;
    
    return strdup(colon_pos + 1); // Duplicate the string after the colon
}