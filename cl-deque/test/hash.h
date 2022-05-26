#include <stdlib.h>

#define MAGIC_NUM 0xdeadbeef

// Dummy hash function
size_t compute_hash(size_t x)
{
    x = (x ^ (x >> 13)) * MAGIC_NUM;
    x = (x ^ (x >> 17)) * MAGIC_NUM;
    x = x ^ (x >> 42);
    return x;
}