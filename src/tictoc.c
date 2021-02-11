#include "tictoc.h"

#include <stddef.h>
#include <sys/time.h>

static struct timeval t1, t2;

void tic() { gettimeofday(&t1, NULL); }

double toc()
{
  double ret;

  gettimeofday(&t2, NULL);

  ret = (t2.tv_sec - t1.tv_sec) * 1000.0;
  ret += (t2.tv_usec - t1.tv_usec) / 1000.0;
  return ret;
}

