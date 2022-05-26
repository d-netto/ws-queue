#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

typedef struct {
    void **buffer;
    size_t capacity;
} array_t;

array_t *create_array(size_t capacity);

#endif