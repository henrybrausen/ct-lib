#include "pool.h"
#include <stdlib.h>

int pool_init(struct pool *pl, size_t capacity, size_t elem_size)
{
  int err;
  char *storage;

  pthread_rwlock_init(&pl->lock, NULL);

  if ((pl->objects = malloc(capacity * sizeof(pl->objects[0]))) == NULL) {
    return -1;
  }

  pl->capacity = capacity;
  pl->count = capacity;
  pl->elem_size = elem_size;

  if ((storage = malloc(elem_size * capacity)) == NULL) { return -1; }

  pl->storage = storage;

  for (size_t i = 0; i < pl->capacity; ++i) {
    pl->objects[i] = storage;
    storage += elem_size;
  }

  return 0;
}

void pool_releaseall(struct pool *pl)
{
  pthread_rwlock_wrlock(&pl->lock);

  char *storage = pl->storage;
  for (size_t i = 0; i < pl->capacity; ++i) {
    pl->objects[i] = storage;
    storage += pl->elem_size;
  }
  pl->count = pl->capacity;

  pthread_rwlock_unlock(&pl->lock);
}

void *pool_acquire(struct pool *pl)
{
  void *elem = NULL;

  pthread_rwlock_wrlock(&pl->lock);

  if (pl->count == 0) { goto done; }

  elem = pl->objects[--pl->count];

done:
  pthread_rwlock_unlock(&pl->lock);
  return elem;
}

int pool_release(struct pool *pl, void *elem)
{
  int ret = 0;

  pthread_rwlock_wrlock(&pl->lock);

  if (pl->count == pl->capacity) {
    ret = -1;
    goto done;
  }

  pl->objects[pl->count++] = elem;

done:
  pthread_rwlock_unlock(&pl->lock);
  return ret;
}
