#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "constants.h"
#include "app_layer/chat.h"
#include "utils.h"

struct Client
{
    int sockfd; // -1 default, Socket file descriptor
    char username[NAME_LENGTH];
    int paired; // -1 if not paired, else number ID of paired client
    bool is_first_connection;
};

struct Client available_clients[MAX_CLIENTS]; // Store client connections
int num_clients = 0;

struct chat_message readMsg;
struct chat_message sendMsg;
struct stringify_result result;
char buffer[CHAT_MSG_MAXSIZE];

/* ASSOCIATED MUTEX FUNCTIONS
 *
 * pthread_mutex_lock - Lock the mutex to ensure exclusive access
 * pthread_mutex_unlock - Unlock the mutex to allow access to others
 */
pthread_mutex_t clients_mutex; // Mutex for protecting shared state

/**
 * @brief Sends a message to a specified client, including pre-join text.
 *
 * @param pre_join_text The initial text to prepend to the message.
 * @param message The actual message to send.
 * @param dest_sockfd The socket file descriptor of the destination client.
 */

/**
 * @brief Initializes the available clients array, setting all socket
 * descriptors to -1 and clearing usernames.
 */


void initialize_clients()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        available_clients[i].sockfd = -1;
        available_clients[i].paired = -1;
        available_clients[i].is_first_connection = true;
        memset(available_clients[i].username, 0, NAME_LENGTH);
    }
}

void debug_message(char *buffer, struct chat_message *msg)
{
    printf("-----------------\n");
    printf("BUFFER: %s\n", buffer);
    print_chat_message(msg);
    printf("-----------------\n");

}

void send_message(int sockfd, char *version, char *action, char *code, char *message, char *aditionalData[], int aditionalDataCount)
{
    bzero(buffer, sizeof(buffer));

    initialize_new_msg(&sendMsg);
    fill_chat_message(&sendMsg, version, action, code, message, aditionalData, aditionalDataCount);
    stringify(buffer, &sendMsg, &result);
    write(sockfd, buffer, strlen(buffer));

    free_chat_message(&sendMsg);
    bzero(buffer, sizeof(buffer));
}

void print_menu(int connfd)
{
    static char output[400];
    strcat(output, "===============================================\n");
    strcat(output, "        Welcome to the Chat Server!            \n");
    strcat(output, "===============================================\n");
    strcat(output, "Use the following commands to interact:\n\n");
    strcat(output, "LIST USERS           - List all available users.\n");
    strcat(output, "CONNECT <user_id>    - Connect to a user for chat.\n");
    strcat(output, "DISCONNECT           - Disconnect from the chat.\n");
    strcat(output, "MENU                 - Display menu.\n");
    strcat(output, "===============================================\n");

    char *aditionalData[] = {"E_MTD:SEND"};
    send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:100", output, aditionalData, 1);
}

void assign_username(int connfd, int client_index)
{

    char username[NAME_LENGTH] = {0}; // Buffer to store the username

    while (strlen(available_clients[client_index].username) == 0)
    {

        char *aditionalData[] = {"E_MTD:SEND"};
        send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:100", "Please assign yourself a username (Max 10 chars.)", aditionalData, 1);

        int n = read(connfd, buffer, sizeof(buffer) - 1);
        int resultParse = parse(buffer, &readMsg);

        debug_message(buffer, &readMsg);

        if (resultParse != -1)
        {
            // If message to long handle error [add]
            strncpy(username, readMsg.message, NAME_LENGTH);
            username[NAME_LENGTH - 1] = '\0';

            pthread_mutex_lock(&clients_mutex); // Lock before modifying
            strncpy(available_clients[client_index].username, username, NAME_LENGTH - 1);
            pthread_mutex_unlock(&clients_mutex); // Unlock after modifying

            print_menu(connfd);
        }
        else
        {
            printf("Error reading username from client %d\n", connfd);
            continue;
        }

        bzero(buffer, sizeof(buffer));
    }
} 

void list_users(int connfd)
{
    static char user_list[MAX_MSG];
    memset(user_list, 0, sizeof(user_list));

    pthread_mutex_lock(&clients_mutex);

    snprintf(user_list, MAX_MSG, "Available users:\n");

    int user_count = 0;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (available_clients[i].sockfd != -1 &&
            available_clients[i].paired == -1 &&
            available_clients[i].sockfd != connfd)
        {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "- User %d: %s", i + 1, available_clients[i].username);
            strncat(user_list, buffer, sizeof(user_list) - strlen(user_list) - 1);
            user_count++;
        }
    }

    if (user_count == 0)
    {
        strncat(user_list, "No users available\n", sizeof(user_list) - strlen(user_list) - 1);
    }

    pthread_mutex_unlock(&clients_mutex); // Unlock the mutex

    char msg[] = "\nConnect using: \n CONNECT <channel>\n";
    char *aditionalData[] = {"E_MTD:JOIN"};
    send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:100", strcat(user_list, msg), aditionalData, 1);
}

void select_user(int connfd, int client_index, int target_client)
{
    pthread_mutex_lock(&clients_mutex);

    if (target_client >= 0 && target_client < MAX_CLIENTS &&
        client_index != target_client &&
        available_clients[target_client].sockfd != -1 &&
        available_clients[target_client].paired == -1)
    {
        // Match clients
        available_clients[client_index].paired = target_client;
        available_clients[target_client].paired = client_index;

        char *aditionalData[] = {"E_MTD:SEND"};

        char msg[50];
        snprintf(msg, sizeof(msg), "You are now chatting with User %s\n", available_clients[target_client].username);
        send_message(available_clients[client_index].sockfd, "1.0", CHAT_ACTION_SEND, "CODE:100", msg, aditionalData, 1);

        snprintf(msg, sizeof(msg), "You are now chatting with %s\n", available_clients[client_index].username);
        send_message(available_clients[target_client].sockfd, "1.0", CHAT_ACTION_SEND, "CODE:100", msg, aditionalData, 1);

        available_clients[client_index].is_first_connection = false;
        available_clients[target_client].is_first_connection = false;
    }
    else
    {
        char *aditionalData[] = {"E_MTD:JOIN"};
        send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:400", "Invalid user selected.\n", aditionalData, 1);
        // write(connfd, "Invalid user selected.\n", 23);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void disconnect_user(int connfd, int client_index, void *connfd_ptr)
{

    printf("Disconnecting client %d at index %d\n", connfd, client_index);
    printf("Previous state: sockfd=%d, paired=%d\n", available_clients[client_index].sockfd, available_clients[client_index].paired);


    if (available_clients[client_index].paired != -1)
    {
        int paired_client = available_clients[client_index].paired;
        available_clients[paired_client].paired = -1;
        available_clients[paired_client].is_first_connection = true;

        char *aditionalData[] = {"E_MTD:JOIN"};
        send_message(available_clients[paired_client].sockfd, "1.0", CHAT_ACTION_SEND, "CODE:100", "User has left the chat.\n", aditionalData, 1);
    }

    available_clients[client_index].sockfd = -1;
    available_clients[client_index].paired = -1;
    available_clients[client_index].is_first_connection = true;
    memset(available_clients[client_index].username, 0, NAME_LENGTH);

    num_clients--;

    printf("Client %d disconnected\n", connfd);
    printf("New state: sockfd=%d, paired=%d\n", available_clients[client_index].sockfd, available_clients[client_index].paired);

    close(connfd);
    pthread_exit(connfd_ptr);
}

void *client_handler(void *connfd_ptr)
{
    int connfd = *((int *)connfd_ptr);
    char command[COMMAND_SIZE];
    int client_index = -1;

    // Find the index of this client in available_clients
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (available_clients[i].sockfd == connfd)
        {
            client_index = i;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    assign_username(connfd, client_index);

    // Infinite loop for chat
    for (;;)
    {
        bzero(buffer, CHAT_MSG_MAXSIZE);
        int bytes_read = read(connfd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
        {
            disconnect_user(connfd, client_index, connfd_ptr);
        }

        if (buffer[0] != '\0')
        {
            printf("\nReceived message: %s\n", buffer);

            int parseResult = parse(buffer, &readMsg);

            if (parseResult != -1)
            {
                if (strncasecmp(readMsg.action, CHAT_ACTION_GET, 4) == 0 )
                {
                    if (strncasecmp(readMsg.message, "LIST", 4) == 0)
                    {
                        list_users(connfd);
                    }
                    if (strncasecmp(readMsg.message, "MENU", 4) == 0)
                    {
                        printf("Menu requested\n");
                        print_menu(connfd);
                        printf("Menu sent\n");
                    }
                }
                else if (strncasecmp(readMsg.message, CHAT_MSG_CONNECT, 7) == 0)
                {
                    int target_client = atoi(readMsg.message + 8) - 1;

                    if (target_client < 0)
                    {
                        char *aditionalData[] = {"E_MTD:JOIN"};
                        send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:400", "Error: Invalid input. Please enter a valid number for the target client.", aditionalData, 1);
                    }
                    else
                    {
                        select_user(connfd, client_index, target_client);
                    }
                }
                else if (strncasecmp(readMsg.action, CHAT_ACTION_LEAVE, 4) == 0 || strncasecmp(readMsg.message, CHAT_MSG_DISCONNECT, 10) == 0)
                {
                    disconnect_user(connfd, client_index, connfd_ptr);
                }
                else if (available_clients[client_index].paired != -1)
                {
                    bzero(buffer, CHAT_MSG_MAXSIZE);
                    stringify(buffer, &readMsg, &result);
                    write(available_clients[available_clients[client_index].paired].sockfd, buffer, strlen(buffer));
                }
            }
            else
            {
                send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:400", "Error: Invalid message format", NULL, 0);
            }
        }
    }

    return NULL;
}

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // Initialize mutex
    pthread_mutex_init(&clients_mutex, NULL);
    initialize_clients();

    // Socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully created..\n");
    }
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Bind newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("Socket bind failed...\n");
        exit(0);
    }
    else
    {
        printf("Socket successfully binded..\n");
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
    {
        printf("Server listening..\n");
    }
    len = sizeof(cli);

    // Accept multiple clients and handle each in a new thread
    while (1)
    {
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
        if (connfd < 0)
        {
            printf("Server accept failed...\n");
            continue;
        }

        // Add the new client socket to the client list
        if (num_clients < MAX_CLIENTS)
        {
            pthread_mutex_lock(&clients_mutex);

            bool client_sucessfully_added = false;

            //Save new client
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                printf("Checking availability for socket %d\n", i);
                if (available_clients[i].sockfd == -1)
                {
                    printf("Socket %d free\n", i);
                    available_clients[i].sockfd = connfd;
                    num_clients++;
                    client_sucessfully_added = true;
                    break;
                }
                printf("Socket %d occupied by %s\n", i, available_clients[i].username);
            }

            if (!client_sucessfully_added)
            {
                printf("Error: Could not add client to available_clients\n");
            }
            else
            {
                printf("Client connected to channel: %d\n", connfd);
            }

            pthread_mutex_unlock(&clients_mutex);

            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_handler, &connfd);
            pthread_detach(client_thread); // Detach the thread to reclaim resources when it exits
        }
        else
        {
            char *aditionalData[] = { "E_MTD:SEND" };
            send_message(connfd, "1.0", CHAT_ACTION_SEND, "CODE:0", "Max clients reached, rejecting client...\n", aditionalData, 1);
            close(connfd);
        }
    }

    close(sockfd);

    pthread_mutex_destroy(&clients_mutex);
}
