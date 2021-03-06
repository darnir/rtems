/**
 * @file
 *
 * @brief Changes the Priority Ceiling of a Mutex and Releases it
 * @ingroup POSIXAPI
 */

/*
 *  COPYRIGHT (c) 1989-2009.
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

int pthread_mutex_setprioceiling(
  pthread_mutex_t   *mutex,
  int                prioceiling,
  int               *old_ceiling
)
{
  register POSIX_Mutex_Control *the_mutex;
  Priority_Control              the_priority;
  Thread_queue_Context          queue_context;

  if ( !old_ceiling )
    return EINVAL;

  if ( !_POSIX_Priority_Is_valid( prioceiling ) )
    return EINVAL;

  the_priority = _POSIX_Priority_To_core( prioceiling );

  /*
   *  Must acquire the mutex before we can change it's ceiling.
   *  POSIX says block until we acquire it.
   */
  (void) pthread_mutex_lock( mutex );

  /*
   *  Do not worry about the return code from this.  The Get operation
   *  will also fail if it is a bad id or was deleted between the two
   *  operations.
   *
   *  NOTE: This makes it easier to get 100% binary coverage since the
   *        bad Id case is handled by the switch.
   */
  the_mutex = _POSIX_Mutex_Get( mutex, &queue_context );

  if ( the_mutex == NULL ) {
    return EINVAL;
  }

  *old_ceiling = _POSIX_Priority_From_core(
    the_mutex->Mutex.Attributes.priority_ceiling
  );
  the_mutex->Mutex.Attributes.priority_ceiling = the_priority;

  /*
   *  We are required to unlock the mutex before we return.
   */
  _CORE_mutex_Surrender(
    &the_mutex->Mutex,
    &queue_context
  );
  return 0;
}
