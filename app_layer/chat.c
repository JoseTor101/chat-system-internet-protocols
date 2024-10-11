
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

void buffer_write_header(int actionOrStatus, char buffer[], struct chat_message *msg, const char *methodStatus[], struct stringify_result *result)
{
    const char *selected_string = actionOrStatus ? msg->action : msg->status; // Choose action or option 1=action 0=option
    int selected_length = actionOrStatus ? msg->action_length : msg->status_length;

    for (int i = 0; methodStatus[i] != NULL; i++)
    {
        if (strcmp(selected_string, methodStatus[i]) == 0)
        {

            strncpy(&buffer[current_buffer_pos], selected_string, selected_length);
            current_buffer_pos += selected_length;

            buffer[current_buffer_pos] = '/';
            current_buffer_pos++;

            return;
        }
    }

    if (buffer[current_buffer_pos] == '\0')
    {
        result->reply = CHAT_GET_BAD_OPTION;
    }
}

void free_chat_message(struct chat_message *msg)
{
    free(msg->protocolVersion);
    free(msg->action);
    free(msg->status);
    for (int i = 0; i < msg->num_additionalData; i++)
    {
        free(msg->additionalData[i]);
    }
}

void stringify_result_factory(struct stringify_result *result)
{
    result->length = 0;
    result->reply = CHAT_GET_EMPTY;
};

char *stringify(
    struct chat_message *msg,
    struct stringify_result *result)
{

    char *buffer = (char *)malloc(CHAT_MSG_MAXSIZE * sizeof(char));
    stringify_result_factory(result);
    current_buffer_pos = 0;

    // Add the protocol version to the buffer only once

    const char *protocol_prefix = "MCP/";
    int prefix_length = strlen(protocol_prefix);
    strncpy(buffer + current_buffer_pos, protocol_prefix, prefix_length);
    current_buffer_pos += prefix_length;

    int protocol_length = strlen(msg->protocolVersion);
    strncpy(buffer + current_buffer_pos, msg->protocolVersion, protocol_length);
    current_buffer_pos += protocol_length;
    buffer[current_buffer_pos++] = '/';

    buffer_write_header(1, buffer, msg, ActionOptions, result);

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
        return NULL;
    }

    /*
    //Add additional data to the buffer
    for (int i = 0; i < msg->num_additionalData; i++) {
        if (msg->additionalData[i] != NULL) {
            strncpy(&buffer[current_buffer_pos], msg->additionalData[i], strlen(msg->additionalData[i]));
            current_buffer_pos += strlen(msg->additionalData[i]);
            buffer[current_buffer_pos++] = '/';
        }
    }


    if (msg->message_length > CHAT_MSG_MAXSIZE - current_buffer_pos - 1) { // -1 for the newline
        result->reply = CHAT_GET_BAD_MSG;
        free(buffer);
        return NULL;
    } else {
        // Add the message to the buffer
        strncpy(&buffer[current_buffer_pos], "\r\n", 2);
        current_buffer_pos += 2;

        strncpy(&buffer[current_buffer_pos], msg->message, msg->message_length);
        current_buffer_pos += msg->message_length;


        buffer[current_buffer_pos] = '\n';
        current_buffer_pos++;
    }

    buffer[current_buffer_pos] = '\0'; */

    return buffer;
}

/*
int parse(char buffer[], struct chat_message *msg) {
    if (buffer == NULL || msg == NULL) {
        return -1; // Error: invalid input
    }

    // Tokenize the message using "/" as a delimiter
    char *save_ptr;
    char *token = strtok_r(buffer, "/", &save_ptr);

    // First part should be "MCP"
    if (token == NULL || strcmp(token, "MCP") != 0) {
        return -1; // Error: invalid protocol prefix
    }

    // Extract protocol version
    token = strtok_r(NULL, "/", &save_ptr);
    if (token == NULL) {
        return -1; // Error: missing protocol version
    }
    msg->protocolVersion = strdup(token); // Use strdup to allocate memory for protocolVersion

    // Extract action
    token = strtok_r(NULL, "/", &save_ptr);
    if (token == NULL) {
        return -1; // Error: missing action
    }
    msg->action = strdup(token); // Use strdup to allocate memory for action
    msg->action_length = strlen(token);

    // Extract status (e.g., OK_S, ERROR_4)
    token = strtok_r(NULL, "/", &save_ptr);
    if (token == NULL) {
        return -1; // Error: missing status
    }
    msg->status = strdup(token); // Use strdup to allocate memory for status
    msg->status_length = strlen(token);

    // Parse additional data if available (e.g., E_MTD, CH, etc.)
    msg->num_additionalData = 0;
    while ((token = strtok_r(NULL, "/", &save_ptr)) != NULL) {
        if (msg->num_additionalData >= MAX_ADDITIONAL_DATA) {
            return -1; // Error: too many additional data entries
        }
        msg->additionalData[msg->num_additionalData] = strdup(token); // Allocate memory for each additionalData entry
        msg->num_additionalData++;
    }

    return 0; // Success
}
*/

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
    printf("Additional Data:\n");
    for (int i = 0; i < msg->num_additionalData; i++)
    {
        printf("  %s\n", msg->additionalData[i] ? msg->additionalData[i] : "NULL");
    }
}

int parse(char buffer[], struct chat_message *msg)
{

    if (buffer == NULL || msg == NULL)
    {
        return -1; // Error: invalid input
    }

    char *ptr = buffer;
    char *end;

    // Ensure buffer starts with "MCP"
    if (strncmp(ptr, "MCP", 3) != 0 || ptr[3] != '/')
    {
        return -1; // Error: invalid protocol prefix
    }
    ptr += 4; // Move past "MCP/"

    // Extract protocol version
    end = strchr(ptr, '/');
    if (end == NULL)
    {
        return -1; // Error: missing protocol version
    }
    msg->protocolVersion = strndup(ptr, end - ptr); // Allocate memory for protocolVersion
    ptr = end + 1;                                  // Move past the delimiter

    // Extract action
    end = strchr(ptr, '/');
    if (end == NULL)
    {
        return -1; // Error: missing action
    }
    msg->action = strndup(ptr, end - ptr); // Allocate memory for action
    msg->action_length = end - ptr;
    ptr = end + 1; // Move past the delimiter

    // Extract status
    end = strchr(ptr, '/');
    if (end == NULL)
    {
        return -1; // Error: missing status
    }
    msg->status = strndup(ptr, end - ptr); // Allocate memory for status
    msg->status_length = end - ptr;
    ptr = end + 1; // Move past the delimiter

    // Parse additional data if available (e.g., E_MTD, CH, etc.)
    msg->num_additionalData = 0;
    while ((end = strchr(ptr, '/')) != NULL)
    {
        if (msg->num_additionalData >= MAX_ADDITIONAL_DATA)
        {
            return -1; // Error: too many additional data entries
        }
        msg->additionalData[msg->num_additionalData] = strndup(ptr, end - ptr); // Allocate memory for each additionalData entry
        msg->num_additionalData++;
        ptr = end + 1; // Move past the delimiter
    }

    // Handle any remaining data (in case there's no trailing slash)
    if (*ptr != '\0')
    {
        if (msg->num_additionalData >= MAX_ADDITIONAL_DATA)
        {
            return -1; // Error: too many additional data entries
        }
        msg->additionalData[msg->num_additionalData] = strdup(ptr); // Allocate memory for the last additionalData entry
        msg->num_additionalData++;
    }

    print_chat_message(msg);
    return 0; // Success
}

// E_MTD:SEND -> E_MTD
char *getStringKey(const char *string)
{
    if (string == NULL)
        return NULL;

    // Find the first occurrence of the ':' character
    char *colon_pos = strchr(string, ':');
    if (colon_pos == NULL)
    {
        return NULL;
    }

    // Calculate the length of the key part (from the start to the colon)
    size_t key_length = colon_pos - string;

    // Allocate memory for the key and copy the key part
    char *key = (char *)malloc(key_length + 1);
    if (key == NULL)
    {
        return NULL;
    }

    strncpy(key, string, key_length);
    key[key_length] = '\0'; // Null-terminate the string

    return key;
}

// E_MTD:SEND -> SEND
char *getStringValue(const char *string)
{
    if (string == NULL)
        return NULL;

    // Find the first occurrence of the ':' character
    char *colon_pos = strchr(string, ':');
    if (colon_pos == NULL)
    {
        return NULL;
    }

    char *value = colon_pos + 1;
    char *result = strdup(value);
    return result;
}

void initialize_new_msg(struct chat_message *newMsg)
{
    if (newMsg == NULL)
    {
        fprintf(stderr, "Error: newMsg is NULL\n");
        return;
    }

    // Asignación de memoria para `protocolVersion`
    newMsg->protocolVersion = malloc(20 * sizeof(char)); // Tamaño arbitrario
    if (newMsg->protocolVersion == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed for protocolVersion\n");
        free_chat_message(newMsg); // Liberar memoria previamente asignada
        return;
    }

    // Asignación de memoria para `message`
    newMsg->message = malloc(100 * sizeof(char)); // Tamaño arbitrario
    if (newMsg->message == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed for message\n");
        free_chat_message(newMsg);
        return;
    }

    // Asignación de memoria para `additionalData` (cada cadena en el arreglo)
    for (int i = 0; i < MAX_ADDITIONAL_DATA; i++)
    {
        newMsg->additionalData[i] = malloc(50 * sizeof(char)); // Tamaño arbitrario
        if (newMsg->additionalData[i] == NULL)
        {
            fprintf(stderr, "Error: Memory allocation failed for additionalData[%d]\n", i);
            free_chat_message(newMsg);
            return;
        }
    }
}

char *readMessage(char *buffer, int client_index)
{

    struct chat_message *newMsg = malloc(sizeof(struct chat_message) * 2 * sizeof(char));

    if (newMsg == NULL)
    {
        return "Error: Memory allocation failed";
    }

    memset(newMsg, 0, sizeof(struct chat_message)); // Initialize to zero
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
        if (strcmp(getStringValue(newMsg->additionalData[1]), "LIST") == 0)
        {
            newMsg->message = list_users(client_index);
            newMsg->action = CHAT_ACTION_SEND;
            newMsg->status = CHAT_SEND_SUCCESS;
            newMsg->num_additionalData = 1;
            newMsg->additionalData[0] = strdup("E_MTD:SEND"); // Duplicate the string

            return stringify(newMsg, &result);
        }
    }

    free_chat_message(newMsg); // Clean up
    return "Error: Unsupported action";
}
