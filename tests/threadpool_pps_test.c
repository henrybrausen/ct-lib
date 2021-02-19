/**
 * \file threadpool_pps_test.c
 * \brief Unit test of threadpool based on computing the parallel prefix sum.
 *
 * Computes the prefix sum of an array of numbers in parallel, and then checks
 * the result against the result of a linear scan.
 */

#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "threadpool.h"
#include "tictoc.h"

#define NUM_ELEMS_POW 22
// Must be 2^N
#define NUM_ELEMS (1ull << NUM_ELEMS_POW)

#define NUM_THREADS 8

int *array, *array2;

struct threadpool tp;

// Argument to a task is semi-open range [begin, end)
struct task_arg {
  size_t begin, end;
  int d; // Parameter for parallel prefix sum
};

void init()
{
  printf("Allocating array...\n");
  if ((array = malloc(NUM_ELEMS * sizeof(*array))) == NULL) {
    printf("Could not allocate array!\n");
    exit(1);
  }
  threadpool_init(&tp, NUM_THREADS);
}

void randomize_array_task(void *arg)
{
  struct task_arg *range = (struct task_arg *)arg;
  unsigned int seed = range->begin;

  for (size_t i = range->begin; i != range->end; ++i) {
    array[i] = rand_r(&seed) - (RAND_MAX >> 1);
  }
}

void pps_upsweep(void *arg)
{
  struct task_arg *range = (struct task_arg *)arg;

  size_t step = 1ull << (range->d + 1ull);
  // Round to next power of 2
  size_t begin = (range->begin + step - 1) & -step;
  for (size_t i = begin; i < range->end; i += step) {
    array[i + step - 1] += array[i + (step >> 1) - 1];
  }
}

void pps_downsweep(void *arg)
{
  struct task_arg *range = (struct task_arg *)arg;

  size_t step = 1ull << (range->d + 1ull);
  // Round to next power of 2
  size_t begin = (range->begin + step - 1) & -step;
  int t;
  for (size_t i = begin; i < range->end; i += step) {
    t = array[i + (step >> 1) - 1];
    array[i + (step >> 1) - 1] = array[i + step - 1];
    array[i + step - 1] += t;
  }
}

void generate_tasks_from_func(size_t num_tasks, void (*task_func)(void *),
                              int d)
{
  size_t elems_per_task = NUM_ELEMS / num_tasks;
  if (elems_per_task == 0) elems_per_task = 1;
  for (size_t i = 0; i < NUM_ELEMS; i += elems_per_task) {
    threadpool_push_task(
        &tp,
        (struct task){
            .func = task_func,
            .arg = &(struct task_arg){.begin = i,
                                      .end = (i + elems_per_task < NUM_ELEMS)
                                                 ? (i + elems_per_task)
                                                 : NUM_ELEMS,
                                      .d = d},
            .arg_size = sizeof(struct task_arg)});
  }
}

#define NUM_PPS_TASKS 32

size_t min(size_t x, size_t y) { return (x < y) ? x : y; }

void run_pps()
{
  for (int d = 0; d <= NUM_ELEMS_POW - 1; ++d) {
    size_t num_tasks = min(NUM_PPS_TASKS, NUM_ELEMS >> (d + 1));
    printf("Generating %d upsweep task(s) d=%d\n", (int)num_tasks, d);
    generate_tasks_from_func(num_tasks, pps_upsweep, d);
    threadpool_push_barrier(&tp);
  }
  printf("Running ...\n");
  threadpool_run(&tp);
  threadpool_wait(&tp);

  array[NUM_ELEMS - 1] = 0;
  for (int d = NUM_ELEMS_POW - 1; d >= 0; --d) {
    size_t num_tasks = min(NUM_PPS_TASKS, NUM_ELEMS >> (d + 1));
    printf("Generating %d downsweep task(s) d=%d\n", (int)num_tasks, d);
    generate_tasks_from_func(num_tasks, pps_downsweep, d);
    threadpool_push_barrier(&tp);
  }
  printf("Running ...\n");
  threadpool_run(&tp);
  threadpool_wait(&tp);
}

#define NUM_RAND_TASKS 32

void doublecheck_prepare()
{
  array2 = malloc(NUM_ELEMS * sizeof(*array2));
  if (array2 == NULL) {
    printf("Could not allocate doublecheck array!\n");
    exit(1);
  }
  memcpy(array2, array, NUM_ELEMS * sizeof(*array2));
}

void doublecheck()
{
  for (size_t i = 1; i < NUM_ELEMS; ++i) {
    array2[i] += array2[i - 1];
  }
  for (size_t i = 0; i < NUM_ELEMS - 1; ++i) {
    if (array2[i] != array[i + 1]) {
      printf("Mismatch i=%d (%d %d)\n", (int)i, array2[i], array[i + 1]);
      exit(1);
    }
  }
}

int main(int argc, char *argv[])
{
  init();

  printf("Generating randomization tasks...\n");
  generate_tasks_from_func(NUM_RAND_TASKS, randomize_array_task, 0);

  printf("Randomizing array...\n");
  tic();
  threadpool_run(&tp);
  threadpool_wait(&tp);
  printf("Done! %.3f ms\n", toc());

  array[NUM_ELEMS - 1] = 0;

  printf("Memcpy...\n");
  tic();
  doublecheck_prepare();
  printf("Done! %.3f ms\n", toc());

  printf("Running PPS...\n");
  tic();
  run_pps();
  printf("Done! %.3f ms\n", toc());

  printf("Double checking work...\n");
  tic();
  doublecheck();
  printf("Done! %.3f ms\n", toc());

  return 0;
}

