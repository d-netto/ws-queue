#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "graph_bench.h"
#include "hash.h"

#define MIN(a, b) a < b ? a : b;
#define MAX(a, b) a > b ? a : b;

volatile _Atomic(bool) ready = false;
volatile _Atomic(bool) all_done = false;

void insert_tree_node(tree_node_t *head, tree_node_t *child)
{
    assert(child);

    if (head->key >= child->key) {
        if (head->left) {
            insert_tree_node(head->left, child);
        }
        else {
            head->left = child;
        }
    }
    else {
        if (head->right) {
            insert_tree_node(head->right, child);
        }
        else {
            head->right = child;
        }
    }
}

tree_node_t *generate_random_tree(size_t nnodes)
{
    tree_node_t *head = create_tree_node(0);

    srand(time(NULL));

    for (size_t i = 0; i < nnodes; ++i) {
        size_t key = rand() % KEY_RANGE;
        tree_node_t *child = create_tree_node(key);
        insert_tree_node(head, child);
    }

    return head;
}

size_t serial_dfs(tree_node_t *head)
{
    array_stack_t stack = create_stack(MAX_NUM_NODES);

    push(&stack, head);

    size_t sum = 0;

    for (;;) {
        // Empty dfs queue
        if (stack.top == stack.bottom) {
            break;
        }

        tree_node_t *n = (tree_node_t *)pop(&stack);
        if (n) {
            sum += compute_hash(n->key);
            if (n->left) {
                push(&stack, n->left);
            }
            if (n->right) {
                push(&stack, n->right);
            }
        }
    }

    destroy_stack(&stack);

    return sum;
}

worker_t *get_random_peer(worker_pool_t pool, int my_id)
{
    for (;;) {
        int peer_id = rand() % (pool.nworkers);
        if (peer_id != my_id) {
            return pool.workers + peer_id;
        }
    }
}

bool all_workers_done(worker_t *me)
{
    if (atomic_load_explicit(&all_done, memory_order_acquire)) {
        return true;
    }

    for (size_t i = 0; i < me->peers.nworkers; ++i) {
        if (i != me->id) {
            worker_t *peer = me->peers.workers + i;
            if (!atomic_load_explicit(&peer->waiting, memory_order_acquire)) {
                return false;
            }
        }
    }

    atomic_store_explicit(&all_done, true, memory_order_release);

    return true;
}

void *dfs_work(void *arg)
{
    while (!atomic_load_explicit(&ready, memory_order_acquire))
        ;

    worker_t *me = (worker_t *)arg;
    cl_deque_t *dq = me->dq;

    size_t *sum = calloc(1, sizeof(size_t));

    // used for exponential backoff in sleep timeout
    size_t timeout_ns = MIN_TIMEOUT_NS;

    for (;;) {
        int64_t t = atomic_load_explicit(&dq->top, memory_order_acquire);
        int64_t b = atomic_load_explicit(&dq->bottom, memory_order_relaxed);

        // Empty dfs queue: try stealing from a peer
        if (t >= b) {
            if (all_workers_done(me)) {
                atomic_store_explicit(&me->waiting, true, memory_order_release);
                destroy_cl_deque(dq);
                return sum;
            }

            for (int i = 0; i < 2 * me->peers.nworkers; ++i) {
                worker_t *peer = get_random_peer(me->peers, me->id);
                tree_node_t *n = (tree_node_t *)cl_deque_steal_from(peer->dq);
                if (n) { // successful steal
                    cl_deque_push(dq, n);
                    timeout_ns = MAX(timeout_ns - 1, MIN_TIMEOUT_NS);
                    goto work;
                }
            }

            atomic_store_explicit(&me->waiting, true, memory_order_release);
            timeout_ns = MIN(timeout_ns * 2, MAX_TIMEOUT_NS);
            sleep(10e-9 * timeout_ns);
            atomic_store_explicit(&me->waiting, false, memory_order_release);
        }

        work : {
            tree_node_t *n = (tree_node_t *)cl_deque_pop(dq);
            if (n) {
                *sum += compute_hash(n->key);
                if (n->left) {
                    cl_deque_push(dq, n->left);
                }
                if (n->right) {
                    cl_deque_push(dq, n->right);
                }
            }
        }
    }
}

worker_t *create_master_and_worker_pool(size_t nworkers)
{
    worker_t *workers = malloc(nworkers * sizeof(worker_t));

    worker_pool_t pool = {nworkers, workers};

    for (size_t i = 0; i < nworkers; ++i) {
        worker_t *worker = workers + i;
        worker->id = i;
        worker->waiting = false;
        worker->dq = create_cl_deque();
        worker->peers = pool;
    }

    // id of 0 is assigned to `master`, who runs
    // in the same thread that's running `main`
    for (size_t i = 1; i < nworkers; ++i) {
        pthread_t *pt = malloc(sizeof(pthread_t));
        pthread_create(pt, NULL, dfs_work, workers + i);
        workers[i].thread = pt;
    }

    // again, id of 0 is assigned to `master`
    return workers;
}

benchmark_result_t run_serial_dfs_benchmark(tree_node_t *head, size_t nruns)
{
    assert(nruns > 0);

    double *times = malloc(nruns * sizeof(double));
    size_t *results = malloc(nruns * sizeof(size_t));
    benchmark_result_t br = {times, results};

    for (size_t i = 0; i < nruns; ++i) {
        struct timespec start, end;

        clock_gettime(CLOCK_MONOTONIC, &start);
        size_t sum = serial_dfs(head);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double tdiff = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);

        br.times[i] = tdiff;
        br.results[i] = sum;
    }

    return br;
}

benchmark_result_t run_parallel_dfs_benchmark(tree_node_t *head, size_t nruns,
                                              size_t nworkers)
{
    assert(nruns > 0);
    assert(nworkers > 0);

    double *times = malloc(nruns * sizeof(double));
    size_t *results = malloc(nruns * sizeof(size_t));
    benchmark_result_t br = {times, results};

    for (size_t i = 0; i < nruns; ++i) {
        struct timespec start, end;

        worker_t *master = create_master_and_worker_pool(nworkers);
        size_t *sum;

        atomic_store_explicit(&ready, false, memory_order_release);
        atomic_store_explicit(&all_done, false, memory_order_release);

        clock_gettime(CLOCK_MONOTONIC, &start);
        {
            cl_deque_push(master->dq, head);
            atomic_store_explicit(&ready, true, memory_order_release);
            sum = dfs_work(master);
            worker_pool_t pool = master->peers;
            for (int i = 1; i < pool.nworkers; ++i) {
                void *worker_sum;
                pthread_join(*pool.workers[i].thread, &worker_sum);
                *sum += *((size_t *)worker_sum);
                free(worker_sum);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        double tdiff = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);

        br.times[i] = tdiff;
        br.results[i] = *sum;
    }

    return br;
}

void verify_and_pretty_print(benchmark_result_t br_serial, benchmark_result_t br_parallel,
                             size_t nruns)
{
    double t_total_serial = 0.0;
    double t_total_parallel = 0.0;

    size_t result_base = br_serial.results[0];

    for (size_t i = 0; i < nruns; ++i) {
        size_t result_serial = br_serial.results[i];
        size_t result_parallel = br_parallel.results[i];
        assert(result_serial == result_base && result_parallel == result_base);

        t_total_serial += br_serial.times[i];
        t_total_parallel += br_parallel.times[i];
    }

    printf("Serial dfs took %lf seconds on average\n", t_total_serial / nruns);
    printf("Parallel dfs took %lf seconds on average\n", t_total_parallel / nruns);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Usage: `graph_bench [num-nodes]");
        return 1;
    }

    // TODO: get this from CLI
    size_t nnodes = MAX_NUM_NODES;
    size_t nruns = 500;
    size_t max_nworkers = 4;

    tree_node_t *head = generate_random_tree(nnodes);

    for (size_t nworkers = 1; nworkers <= max_nworkers; ++nworkers) {
        benchmark_result_t br_serial = run_serial_dfs_benchmark(head, nruns);
        benchmark_result_t br_parallel = run_parallel_dfs_benchmark(head, nruns, nworkers);
        verify_and_pretty_print(br_serial, br_parallel, nruns);
    }

    return 0;
}