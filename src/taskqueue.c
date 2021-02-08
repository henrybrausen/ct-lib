#include "taskqueue.h"
#include <stdlib.h>

void task_init(struct task *t, void (*func)(void *), void *func_args)
{
  t->next = t->prev = NULL;
  t->func = func;
  t->func_args = func_args;
}

void task_execute(struct task *t) { t->func(t->func_args); }

int taskqueue_init(struct taskqueue *q)
{
  int err;

  q->head = q->tail = NULL;
  q->count = 0;

  err = pthread_mutex_init(&q->lock, NULL);
  if (err) { return err; }

  err = pthread_cond_init(&q->work_ready, NULL);
  if (err) { return err; }

  return 0;
}

void taskqueue_push(struct taskqueue *q, struct task *t)
{
  pthread_mutex_lock(&q->lock);

  t->next = q->head;
  t->prev = NULL;

  if (q->head == NULL) { q->tail = t; }
  else {
    q->head->prev = t;
  }
  q->head = t;

  q->count += 1;

  pthread_mutex_unlock(&q->lock);
}

struct task *taskqueue_pop_locked(struct taskqueue *q)
{
  struct task *elem = NULL;

  if (q->count == 0) { return NULL; }

  elem = q->tail;

  if (q->count != 1) { q->tail = q->tail->prev; }
  else {
    q->head = q->tail = NULL;
  }

  q->count--;

  elem->next = elem->prev = NULL;

  return elem;
}

struct task *taskqueue_pop(struct taskqueue *q)
{
  struct task *elem = NULL;

  pthread_mutex_lock(&q->lock);

  elem = taskqueue_pop_locked(q);

  pthread_mutex_unlock(&q->lock);

  return elem;
}

size_t taskqueue_count(struct taskqueue *q)
{
  size_t count;

  pthread_mutex_lock(&q->lock);
  count = q->count;
  pthread_mutex_unlock(&q->lock);

  return count;
}

struct task *taskqueue_wait_for_work(struct taskqueue *q)
{
  struct task *ret;
  pthread_mutex_lock(&q->lock);

  while (q->count == 0) {
    pthread_cond_wait(&q->work_ready, &q->lock);
  }
  if ((ret = taskqueue_pop_locked(q)) == NULL) {
    // Error condition
  }
  pthread_mutex_unlock(&q->lock);
  return ret;
}

int taskqueue_work_ready(struct taskqueue *q)
{
  return pthread_cond_broadcast(&q->work_ready);
}

