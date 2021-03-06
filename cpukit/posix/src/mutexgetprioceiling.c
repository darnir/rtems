/**
 * @file
 *
 * @brief Returns the Current Priority Ceiling of the Mutex
 * @ingroup POSIXAPI
 */

/*
 *  COPYRIGHT (c) 1989-2007.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/posix/muteximpl.h>
#include <rtems/posix/priorityimpl.h>

/*
 *  13.6.2 Change the Priority Ceiling of a Mutex, P1003.1c/Draft 10, p. 131
 */

int pthread_mutex_getprioceiling(
  pthread_mutex_t   *mutex,
  int               *prioceiling
)
{
  POSIX_Mutex_Control  *the_mutex;
  Thread_queue_Context  queue_context;

  if ( prioceiling == NULL ) {
    return EINVAL;
  }

  the_mutex = _POSIX_Mutex_Get( mutex, &queue_context );

  if ( the_mutex == NULL ) {
    return EINVAL;
  }

  _CORE_mutex_Acquire_critical( &the_mutex->Mutex, &queue_context );

  *prioceiling = _POSIX_Priority_From_core(
    the_mutex->Mutex.Attributes.priority_ceiling
  );

  _CORE_mutex_Release( &the_mutex->Mutex, &queue_context );

  return 0;
}
