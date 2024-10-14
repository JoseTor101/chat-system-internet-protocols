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
#include "methods.h"
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

void initialize_clients();

/**
 * @brief Assigns a username to the client connected at the specified
 * socket file descriptor. Prompts the user until a valid username
 * is entered.
 *
 * @param connfd The socket file descriptor of the client.
 */
void assign_username(int connfd)
{

    char username[NAME_LENGTH] = {0}; // Buffer to store the username

    while (strlen(available_clients[num_clients - 1].username) == 0)
    {

        initialize_new_msg(&sendMsg);
        char *aditionalData[] = {"E_MTD:SEND"};

        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:100", "Please assign yourself a username (Max 10 chars.)", aditionalData, 1);
        stringify(buffer, &sendMsg, &result);

        write(connfd, buffer, strlen(buffer));

        bzero(buffer, sizeof(buffer));
        free_chat_message(&sendMsg);

        int n = read(connfd, buffer, sizeof(buffer) - 1); // Read username from socket
        initialize_new_msg(&readMsg);

        int resultParse = parse(buffer, &readMsg);

        print_chat_message(&readMsg);

        if (resultParse != -1)
        {
            // If message to long handle error [add]
            strcpy(username, readMsg.message);
            username[n - 1] = '\0';

            pthread_mutex_lock(&clients_mutex); // Lock before modifying
            strncpy(available_clients[num_clients - 1].username, username, NAME_LENGTH - 1);
            pthread_mutex_unlock(&clients_mutex); // Unlock after modifying
        }
        else
        {
            printf("Error reading username from client %d\n", connfd);
            continue;
        }

        bzero(buffer, sizeof(buffer));

        char welcome_msg[70];
        initialize_new_msg(&sendMsg);
        char *aditionalData1[] = {"E_MTD:GET"};
        snprintf(welcome_msg, sizeof(welcome_msg), "Welcome, %s\nWrite LIST to show available users", username);

        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:100", welcome_msg, aditionalData1, 1);
        stringify(buffer, &sendMsg, &result);
        write(connfd, buffer, strlen(buffer));

        free_chat_message(&readMsg);
        bzero(buffer, sizeof(buffer));
    }
}

/**
 * @brief Sends a list of available users to the specified client.
 *
 * @param connfd The socket file descriptor of the client.
 */
void list_users(int connfd)
{
    static char user_list[MAX_MSG];          // Static buffer for the user list
    memset(user_list, 0, sizeof(user_list)); // Clear the buffer

    pthread_mutex_lock(&clients_mutex);

    snprintf(user_list, MAX_MSG, "Available users:\n");

    int user_count = 0;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (available_clients[i].sockfd != -1 &&
            available_clients[i].paired == -1
            )
        {
            //  && available_clients[i].sockfd != connfd
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

    initialize_new_msg(&sendMsg);
    char *aditionalData[] = {"E_MTD:JOIN"};
    fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:100", strcat(user_list, msg), aditionalData, 1);
    stringify(buffer, &sendMsg, &result);
    write(connfd, buffer, sizeof(buffer));

    bzero(buffer, sizeof(buffer));
    free_chat_message(&sendMsg);
}

/**
 * @brief Selects a user for chatting by matching the current client
 * with the target client.
 *
 * @param connfd The socket file descriptor of the client requesting to
 * select another user.
 * @param client_index The index of the current client in the available
 * clients array.
 * @param target_client The index of the target client to chat with.
 */

void select_user(int connfd, int client_index, int target_client)
{
    pthread_mutex_lock(&clients_mutex);
    initialize_new_msg(&sendMsg);

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
        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:100", msg, aditionalData, 1);
        stringify(buffer, &sendMsg, &result);
        write(available_clients[client_index].sockfd, buffer, strlen(buffer));

        free_chat_message(&sendMsg);
        bzero(buffer, sizeof(buffer));
        initialize_new_msg(&sendMsg);

        snprintf(msg, sizeof(msg), "You are now chatting with %s\n", available_clients[client_index].username);
        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:100", msg, aditionalData, 1);
        stringify(buffer, &sendMsg, &result);
        write(available_clients[target_client].sockfd, buffer, strlen(buffer));

        available_clients[client_index].is_first_connection = false;
        available_clients[target_client].is_first_connection = false;
    }
    else
    {
        char *aditionalData[] = {"E_MTD:JOIN"};

        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:400", "Invalid user selected.\n", aditionalData, 1);
        stringify(buffer, &sendMsg, &result);
        write(connfd, buffer, strlen(buffer));
        //write(connfd, "Invalid user selected.\n", 23);
    }
    free_chat_message(&sendMsg);
    bzero(buffer, sizeof(buffer));

    pthread_mutex_unlock(&clients_mutex);

}

/**
 * @brief The main handler for each client thread. Responsible for
 * managing user interactions, message handling, and user selection.
 *
 * @param connfd_ptr A pointer to the socket file descriptor of the
 * client.
 * @return NULL upon completion.
 */

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

    assign_username(connfd);
    // list_users(connfd);

    // Infinite loop for chat
    for (;;)
    {
        bzero(buffer, CHAT_MSG_MAXSIZE);
        int bytes_read = read(connfd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
        {
            // available_clients[client_index].paired = -1;
            // available_clients[client_index].is_first_connection = true;
            // available_clients[client_index].sockfd = -1;
            printf("Client disconnected...\n");
            close(connfd);
            pthread_exit(NULL);
        }

        if (buffer[0] != '\0')
        {
            printf("\nReceived message: %s\n", buffer);

            int parseResult = parse(buffer, &readMsg);

            if (parseResult != -1)
            {
                if (strncasecmp(readMsg.message, "LIST", 4) == 0)
                {
                    printf("Message, %s\n", readMsg.message);
                    list_users(connfd);
                }
                else if (strncasecmp(readMsg.message, "CONNECT", 7) == 0)
                {
                    int target_client = atoi(readMsg.message + 8) - 1;

                    if(target_client < 0){
                        initialize_new_msg(&sendMsg);
                        char *aditionalData[] = {"E_MTD:JOIN"};
                        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:400", "Error: Invalid input. Please enter a valid number for the target client.", aditionalData, 1);
                        stringify(buffer, &sendMsg, &result);
                        write(connfd, buffer, strlen(buffer));
                        free_chat_message(&sendMsg);
                        bzero(buffer, sizeof(buffer));
                    }
                    else
                    {
                        select_user(connfd, client_index, target_client);
                    }
                }else if(available_clients[client_index].paired != -1)
                {
                    bzero(buffer, CHAT_MSG_MAXSIZE);
                    stringify(buffer, &readMsg, &result);

                    printf("------------\n");

                    printf("BUFFER: %s\n", buffer);
                    print_chat_message(&readMsg);
                     
                    printf("SOCKET: %d\n", available_clients[client_index].sockfd);
                    printf("USERNAME: %s\n", available_clients[client_index].username);
                    printf("PAIRED: %d\n", available_clients[client_index].paired);
                    printf("SOCKET: %d\n", available_clients[available_clients[client_index].paired].sockfd);
                    printf("USERNAME: %s\n", available_clients[available_clients[client_index].paired].username);
                    printf("PAIRED: %d\n", available_clients[available_clients[client_index].paired].paired);

                    printf("\n------------");

                    write(available_clients[available_clients[client_index].paired].sockfd, buffer, strlen(buffer));
                }
            }
            else
            {
                write(connfd, "Error: Invalid message format\n", 30);
            }
        }

        /*if(available_clients[client_index].paired != -1)
        {
            char message[MAX_MSG];
            snprintf(message, sizeof(message), "Message from %s: %.*s",
                    available_clients[client_index].username,
                    (int)(MAX_MSG - (14 + strlen(available_clients[client_index].username))),
                    buff);

            int target = available_clients[client_index].paired;
            write(available_clients[target].sockfd, message, strlen(message));
        }*/

        //  buff = "MCP/1.0/GET/CODE:0/E_MTD:SEND/RESOURCE:<resource>"

        // char *newMsg = readMessage(buff, client_index);
        // char *newMsg = readMessage("MCP/1.0/GET/CODE:0/E_MTD:SEND/RESOURCE:LIST", client_index);
        // write(connfd, newMsg, sizeof(newMsg));

        // Check command
        // extract_command(buff, command, sizeof(command));
        // parseRequest(buff);

        // SELECT USER
        /*
            if (strncmp(command, "select", 6) == 0)
            {
                int target_client = atoi(buff + 7) - 1; // Get user number from the message

                select_user(connfd, client_index, target_client);
            }
            // LIST USERS
            else if (strncmp(command, "list", 4) == 0)
            {
                list_users(connfd);
            }
            else if (strncmp(command, "exit", 4) == 0)
            {
                if (available_clients[client_index].paired != -1)
                {
                    int target = available_clients[client_index].paired;
                    char msg[50];
                    snprintf(msg, sizeof(msg), "%s has left the chat.\n", available_clients[client_index].username);
                    write(available_clients[target].sockfd, msg, strlen(msg));
                }

                pthread_mutex_lock(&clients_mutex);
                available_clients[client_index].paired = -1;
                close(available_clients[client_index].sockfd);
                pthread_mutex_unlock(&clients_mutex);
                printf("Client %s disconnected...\n", available_clients[client_index].username);
                pthread_exit(NULL);
            }
            // MSG TO PAIRED USER
            else if (available_clients[client_index].paired != -1)
            {

                char message[MAX_MSG];

                snprintf(message, sizeof(message), "Message from %s: %.*s",
                        available_clients[client_index].username,
                        (int)(MAX_MSG - (14 + strlen(available_clients[client_index].username))),
                        buff);

                int target = available_clients[client_index].paired;
                write(available_clients[target].sockfd, message, strlen(message));
            }
            // DESIRED ALREADY PAIRED
            else if (available_clients[client_index].paired == -1)
            {
                if (!available_clients[client_index].is_first_connection)
                {
                    char msg[] = "User busy\n";
                    write(connfd, msg, sizeof(msg));
                }
            }
            else if (strncmp(command, "exit", 4))
            {
            }
            else
            {
                write(connfd, "Please select a user to chat with using 'select <user_number>'\n", 67);
            }
        */
    }

    return NULL;
}

/**
 * @brief The main function of the server. Sets up the server socket,
 * listens for incoming connections, and creates a new thread for each
 * connected client.
 *
 * @return 0 on successful execution.
 */

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // Initialize mutex
    pthread_mutex_init(&clients_mutex, NULL);
    initialize_clients(); // Initialize available clients

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
            available_clients[num_clients].sockfd = connfd;
            num_clients++;
            pthread_mutex_unlock(&clients_mutex);
            printf("Client connected to channel: %d\n", connfd);

            pthread_t client_thread;
            pthread_create(&client_thread, NULL, client_handler, &connfd);
            pthread_detach(client_thread); // Detach the thread to reclaim resources when it exits
        }
        else
        {
            printf("Max clients reached, rejecting client...\n");
            close(connfd);
        }
    }

    close(sockfd);

    pthread_mutex_destroy(&clients_mutex);
}
