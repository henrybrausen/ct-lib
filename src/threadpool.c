/**
 * \file threadpool.c
 * \brief Implementation of threadpool, including private methods.
 */

#include "threadpool.h"
#include "barrier.h"
#include "error.h"
#include "queue.h"
#include "task.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

enum ct_err threadpool_push_n_(struct threadpool *tp, struct task t, size_t n);
enum ct_err threadpool_pop_locked_(struct threadpool *tp, struct task *t);
void threadpool_cleanup_(void *mutex);
enum ct_err threadpool_wait_for_work_(struct threadpool *tp, struct task *t);
void threadpool_task_complete_(struct threadpool *tp);
void threadpool_barrier_task_func_(void *arg);
void *threadpool_worker_func_(void *tpp);

enum ct_err threadpool_init(struct threadpool *tp, size_t num_threads)
{
  int err;

  err = queue_init(&tp->taskqueue);
  if (err) { return err; }

  err = pthread_mutex_init(&tp->lock, NULL);
  if (err) { return CT_EMUTEX_INIT; }

  err = pthread_cond_init(&tp->notify, NULL);
  if (err) { return CT_ECOND_INIT; }

  if ((tp->threads = malloc(num_threads * sizeof(*tp->threads))) == NULL) {
    return CT_EMALLOC;
  }

  tp->num_threads = num_threads;

  // Spawn threads
  for (size_t i = 0; i < num_threads; ++i) {
    err = pthread_create(&tp->threads[i], NULL, threadpool_worker_func_, tp);
    if (err) { return CT_ETHREAD_CREATE; }
  }

  return CT_SUCCESS;
}

enum ct_err threadpool_destroy(struct threadpool *tp)
{
  int err;

  pthread_mutex_lock(&tp->lock);

  // There must not be any pending tasks.
  if (queue_count(&tp->taskqueue) != 0) {
    err = CT_EPENDING_TASKS;
    goto locked_err;
  }

  // There must not be any running tasks.
  if (tp->num_running != 0) {
    err = CT_ERUNNING_TASKS;
    goto locked_err;
  }

  pthread_mutex_unlock(&tp->lock);

  // Cancel and join all worker threads.
  for (size_t i = 0; i < tp->num_threads; ++i) {
    pthread_cancel(tp->threads[i]);
    pthread_join(tp->threads[i], NULL);
  }

  err = queue_destroy(&tp->taskqueue);
  if (err) { return err; }

  err = pthread_cond_destroy(&tp->notify);
  if (err) { return CT_ECOND_DESTROY; }

  err = pthread_mutex_destroy(&tp->lock);
  if (err) { return CT_EMUTEX_DESTROY; }

  return CT_SUCCESS;

locked_err:
  pthread_mutex_unlock(&tp->lock);
  return err;
}

void threadpool_wait(struct threadpool *tp)
{
  pthread_mutex_lock(&tp->lock);

  // Prepare a cancellation cleanup handler that unlocks &tq->lock.
  // This way, if the thread calling wait_for_work_ gets cancelled, the mutex
  // will be released.
  pthread_cleanup_push(threadpool_cleanup_, &tp->lock);

  while (!(queue_count(&tp->taskqueue) == 0 && tp->num_running == 0)) {
    pthread_cond_wait(&tp->notify, &tp->lock);
  }

  // Release mutex.
  pthread_cleanup_pop(1);
}

enum ct_err threadpool_push_task(struct threadpool *tp, struct task t)
{
  int err;

  struct task *t_new = malloc(sizeof(*t_new));
  if (t_new == NULL) { return CT_EMALLOC; }

  *t_new = t;

  err = task_freeze(t_new);
  if (err) { return err; }

  pthread_mutex_lock(&tp->lock);

  err = queue_push(&tp->taskqueue, t_new);

  pthread_mutex_unlock(&tp->lock);

  return err;
}

size_t threadpool_num_threads(struct threadpool *tp)
{
  size_t ret;
  pthread_mutex_lock(&tp->lock);
  ret = tp->num_threads;
  pthread_mutex_unlock(&tp->lock);
  return ret;
}

void threadpool_notify(struct threadpool *tp)
{
  pthread_cond_broadcast(&tp->notify);
}

size_t threadpool_num_pending(struct threadpool *tp)
{
  size_t count;

  pthread_mutex_lock(&tp->lock);

  count = queue_count(&tp->taskqueue);

  pthread_mutex_unlock(&tp->lock);

  return count;
}

enum ct_err threadpool_push_barrier(struct threadpool *tp)
{
  int err;

  struct barrier *bar = malloc(sizeof(*bar));
  if (bar == NULL) { return CT_EMALLOC; }

  err = barrier_init(bar, tp->num_threads);
  if (err) { return err; }

  err = threadpool_push_n_(
      tp, (struct task){.func = threadpool_barrier_task_func_, .arg = bar},
      tp->num_threads);
  if (err) { return err; }

  return CT_SUCCESS;
}

/**
 * \brief Queue up N identical tasks for execution.
 * \memberof threadpool
 * \private
 *
 * \param tp The thread pool.
 * \param t Task to add to queue.
 * \param n Number of copies to push to task queue.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err threadpool_push_n_(struct threadpool *tp, struct task t, size_t n)
{
  int err;

  pthread_mutex_lock(&tp->lock);

  for (size_t i = 0; i < n; ++i) {
    struct task *t_new = malloc(sizeof(*t_new));
    if (t_new == NULL) {
      err = CT_EMALLOC;
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

/**
 * \brief Pop a task from the thread pool's task queue. Assumes thread pool has
 * been locked by the caller. \memberof threadpool \private
 *
 * \param tp The thread pool.
 * \param t Pointer to storage for popped task. If t == NULL, discard popped
 * task.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err threadpool_pop_locked_(struct threadpool *tp, struct task *t)
{
  int err;

  if (queue_count(&tp->taskqueue) == 0) { return CT_EQUEUE_EMPTY; }

  struct task *tsk;

  err = queue_pop(&tp->taskqueue, (void **)&tsk);
  if (err) { return err; }

  // If we are discarding the task, make sure to free any associated resources.
  if (t == NULL) { task_destroy(tsk); }
  else {
    *t = *tsk;
  }

  free(tsk);

  return CT_SUCCESS;
}

/**
 * \brief Cancellation cleanup handler for worker thread.
 */
void threadpool_cleanup_(void *mutex)
{
  pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

/**
 * \brief Block and wait for work to appear on the queue.
 * \memberof threadpool
 * \private
 *
 * If there is a pending task on the queue, this function pops the task, and
 * stores it at the location pointed to by t immediately. If no task is pending,
 * this function blocks until there is a pending task and the queue is notified.
 *
 * \param tp The thread pool.
 * \param t Pointer at which to store the popped task.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err threadpool_wait_for_work_(struct threadpool *tp, struct task *t)
{
  int ret;
  pthread_mutex_lock(&tp->lock);

  // Prepare a cancellation cleanup handler that unlocks &tq->lock.
  // This way, if the thread calling wait_for_work gets cancelled, the mutex
  // will be released.
  pthread_cleanup_push(threadpool_cleanup_, &tp->lock);

  while (queue_count(&tp->taskqueue) == 0) {
    pthread_cond_wait(&tp->notify, &tp->lock);
  }
  if ((ret = threadpool_pop_locked_(tp, t)) != 0) {
    // NOTE: This is an error condition.
  }
  else {
    tp->num_running += 1;
  }

  // Release mutex.
  pthread_cleanup_pop(1);

  return ret;
}

/**
 * \brief Signal that a running task has completed.
 * \memberof threadpool
 * \private
 *
 * \param tp The thread pool.
 */
void threadpool_task_complete_(struct threadpool *tp)
{
  pthread_mutex_lock(&tp->lock);
  tp->num_running -= 1;
  pthread_mutex_unlock(&tp->lock);
  threadpool_notify(tp);
}

/**
 * \brief Barrier task for threadpool.
 * \memberof threadpool
 * \private
 *
 * Wait for all threads to reach a synchronization barrier, and clean up if
 * necessary.
 *
 * \param arg Pointer to barrier struct casted to void *
 */
void threadpool_barrier_task_func_(void *arg)
{
  struct barrier *bar = (struct barrier *)arg;
  int ret = barrier_wait(bar);

  if (ret == BARRIER_FINAL_THREAD) {
    // If we were the final thread to exit the barrier, clean up after ourselves
    barrier_destroy(bar);
  }
}

/**
 * \brief Worker thread function for use with thread pool.
 * \memberof threadpool
 * \private
 *
 * This function loops forever, consuming and executing tasks on the queue.
 * When no work is available, this function will block on
 * threadpool_wait_for_work_(). Call threadpool_notify() to wake up all blocked
 * threads and resume task execution.
 *
 * \param tp Thread pool to which worker thread belongs (casted to void *)
 */
void *threadpool_worker_func_(void *tpp)
{
  struct threadpool *tp = (struct threadpool *)tpp;
  struct task t = {};

  for (;;) {
    threadpool_wait_for_work_(tp, &t);
    task_execute(&t);
    threadpool_task_complete_(tp);
  }
  return (void *)0;
}

