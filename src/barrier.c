#include "barrier.h"

#include <pthread.h>
#include <stddef.h>

int barrier_init(struct barrier *b, size_t num_threads)
{
  int err;
  
  err = pthread_mutex_init(&b->lock, NULL);
  if (err) { return err; }

  err = pthread_cond_init(&b->notify, NULL);
  if (err) { return err; }

  b->num_threads = num_threads;
  b->num_blocked = 0;
  b->barrier_reached = 0;

  return 0;
}

int barrier_destroy(struct barrier *b)
{
  int err;

  err = pthread_cond_destroy(&b->notify);
  if (err) { return err; }

  err = pthread_mutex_destroy(&b->lock);
  if (err) { return err; }

  return 0;
}

int barrier_wait(struct barrier *b)
{
  int ret = BARRIER_SERIAL_THREAD;

  pthread_mutex_lock(&b->lock);

  b->num_blocked += 1;

  while (!b->barrier_reached && b->num_blocked != b->num_threads) {
    ret = 0;
    pthread_cond_wait(&b->notify, &b->lock);
  }

  if (ret == BARRIER_SERIAL_THREAD) {
    b->barrier_reached = 1;
  }

  if (--b->num_threads == 0) {
    b->barrier_reached = 0;
    ret = BARRIER_FINAL_THREAD;
  }

  pthread_mutex_unlock(&b->lock);

  return ret;
}

