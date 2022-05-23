#include <stdio.h>

#include "cl_deque.h"

cl_deque_t *create_cl_deque()
{
    void **buffer = (void **)malloc(MAX_BUFFER_SIZE * sizeof(void *));
    cl_deque_t *dq = malloc(sizeof(cl_deque_t));

    dq->top = 0;
    dq->bottom = 0;
    dq->buffer = buffer;

    return dq;
}

void destroy_cl_deque(cl_deque_t *dq)
{
    free(dq->buffer);
}

void cl_deque_push(cl_deque_t *dq, void *elt)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
    void **buffer = dq->buffer;
    if (b - t > MAX_BUFFER_SIZE - 1) {
        // Queue is full and resize is not implemented yet
        fprintf(stderr, "Queue is full!\n");
        abort();
    }
    atomic_store_explicit((_Atomic(void *) *)&buffer[b % MAX_BUFFER_SIZE], elt,
                          memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&dq->bottom, b + 1, memory_order_relaxed);
}

void *cl_deque_pop(cl_deque_t *dq)
{
    int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed) - 1;
    void **buffer = dq->buffer;
    atomic_store_explicit(&dq->bottom, b, memory_order_relaxed);
    atomic_thread_fence(memory_order_seq_cst);
    int64_t t = atomic_load_explicit(&dq->top, memory_order_relaxed);
    void *elt;
    if (t <= b) {
        elt = atomic_load_explicit((_Atomic(void *) *)&buffer[b % MAX_BUFFER_SIZE],
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
        void **buffer = dq->buffer;
        elt = atomic_load_explicit((_Atomic(void *) *)&buffer[t % MAX_BUFFER_SIZE],
                                   memory_order_relaxed);
        if (!atomic_compare_exchange_strong_explicit(
                &dq->top, &t, t + 1, memory_order_seq_cst, memory_order_relaxed))
            return NULL;
    }
    return elt;
}