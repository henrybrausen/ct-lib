/**
 * \file error.h
 * \brief Error return codes and associated utility functions.
 */

#ifndef ERROR_H
#define ERROR_H

/**
 * \brief Error code enumeration for threadpool library.
 */
enum ct_err {
  CT_SUCCESS = 0,
  CT_FAILURE,

  CT_EMALLOC,

  CT_EQUEUE_EMPTY,

  CT_EMUTEX_INIT,
  CT_ECOND_INIT,
  CT_EMUTEX_DESTROY,
  CT_ECOND_DESTROY,

  CT_ETHREAD_CREATE,
  CT_EPENDING_TASKS,
  CT_ERUNNING_TASKS
};

/**
 * \brief Convert an enum ct_err to a string describing the error code.
 *
 * \param e Error return code.
 * \return Pointer to error string.
 */
const char *ct_err_str(enum ct_err e);

#endif // ERROR_H

