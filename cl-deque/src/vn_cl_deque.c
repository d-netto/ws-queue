#include <stdio.h>

#include "vn_cl_deque.h"

vn_cl_deque_t *create_vn_cl_deque()
{
    vn_cl_deque_t *dq = malloc(sizeof(vn_cl_deque_t));

    dq->top = 0;
    dq->bottom = 0;

    void **buffer = malloc(BUFFER_INIT_SIZE * sizeof(void *));
    TAG_BUFFER(buffer);
    dq->buffer = buffer;

    size_t capacity = BUFFER_INIT_SIZE;
    TAG_CAPACITY(capacity);
    dq->capacity = capacity;

    return dq;
}

void destroy_vn_cl_deque(vn_cl_deque_t *dq)
{
    // free(dq->buffer);
}

void vn_cl_deque_push(vn_cl_deque_t *dq, void *elt)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
    void **buffer = READ_BUFFER(atomic_load_explicit(&dq->buffer, memory_order_relaxed));
    size_t capacity =
        READ_CAPACITY(atomic_load_explicit(&dq->capacity, memory_order_relaxed));
    if (b - t > capacity - 1) {
        // Queue is full: resize it
        buffer = READ_BUFFER(atomic_load_explicit(&dq->buffer, memory_order_relaxed));
        capacity = READ_CAPACITY(atomic_load_explicit(&dq->capacity, memory_order_relaxed));
    }
    atomic_store_explicit((_Atomic(void *) *)&buffer[b % capacity], elt,
                          memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&dq->bottom, b + 1, memory_order_relaxed);
}

void vn_cl_resize(vn_cl_deque_t *dq)
{
    size_t old_top = atomic_load_explicit(&dq->top, memory_order_relaxed);

    void **old_buffer = atomic_load_explicit(&dq->buffer, memory_order_relaxed);
    size_t old_capacity = atomic_load_explicit(&dq->capacity, memory_order_relaxed);

    void **new_buffer = malloc(2 * dq->capacity);
    size_t new_capacity = 2 * old_capacity;

    // Copy elements into new buffer
    // TODO(netto): do we need atomics here?
    for (size_t i = 0; i < old_capacity; ++i) {
        new_buffer[(old_top + i) % (new_capacity)] =
            old_buffer[(old_top + i) % old_capacity];
    }

    atomic_store_explicit(&dq->buffer, TAG_BUFFER(new_buffer), memory_order_release);
    atomic_store_explicit(&dq->capacity, TAG_CAPACITY(new_capacity), memory_order_release);
}

void *vn_cl_deque_pop(vn_cl_deque_t *dq)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed) - 1;
    void **buffer = READ_BUFFER(atomic_load_explicit(&dq->buffer, memory_order_relaxed));
    size_t capacity =
        READ_CAPACITY(atomic_load_explicit(&dq->capacity, memory_order_relaxed));
    atomic_store_explicit(&dq->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_relaxed);
    void *elt;
    if (t <= b) {
        elt = atomic_load_explicit((_Atomic(void *) *)&buffer[b % capacity],
                                   memory_order_relaxed);
        if (t == b) {
            if (!atomic_compare_exchange_strong_explicit(
                    &dq->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed))
                elt = NULL;
            atomic_store_explicit(&dq->bottom, b + 1, memory_order_relaxed);
        }
    }
    else {
        elt = NULL;
        atomic_store_explicit(&dq->bottom, b + 1, memory_order_relaxed);
    }
    return elt;
}

void *vn_cl_deque_steal_from(vn_cl_deque_t *dq)
{
    int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_acquire);
    void *elt = NULL;
    if (t < b) {
        void **buffer =
            READ_BUFFER(atomic_load_explicit(&dq->buffer, memory_order_relaxed));
        size_t capacity =
            READ_CAPACITY(atomic_load_explicit(&dq->capacity, memory_order_relaxed));
        if (!VN_MATCH(buffer, capacity))
            return NULL;
        elt = atomic_load_explicit((_Atomic(void *) *)&buffer[t % capacity],
                                   memory_order_relaxed);
        if (!atomic_compare_exchange_strong_explicit(
                &dq->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed))
            return NULL;
    }
    return elt;
}