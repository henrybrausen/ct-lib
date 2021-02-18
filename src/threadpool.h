/**
 * \file threadpool.h
 * \brief Basic thread / worker pool implementation.
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stddef.h>

#include "task.h"
#include "queue.h"

enum threadpool_thread_command {
  THREAD_COMMAND_RUN,
  THREAD_COMMAND_PAUSE
};

/**
 * \brief Threadpool / worker pool.
 *
 * \class threadpool
 *
 * This struct represents a threadpool instance: a collection of worker threads
 * and an associated work queue.
 *
 * \example sum_example.c
 * Example to demonstrate using threadpool to sum an array in parallel.
 *
 */
struct threadpool {
  struct queue taskqueue;

  pthread_t *threads;

  size_t num_threads;
  size_t num_running;

  pthread_mutex_t lock;
  pthread_cond_t notify;
};

/**
 * \brief Initialize threadpool with num_threads threads.
 * \memberof threadpool
 *
 * \param tp Pointer to thread pool to initialize.
 * \param num_threads Number of worker threads to create in the pool.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_init(struct threadpool *tp, size_t num_threads);

/**
 * \brief Destroy threadpool, freeing resources and leaving threadpool in an uninitialized state.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_destroy(struct threadpool *tp);

/**
 * \brief Block until worker threads complete all queued tasks.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 */
void threadpool_wait(struct threadpool *tp);

/**
 * \brief Queue up task for execution.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 * \param t Task to add to queue.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_push_task(struct threadpool *tp, struct task t);

/**
 * \brief Queue up N identical tasks for execution.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 * \param t Task to add to queue.
 * \param n Number of copies to push to task queue.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_push_n(struct threadpool *tp, struct task t, size_t n);

/**
 * \brief Get number of threads currently in the threadpool.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 */
size_t threadpool_get_num_threads(struct threadpool *tp);

/**
 * \brief Block and wait for work to appear on the queue.
 * \memberof threadpool
 *
 * If there is a pending task on the queue, this function pops the task, and
 * stores it at the location pointed to by t immediately. If no task is pending,
 * this function blocks until there is a pending task and the queue is notified.
 *
 * \param tp The thread pool.
 * \param t Pointer at which to store the popped task.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_wait_for_work(struct threadpool *tp, struct task *t);

/**
 * \brief Signal that a running task has completed.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 */
void threadpool_task_complete(struct threadpool *tp);

/**
 * \brief Notify any blocked processes that the threadpool state has changed.
 * \memberof threadpool
 *
 * Calling threadpool_notify() wakes up any processes that are blocked in calls
 * to threadpool_wait_for_work() or threadpool_wait_for_complete(). These
 * processes then re-evaluate the queue state, going back to sleep, or returning
 * depending on the outcome.
 *
 * \param tp The thread pool.
 */
int threadpool_notify(struct threadpool *tp);

/**
 * \brief Resize threadpool to num_threads threads.
 * \memberof threadpool
 *
 * \param tp The thread pool
 * \param num_threads New number of threads in pool
 * \return 0 on success, non-zero on failure.
 */
int threadpool_set_num_threads(struct threadpool *tp, size_t num_threads);

/**
 * \brief Get number of pending tasks.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 * \return Number of pending tasks.
 */
size_t threadpool_num_pending(struct threadpool *tp);

/**
 * \brief Push barrier / synchronization point to queue.
 * \memberof threadpool
 *
 * A barrier / synchronization point is a synchronization event on the task
 * queue. All threads must reach the barrier before execution of subsequent
 * tasks on the queue can continue.
 *
 * \param tp The thread pool.
 * \return 0 on success, non-zero on failure.
 */
int threadpool_push_barrier(struct threadpool *tp);

/**
 * \brief Worker thread function for use with thread pool.
 *
 * This function loops forever, consuming and executing tasks on the queue.
 * When no work is available, this function will block on
 * threadpool_wait_for_work(). Call threadpool_notify() to wake up all blocked
 * threads and resume task execution.
 *
 * \param tp Thread pool to which worker thread belongs (casted to void *)
 */
void *threadpool_worker_func(void *qp);

#endif // THREADPOOL_H

