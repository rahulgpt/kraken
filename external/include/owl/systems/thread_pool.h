#ifndef thread_pool_h
#define thread_pool_h

#include "../collections/queue.h"
#include <pthread.h>
#include <sys/types.h>

typedef struct __opaq_owl__worker_task
{
    void *(*execute)(void *arg);
    void *arg;
} owl_worker_task_t;

typedef struct __opaq_owl__thread_pool owl_thread_pool_t;

owl_thread_pool_t *owl_thread_pool_init(int num_threads);
void owl_thread_pool_free(owl_thread_pool_t *tp);
owl_worker_task_t owl_worker_task_init(void *(*execute)(void *arg), void *arg);
int owl_thread_pool_enqueue_task(owl_thread_pool_t *tp, owl_worker_task_t *worker_task);

#endif /* thread_pool_h */
