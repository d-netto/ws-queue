#ifndef CL_DEQUE_H
#define CL_DEQUE_H

#include <stdatomic.h>
#include <stdlib.h>

static size_t MAX_BUFFER_SIZE = 100000;

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
    void **buffer;
} cl_deque_t;

cl_deque_t *create_cl_deque();

void destroy_cl_deque(cl_deque_t *dq);

void cl_deque_push(cl_deque_t *dq, void *elt);

void *cl_deque_pop(cl_deque_t *dq);

void *cl_deque_steal_from(cl_deque_t *dq);

#endif