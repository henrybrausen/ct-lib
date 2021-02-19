#include "task.h"
#include "error.h"

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

enum ct_err task_freeze(struct task *t)
{
  if (t->arg_size == 0) {
    // Special case, do not allocate.
    return CT_SUCCESS;
  }

  void *p = malloc(t->arg_size);

  if (p == NULL) { return CT_EMALLOC; }

  t->arg = memcpy(p, t->arg, t->arg_size);

  return CT_SUCCESS;
}

