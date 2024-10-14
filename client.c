
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close(), fork()
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

void printMenu() {
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

void chatReceiveLoop(int sockfd) {
    int n;
    while (1) {
        bzero(buff, sizeof(buff));
        initialize_new_msg(&readMsg); // Initialize readMsg for a new message

        if (read(sockfd, buff, sizeof(buff)) > 0) {
            printf(BLUE "--> %s" RESET, buff);
            int resultParse = parse(buff, &readMsg);
            if (resultParse == -1) {
                printf("Error parsing message\n");
            } else {
                // Handle received message here if needed
            }
        }
    }
}

void chatSendLoop(int sockfd) {
    while (1) {
        bzero(buff, sizeof(buff));
        fgets(buff, sizeof(buff), stdin); // Read input from the user

        if (strncmp(buff, "exit", 4) == 0) {
            printf(RED "Client Exit...\n" RESET);
            break;
        }

        // Prepare message for sending
        if (readMsg.action != NULL) {
            char *action = "SEND";
            printf("Additional data: %d\n", readMsg.num_additionalData);

            // Example: Just echoing SEND action for now
            if (readMsg.num_additionalData > 0) {
                if (strcmp(getStringValue(readMsg.additionalData[0]), CHAT_ACTION_SEND) == 0) {
                    char *additionalData[] = {"E_MTD:SEND"};
                    fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, additionalData, 1);
                }
                // Handle other actions similarly...
                stringify(buff, &sendMsg, &result);
                printf("------------\n");
                printf("Sending message: %s\n", buff);
                print_chat_message(&sendMsg);
                printf("------------\n");
                write(sockfd, buff, sizeof(buff));
            }
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf(RED "Socket creation failed...\n" RESET);
        exit(0);
    } else {
        printf(GREEN "Socket successfully created..\n" RESET);
    }
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP and PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // Connect the client to the server
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0) {
        printf(RED "Connection with the server failed...\n" RESET);
        exit(0);
    } else {
        printf(GREEN "Connected to the server..\n" RESET);
    }

    printMenu();

    // Create a child process to receive messages
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        close(sockfd);
        exit(1);
    } else if (pid == 0) {
        // Child process for receiving messages
        chatReceiveLoop(sockfd);
    } else {
        // Parent process for sending messages
        chatSendLoop(sockfd);
    }

    close(sockfd);
    return 0;
}
