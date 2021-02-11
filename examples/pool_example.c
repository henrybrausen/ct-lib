/**
 * \file pool_example.c
 * \brief Example to demonstrate use of pool.
 */

#include <stdio.h>
#include <stdlib.h>

#include "pool.h"

#define POOL_SIZE 256

struct pool pl;

int main(int argc, char *argv[])
{
  int err;

  // Attempt to initialize pool.
  err = pool_init(&pl, POOL_SIZE, sizeof(int));
  
  if (err) {
    printf("Could not initialize pool.\n");
    exit(1);
  }

  // Retrieve an integer from the pool.
  int *p;
  if ((p = pool_acquire(&pl)) == NULL) {
    printf("Could not acquire int from pool.\n");
    exit(1);
  }

  // Work with the integer.
  *p = 5;

  // Return integer to pool when done with it.
  err = pool_release(&pl, p);
  
  if (err) {
    printf("Could not release int back into pool.\n");
    exit(1);
  }

  return 0;
}

