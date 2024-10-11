#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include "constants.h"

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

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

// Function to receive messages from the server
void receiveMessages(int sockfd)
{
    char buff[MAX];
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
};

// Function to send messages to the server
void sendMessages(int sockfd)
{
    char buff[MAX];
    int n;
    while (1)
    {
        bzero(buff, sizeof(buff));
        n = 0;

        printf(WHITE);
        while ((buff[n++] = getchar()) != '\n')
            ;

        write(sockfd, buff, sizeof(buff));

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
};

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
    sendMessages(sockfd);

    // Close socket
    close(sockfd);

    return 0;
}