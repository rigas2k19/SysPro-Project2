#include "myDeclarations.h"
#include "common_types.h"

#define QUEUE_BOF (Node)0
#define QUEUE_EOF (Node)0

typedef struct queue* Queue;
typedef struct queue_node* Node;

struct queue_node{
    Node next;
    AddVars value;
};

struct queue{
    Node dummy;
    Node last;
    int size;
    int max_size;
    DestroyFunc destroy_value;
};

Queue q_create(DestroyFunc a, int max);

int q_size(Queue q);

int q_maxsize(Queue q);

void q_insert_next(Queue q, Node node, AddVars value);

Node queue_first(Queue q);

Node queue_last(Queue q);

Node queue_next(Queue q, Node node);

void q_insert(Queue q, AddVars value);

Node queue_find_node(Queue q, AddVars value);

void q_remove_next(Queue q, Node node);

AddVars queue_find(Queue q, AddVars value);

void q_destroy(Queue q);

void q_remove_first(Queue q);

AddVars q_first_value(Queue q);