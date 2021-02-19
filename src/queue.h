/**
 * \file queue.h
 * \brief Simple linked-list FIFO implementation.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "error.h"

#include <stddef.h>

/**
 * \brief A single entry in the FIFO.
 *
 * \class queue_entry
 *
 * A queue_entry is an element in a doubly-linked list which contains a pointer
 * to a data element.
 */
struct queue_entry {
  struct queue_entry *next;
  struct queue_entry *prev;

  void *data;
};

/**
 * \brief Simple linked-list based FIFO
 *
 * \class queue
 *
 * This struct represents a queue instance, and all of its associated
 * state.
 */
struct queue {
  struct queue_entry *head;
  struct queue_entry *tail;

  size_t count; /**< Number of tasks currently in the queue. */
};

/**
 * \brief Initialize a queue.
 * \memberof queue
 *
 * \param q Pointer to queue to initialize.
 * \return 0 on success, non-zero on failure.
 */
enum ct_err queue_init(struct queue *q);

/**
 * \brief Destroy queue referred to by q, leaving it uninitialized.
 * \memberof queue
 *
 * \param q Pointer to queue to destroy.
 * \return 0 on success, non-zero on error.
 */
enum ct_err queue_destroy(struct queue *q);

/**
 * \brief Push a new element into the queue.
 * \memberof queue
 *
 * \param q The task queue.
 * \param data Data to push.
 * \return 0 on success, non-zero on failure
 */
enum ct_err queue_push(struct queue *q, void *data);

/**
 * \brief Retrieve and pop an element from the queue.
 * \memberof queue
 *
 * \param q The queue.
 * \param t Pointer at which to store pointer to retrieved data.
 * \return 0 on success, non-zero on failure
 */
enum ct_err queue_pop(struct queue *q, void **data);

/**
 * \brief Get number of elements in the queue.
 * \memberof queue
 *
 * \param q The queue.
 * \return Number of elements in the queue.
 */
size_t queue_count(struct queue *q);

#endif // LISTQUEUE_H

