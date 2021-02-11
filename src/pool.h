/**
 * \file pool.h
 * \brief Basic object pool implementation which supports concurrent access.
 *
 * This header declares a struct to represent a simple, generic object pool,
 * along with functions that operate on the pool.
 *
 * \todo Add option to align stored object on cache line boundaries to prevent
 * possible performance degredation due to false sharing.
 */

#ifndef POOL_H
#define POOL_H

#include <pthread.h>
#include <stddef.h>


/**
 * \brief Generic object pool that supports concurrent access.
 *
 * \class pool
 *
 * This struct represents an object pool instance, and all its associated
 * state.
 */
struct pool {
  size_t capacity;  /**< Maximum number of objects that the pool can store. */
  size_t ac_count;  /**< Number of objects not currently in the pool. */
  size_t elem_size; /**< Size of a stored object, in bytes. */
  void **objects;   /**< Array of pointers to objects currently in pool. */

  pthread_mutex_t lock; /**< Mutex for concurrent access. */

  char *storage; /**< Pointer to underlying object storage. Internal use. */
};

/**
 * \brief Initialize object pool and allocate pool storage.
 * \memberof pool
 *
 * \param pl Pointer to pool to initialize.
 * \param capacity Number of objects to store in the pool.
 * \param elem_size Size of a stored object, in bytes.
 * \return 0 on success, -1 on error.
 */
int pool_init(struct pool *pl, size_t capacity, size_t elem_size);

/**
 * \brief Destroy object pool referred to by pl, leaving it uninitialized.
 * \memberof pool
 *
 * \param pl Pointer to pool to destroy.
 * \return 0 on success, non-zero on error.
 */
int pool_destroy(struct pool *pl);

/**
 * \brief Release/return all stored objects back into the pool. This invalidates
 * any outstanding references to pool objects!
 * \memberof pool
 *
 * \param pl Pointer to the pool.
 */
void pool_releaseall(struct pool *pl);

/**
 * \brief Acquire an object from the pool.
 * \memberof pool
 *
 * \param pl Pointer to the pool.
 * \return Pointer to storage for the object, or NULL if error.
 */
void *pool_acquire(struct pool *pl);

/**
 * \brief Release/return an object back into the pool.
 * \memberof pool
 *
 * \param pl Pointer to the pool.
 * \return 0 on success, -1 on error.
 */
int pool_release(struct pool *pl, void *elem);

#endif // POOL_H

