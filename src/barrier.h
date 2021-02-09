/**
 * \file barrier.h
 * \brief Thread barrier / synchronization point implementation.
 *
 * Unfortunately, not all platforms that implement pthreads support
 * pthread_barrier_t. Hence this simple implementation of thread sync barriers
 * using condition variables and an integral count.
 */

#ifndef BARRIER_H
#define BARRIER_H

#include <pthread.h>

/**
 * \brief Struct to represent an instance of a thread barrier.
 *
 * \class barrier
 */
struct barrier {
  pthread_mutex_t lock;
  pthread_cond_t notify;

  /** Total number of threads synchronized by this barrier. */
  size_t num_threads;

  /** Number of threads currently blocked at this barrier. */
  size_t num_blocked;
};

int barrier_init(struct barrier *b);

int barrier_destroy(struct barrier *b);

void barrier_wait(struct barrier *b);

#endif // BARRIER_H

