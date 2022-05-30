#ifndef ARRAY_QUEUE_H
#define ARRAY_QUEUE_H

#include <stdlib.h>

typedef struct {
    size_t capacity;
    void **top;
    void **bottom;
} array_queue_t;

array_queue_t create_queue(size_t capacity);

void destroy_queue(array_queue_t *stack);

void push(array_queue_t *q, void *elt);

void *pop_top(array_queue_t *q);

void *pop_bottom(array_queue_t *q);

#endif
