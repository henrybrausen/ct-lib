#include "taskqueue.h"
#include "queue.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void task_execute(struct task *t)
{
  t->func(t->arg);

  // Destroy task after execution, freeing memory associated with task argument
  task_destroy(t);
}

void task_destroy(struct task *t)
{
  if (t->arg_size > 0) { free(t->arg); }
}

int task_freeze(struct task *t)
{
  if (t->arg_size == 0) {
    // Special case, do not allocate.
    return 0;
  }

  void *p = malloc(t->arg_size);

  if (p == NULL) { return -1; }

  t->arg = memcpy(p, t->arg, t->arg_size);

  return 0;
}

int taskqueue_init(struct taskqueue *q)
{
  int err;

  err = queue_init(&q->queue);
  if (err) { return err; }

  err = pthread_mutex_init(&q->lock, NULL);
  if (err) { return err; }

  err = pthread_cond_init(&q->notify, NULL);
  if (err) { return err; }

  return 0;
}

int taskqueue_destroy(struct taskqueue *q)
{
  int err;

  // Clean up any remaining tasks in the queue
  while (queue_count(&q->queue) > 0) {
    struct task *t;

    err = queue_pop(&q->queue, (void **)&t);
    if (err) { return err; }

    task_destroy(t);
  }

  err = queue_destroy(&q->queue);
  if (err) { return err; }

  err = pthread_cond_destroy(&q->notify);
  if (err) { return err; }

  err = pthread_mutex_destroy(&q->lock);
  if (err) { return err; }

  return 0;
}

int taskqueue_push(struct taskqueue *q, struct task t)
{
  int err;

  struct task *tp = malloc(sizeof(*tp));
  if (tp == NULL) { return -1; }

  *tp = t;

  err = task_freeze(tp);
  if (err) { return err; }

  pthread_mutex_lock(&q->lock);

  err = queue_push(&q->queue, tp);

  pthread_mutex_unlock(&q->lock);

  return err;
}

int taskqueue_push_n(struct taskqueue *q, struct task t, size_t n)
{
  int err;

  struct task *tp = malloc(sizeof(*tp));
  if (tp == NULL) { return -1; }

  *tp = t;

  err = task_freeze(tp);
  if (err) { return err; }

  pthread_mutex_lock(&q->lock);

  for (size_t i = 0; i < n; ++i) {
    err = queue_push(&q->queue, tp);
    if (err) { break; }
  }

  pthread_mutex_unlock(&q->lock);

  return err;
}

int taskqueue_pop_locked(struct taskqueue *q, struct task *t)
{
  int err;

  if (t == NULL) { return -1; }

  if (queue_count(&q->queue) == 0) { return -1; }

  struct task *tp;

  err = queue_pop(&q->queue, (void **)&tp);
  if (err) { return err; }

  *t = *tp;

  free(tp);

  return 0;
}

int taskqueue_pop(struct taskqueue *q, struct task *t)
{
  int err;
  pthread_mutex_lock(&q->lock);

  err = queue_pop(&q->queue, (void **)t);

  pthread_mutex_unlock(&q->lock);

  return err;
}

size_t taskqueue_count(struct taskqueue *q)
{
  size_t count;

  pthread_mutex_lock(&q->lock);

  count = queue_count(&q->queue);

  pthread_mutex_unlock(&q->lock);

  return count;
}

int taskqueue_wait_for_work(struct taskqueue *q, struct task *t)
{
  int ret;
  pthread_mutex_lock(&q->lock);

  while (queue_count(&q->queue) == 0) {
    pthread_cond_wait(&q->notify, &q->lock);
  }
  if ((ret = taskqueue_pop_locked(q, t)) != 0) {
    // NOTE: This is an error condition.
  }
  else {
    q->num_running += 1;
  }
  pthread_mutex_unlock(&q->lock);
  return ret;
}

void taskqueue_wait_for_complete(struct taskqueue *q)
{
  pthread_mutex_lock(&q->lock);

  while (!(queue_count(&q->queue) == 0 && q->num_running == 0)) {
    pthread_cond_wait(&q->notify, &q->lock);
  }

  pthread_mutex_unlock(&q->lock);
}

int taskqueue_notify(struct taskqueue *q)
{
  return pthread_cond_broadcast(&q->notify);
}

void taskqueue_task_complete(struct taskqueue *q)
{
  pthread_mutex_lock(&q->lock);
  q->num_running -= 1;
  pthread_mutex_unlock(&q->lock);
  taskqueue_notify(q);
}

void *taskqueue_basic_worker_func(void *qp)
{
  struct taskqueue *q = (struct taskqueue *)qp;
  struct task t = {};

  for (;;) {
    taskqueue_wait_for_work(q, &t);
    task_execute(&t);
    taskqueue_task_complete(q);
  }
  return (void *)0;
}

