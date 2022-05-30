#include "array_queue.h"

array_queue_t create_queue(size_t capacity)
{
    void **buf = (void **)malloc(capacity * sizeof(void *));
    array_queue_t q = {capacity, buf, buf};
    return q;
}

void destroy_queue(array_queue_t *q)
{
    free(q->top);
}

void push(array_queue_t *q, void *elt)
{
    *q->bottom = elt;
    q->bottom++;
}

void *pop_top(array_queue_t *q)
{
    if (q->top == q->bottom) {
        return NULL;
    }
    void *elt = *(q->top);
    q->top++;
    return elt;
}

void *pop_bottom(array_queue_t *q)
{
    q->bottom--;
    void *elt = *(q->bottom);
    return elt;
}