
/*

 | PROTOCOLO | VERSION | ACCIÓN | STATUS_CODE | METODO DE RESPUESTA | MENSAJE |

    MCP/1.0/SEND/OK_S/EXPECTED_METHOD:METHOD/MESSAGE

    EXPECTED METHOD:

        NONE
        GET
        SEND
        JOIN
        LEAVE

    / CODIGOS /

    0 - Sin codigo
    100 - Proceda haga esto
    200 - Exito
    400 - Bad request
    401 - Not autorized
    408 - request timeout


    // SEND

    | PROTOCOLO | VERSION | ACCIÓN | STATUS_CODE | E_MTD | MENSAJE |

    1. Usuario se conecta
    2. Servidor envia mensaje de bienvenida
    3. Servidor pide nombre de usuario
        > [S] SEND
        MCP/1.0/SEND/CODE:100/E_MTD:SEND/MSG:Please assign yourself a username (Max 10 chars.)

    4. Usuario envia nombre de usuario
        > [C] SEND
        MCP/1.0/SEND/CODE:0/E_MTD:SEND/MSG:<username>

        Si <usuarname> valido:
            > [S] send
            MCP/1.0/SEND/CODE:200/E_MTD:NONE/MSG:Registrado, valido

        si no:

            while (username != valid) {
                > [S] send
                MCP/1.0/SEND/CODE:400/E_MTD:SEND/MSG:Nombre de usuario no valido

                > [C] send
                MCP/1.0/SEND/CODE:0/E_MTD:NONE/MSG:<username>

                > [S] send
                MCP/1.0/SEND/CODE:200/E_MTD:NONE/MSG:Registrado, valido


    5. Servidor envia lista de usuarios
        > [S] send
        MCP/1.0/SEND/CODE:100/E_MTD:JOIN/MSG:
            <Lista de usuarios>
            Please select a user to chat with using 'JOIN <user_number>'

    >JOIN

    | PROTOCOLO | VERSION | ACCIÓN | STATUS_CODE | E_MTD [ *, |, 1] | CHANNEL
    6. Se pide que seleccione un usuario

        > [C] send
            MCP/1.0/JOIN/CODE:0/E_MTD:SEND/CH:<USER>

        >[S]response:

            Si exito
                >[S]
                MCP/1.0/SEND/CODE:200/E_MTD:SEND|LEAVE/MSG: Ahora estás chateando con <username>
            sino
                >[S]
                    * MCP/1.0/SEND/CODE:400/E_MTD:JOIN/MSG:Usuario no encontrado
                    * MCP/1.0/SEND/CODE:401/E_MTD:JOIN/MSG:Usuario ocupado
                >Paso 5

    >LEAVE

    | PROTOCOLO | VERSION | ACCIÓN | STATUS_CODE | E_MTD [ *, |, 1] | LEVEL_EXIT: 1 -> client 0 -> server

    7. Usuario se desconecta
        > [C] send
        MCP/1.0/LEAVE/CODE:0/E_MTD:SEND/LEVEL_EXIT:0|1

        > [S] send
            MCP/1.0/SEND/CODE:200/E_MTD:SEND/MSG:Desconectado

    >GET

    | PROTOCOLO | VERSION | ACCIÓN | STATUS_CODE | E_MTD [ *, |, 1] | RESOURCE

    8. Usuario pide un recurso
        > [C] send
        MCP/1.0/GET/CODE:0/E_MTD:SEND/RESOURCE:<resource>

        > [S] send
            MCP/1.0/SEND/CODE:200/E_MTD:SEND/MSG:<resource>

*/

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

    //char *buffer = (char *)malloc(CHAT_MSG_MAXSIZE * sizeof(char));
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
        //return NULL;
    }

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
        strncpy(&buffer[current_buffer_pos], "\r\n", 2);
        current_buffer_pos += 2;

        strncpy(&buffer[current_buffer_pos], msg->message, msg->message_length);
        current_buffer_pos += msg->message_length;

        buffer[current_buffer_pos] = '\n';
        current_buffer_pos++;
    }

    buffer[current_buffer_pos] = '\0';

    //return buffer;
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
        //printf("ptr: %s\n", ptr);
        //if (msg->num_additionalData >= MAX_ADDITIONAL_DATA)
        //    return -1;
        
        //MSG:<message>
        msg->message = strdup(ptr+2);
        //msg->additionalData[msg->num_additionalData] = strdup(ptr);
        //msg->num_additionalData++;
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

char *readMessage(char *buffer, int client_index)
{
    struct chat_message *newMsg;

    initialize_new_msg(newMsg);

    struct stringify_result result;
    int parseResult = parse(buffer, newMsg);

    if (parseResult != 0)
    {
        free_chat_message(newMsg); // Free allocated resources
        return "Error: Invalid message format";
    }

    if (strcmp(newMsg->action, CHAT_ACTION_GET) == 0)
    {
        // e.g. "MCP/1.0/GET/CODE:0/E_MTD:SEND/RESOURCE:LIST"
        if (strcmp(getStringValue(newMsg->additionalData[1]), "JOIN") == 0)
        {
            newMsg->action = CHAT_ACTION_SEND;
            newMsg->status = CHAT_SEND_SUCCESS;
            newMsg->num_additionalData = 1;
            newMsg->additionalData[0] = strdup("E_MTD:SEND");
            //newMsg->message = list_users(client_index);
            //return stringify(newMsg, &result);
        }else if (strcmp(getStringValue(newMsg->additionalData[1]), "LIST") == 0)
        {
            newMsg->action = CHAT_ACTION_SEND;
            newMsg->status = CHAT_SEND_SUCCESS;
            newMsg->num_additionalData = 1;
            newMsg->additionalData[0] = strdup("E_MTD:SEND"); // Duplicate the string
            //newMsg->message = list_users(client_index);
            //return stringify(newMsg, &result);
        }
    }

    free_chat_message(newMsg); // Clean up
    return "Error: Unsupported action";
}


/*
void test_stringify() {
    struct chat_message msg;
    initialize_new_msg(&msg);
    struct stringify_result result;
    
    // Example data to fill the message
    char *additionalData[] = {"E_MTD:SEND", "RESOURCE:LIST"};
    fill_chat_message(&msg, "1.0", CHAT_ACTION_SEND, CHAT_SEND_SUCCESS, "Hello, World!", additionalData, 2);
    printf("Filling chat message...\n");

    print_chat_message(&msg);
    /*char *result_string = stringify(&msg, &result);
    if (result.reply == CHAT_GET_EMPTY) {
        printf("stringify failed: %s\n", result.reply);
    } else {
        printf("stringify result: %s\n", result_string);
    }

    //free(result_string);
    free_chat_message(&msg);
}

// Helper function to test parse
void test_parse() {
    struct chat_message msg;

    initialize_new_msg(&msg);
    char buffer[256];

    // Valid message
    strcpy(buffer, "MCP/1.0/SEND/SUCCESS/E_MTD:SEND/RESOURCE:LIST/MSG:Hello, World!");

   
    int parseResult = parse(buffer, &msg); // Parse the buffer
    if (parseResult == 0) {
        print_chat_message(&msg); // Print only if parse was successful
    } else {
        printf("Error: Invalid message format\n");
    }

    free_chat_message(&msg);
}

// Main test function
int main() {
    printf("Running tests for stringify function...\n");
    test_stringify();
    printf("\nRunning tests for parse function...\n");
    test_parse();

    return 0;
}*/