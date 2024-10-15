#ifndef QUEUE_H
#define QUEUE_H

typedef struct node {
    char *data;
    struct node *next;
} node_t;

typedef struct {
    node_t *front, *rear;
} queue_t;

queue_t* create_queue();
void enqueue(queue_t *q, char *msg);
char* dequeue(queue_t *q);
int is_queue_empty(queue_t *q);

#endif // QUEUE_H
