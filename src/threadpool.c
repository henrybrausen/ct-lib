#include "threadpool.h"
#include "barrier.h"
#include "pool.h"
#include "taskqueue.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

int threadpool_init(struct threadpool *tp, size_t num_threads)
{
  int err;

  err = taskqueue_init(&tp->queue);
  if (err) { return err; }

  err = pool_init(&tp->barrier_pool, THREADPOOL_DEFAULT_BARRIER_POOLSIZE,
                  sizeof(struct barrier));
  if (err) { return err; }

  err =
      pool_init(&tp->barrier_taskarg_pool, THREADPOOL_DEFAULT_BARRIER_POOLSIZE,
                sizeof(struct threadpool_barrier_args));
  if (err) { return err; }

  if ((tp->threads = malloc(num_threads * sizeof(*tp->threads))) == NULL) {
    return -1;
  }

  for (size_t i = 0; i < num_threads; ++i) {
    err = pthread_create(&tp->threads[i], NULL, taskqueue_basic_worker_func,
                         &tp->queue);
    if (err) { return err; }
  }

  return 0;
}

void threadpool_wait(struct threadpool *tp)
{
  taskqueue_wait_for_complete(&tp->queue);
}

int threadpool_push_task(struct threadpool *tp, struct task t)
{
  return taskqueue_push(&tp->queue, t);
}

void threadpool_notify(struct threadpool *tp) { taskqueue_notify(&tp->queue); }

void threadpool_barrier_task_func(void *arg)
{
  struct threadpool_barrier_args *ta = (struct threadpool_barrier_args *)arg;

  int ret = barrier_wait(ta->bar);

  if (ret == BARRIER_FINAL_THREAD) {
    // If we were the final thread to exit the barrier, clean up after ourselves
    pool_release(&ta->tp->barrier_pool, ta->bar);
    pool_release(&ta->tp->barrier_taskarg_pool, ta);
  }
}

int threadpool_push_barrier(struct threadpool *tp)
{
  int err;

  struct threadpool_barrier_args *ta = pool_acquire(&tp->barrier_taskarg_pool);
  if (ta == NULL) { return -1; }

  struct barrier *bar = pool_acquire(&tp->barrier_pool);
  if (bar == NULL) { return -1; }

  err = barrier_init(bar, tp->num_threads);
  if (err) { return err; }

  for (size_t i = 0; i < tp->num_threads; ++i) {
    err = threadpool_push_task(
        tp,
        (struct task){.func = threadpool_barrier_task_func, .func_args = ta});
    if (err) { return err; }
  }

  return 0;
}

