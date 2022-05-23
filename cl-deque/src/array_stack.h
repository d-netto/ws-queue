#ifndef ARRAY_STACK_H
#define ARRAY_STACK_H

#include <stdlib.h>

typedef struct {
    size_t capacity;
    void **top;
    void **bottom;
} array_stack_t;

array_stack_t create_stack(size_t capacity);

void destroy_stack(array_stack_t *stack);

void push(array_stack_t *stack, void *elt);

void *pop(array_stack_t *stack);

#endif
