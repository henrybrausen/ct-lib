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
#include <stddef.h>

#define BARRIER_SERIAL_THREAD 1

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

  /** Whether or not barrier has been reached by all threads. */
  int barrier_reached;
};

/**
 * \brief Initialize synchronization barrier.
 * \memberof barrier
 *
 * \param b Pointer to barrier to initialize.
 * \param num_threads Number of threads that this barrier will synchronize.
 */
int barrier_init(struct barrier *b, size_t num_threads);

/**
 * \brief Destroy barrier referred to by b, leaving it uninitialized.
 * \memberof barrier
 *
 * \param b Pointer to barrier to destroy.
 */
int barrier_destroy(struct barrier *b);

/**
 * \brief Synchronize calling thread with barrier.
 * \memberof barrier
 *
 * The calling thread will block until the required number of threads have
 * reached the barrier.
 *
 * \param b The barrier.
 * \return BARRIER_SERIAL_THREAD if the calling thread did not block (calling
 * thread was the final thread to reach the barrier), zero otherwise.
 */
int barrier_wait(struct barrier *b);

#endif // BARRIER_H

