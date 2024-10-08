#ifndef UTILS_H
#define UTILS_H
#include <string.h>
#include <unistd.h>

// Function to clear console for the client
void clear_console(int connfd) {
    const char *clear_console_msg = "\033[H\033[J"; // ANSI escape code for clearing console
    write(connfd, clear_console_msg, strlen(clear_console_msg)); // Send to client
}


// Function to extract the first word until a '$' character
void extract_command(const char *input, char *output, size_t output_size) {
    const char *dollar_pos = strchr(input, '$');

    if (dollar_pos != NULL) {
        size_t length = dollar_pos - input;
        if (length >= output_size) {
            length = output_size - 1;
        }
        strncpy(output, input, length); 
        output[length] = '\0';
    } else {
        // If no '$' found, copy the whole string
        strncpy(output, input, output_size - 1);
        output[output_size - 1] = '\0';
    }
}


#endif
