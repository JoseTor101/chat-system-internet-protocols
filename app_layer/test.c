#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    LEAVE,
    SEND,
    LEVEL_EXIT
} chat_action;

typedef enum {
    CODE_0,
    E_MTD,
    LEVEL_EXIT_0,
    LEVEL_EXIT_1
} action_status;

struct chat_message {
    char *protocolVersion;
    chat_action action;
    action_status status;
    char *message;
    char *additionalData;
    int num_params;
    char *params[]; // Flexible array member for variable number of parameters
};

// Function to create a new chat_message
struct chat_message* create_chat_message(const char *protocolVersion, chat_action action, action_status status, const char *message, const char *additionalData, int num_params, char *params[]) {
    struct chat_message *msg = malloc(sizeof(struct chat_message) + num_params * sizeof(char *));
    if (!msg) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    msg->protocolVersion = strdup(protocolVersion);
    msg->action = action;
    msg->status = status;
    msg->message = strdup(message);
    msg->additionalData = strdup(additionalData);
    msg->num_params = num_params;

    for (int i = 0; i < num_params; i++) {
        msg->params[i] = strdup(params[i]);
    }

    return msg;
}

// Function to free a chat_message
void free_chat_message(struct chat_message *msg) {
    free(msg->protocolVersion);
    free(msg->message);
    free(msg->additionalData);
    for (int i = 0; i < msg->num_params; i++) {
        free(msg->params[i]);
    }
    free(msg);
}

int main() {
    char *params[] = {"MCP/1.0", "LEAVE", "CODE:0", "E_MTD:SEND", "LEVEL_EXIT:0|1"};
    struct chat_message *msg = create_chat_message("MCP/1.0", LEAVE, CODE_0, "Test message", "Additional data", 5, params);

    // Print the message details
    printf("Protocol Version: %s\n", msg->protocolVersion);
    printf("Action: %d\n", msg->action);
    printf("Status: %d\n", msg->status);
    printf("Message: %s\n", msg->message);
    printf("Additional Data: %s\n", msg->additionalData);
    printf("Parameters:\n");
    for (int i = 0; i < msg->num_params; i++) {
        printf("  %s\n", msg->params[i]);
    }

    // Free the message
    free_chat_message(msg);

    return 0;
}