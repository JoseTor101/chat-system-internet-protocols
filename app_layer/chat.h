#include <stdint.h>
#include <string.h>

typedef char *chat_action;
typedef char *action_status;

#define CHAT_ACTION_JOIN "JOIN"
#define CHAT_ACTION_LEAVE "LEAVE"
#define CHAT_ACTION_GET "GET"
#define CHAT_ACTION_SEND "SEND"
#define CHAT_ACTION_EMPTY "..."

#define CHAT_JOIN_SUCCESS "OK_J"
#define CHAT_JOIN_FAIL "ERROR 1"

#define CHAT_LEAVE_SUCCESS "OK_L"
#define CHAT_LEAVE_FAIL "ERROR_2"

#define CHAT_GET_SUCCESS "OK_G"
#define CHAT_GET_BAD_ACTION "ERROR_3-1"
#define CHAT_GET_BAD_OPTION "ERROR_3-2"
#define CHAT_GET_BAD_MSG "ERROR_3-3"
#define CHAT_GET_EMPTY "NOMSG"

#define CHAT_SEND_SUCCESS "OK_S"
#define CHAT_SEND_FAIL "ERROR_4"

#define CHAT_MSG_MAXSIZE 1024
#define MAX_ADDITIONAL_DATA 10

struct chat_message
{
    char *protocolVersion;

    chat_action action;
    int action_length;

    action_status status;
    int status_length;

    char *message;
    int message_length;

    char *additionalData[MAX_ADDITIONAL_DATA];
    int num_additionalData;
};

struct stringify_result
{
    int length;
    action_status reply;
};

char* stringify(struct chat_message *msg, struct stringify_result *result);
int parse(char *buffer, struct chat_message *msg);
char* readMessage(char *buffer, int client_index);