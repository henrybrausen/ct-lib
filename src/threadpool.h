/**
 * \file threadpool.h
 * \brief Basic thread / worker pool.
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stddef.h>

#include "queue.h"
#include "task.h"
#include "error.h"

enum threadpool_state {
  THREADPOOL_RUNNING,
  THREADPOOL_PAUSED
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

  enum threadpool_state state;
};

/**
 * \brief Initialize threadpool with num_threads threads.
 * \memberof threadpool
 *
 * \param tp Pointer to thread pool to initialize.
 * \param num_threads Number of worker threads to create in the pool.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err threadpool_init(struct threadpool *tp, size_t num_threads);

/**
 * \brief Destroy threadpool, freeing resources and leaving threadpool in an
 * uninitialized state.
 * \memberof threadpool
 *
 * The thread pool must not have any pending or running tasks.
 *
 * \param tp The thread pool.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err threadpool_destroy(struct threadpool *tp);

/**
 * \brief Signal thread pool to begin task execution.
 * \memberof threadpool
 * 
 * \param tp The thread pool.
 */
void threadpool_run(struct threadpool *tp);

/**
 * \brief Signal thread pool to pause task execution.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 */
void threadpool_pause(struct threadpool *tp);

/**
 * \brief Block until worker threads complete all queued tasks.
 * \memberof threadpool
 *
 * Note: This function blocks until all tasks are completed. If the threadpool
 * is paused, threadpool_wait() will continue to block!
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
enum ct_err threadpool_push_task(struct threadpool *tp, struct task t);

/**
 * \brief Get number of threads currently in the threadpool.
 * \memberof threadpool
 *
 * \param tp The thread pool.
 */
size_t threadpool_num_threads(struct threadpool *tp);

/**
 * \brief Notify any blocked processes that the threadpool state has changed.
 * \memberof threadpool
 *
 * Calling threadpool_notify() wakes up any processes that are blocked in calls
 * to threadpool_wait_for_work_() or threadpool_wait_for_complete_(). These
 * processes then re-evaluate the queue state, going back to sleep, or returning
 * depending on the outcome.
 *
 * \param tp The thread pool.
 */
void threadpool_notify(struct threadpool *tp);

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
enum ct_err threadpool_push_barrier(struct threadpool *tp);

#endif // THREADPOOL_H

