#ifndef TASKQUEUE_H
#define TASKQUEUE_H

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
  size_t num_running;
  pthread_mutex_t lock;
  pthread_cond_t notify;
};

int taskqueue_init(struct taskqueue *q);
void taskqueue_push(struct taskqueue *q, struct task *t);
struct task *taskqueue_pop(struct taskqueue *q);
size_t taskqueue_count(struct taskqueue *q);
struct task *taskqueue_wait_for_work(struct taskqueue *q);
void taskqueue_wait_for_complete(struct taskqueue *q);
void taskqueue_task_complete(struct taskqueue *q);
int taskqueue_notify(struct taskqueue *q);

#endif

