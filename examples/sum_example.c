/**
 * \file sum_example.c
 * \brief Example of computing the sum of an array of doubles in parallel,
 * using a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "threadpool.h"

#define ARRAY_LEN 1000000ull
#define NUM_TASKS 32
#define NUM_THREADS 4

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct threadpool tp;

double array[ARRAY_LEN];

// The argument to the sum computing task is a range, as well as where to store
// the result.
struct task_arg {
  size_t begin;
  size_t end;
  double *result;
};

// Task to compute the partial sum of an array, over the range [begin, end)
void compute_sum_task(void *argp)
{
  struct task_arg *arg = argp;

  double avg = 0.0;

  for (size_t i = arg->begin; i < arg->end; ++i) {
    avg += array[i];
  }

  *arg->result = avg;
}

// Compute the sum of the array in parallel, splitting the array into NUM_TASKS
// separate ranges, and assigning each range to a task.
double compute_sum()
{
  double ret = 0.0;

  size_t step = ARRAY_LEN / NUM_TASKS;
  if (step == 0) { step = 1; }

  double partial_sums[NUM_TASKS];

  // Construct tasks and push them into the threadpool.
  for (size_t i = 0; i < ARRAY_LEN; i += step) {
    threadpool_push_task(&tp,
        (struct task) {
          .func = compute_sum_task,
          .arg = &(struct task_arg) {
            .begin = i,
            .end = MIN(i + step, ARRAY_LEN),
            .result = &partial_sums[i/step]
          },
          .arg_size = sizeof(struct task_arg)
        });      
  }

  // Begin computation.
  threadpool_notify(&tp);

  // Wait for all threads to complete.
  threadpool_wait(&tp);

  // Sum our partial sums to find the total.
  for (size_t i = 0; i < NUM_TASKS; ++i) {
    ret += partial_sums[i];
  }

  return ret;
}

void randomize_array()
{
  srand(time(NULL));

  for (size_t i = 0; i < ARRAY_LEN; ++i) {
    array[i] = (double)rand() / RAND_MAX;
  }
}

int main(int argc, char *argv[])
{
  printf("Initializing threadpool ...\n");
  threadpool_init(&tp, NUM_THREADS);
  printf("Done!\n");

  printf("Randomizing array ...\n");
  randomize_array();
  printf("Done!\n");

  printf("Computing parallel sum ...\n");
  double sum = compute_sum();
  printf("Done! %lf\n", sum);

  return 0;
}
