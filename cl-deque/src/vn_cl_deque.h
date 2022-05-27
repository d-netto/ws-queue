#ifndef VN_CL_DEQUE_H
#define VN_CL_DEQUE_H

#include <stdatomic.h>
#include <stdlib.h>

#define BUFFER_INIT_SIZE (1 << 6)

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
} vn_cl_deque_t;

#define NUM_TAG_BITS 2

#define READ_BUF_VN(buffer) ((uintptr_t)buffer & ((uintptr_t)(1 << NUM_TAG_BITS)))

#define READ_BUFFER(buffer) (void **)((uintptr_t)buffer & (~(uintptr_t)(1 << NUM_TAG_BITS)))

#define TAG_BUFFER(buffer) \
    buffer = (void **)((uintptr_t)buffer | (uintptr_t)(1 << NUM_TAG_BITS))

#define READ_CAPACITY_VN(capacity) (capacity & ((size_t)(1 << NUM_TAG_BITS)))

#define READ_CAPACITY(capacity) (size_t)(capacity >> (NUM_TAG_BITS + 1))

#define TAG_CAPACITY(capacity) \
    capacity = (capacity << (NUM_TAG_BITS + 1) | ((size_t)(1 << (NUM_TAG_BITS))))

#define VN_MATCH(buffer, capacity) (READ_BUF_VN(buffer) == READ_CAPACITY_VN(capacity))

vn_cl_deque_t *create_vn_cl_deque();

void destroy_vn_cl_deque(vn_cl_deque_t *dq);

void vn_cl_deque_push(vn_cl_deque_t *dq, void *elt);

void vn_cl_resize(vn_cl_deque_t *dq);

void *vn_cl_deque_pop(vn_cl_deque_t *dq);

void *vn_cl_deque_steal_from(vn_cl_deque_t *dq);

#endif