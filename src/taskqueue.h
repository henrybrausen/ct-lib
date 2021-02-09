/**
 * \file taskqueue.h
 * \brief A concurrent task queue for delegation of generic tasks to worker
 * threads.
 *
 * This header declares a struct to represent an abstract task, and a struct to
 * represent a FIFO of tasks awaiting execution by one or more worker threads.
 */

#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <pthread.h>
#include <stddef.h>

#include "pool.h"

#define TASKQUEUE_DEFAULT_POOLSIZE 256

/**
 * \brief Generic task that can be scheduled for execution.
 *
 * \class task
 *
 * A task is a function pointer and argument pair.
 * Execution of a task corresponds to calling func(func_args)
 */
struct task {
  void (*func)(void *);
  void *func_args;
};

/**
 * \brief Execute a task.
 * \memberof task
 *
 * \param t Task to execute.
 */
void task_execute(struct task *t);

/**
 * \brief A single entry in the task queue FIFO.
 *
 * \class taskqueue_entry
 *
 * A taskqueue_entry is an element in a doubly-linked list which contains a
 * task.
 */
struct taskqueue_entry {
  struct taskqueue_entry *next;
  struct taskqueue_entry *prev;

  struct task task;
};

/**
 * \brief Initialize a taskqueue_entry to represent a call to func(func_args).
 * \memberof taskqueue_entry
 *
 * \param te Pointer to taskqueue_entry to initialize
 * \param func Function call for task stored in entry.
 * \param func_args Argument to task function.
 */
void taskqueue_entry_init(struct taskqueue_entry *te, void (*func)(void *),
                          void *func_args);

/**
 * \brief Task queue FIFO for assigning tasks to worker threads.
 *
 * \class taskqueue
 *
 * This struct represents a task queue instance, and all of its associated
 * state.
 */
struct taskqueue {
  struct taskqueue_entry *head;
  struct taskqueue_entry *tail;

  size_t count;         /**< Number of tasks currently in the queue. */
  size_t num_running;   /**< Number of tasks currently running. */
  pthread_mutex_t lock; /**< Mutex for concurrent access. */

  /** Condition variable for notification of waiting threads. */
  pthread_cond_t notify;

  /** Object pool to store taskqueue_entry instances */
  struct pool entry_pool;
};

/**
 * \brief Initialize a task queue.
 * \memberof taskqueue
 *
 * \param q Task queue to initialize.
 * \return 0 on success, non-zero on failure.
 */
int taskqueue_init(struct taskqueue *q);

/**
 * \brief Push a new task into the task queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \param t Task to push.
 * \return 0 on success, non-zero on failure
 */
int taskqueue_push(struct taskqueue *q, struct task t);

/**
 * \brief Retrieve and pop a task from the task queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \param t Pointer at which to store retrieved task.
 * \return 0 on success, non-zero on failure
 */
int taskqueue_pop(struct taskqueue *q, struct task *t);

/**
 * \brief Get number of pending tasks in the queue.
 * \memberof taskqueue
 *
 * \param q The task queue.
 * \return Number of tasks in the queue.
 */
size_t taskqueue_count(struct taskqueue *q);

/**
 * \brief Block and wait for work to appear on the queue.
 * \memberof taskqueue
 *
 * If there is a pending task on the queue, this function pops the task, and
 * stores it at the location pointed to by t immediately. If no task is pending,
 * this function blocks until there is a pending task and the queue is notified.
 *
 * \param q The task queue.
 * \param t Pointer at which to store the popped task.
 * \return 0 on success, non-zero on failure.
 */
int taskqueue_wait_for_work(struct taskqueue *q, struct task *t);

/**
 * \brief Block and wait for all queued and running tasks to complete.
 * \memberof taskqueue
 *
 * \param q The task queue.
 */
void taskqueue_wait_for_complete(struct taskqueue *q);

/**
 * \brief Signal to the queue that a running task has completed.
 * \memberof taskqueue
 *
 * \param q The task queue.
 */
void taskqueue_task_complete(struct taskqueue *q);

/**
 * \brief Notify any blocked processes that the queue state has changed.
 * \memberof taskqueue
 *
 * Calling taskqueue_notify() wakes up any processes that are blocked in calls
 * to taskqueue_wait_for_work() or taskqueue_wait_for_complete(). These
 * processes then re-evaluate the queue state, going back to sleep, or returning
 * depending on the outcome.
 *
 * \param q The task queue.
 */
int taskqueue_notify(struct taskqueue *q);

#endif // TASKQUEUE_H

