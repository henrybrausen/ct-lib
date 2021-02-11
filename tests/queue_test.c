/**
 * \file queue_test.c
 * \brief Test queue data structure.
 *
 * Pre-fill queue with some data, and then randomly push/pop queue, verifying
 * the integrity of the underlying list along the way.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "queue.h"

#define MAX_QUEUESIZE 10000
#define NUM_OPS 10000

// Simple function to check the integrity of the queue's linked list.
void check_integrity(struct queue *q)
{
  if (q->count == 0) {
    assert(q->head == NULL);
    assert(q->tail == NULL);
  }
  else if (q->count == 1) {
    assert(q->head != NULL);
    assert(q->head == q->tail);
  }
  else {
    assert(q->head != NULL);
    int count = 0;
    struct queue_entry *e = q->head;
    while (e->next != NULL) {
      e = e->next;
      assert(e->prev != NULL);
      count += 1;
    }
    assert(count == q->count);
  }
}

int main(int argc, char *argv[])
{
  srand(time(NULL));

  struct queue q;

  queue_init(&q);

  // Put some data into the queue.
  for (size_t i = 0; i < MAX_QUEUESIZE / 2; ++i) {
    queue_push(&q, 0);
  }

  void *data;

  for (size_t i = 0; i < NUM_OPS; ++i) {
    size_t count = queue_count(&q);
    if (count == 0) { queue_push(&q, 0); }
    else if (count == MAX_QUEUESIZE) {
      queue_pop(&q, &data);
    }
    else {
      if (rand() > (RAND_MAX / 2)) { queue_push(&q, 0); }
      else {
        queue_pop(&q, &data);
      }
    }
    check_integrity(&q);
  }

  queue_destroy(&q);

return 0;
}

