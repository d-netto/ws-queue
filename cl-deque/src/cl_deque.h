#ifndef CL_DEQUE_H
#define CL_DEQUE_H

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#include "array.h"

#define BUFFER_INIT_SIZE (1 << 6)

// =======
// Chase and Lev's work-stealing queue, optimized for
// weak memory models by Le et al.
//
// * Chase D., Lev. Y. Dynamic Circular Work-Stealing Deque
// * Le N. M. et al. Correct and Efficient Work-Stealing for
//   Weak Memory Models
// =======

typedef struct {
    _Atomic(int64_t) top;
    _Atomic(int64_t) bottom;
    _Atomic(array_t *) array;
} cl_deque_t;

cl_deque_t *create_cl_deque();

void destroy_cl_deque(cl_deque_t *dq);

void cl_deque_push(cl_deque_t *dq, void *elt);

array_t *cl_resize(cl_deque_t *dq);

void *cl_deque_pop(cl_deque_t *dq);

void *cl_deque_steal_from(cl_deque_t *dq);

#endif
