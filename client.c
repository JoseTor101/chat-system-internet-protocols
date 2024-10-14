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

static struct chat_message readMsg; // Static variable
struct chat_message sendMsg;
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
}

void latestMessage()
{
    printf("Latest message: %s\n", readMsg.message);
    print_chat_message(&readMsg);
}

// Function to send messages to the server
void sendMessages(int sockfd)
{
    free_chat_message(&sendMsg);
    initialize_new_msg(&sendMsg);

    printf("Preparing to send");
    int n;
    while (1)
    {
        bzero(buff, sizeof(buff));
        n = 0;

        while ((buff[n++] = getchar()) != '\n')
            ;

        // Print the current value of readMsg
        latestMessage();

        // Check the action of readMsg
        if (readMsg.action != NULL)
        {
            char *a = getStringValue(readMsg.additionalData[0]);
            printf("DATO: %s", a);

            if (strcmp(a, CHAT_ACTION_SEND) == 0)
            {
                char *additionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, additionalData, 1);
                stringify(buff, &sendMsg, &result);
                write(sockfd, buff, sizeof(buff));
            }
            else if (strcmp(a, CHAT_ACTION_JOIN) == 0)
            {
                char *additionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_JOIN, "CODE:0", buff, additionalData, 1);
                stringify(buff, &sendMsg, &result);
                write(sockfd, buff, sizeof(buff));
            }
            else if (strcmp(a, CHAT_ACTION_GET) == 0)
            {
                char *additionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_GET, "CODE:0", buff, additionalData, 1);
                stringify(buff, &sendMsg, &result);
                write(sockfd, buff, sizeof(buff));
            }
            else if (strcmp(a, CHAT_ACTION_LEAVE) == 0)
            {
                char *additionalData[] = {"E_MTD:..."}; // Just an example
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_LEAVE, "CODE:0", buff, additionalData, 1);
                stringify(buff, &sendMsg, &result);
                write(sockfd, buff, sizeof(buff));
            }
            else
            {
                printf("Invalid action\n");
            }

            if (n > 1)
            {
                printf(GREEN "<-- %s" RESET, buff);
                printf(WHITE); // Reset color to white for the next input
            }

            // Exit command
            if ((strncmp(buff, "exit", 4)) == 0)
            {
                printf(RED "Client Exit...\n" RESET);
                break;
            }
        }
    }
}


void receiveMessages(int sockfd)
{
    // printf("Receiving messages...\n");
    int n;
    for(;;)
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

            
                    if (strcmp(action, CHAT_ACTION_SEND) == 0)
                    {
                        char *additionalData[] = {"E_MTD:SEND"};
                        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0",buff , additionalData, 1);
                        stringify(buff, &sendMsg, &result);
                    }
                    else if (strcmp(action, CHAT_ACTION_JOIN) == 0)
                    {
                        char *additionalData[] = {"E_MTD:SEND"};
                        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_JOIN, "CODE:0", buff, additionalData, 1);
                        stringify(buff, &sendMsg, &result);
                        
                    }
                    else if (strcmp(action, CHAT_ACTION_GET) == 0)
                    {
                        char *additionalData[] = {"E_MTD:SEND"};
                        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_GET, "CODE:0", buff, additionalData, 1);
                        stringify(buff, &sendMsg, &result);
                        
                    }
                    else if (strcmp(action, CHAT_ACTION_LEAVE) == 0)
                    {
                        char *additionalData[] = {"E_MTD:..."}; // Just an example
                        fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_LEAVE, "CODE:0", buff, additionalData, 1);
                        stringify(buff, &sendMsg, &result);
                    }
                    else
                    {
                        printf("Invalid action\n");
                    }

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
        // print_chat_message(&readMsg);
    }
}

/*
void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        printf("Enter the string : ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n')
            ;
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server : %s", buff);
        if ((strncmp(buff, "exit", 4)) == 0) {
            printf("Client Exit...\n");
            break;
        }
    }
}
*/


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

    // Start receiving messages
    // if (fork() == 0)
    receiveMessages(sockfd); // Child process to receive messages

    //func(sockfd);
    // Function to send messages to the server
    // sendMessages(sockfd); // Parent process to send messages

    // Close socket
    close(sockfd);

    return 0;
}
