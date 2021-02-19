#include "error.h"

const char *ct_err_str(enum ct_err e)
{
  switch (e) {
    case CT_SUCCESS:
      return "Operation successful.";
    case CT_FAILURE:
      return "Operation failed.";
    case CT_EMALLOC:
      return "malloc() failed.";
    case CT_EQUEUE_EMPTY:
      return "Queue is empty.";
    case CT_EMUTEX_INIT:
      return "Could not initialize mutex.";
    case CT_ECOND_INIT:
      return "Could not initialize condition variable.";
    case CT_EMUTEX_DESTROY:
      return "Could not destroy mutex.";
    case CT_ECOND_DESTROY:
      return "Could not destroy condition variable.";
    case CT_ETHREAD_CREATE:
      return "Could not create thread.";
    case CT_EPENDING_TASKS:
      return "There are pending tasks.";
    case CT_ERUNNING_TASKS:
      return "There are running tasks.";
    default:
      return "Unknown error.";
  }
}

