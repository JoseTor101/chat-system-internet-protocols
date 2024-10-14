#include <stdlib.h>
#include <string.h>
#include "queue.h"

// Function to create a queue
queue_t* create_queue() {
    queue_t *q = (queue_t*)malloc(sizeof(queue_t));
    q->front = q->rear = NULL;
    return q;
}

// Function to enqueue a message to the queue
void enqueue(queue_t *q, char *msg) {
    node_t *new_node = (node_t*)malloc(sizeof(node_t));
    new_node->data = strdup(msg); // Duplicate the message
    new_node->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
}

// Function to dequeue a message from the queue
char* dequeue(queue_t *q) {
    if (q->front == NULL) {
        return NULL;
    }
    node_t *temp = q->front;
    char *msg = temp->data;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return msg;
}

// Check if the queue is empty
int is_queue_empty(queue_t *q) {
    return q->front == NULL;
}