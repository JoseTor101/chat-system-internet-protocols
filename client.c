#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include "constants.h"
#include "app_layer/chat.h"
#include "utils.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

struct chat_message readMsg; // Static variable
struct chat_message sendMsg;
struct stringify_result result;
// char buff[CHAT_MSG_MAXSIZE];

void printMenu()
{
    printf("===============================================\n");
    printf("        Welcome to the Chat Server!            \n");
    printf("===============================================\n");
    printf("Use the following commands to interact:\n\n");

    printf("LIST USERS           - List all available users.\n");
    printf("CONNECT <user_id>    - Connect to a user for chat.\n");
    printf("SEND <message>       - Send a message to the current chat.\n");
    printf("DISCONNECT           - Disconnect from the chat.\n");
    printf("STATUS               - Show current chat status.\n");
    printf("HELP                 - Display help information.\n");

    printf("===============================================\n");
}

/*
void chatLoop(int sockfd)
{
    // printf("Receiving messages...\n");
    int n;
    for (;;)
    {
        bzero(buff, sizeof(buff));
        initialize_new_msg(&readMsg); // Initialize readMsg for a new message

        if (read(sockfd, buff, sizeof(buff)) > 0)
        {
            printf(BLUE "--> %s" RESET, buff);
            int resultParse = parse(buff, &readMsg);

            if (resultParse == -1)
            {
                printf("Error parsing message\n");
            }
            else
            {

                bzero(buff, sizeof(buff));
                n = 0;

                while ((buff[n++] = getchar()) != '\n')
                    ;

                if (readMsg.action != NULL)
                {

                    char *action = "SEND";
                    printf("Aditional data: %d\n", readMsg.num_additionalData);

                    if (readMsg.num_additionalData > 0)
                    {
                        if (strcmp(getStringValue(readMsg.additionalData[0]), CHAT_ACTION_SEND) == 0)
                        {
                            char *additionalData[] = {"E_MTD:SEND"};
                            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, additionalData, 1);
                        }
                        else if (strcmp(getStringValue(readMsg.additionalData[0]), CHAT_ACTION_JOIN) == 0)
                        {
                            char *additionalData[] = {"E_MTD:SEND"};
                            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_JOIN, "CODE:0", buff, additionalData, 1);
                        }
                        else if (strcmp(getStringValue(readMsg.additionalData[0]), CHAT_ACTION_GET) == 0)
                        {
                            char *additionalData[] = {"E_MTD:SEND"};
                            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_GET, "CODE:0", buff, additionalData, 1);
                        }
                        else if (strcmp(getStringValue(readMsg.additionalData[0]), CHAT_ACTION_LEAVE) == 0)
                        {
                            char *additionalData[] = {"E_MTD:..."};
                            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_LEAVE, "CODE:0", buff, additionalData, 1);
                        }
                        else
                        {
                            printf("Invalid action\n");
                        }

                        stringify(buff, &sendMsg, &result);
                        printf("------------\n");
                        printf("Sending message: %s\n", buff);
                        print_chat_message(&sendMsg);
                        printf("------------\n");

                        write(sockfd, buff, sizeof(buff));
                        // Exit command

                        if ((strncmp(buff, "exit", 4)) == 0)
                        {
                            printf(RED "Client Exit...\n" RESET);
                            break;
                        }
                    }
                }
            }
        }
    }
}
*/

void receiveChatMessages(int sockfd)
{
    char buff[CHAT_MSG_MAXSIZE];

    while (1)
    {
        bzero(buff, sizeof(buff));
        // Read message from server (relayed from another client)
        if (read(sockfd, buff, sizeof(buff)) > 0)
        {
            // Print received messages in blue
            printf(BLUE "--> %s" RESET, buff);
            printf(WHITE); // Reset color to white for the next input
        }
    }
}

void sendChatMessages(int sockfd)
{
    char buff[CHAT_MSG_MAXSIZE];
    int n;
    while (1)
    {
        bzero(buff, sizeof(buff));
        n = 0;

        while ((buff[n++] = getchar()) != '\n')
            ;

        if (n > 1) {
            initialize_new_msg(&sendMsg);
            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, NULL, 0);
            stringify(buff, &sendMsg, &result);
            write(sockfd, buff, sizeof(buff));
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf(RED "Socket creation failed...\n" RESET);
        exit(0);
    }
    else
    {
        printf(GREEN "Socket successfully created..\n" RESET);
    }
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Connect the client to the server
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf(RED "Connection with the server failed...\n" RESET);
        exit(0);
    }
    else
    {
        printf(GREEN "Connected to the server..\n" RESET);
    }

    printMenu();

    // chatLoop(sockfd); // Child process to receive messages

    if (fork() == 0)
    {
        receiveChatMessages(sockfd);
    }

    sendChatMessages(sockfd);

    close(sockfd);

    return 0;
}

/*
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <sys/shm.h>  // Shared memory headers
#include <sys/ipc.h>  // Key generation for shared memory
#include <unistd.h> // read(), write(), close()
#include "constants.h"
#include "app_layer/chat.h"
#include "utils.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

struct chat_message readMsg; // Static variable
struct chat_message sendMsg;
struct stringify_result result;

// Shared memory structure
struct shared_data {
    char received_message[CHAT_MSG_MAXSIZE];
    int newMessageFlag; // 1 if a new message was received, 0 otherwise
};

void printMenu()
{
    printf("===============================================\n");
    printf("        Welcome to the Chat Server!            \n");
    printf("===============================================\n");
    printf("Use the following commands to interact:\n\n");

    printf("LIST USERS           - List all available users.\n");
    printf("CONNECT <user_id>    - Connect to a user for chat.\n");
    printf("SEND <message>       - Send a message to the current chat.\n");
    printf("DISCONNECT           - Disconnect from the chat.\n");
    printf("STATUS               - Show current chat status.\n");
    printf("HELP                 - Display help information.\n");

    printf("===============================================\n");
}

void receiveChatMessages(int sockfd, struct shared_data *shared)
{
    char buff[CHAT_MSG_MAXSIZE];

    while (1)
    {
        bzero(buff, sizeof(buff));
        // Read message from server (relayed from another client)
        if (read(sockfd, buff, sizeof(buff)) > 0)
        {
            // Write received message to shared memory
            strcpy(shared->received_message, buff);
            shared->newMessageFlag = 1; // Indicate new message received

            // Print received messages in blue
            //printf(BLUE "--> %s" RESET, buff);
            //printf(WHITE); // Reset color to white for the next input
        }
    }
}

void sendChatMessages(int sockfd, struct shared_data *shared)
{
    char buff[CHAT_MSG_MAXSIZE];
    int n;

    while (1)
    {
        // Check if a new message was received
        if (shared->newMessageFlag)
        {
            printf(GREEN "New message received: %s\n" RESET, shared->received_message);
            shared->newMessageFlag = 0; // Reset flag after reading
            //initialize_new_msg(&sendMsg);
        }

        // Send a new message
        bzero(buff, sizeof(buff));
        n = 0;

        while ((buff[n++] = getchar()) != '\n')
            ;

        if (n > 1)
        {
            initialize_new_msg(&sendMsg);
            fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, NULL, 0);
            stringify(buff, &sendMsg, &result);
            write(sockfd, buff, sizeof(buff));
        }
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf(RED "Socket creation failed...\n" RESET);
        exit(0);
    }
    else
    {
        printf(GREEN "Socket successfully created..\n" RESET);
    }
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Connect the client to the server
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf(RED "Connection with the server failed...\n" RESET);
        exit(0);
    }
    else
    {
        printf(GREEN "Connected to the server..\n" RESET);
    }

    printMenu();

    // Shared memory setup
    key_t key = ftok("shmfile", 65); // Create a unique key
    int shmid = shmget(key, sizeof(struct shared_data), 0666 | IPC_CREAT); // Create shared memory segment

    // Attach shared memory
    struct shared_data *shared = (struct shared_data *)shmat(shmid, (void *)0, 0);
    shared->newMessageFlag = 0; // Initialize flag

    // Fork to handle sending and receiving in parallel
    if (fork() == 0)
    {
        // Child process: receive messages
        receiveChatMessages(sockfd, shared);
    }
    else
    {
        // Parent process: send messages
        sendChatMessages(sockfd, shared);
    }

    // Detach shared memory
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);

    close(sockfd);

    return 0;
}*/

