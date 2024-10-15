#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "chat.h"
#include "../server.h"
#include <string.h>
#include "../utils.h"

const char *ActionOptions[] = {
    CHAT_ACTION_JOIN,
    CHAT_ACTION_LEAVE,
    CHAT_ACTION_GET,
    CHAT_ACTION_SEND,
    CHAT_ACTION_EMPTY,
    NULL};

const char *JoinStatus[] = {
    CHAT_JOIN_SUCCESS,
    CHAT_JOIN_FAIL,
    NULL};

const char *LeaveStatus[] = {
    CHAT_LEAVE_SUCCESS,
    CHAT_LEAVE_FAIL,
    NULL};

const char *GetStatus[] = {
    CHAT_GET_SUCCESS,
    CHAT_GET_BAD_ACTION,
    CHAT_GET_BAD_OPTION,
    CHAT_GET_EMPTY,
    NULL};

const char *SendStatus[] = {
    CHAT_SEND_SUCCESS,
    CHAT_SEND_FAIL,
    NULL};

int current_buffer_pos = 0;

void buffer_write_header(int isAction, char buffer[], struct chat_message *msg, const char *methodStatus[], struct stringify_result *result)
{
    const char *selected_string = isAction ? msg->action : msg->status; // Choose action or option 1=action 0=option
    int selected_length = isAction ? msg->action_length : msg->status_length;

    for (int i = 0; methodStatus[i] != NULL; i++)
    {
        if (strcmp(selected_string, methodStatus[i]) == 0)
        {
            strncpy(&buffer[current_buffer_pos], selected_string, selected_length);
            current_buffer_pos += selected_length;
            buffer[current_buffer_pos++] = '/';
            return;
        }
    }
    if (buffer[current_buffer_pos] == '\0')
    {
        result->reply = CHAT_GET_BAD_OPTION;
    }
}

void stringify_result_factory(struct stringify_result *result)
{
    result->length = 0;
    result->reply = CHAT_GET_EMPTY;
};

void stringify(
    char *buffer,
    struct chat_message *msg,
    struct stringify_result *result)
{
    stringify_result_factory(result);
    current_buffer_pos = 0;

    // Protocol version
    const char *protocol_prefix = "MCP/";
    int prefix_length = strlen(protocol_prefix);
    strncpy(buffer + current_buffer_pos, protocol_prefix, prefix_length);
    current_buffer_pos += prefix_length;

    int protocol_length = strlen(msg->protocolVersion);
    strncpy(buffer + current_buffer_pos, msg->protocolVersion, protocol_length);
    current_buffer_pos += protocol_length;
    buffer[current_buffer_pos++] = '/';

    // Add action to the buffer
    buffer_write_header(1, buffer, msg, ActionOptions, result);

    // Add status to the buffer
    if (strcmp(msg->action, CHAT_ACTION_JOIN) == 0)
    {
        buffer_write_header(0, buffer, msg, JoinStatus, result);
    }
    else if (strcmp(msg->action, CHAT_ACTION_LEAVE) == 0)
    {
        buffer_write_header(0, buffer, msg, LeaveStatus, result);
    }
    else if (strcmp(msg->action, CHAT_ACTION_GET) == 0)
    {
        buffer_write_header(0, buffer, msg, GetStatus, result);
    }
    else if (strcmp(msg->action, CHAT_ACTION_SEND) == 0)
    {
        buffer_write_header(0, buffer, msg, SendStatus, result);
    }
    else if (strcmp(msg->action, CHAT_ACTION_EMPTY) == 0)
    {
        result->reply = CHAT_GET_BAD_ACTION;
        free(buffer);
        printf("Error: Action is empty\n");
        return;
    }

    // Append status code
    strncpy(buffer + current_buffer_pos, msg->status, msg->status_length);
    current_buffer_pos += msg->status_length;
    buffer[current_buffer_pos++] = '/';

    // Add additional data to the buffer
    for (int i = 0; i < msg->num_additionalData; i++)
    {
        if (msg->additionalData[i] != NULL)
        {
            strncpy(&buffer[current_buffer_pos], msg->additionalData[i], strlen(msg->additionalData[i]));
            current_buffer_pos += strlen(msg->additionalData[i]);
            buffer[current_buffer_pos++] = '/';
        }
    }


    // Add message to the buffer
    if (msg->message_length > CHAT_MSG_MAXSIZE - current_buffer_pos - 1)
    { // -1 for the newline
        result->reply = CHAT_GET_BAD_MSG;
        free(buffer);
        return;
    }
    else
    {
        // Add the message to the buffer
        strncpy(&buffer[current_buffer_pos], "MSG:\r\n", 6);
        current_buffer_pos += 6;

        strncpy(&buffer[current_buffer_pos], msg->message, msg->message_length);
        current_buffer_pos += msg->message_length;

        buffer[current_buffer_pos] = '\n';
        current_buffer_pos++;
    }

    buffer[current_buffer_pos] = '\0';

    return;
}

int parse(char buffer[], struct chat_message *msg)
{
    if (buffer == NULL || msg == NULL)
        return -1;

    char *ptr = buffer, *end;

    if (strncmp(ptr, "MCP", 3) != 0 || ptr[3] != '/')
        return -1;
    ptr += 4;

    // Protocol version
    end = strchr(ptr, '/');
    if (!end)
        return -1;
    msg->protocolVersion = strndup(ptr, end - ptr);
    ptr = end + 1;

    // Action
    end = strchr(ptr, '/');
    
    if (!end)
        return -1;
    msg->action = strndup(ptr, end - ptr);
    msg->action_length = end - ptr;
    ptr = end + 1;

    // Status
    end = strchr(ptr, '/');
    if (!end)
        return -1;
    msg->status = strndup(ptr, end - ptr);
    msg->status_length = end - ptr;
    ptr = end + 1;

    // Additional data
    msg->num_additionalData = 0;
    while ((end = strchr(ptr, '/')) != NULL)
    {
        if (msg->num_additionalData >= MAX_ADDITIONAL_DATA)
            return -1;

        msg->additionalData[msg->num_additionalData] = strndup(ptr, end - ptr);
        msg->num_additionalData++;
        ptr = end + 1;
    }
    
    if (*ptr != '\0')
    {
        
        msg->message = strdup(ptr+6);
        msg->message_length = strlen(msg->message);
    }

    return 0;
}

void initialize_new_msg(struct chat_message *newMsg)
{
    newMsg = malloc(sizeof(struct chat_message) * sizeof(char));

    if (newMsg == NULL)
    {
        printf("Error: Memory allocation failed\n");
    }

    memset(newMsg, 0, sizeof(struct chat_message)); // Initialize to zero
    //change value
    int msg_length = 200;

    if (!newMsg)
    {
        fprintf(stderr, "Error: newMsg is NULL\n");
    }

    newMsg->protocolVersion = malloc(20 * sizeof(char));
    newMsg->message = malloc(msg_length * sizeof(char));

    for (int i = 0; i < MAX_ADDITIONAL_DATA; i++)
    {
        newMsg->additionalData[i] = malloc(50 * sizeof(char));
    }
}

void fill_chat_message(
    struct chat_message *msg,
    char *protocolVersion,
    char *action,
    char *status,
    char *message,
    char *additionalData[],
    int num_additionalData) 
{
    msg->protocolVersion = strdup(protocolVersion);
    msg->action = strdup(action);
    msg->action_length = strlen(action);
    msg->status = strdup(status);
    msg->status_length = strlen(status);
    msg->message = strdup(message);
    msg->message_length = strlen(message);

    if (num_additionalData > 0)
    {
        msg->num_additionalData = num_additionalData;
        for (int i = 0; i < num_additionalData; i++)
        {
            msg->additionalData[i] = strdup(additionalData[i]);
        }

    }
}

void print_chat_message(struct chat_message *msg)
{
    if (msg == NULL)
    {
        printf("msg is NULL\n");
        return;
    }

    printf("Protocol Version: %s\n", msg->protocolVersion ? msg->protocolVersion : "NULL");
    printf("Action: %s\n", msg->action ? msg->action : "NULL");
    printf("Status: %s\n", msg->status ? msg->status : "NULL");
    printf("Message: %s\n", msg->message ? msg->message : "NULL");


    if (msg->num_additionalData > 0)
    {
        printf("Additional Data:\n");
        for (int i = 0; i < msg->num_additionalData; i++)
        {
            printf("  %s\n", msg->additionalData[i] ? msg->additionalData[i] : "NULL");
        }
    }
}

void free_chat_message(struct chat_message *msg)
{
    
    free(msg->protocolVersion);
    free(msg->action);
    free(msg->status);
    free(msg->message);
    for (int i = 0; i < msg->num_additionalData; i++)
    {
        free(msg->additionalData[i]);
    }

    msg->protocolVersion = NULL;
    msg->action = NULL;
    msg->status = NULL;
    msg->message = NULL;
}
