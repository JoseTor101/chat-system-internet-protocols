#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
-- METHODS --

GET: 
    REQUEST:
         "GET/MCP/1.0/\r\nURL:/list"
    RESPONSE:
        MCP/1.0 200 OK
        Content-Type: text/plain
        Content-Length: 13
        Body:
        <list of items>  // This could be a placeholder for the actual response content

POST: 
    REQUEST:
        "POST/MCP/1.0/\r\nURL:/submit\r\nContent-Type: application/json\r\nContent-Length: <length>\r\n\r\n{<json_payload>}"
        Example:
        POST/MCP/1.0/
        URL:/submit
        Content-Type: application/json
        Content-Length: 34

        {
          "name": "John Doe",
          "email": "john@example.com"
        }

    RESPONSE:
        MCP/1.0 201 Created
        Content-Type: application/json
        Content-Length: <length>
        Body:
        {
          "status": "success",
          "message": "Data submitted successfully"
        }
*/


// Function to get the version from the message
char* getVersion(size_t start, char *msg) {
    const char *end = strchr(msg + start, '/'); // Find the next '/' after the method
    if (end == NULL) {
        return NULL; 
    }

    size_t versionLength = strchr(end + 1, '\r') - end - 1; // Length until the end of the version
    if (versionLength <= 0) {
        return NULL; 
    }

    char *version = (char*)malloc(versionLength + 1);  
    strncpy(version, end + 1, versionLength); 
    version[versionLength] = '\0';  

    return version;
}

// Function to create a response and return it as an array of strings
char** createResponse(const char *version, int statusCode, const char *contentType, int contentLength) {
    // Allocate memory for the response array
    char **responseArray = (char**)malloc(4 * sizeof(char*));
    if (responseArray == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    // Prepare the version string
    responseArray[0] = (char*)malloc(strlen(version) + 20); // Enough space for status code and other info
    if (responseArray[0] == NULL) {
        printf("Memory allocation failed!\n");
        free(responseArray);
        return NULL;
    }
    sprintf(responseArray[0], "MCP/%s %d", version, statusCode); // First element: Version and Status

    // Prepare the content type string
    responseArray[1] = (char*)malloc(strlen(contentType) + 16); // Enough space for "Content-Type: "
    if (responseArray[1] == NULL) {
        printf("Memory allocation failed!\n");
        free(responseArray[0]);
        free(responseArray);
        return NULL;
    }
    sprintf(responseArray[1], "Content-Type: %s", contentType); // Second element: Content Type

    // Prepare the content length string
    responseArray[2] = (char*)malloc(30); // Enough space for "Content-Length: "
    if (responseArray[2] == NULL) {
        printf("Memory allocation failed!\n");
        free(responseArray[1]);
        free(responseArray[0]);
        free(responseArray);
        return NULL;
    }
    sprintf(responseArray[2], "Content-Length: %d", contentLength); // Third element: Content Length

    // Fourth element could be any additional info or an empty string for now
    responseArray[3] = NULL; // Terminate the array with NULL for easy iteration

    return responseArray;
}

// Function to handle GET requests
void handleGET(char *rawMsg, size_t methodLength) {
    size_t versionStart = methodLength + 1;  // Start after "GET/"
    char *version = getVersion(versionStart, rawMsg);
    if (version == NULL) {
        printf("Error: Version not found!\n");
        return;
    }

    //printf("Version: %s\n", version);

    // Find the "URL:" part of the message
    char *urlPos = strstr(rawMsg, "URL:");
    if (urlPos == NULL) {
        printf("Error: URL not found!\n");
        free(version);
        return;
    }

    char *url = urlPos + 4; // Skip "URL:"
    printf("URL: %s\n", url);

    if (strncmp(url, "/list", 5) == 0) {
        const char *contentType = "text/plain";
        int statusCode = 200;
        int contentLength = 13;



        char **response = createResponse(version, statusCode, contentType, contentLength);
        
    }

    // Cleanup
    free(version);
}


char* parseRequest(char *rawMsg) {
    // Find the first '/' character to identify the method and version
    const char *charPos = strchr(rawMsg, '/');
    if (charPos == NULL) {
        printf("Error: Invalid request format!\n");
        return NULL;
    }

    size_t methodLength = charPos - rawMsg;
    char method[methodLength + 1];   
    strncpy(method, rawMsg, methodLength);
    method[methodLength] = '\0';     

    if (strncmp(method, "GET", methodLength) == 0) {
        printf("Method: %s\n", method);
        handleGET(rawMsg, methodLength);
    } else {
        printf("Error: Unsupported method!\n");
        return NULL;
    }

    return NULL;
}

// Main function to demonstrate the code
int main() {
    char request[] = "GET/MCP/1.0/\r\nURL:/list"; // Sample request

    parseRequest(request); // Parse the request

    return 0;
}
