#include "queue.h"

#include <stddef.h>
#include <stdlib.h>

int queue_init(struct queue *q)
{
  q->head = q->tail = NULL;
  q->count = 0;

  return 0;
}

int queue_destroy(struct queue *q)
{
  struct queue_entry *cur = q->head;
  struct queue_entry *next;

  while (cur != NULL) {
    next = cur->next;
    free(cur);
    cur = next;
  }

  return 0;
}

int queue_push(struct queue *q, void *data)
{
  struct queue_entry *e = malloc(sizeof(*e));

  if (e == NULL) { return -1; }

  e->next = q->head;
  e->prev = NULL;
  e->data = data;

  if (q->head == NULL) { q->tail = e; }
  else {
    q->head->prev = e;
  }
  q->head = e;

  q->count += 1;

  return 0;
}

int queue_pop(struct queue *q, void **data)
{
  if (data == NULL) { return -1; }

  if (q->count == 0) { return -1; }

  struct queue_entry *e = q->tail;

  if (q->count != 1) {
    q->tail = q->tail->prev;
    q->tail->next = NULL;
  }
  else {
    q->head = q->tail = NULL;
  }

  q->count--;

  *data = e->data;

  free(e);

  return 0;
}

size_t queue_count(struct queue *q) { return q->count; }

