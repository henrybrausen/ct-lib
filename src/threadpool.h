/**
 * \file threadpool.h
 * \brief Basic thread / worker pool implementation.
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stddef.h>

#include "taskqueue.h"

#define THREADPOOL_DEFAULT_BARRIER_POOLSIZE 8

struct threadpool;

struct threadpool_barrier_args {
  struct threadpool *tp;
  struct barrier *bar;
};

/**
 * \brief Threadpool / worker pool.
 *
 * \class threadpool
 *
 * This struct represents a threadpool instance: a collection of worker threads
 * and an associated work queue.
 */
struct threadpool {
  struct taskqueue queue;

  pthread_t *threads;

  size_t num_threads;
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
 * \brief Wake up worker threads. Workers will begin executing any pending
 * tasks. \memberof threadpool
 *
 * \param tp The thread pool.
 */
void threadpool_notify(struct threadpool *tp);

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

#endif // THREADPOOL_H

