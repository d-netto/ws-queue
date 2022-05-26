#include <stdio.h>

#include "cl_deque.h"

cl_deque_t *create_cl_deque()
{
    cl_deque_t *dq = malloc(sizeof(cl_deque_t));

    dq->top = 0;
    dq->bottom = 0;
    dq->array = create_array(BUFFER_INIT_SIZE);

    return dq;
}

void destroy_cl_deque(cl_deque_t *dq)
{
    // free(dq->buffer);
}

void cl_deque_push(cl_deque_t *dq, void *elt)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
    array_t *a = atomic_load_explicit(&dq->array, memory_order_relaxed);
    if (b - t > a->capacity - 1) {
        // Queue is full: resize it
        a = cl_resize(dq);
    }
    atomic_store_explicit((_Atomic(void *) *)&a->buffer[b % a->capacity], elt,
                          memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&dq->bottom, b + 1, memory_order_relaxed);
}

array_t *cl_resize(cl_deque_t *dq)
{
    size_t old_top = atomic_load_explicit(&dq->top, memory_order_relaxed);

    array_t *old_a = atomic_load_explicit(&dq->array, memory_order_relaxed);
    array_t *new_a = create_array(2 * old_a->capacity);
    atomic_store_explicit(&dq->array, new_a, memory_order_release);

    // Copy elements into new buffer
    // TODO: do we need atomics here?
    for (size_t i = 0; i < old_a->capacity; ++i) {
        new_a->buffer[(old_top + i) % (2 * old_a->capacity)] =
            old_a->buffer[(old_top + i) % old_a->capacity];
    }

    return new_a;
}

void *cl_deque_pop(cl_deque_t *dq)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed) - 1;
    array_t *a = atomic_load_explicit(&dq->array, memory_order_acquire);
    atomic_store_explicit(&dq->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_relaxed);
    void *elt;
    if (t <= b) {
        elt = atomic_load_explicit((_Atomic(void *) *)&a->buffer[b % a->capacity],
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

void *cl_deque_steal_from(cl_deque_t *dq)
{
    int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_acquire);
    void *elt = NULL;
    if (t < b) {
        array_t *a = atomic_load_explicit(&dq->array, memory_order_relaxed);
        elt = atomic_load_explicit((_Atomic(void *) *)&a->buffer[t % a->capacity],
                                   memory_order_relaxed);
        if (!atomic_compare_exchange_strong_explicit(
                &dq->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed))
            return NULL;
    }
    return elt;
}