/**
 * @file
 *
 * @brief Inlined Routines Associated with the SuperCore Spinlock
 *
 * This include file contains all of the inlined routines associated
 * with the SuperCore spinlock.
 */

/*
 *  COPYRIGHT (c) 1989-2008.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_CORESPINLOCKIMPL_H
#define _RTEMS_SCORE_CORESPINLOCKIMPL_H

#include <rtems/score/corespinlock.h>
#include <rtems/score/watchdog.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ScoreSpinlock
 */
/**@{**/

/**
 *  Core Spinlock handler return statuses.
 */
typedef enum {
  /** This status indicates that the operation completed successfully. */
  CORE_SPINLOCK_SUCCESSFUL,
  /** This status indicates that the current thread already holds the spinlock.
   *  An attempt to relock it will result in deadlock.
   */
  CORE_SPINLOCK_HOLDER_RELOCKING,
  /** This status indicates that the current thread is attempting to unlock a
   *  spinlock that is held by another thread.
   */
  CORE_SPINLOCK_NOT_HOLDER,
  /** This status indicates that a thread reached the limit of time it
   *  was willing to wait on the spin lock.
   */
  CORE_SPINLOCK_TIMEOUT,
  /** This status indicates that a thread is currently waiting for this
   *  spin lock.
   */
  CORE_SPINLOCK_IS_BUSY,
  /** This status indicates that the spinlock is currently locked and thus
   *  unavailable.
   */
  CORE_SPINLOCK_UNAVAILABLE
}   CORE_spinlock_Status;

/** This is a shorthand for the last status code. */
#define CORE_SPINLOCK_STATUS_LAST CORE_SPINLOCK_UNAVAILABLE

/** This indicates the lock is available. */
#define CORE_SPINLOCK_UNLOCKED 0

/** This indicates the lock is unavailable. */
#define CORE_SPINLOCK_LOCKED   1

/**
 *  @brief Initialize the spinlock.
 *
 *  This routine initializes the spinlock based on the parameters passed.
 *
 *  @param[in] the_spinlock is the spinlock control block to initialize
 */
RTEMS_INLINE_ROUTINE void _CORE_spinlock_Initialize(
  CORE_spinlock_Control *the_spinlock
)
{
  memset( the_spinlock, 0, sizeof( *the_spinlock ) );
}

RTEMS_INLINE_ROUTINE void _CORE_spinlock_Acquire_critical(
  CORE_spinlock_Control *the_spinlock,
  ISR_lock_Context      *lock_context
)
{
  _ISR_lock_Acquire( &the_spinlock->Lock, lock_context );
}

RTEMS_INLINE_ROUTINE void _CORE_spinlock_Release(
  CORE_spinlock_Control *the_spinlock,
  ISR_lock_Context      *lock_context
)
{
  _ISR_lock_Release_and_ISR_enable( &the_spinlock->Lock, lock_context );
}

/**
 *  @brief Wait for spinlock.
 *
 *  This routine wait for the spinlock to be released.  If the spinlock
 *  is set to automatic and this is the appropriate thread, then it returns
 *  immediately.  Otherwise, the calling thread is blocked until the spinlock
 *  is released.
 *
 *  @param[in] the_spinlock is the spinlock to wait for
 *  @param[in] wait is true if willing to wait
 *  @param[in] timeout is the maximum number of ticks to spin (0 is forever)
 *
 * @retval A status is returned which indicates the success or failure of
 *         this operation.
 */
CORE_spinlock_Status _CORE_spinlock_Seize(
  CORE_spinlock_Control *the_spinlock,
  bool                   wait,
  Watchdog_Interval      timeout,
  ISR_lock_Context      *lock_context
);

/**
 * @brief Manually release the spinlock.
 *
 *  This routine manually releases the spinlock.  All of the threads waiting
 *  for the spinlock will be readied.
 *
 *  @param[in] the_spinlock is the spinlock to surrender
 */
CORE_spinlock_Status _CORE_spinlock_Surrender(
  CORE_spinlock_Control *the_spinlock,
  ISR_lock_Context      *lock_context
);

/**
 * This method is used to determine if the spinlock is available or not.
 *
 * @param[in] the_spinlock will be checked
 *
 * @return This method will return true if the spinlock is busy
 *         and false otherwise.
 */
RTEMS_INLINE_ROUTINE bool _CORE_spinlock_Is_busy(
  CORE_spinlock_Control  *the_spinlock
)
{
  return (the_spinlock->users != 0);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
