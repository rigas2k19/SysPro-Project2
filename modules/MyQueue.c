#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/MyQueue.h"

Queue q_create(DestroyFunc a, int max){
    Queue q = malloc(sizeof(*q));
    
    q->destroy_value = a;
    q->size = 0;
    q->max_size = max;
    q->dummy = malloc(sizeof(*q->dummy));
    q->dummy->next = NULL;
    q->last = q->dummy;

    return q;
}

int q_size(Queue q){ return q->size; }

int q_maxsize(Queue q){ return q->max_size; }

void q_insert_next(Queue q, Node node, AddVars value){
    if(node == NULL){
        node = q->dummy;
    }
    
    /* If Node exists - we dont add to queue 
    Node mynode = malloc(sizeof(*mynode));
    if((mynode = queue_find_node(q, value)) != NULL){
        return;
    }*/

    /* Create new node */
    Node new = malloc(sizeof(*new));
    new->value = value;
    
    /* Link new with node and node->next */
    new->next = node->next;
    node->next = new;

    /* Update size */
    q->size++;
    if(q->last == node){
        q->last = new;
    }
}

Node queue_first(Queue q){ return q->dummy->next; }

Node queue_last(Queue q) { 
    if(q->last == q->dummy){
        return QUEUE_EOF;
    }
    else{
        return q->last;
    }
}

Node queue_next(Queue q, Node node){ return node->next; }

void q_insert(Queue q, AddVars value){
    int size = q_size(q);

    if(size == 0){
        q_insert_next(q, QUEUE_BOF, value);
    }
    else if(size < q->max_size){
        Node before = queue_first(q);
        for(Node node = queue_first(q); node != QUEUE_EOF; node = queue_next(q, node)){
            before = node;
            if(queue_next(q, node) == QUEUE_EOF){
                q_insert_next(q, before, value);
                break;
            }
            before = node;
        }
    }
}

Node queue_find_node(Queue q, AddVars value){
    for(Node node = q->dummy->next; node != NULL; node = node->next){
        if(strcmp(value->path, node->value->path) == 0){
            printf("value->path: %s and node->value->path:%s\n", value->path, node->value->path);
            printf("Node exists!!!!!!!!!!!!!!!!!\n\n");
            return node;
        }
    }
    return NULL;
}

void q_remove_next(Queue q, Node node){
    if(node == NULL){
        node = q->dummy;
    }

    Node removed = node->next;

    if(q->destroy_value != NULL){
        q->destroy_value(removed->value);
    }

    node->next = removed->next;
    free(removed);

    q->size--;
    if(q->last == removed){
        q->last = node;
    }
}

void q_remove_first(Queue q){
    Node node = QUEUE_BOF;
    q_remove_next(q, node);
}

AddVars queue_find(Queue q, AddVars value){
    Node node = queue_find_node(q, value);
    return node == NULL ? NULL : node->value;
}

AddVars q_first_value(Queue q){
    return q->dummy->next->value;
}

void q_destroy(Queue q){
    Node node = q->dummy;
    while(node != NULL){
        Node next = node->next;

        if(node != q->dummy && q->destroy_value != NULL){
            q->destroy_value(node->value);
        }

        free(node);
        node = next;
    }
    free(q);
}