#ifndef VN_CL_DEQUE_H
#define VN_CL_DEQUE_H

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFFER_INIT_SIZE (1 << 4)

// =======
// Chase and Lev's work-stealing queue, optimized for
// weak memory models by Le et al and with further
// optimizations to save some cache misses on reading
// the buffer of elements and capacity (lower bits of
// buffer pointer and capacity contain version number tag)
//
// * Chase D., Lev. Y. Dynamic Circular Work-Stealing Deque
// * Le N. M. et al. Correct and Efficient Work-Stealing for
//   Weak Memory Models
// =======

typedef struct {
    _Atomic(int64_t) top;
    _Atomic(int64_t) bottom;
    _Atomic(void **) buffer;
    _Atomic(size_t) capacity;
    uint8_t vn;
} vn_cl_deque_t;

#define ALIGNMENT 4

// This has all lower `ALIGNMENT` bits equal to 1
#define ALIGNMENT_POW2 15

#define READ_BUF_VN(buffer) ((uintptr_t)buffer & (uintptr_t)ALIGNMENT_POW2)

#define READ_BUFFER(buffer) (void **)((uintptr_t)buffer & (uintptr_t)(~ALIGNMENT_POW2))

#define TAG_BUFFER(buffer, vn) buffer = (void **)((uintptr_t)buffer | (uintptr_t)vn)

#define READ_CAPACITY_VN(capacity) (capacity & (size_t)ALIGNMENT_POW2)

#define READ_CAPACITY(capacity) (size_t)(capacity >> ALIGNMENT)

#define TAG_CAPACITY(capacity, vn) capacity = ((capacity << ALIGNMENT) | (size_t)vn)

#define VN_MATCH(buffer, capacity) (READ_BUF_VN(buffer) == READ_CAPACITY_VN(capacity))

vn_cl_deque_t *create_vn_cl_deque();

void destroy_vn_cl_deque(vn_cl_deque_t *dq);

void vn_cl_deque_push(vn_cl_deque_t *dq, void *elt);

void vn_cl_resize(vn_cl_deque_t *dq);

void *vn_cl_deque_pop(vn_cl_deque_t *dq);

void *vn_cl_deque_steal_from(vn_cl_deque_t *dq);

#endif
