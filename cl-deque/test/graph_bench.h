#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../src/array_stack.h"
#include "../src/cl_deque.h"

const size_t KEY_RANGE = 100;

const size_t MAX_NUM_NODES = 500000;

const size_t MIN_TIMEOUT_NS = 1e2;
const size_t MAX_TIMEOUT_NS = 1e4;

// =======
// Tree node
// =======

typedef struct _tree_node_t {
    size_t key;
    struct _tree_node_t *left;
    struct _tree_node_t *right;
} tree_node_t;

tree_node_t *create_tree_node(size_t key)
{
    tree_node_t *head = malloc(sizeof(tree_node_t));
    head->key = key;
    head->left = NULL;
    head->right = NULL;
    return head;
}

// =======
// Worker pool
// =======

struct _worker_t;

typedef struct {
    size_t nworkers;
    struct _worker_t *workers;
} worker_pool_t;

typedef struct _worker_t {
    size_t id;
    _Atomic(bool) waiting;
    pthread_t *thread;
    cl_deque_t *dq;
    worker_pool_t peers;
} worker_t;

// =======
// Benchmark results DS
// =======

typedef struct {
    double *times;
    size_t *results;
} benchmark_result_t;
