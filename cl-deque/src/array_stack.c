#include "array_stack.h"

array_stack_t create_stack(size_t capacity)
{
    void **buf = (void **)malloc(capacity * sizeof(void *));
    array_stack_t stack = {capacity, buf, buf};
    return stack;
}

void destroy_stack(array_stack_t *stack)
{
    free(stack->top);
}

void push(array_stack_t *stack, void *elt)
{
    *stack->bottom = elt;
    stack->bottom++;
}

void *pop(array_stack_t *stack)
{
    void *elt = *(stack->bottom - 1);
    stack->bottom--;
    return elt;
}