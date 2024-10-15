#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PORT 8080
#define MAX_CLIENTS 10
#define NAME_LENGTH 10
#define COMMAND_SIZE 10

//Used in chat.c
#define CHAT_MSG_MAXSIZE 1024
#define MAX_ADDITIONAL_DATA 10

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define WHITE   "\033[37m" 
#define CLEAR_CONSOLE "\033[H\033[J"

#endif // CONSTANTS_H