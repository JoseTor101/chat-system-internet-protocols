#include "chat.h"
#include <stdio.h>

int main() {
    // Create a chat_message instance
    struct chat_message msg;
    initialize_new_msg(&msg);

    // Test data
    char *protocolVersion = "MCP/1.0";
    char *action = "SEND";
    char *status = "CODE:0";
    char *message = "Hello, World!";
    char *additionalData[] = {"E_MTD:SEND", "E_MTD:JOIN"};

    // Fill the chat message
    fill_chat_message(&msg, protocolVersion, action, status, message, additionalData, 2);

    // Print the filled message
    printf("Protocol Version: %s\n", msg.protocolVersion);
    printf("Action: %s\n", msg.action);
    printf("Action Length: %d\n", msg.action_length);
    printf("Status: %s\n", msg.status);
    printf("Status Length: %d\n", msg.status_length);
    printf("Message: %s\n", msg.message);
    printf("Message Length: %d\n", msg.message_length);
    printf("Number of Additional Data: %d\n", msg.num_additionalData);
    for (int i = 0; i < msg.num_additionalData; i++) {
        printf("Additional Data [%d]: %s\n", i, msg.additionalData[i]);
    }
    char buffer[CHAT_MSG_MAXSIZE];
    struct stringify_result result;
    stringify(buffer, &msg, &result);
    
    printf("Stringified message: %s\n", buffer);

    // Free allocated memory
    free_chat_message(&msg);
    return 0;
}
