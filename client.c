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

struct chat_message msg;
struct stringify_result result;
char buff[CHAT_MSG_MAXSIZE];

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
};

// Function to send messages to the server
void sendMessages(int sockfd, char *action)
{

    free_chat_message(&msg);
    initialize_new_msg(&msg);

    int n;
    while (1)
    {
        bzero(buff, sizeof(buff));
        n = 0;

        printf(WHITE);
        while ((buff[n++] = getchar()) != '\n')
            ;

        if (strcmp(action, CHAT_ACTION_SEND) == 0)
        {
            // MCP/1.0/SEND/CODE:0/E_MTD:SEND/MSG:<MSG>
            char *additionalData[] = {"E_MTD:SEND"};
            fill_chat_message(&msg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, additionalData, 1);
            stringify(buff, &msg, &result);
            write(sockfd, buff, sizeof(buff));
        }
        else if (strcmp(action, CHAT_ACTION_JOIN) == 0)
        {
            // MCP/1.0/JOIN/CODE:0/E_MTD:SEND/MSG:CONNECT 5
            char *additionalData[] = {"E_MTD:SEND"};
            fill_chat_message(&msg, "1.0", CHAT_ACTION_JOIN, "CODE:0", buff, additionalData, 1);
            stringify(buff, &msg, &result);
            write(sockfd, buff, sizeof(buff));
        }
        else if(strcmp(action, CHAT_ACTION_GET) == 0)
        {
            // MCP/1.0/GET/CODE:0/E_MTD:SEND/RESOURCE:LIST
            char *additionalData[] = {"E_MTD:SEND"};
            fill_chat_message(&msg, "1.0", CHAT_ACTION_GET, "CODE:0", buff, additionalData, 1);
            stringify(buff, &msg, &result);
            write(sockfd, buff, sizeof(buff));
        }
        else if(strcmp(action, CHAT_ACTION_LEAVE) == 0)
        {
            // MCP/1.0/LEAVE/CODE:0/E_MTD:SEND/MSG:DISCONNECT
            char *additionalData[] = {"E_MTD:..."};
            fill_chat_message(&msg, "1.0", CHAT_ACTION_LEAVE, "CODE:0", buff, additionalData, 1);
            stringify(buff, &msg, &result);
            write(sockfd, buff, sizeof(buff));
        }
        else
        {
            printf("Invalid action\n");
        }

        if (n > 1)
        {
            // Print sent messages in green
            printf(GREEN "<-- %s" RESET, buff);
            printf(WHITE); // Reset color to white for the next input
        }

        // If the client types "exit", the connection is closed
        if ((strncmp(buff, "exit", 4)) == 0)
        {
            printf(RED "Client Exit...\n" RESET);
            break;
        }
    }
}

// Function to receive messages from the server
void receiveMessages(int sockfd)
{
    initialize_new_msg(&msg);

    while (1)
    {
        bzero(buff, sizeof(buff));
        // Read message from server (relayed from another client)
        if (read(sockfd, buff, sizeof(buff)) > 0)
        {
            // Print received messages in blue

            int resultParse = parse(buff, &msg);
            bzero(buff, sizeof(buff));
            printf(BLUE "--> %s" RESET, msg.message);

            if (resultParse == 1)
            {
                printf("Error parsing message\n");
            }
            else
            {

                if (strcmp(msg.action, CHAT_ACTION_SEND) == 0 && strcmp(getStringValue(msg.additionalData[0]), "E_MTD:SEND") == 0)
                {
                    sendMessages(sockfd, CHAT_ACTION_SEND);
                }
                else if (strcmp(msg.action, CHAT_ACTION_SEND) == 0 && strcmp(getStringValue(msg.additionalData[0]), "E_MTD:JOIN") == 0)
                {
                    sendMessages(sockfd, CHAT_ACTION_JOIN);
                }
                printf(WHITE); // Reset color to white for the next input
            }
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

    void printMenu();

    // Start receiving messages
    if (fork() == 0)
    {
        receiveMessages(sockfd);
    }

    // Function to send messages to the server
    sendMessages(sockfd, CHAT_ACTION_SEND);

    // Close socket
    close(sockfd);

    return 0;
}