/**
 * @file
 *
 * @brief Obtain a Read Lock on a RWLock Instance
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

#include <rtems/posix/rwlockimpl.h>

int pthread_rwlock_rdlock(
  pthread_rwlock_t  *rwlock
)
{
  POSIX_RWLock_Control *the_rwlock;
  Thread_queue_Context  queue_context;
  Thread_Control       *executing;

  the_rwlock = _POSIX_RWLock_Get( rwlock, &queue_context );

  if ( the_rwlock == NULL ) {
    return EINVAL;
  }

  executing = _Thread_Executing;
  _CORE_RWLock_Seize_for_reading(
    &the_rwlock->RWLock,
    executing,
    true,                 /* we are willing to wait forever */
    0,
    &queue_context
  );
  return _POSIX_RWLock_Translate_core_RWLock_return_code(
    (CORE_RWLock_Status) executing->Wait.return_code
  );
}
