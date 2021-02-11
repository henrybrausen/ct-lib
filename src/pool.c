#include "pool.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

int pool_init(struct pool *pl, size_t capacity, size_t elem_size)
{
  char *storage;

  pthread_mutex_init(&pl->lock, NULL);

  if ((pl->objects = malloc(capacity * sizeof(pl->objects[0]))) == NULL) {
    return -1;
  }

  pl->capacity = capacity;
  pl->ac_count = 0;
  pl->elem_size = elem_size;

  if ((storage = malloc(elem_size * capacity)) == NULL) { return -1; }

  pl->storage = storage;

  for (size_t i = 0; i < pl->capacity; ++i) {
    pl->objects[i] = storage;
    storage += elem_size;
  }

  return 0;
}

int pool_destroy(struct pool *pl)
{
  free(pl->objects);
  free(pl->storage);
  return pthread_mutex_destroy(&pl->lock);
}

void pool_releaseall(struct pool *pl)
{
  pthread_mutex_lock(&pl->lock);

  char *storage = pl->storage;
  for (size_t i = 0; i < pl->capacity; ++i) {
    pl->objects[i] = storage;
    storage += pl->elem_size;
  }
  pl->ac_count = 0;

  pthread_mutex_unlock(&pl->lock);
}

void *pool_acquire(struct pool *pl)
{
  void *elem = NULL;

  pthread_mutex_lock(&pl->lock);

  if (pl->ac_count == pl->capacity) {
    // We've run out of objects in our pool.
    goto done; // Error condition, resize failed
  }

  elem = pl->objects[pl->ac_count++];

done:
  pthread_mutex_unlock(&pl->lock);
  return elem;
}

int pool_release(struct pool *pl, void *elem)
{
  int ret = 0;

  pthread_mutex_lock(&pl->lock);

  if (pl->ac_count == 0) {
    ret = -1;
    goto done;
  }

  pl->objects[--pl->ac_count] = elem;

done:
  pthread_mutex_unlock(&pl->lock);
  return ret;
}

