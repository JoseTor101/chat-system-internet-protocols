#ifndef QUEUE_H
#define QUEUE_H

typedef struct node {
    char *data;
    struct node *next;
} node_t;

typedef struct {
    node_t *front, *rear;
} queue_t;

// Function to create a queue
queue_t* create_queue();

// Function to enqueue a message to the queue
void enqueue(queue_t *q, char *msg);

// Function to dequeue a message from the queue
char* dequeue(queue_t *q);

// Check if the queue is empty
int is_queue_empty(queue_t *q);

#endif // QUEUE_H
