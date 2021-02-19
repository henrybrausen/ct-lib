#include "queue.h"
#include "error.h"

#include <stddef.h>
#include <stdlib.h>

enum ct_err queue_init(struct queue *q)
{
  q->head = q->tail = NULL;
  q->count = 0;

  return CT_SUCCESS;
}

enum ct_err queue_destroy(struct queue *q)
{
  struct queue_entry *cur = q->head;
  struct queue_entry *next;

  while (cur != NULL) {
    next = cur->next;
    free(cur);
    cur = next;
  }

  return CT_SUCCESS;
}

enum ct_err queue_push(struct queue *q, void *data)
{
  struct queue_entry *e = malloc(sizeof(*e));

  if (e == NULL) { return CT_EMALLOC; }

  e->next = q->head;
  e->prev = NULL;
  e->data = data;

  if (q->head == NULL) { q->tail = e; }
  else {
    q->head->prev = e;
  }
  q->head = e;

  q->count += 1;

  return CT_SUCCESS;
}

enum ct_err queue_pop(struct queue *q, void **data)
{
  if (q->count == 0) { return CT_EQUEUE_EMPTY; }

  struct queue_entry *e = q->tail;

  if (q->count != 1) {
    q->tail = q->tail->prev;
    q->tail->next = NULL;
  }
  else {
    q->head = q->tail = NULL;
  }

  q->count--;

  if (data != NULL) {
    *data = e->data;
  }

  free(e);

  return CT_SUCCESS;
}

size_t queue_count(struct queue *q) { return q->count; }

