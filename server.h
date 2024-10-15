#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <pthread.h>
#include "constants.h"

struct Client {
    int sockfd;                // Socket file descriptor
    char username[NAME_LENGTH]; // Username of the client
    int paired;                // Paired client ID, -1 if not paired
    bool is_first_connection;   // Flag for the first connection
};

extern struct Client available_clients[MAX_CLIENTS]; 
extern int num_clients; 

extern pthread_mutex_t clients_mutex;

void print_menu(int connfd);
void send_message(const char *pre_join_text, const char *message, int dest_sockfd);
void initialize_clients();
void list_users(int connfd);
void assign_username(int connfd);
void select_user(int connfd, int client_index, int target_client);
void *client_handler(void *connfd_ptr);
int main();

#endif // SERVER_H
