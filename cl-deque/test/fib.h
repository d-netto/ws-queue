#include <stdlib.h>

size_t compute_fib(size_t x)
{
    if (x == 0 || x == 1) {
        return 1;
    }
    return compute_fib(x - 1) + compute_fib(x - 2);
}