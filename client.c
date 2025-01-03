#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <pthread.h> // pthread for concurrency
#include "queue.h" 
#include "constants.h"
#include "app_layer/chat.h"
#include "utils.h"
#define SA struct sockaddr

struct chat_message readMsg;
struct chat_message sendMsg;
struct stringify_result result;
char buff[CHAT_MSG_MAXSIZE];  

queue_t* message_queue; 

queue_t* action_queue;

void* send_chat_messages(void* sockfd_ptr) {
    int sockfd = *(int*) sockfd_ptr;
    while (1) {
        bzero(buff, sizeof(buff));
        fgets(buff, sizeof(buff), stdin);

        if (strlen(buff) > 1) {
            initialize_new_msg(&sendMsg);
            if(strncasecmp(buff, "LIST", 4) == 0 || strncasecmp(buff, "MENU", 4) == 0){
                char *aditionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_GET, "CODE:0", buff, aditionalData, 1);
            }else if(strncasecmp(buff, "CONNECT", 7) == 0){
                char *aditionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_JOIN, "CODE:0", buff, aditionalData, 1);
            }else if(strncasecmp(buff, "DISCONNECT", 10) == 0){
                char *aditionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_LEAVE, "CODE:0", buff, aditionalData, 1);
            }else{
                char *aditionalData[] = {"E_MTD:SEND"};
                fill_chat_message(&sendMsg, "1.0", CHAT_ACTION_SEND, "CODE:0", buff, aditionalData, 1);
            }
            stringify(buff, &sendMsg, &result);
            write(sockfd, buff, sizeof(buff));
        }
    }
    return NULL;
}

void *process_messages()
{
    char* next_action;
    while (1)
    {
        if (!is_queue_empty(message_queue))
        {
            char *message = dequeue(message_queue);
            if (message != NULL)
            {
                free_chat_message(&readMsg);
                initialize_new_msg(&readMsg);
                int parseResult = parse(message, &readMsg);
                if (parseResult != 0)
                {
                    printf("Error: Invalid message format\n");
                }
                else
                {
                    printf(BLUE ">: %s" RESET, readMsg.message);
                    free(message);
                }
            }
        }

        sleep(1); // Prevent busy-waiting
    }
    return NULL;
}

void* receive_chat_messages(void* sockfd_ptr) {
    int sockfd = *(int*) sockfd_ptr;
    char recv_buff[CHAT_MSG_MAXSIZE];

    while (1) {
        bzero(recv_buff, sizeof(recv_buff));
        if (read(sockfd, recv_buff, sizeof(recv_buff)) > 0) {
            char* message_copy = strdup(recv_buff); 
            enqueue(message_queue, message_copy);
        }
    }
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    message_queue = create_queue();
    action_queue = create_queue();

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf(RED "Socket creation failed...\n" RESET);
        exit(0);
    }
    else {
        printf(GREEN "Socket successfully created..\n" RESET);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf(RED "Connection with the server failed...\n" RESET);
        exit(0);
    }
    else {
        printf(GREEN "Connected to the server..\n" RESET);
    }

    pthread_t recv_thread, send_thread, process_thread;

    pthread_create(&recv_thread, NULL, receive_chat_messages, &sockfd);
    pthread_create(&process_thread, NULL, process_messages, &sockfd);
    pthread_create(&send_thread, NULL, send_chat_messages, &sockfd);

    pthread_join(recv_thread, NULL);
    pthread_join(process_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sockfd);
    return 0;
}
