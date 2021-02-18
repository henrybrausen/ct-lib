/**
 * \file task.h
 * \brief A single task that can be scheduled for execution by a threadpool.
 */

#ifndef TASK_H
#define TASK_H

#include <stddef.h>

/**
 * \brief Generic task that can be scheduled for execution.
 *
 * \class task
 *
 * A task is a function pointer and argument pair.
 * Execution of a task corresponds to calling func(arg)
 *
 * Each task owns the memory associated with its argument, and frees this
 * memory after execution. As a result, a task may only be executed once!
 */
struct task {
  void (*func)(void *);
  void *arg;
  size_t arg_size;
};

/**
 * \brief Execute a task.
 * \memberof task
 *
 * \param t Task to execute.
 */
void task_execute(struct task *t);

/**
 * \brief Free any resources associated with task t, leaving t in an
 * uninitialized state.
 * \memberof task
 *
 * \param t Task to destroy.
 */
void task_destroy(struct task *t);

/**
 * \brief Create local copy of function argument for task.
 * \memberof task
 *
 * \param t Task to freeze.
 * \return 0 on success, non-zero on failure.
 */
int task_freeze(struct task *t);


#endif // TASK_H

