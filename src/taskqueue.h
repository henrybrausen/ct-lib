#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__

#include <pthread.h>
#include <stddef.h>

// Task and Task-Queue implementation
struct task {
  struct task *next;
  struct task *prev;

  // Execution of task corresponds to calling func(func_args)
  void (*func)(void *);
  void *func_args;
};

void task_init(struct task *t, void (*func)(void *), void *func_args);
void task_execute(struct task *t);

struct taskqueue {
  struct task *head;
  struct task *tail;
  size_t count;
  pthread_mutex_t lock;
  pthread_cond_t work_ready;
};

int taskqueue_init(struct taskqueue *q);
void taskqueue_push(struct taskqueue *q, struct task *t);
struct task *taskqueue_pop(struct taskqueue *q);
size_t taskqueue_count(struct taskqueue *q);
struct task *taskqueue_wait_for_work(struct taskqueue *q);
int taskqueue_work_ready(struct taskqueue *q);

#endif

