#ifndef __POOL_H__
#define __POOL_H__

#include <pthread.h>
#include <stddef.h>

// Simple object pool implementation
struct pool {
  size_t capacity;
  size_t count;
  size_t elem_size;
  void **objects;
  pthread_rwlock_t lock;

  char *storage; // Internal use
};

int pool_init(struct pool *pl, size_t capacity, size_t elem_size);

void pool_releaseall(struct pool *pl);

void *pool_acquire(struct pool *pl);

int pool_release(struct pool *pl, void *elem);

#endif // __POOL_H__

