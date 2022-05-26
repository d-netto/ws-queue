#include "array.h"

array_t *create_array(size_t capacity)
{
    array_t *a = malloc(sizeof(array_t));
    a->buffer = malloc(capacity * sizeof(void *));
    a->capacity = capacity;
    return a;
}