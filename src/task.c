#include "task.h"

#include <stdlib.h>
#include <string.h>

void task_execute(struct task *t)
{
  t->func(t->arg);

  // Destroy task after execution, freeing memory associated with task argument
  task_destroy(t);
}

void task_destroy(struct task *t)
{
  if (t->arg_size > 0) { free(t->arg); }
}

int task_freeze(struct task *t)
{
  if (t->arg_size == 0) {
    // Special case, do not allocate.
    return 0;
  }

  void *p = malloc(t->arg_size);

  if (p == NULL) { return -1; }

  t->arg = memcpy(p, t->arg, t->arg_size);

  return 0;
}

