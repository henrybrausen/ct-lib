#include "threadpool.h"
#include "barrier.h"
#include "queue.h"
#include "task.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

int threadpool_init(struct threadpool *tp, size_t num_threads)
{
  int err;

  err = queue_init(&tp->taskqueue);
  if (err) { return err; }

  err = pthread_mutex_init(&tp->lock, NULL);
  if (err) { return err; }

  err = pthread_cond_init(&tp->notify, NULL);
  if (err) { return err; }

  if ((tp->threads = malloc(num_threads * sizeof(*tp->threads))) == NULL) {
    return -1;
  }

  tp->num_threads = num_threads;

  for (size_t i = 0; i < num_threads; ++i) {
    err = pthread_create(&tp->threads[i], NULL, threadpool_worker_func, tp);
    if (err) { return err; }
  }

  return 0;
}

int threadpool_destroy(struct threadpool *tp)
{
  int err;

  // Clean up any remaining tasks in the queue
  while (queue_count(&tp->taskqueue) > 0) {
    struct task *t;

    err = queue_pop(&tp->taskqueue, (void **)&t);
    if (err) { return err; }

    task_destroy(t);
  }

  err = queue_destroy(&tp->taskqueue);
  if (err) { return err; }

  err = pthread_cond_destroy(&tp->notify);
  if (err) { return err; }

  err = pthread_mutex_destroy(&tp->lock);
  if (err) { return err; }

  return 0;
}

int threadpool_push_task(struct threadpool *tp, struct task t)
{
  int err;

  struct task *t_new = malloc(sizeof(*t_new));
  if (t_new == NULL) { return -1; }

  *t_new = t;

  err = task_freeze(t_new);
  if (err) { return err; }

  pthread_mutex_lock(&tp->lock);

  err = queue_push(&tp->taskqueue, t_new);

  pthread_mutex_unlock(&tp->lock);

  return err;
}

int threadpool_push_n(struct threadpool *tp, struct task t, size_t n)
{
  int err;

  pthread_mutex_lock(&tp->lock);

  for (size_t i = 0; i < n; ++i) {
    struct task *t_new = malloc(sizeof(*t_new));
    if (t_new == NULL) {
      err = -1;
      break;
    }

    *t_new = t;

    err = task_freeze(t_new);
    if (err) { break; }

    err = queue_push(&tp->taskqueue, t_new);
    if (err) { break; }
  }

  pthread_mutex_unlock(&tp->lock);

  return err;
}

int threadpool_pop_locked(struct threadpool *tp, struct task *t)
{
  int err;

  if (t == NULL) { return -1; }

  if (queue_count(&tp->taskqueue) == 0) { return -1; }

  struct task *tsk;

  err = queue_pop(&tp->taskqueue, (void **)&tsk);
  if (err) { return err; }

  *t = *tsk;

  free(tsk);

  return 0;
}

void threadpool_cleanup(void *mutex)
{
  pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

int threadpool_wait_for_work(struct threadpool *tp, struct task *t)
{
  int ret;
  pthread_mutex_lock(&tp->lock);

  // Prepare a cancellation cleanup handler that unlocks &tq->lock.
  // This way, if the thread calling wait_for_work gets cancelled, the mutex
  // will be released.
  pthread_cleanup_push(threadpool_cleanup, &tp->lock);

  while (queue_count(&tp->taskqueue) == 0) {
    pthread_cond_wait(&tp->notify, &tp->lock);
  }
  if ((ret = threadpool_pop_locked(tp, t)) != 0) {
    // NOTE: This is an error condition.
  }
  else {
    tp->num_running += 1;
  }

  // Release mutex.
  pthread_cleanup_pop(1);

  return ret;
}

void threadpool_wait(struct threadpool *tp)
{
  pthread_mutex_lock(&tp->lock);

  // Prepare a cancellation cleanup handler that unlocks &tq->lock.
  // This way, if the thread calling wait_for_work gets cancelled, the mutex
  // will be released.
  pthread_cleanup_push(threadpool_cleanup, &tp->lock);

  while (!(queue_count(&tp->taskqueue) == 0 && tp->num_running == 0)) {
    pthread_cond_wait(&tp->notify, &tp->lock);
  }

  // Release mutex.
  pthread_cleanup_pop(1);
}

int threadpool_notify(struct threadpool *tp)
{
  return pthread_cond_broadcast(&tp->notify);
}

void threadpool_task_complete(struct threadpool *tp)
{
  pthread_mutex_lock(&tp->lock);
  tp->num_running -= 1;
  pthread_mutex_unlock(&tp->lock);
  threadpool_notify(tp);
}

size_t threadpool_num_pending(struct threadpool *tp)
{
  size_t count;

  pthread_mutex_lock(&tp->lock);

  count = queue_count(&tp->taskqueue);

  pthread_mutex_unlock(&tp->lock);

  return count;
}

void threadpool_barrier_task_func(void *arg)
{
  struct barrier *bar = (struct barrier *)arg;
  int ret = barrier_wait(bar);

  if (ret == BARRIER_FINAL_THREAD) {
    // If we were the final thread to exit the barrier, clean up after ourselves
    barrier_destroy(bar);
  }
}

int threadpool_push_barrier(struct threadpool *tp)
{
  int err;

  struct barrier *bar = malloc(sizeof(*bar));
  if (bar == NULL) { return -1; }

  err = barrier_init(bar, tp->num_threads);
  if (err) { return err; }

  err = threadpool_push_n(
      tp, (struct task){.func = threadpool_barrier_task_func, .arg = bar},
      tp->num_threads);
  if (err) { return err; }

  return 0;
}

void *threadpool_worker_func(void *tpp)
{
  struct threadpool *tp = (struct threadpool *)tpp;
  struct task t = {};

  for (;;) {
    threadpool_wait_for_work(tp, &t);
    task_execute(&t);
    threadpool_task_complete(tp);
  }
  return (void *)0;
}

