/**
 * @file
 *
 * @ingroup ClassicSem
 *
 * @brief Classic Semaphores Implementation
 */

/*  COPYRIGHT (c) 1989-2008.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_RTEMS_SEMIMPL_H
#define _RTEMS_RTEMS_SEMIMPL_H

#include <rtems/rtems/sem.h>
#include <rtems/score/coremuteximpl.h>
#include <rtems/score/coresemimpl.h>
#include <rtems/score/mrspimpl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  The following defines the information control block used to manage
 *  this class of objects.
 */
extern Objects_Information _Semaphore_Information;

extern const rtems_status_code
  _Semaphore_Translate_core_mutex_return_code_[];

extern const rtems_status_code
  _Semaphore_Translate_core_semaphore_return_code_[];

/**
 * @brief Semaphore Translate Core Mutex Return Code
 *
 * This function returns a RTEMS status code based on the mutex
 * status code specified.
 *
 * @param[in] status is the mutex status code to translate
 *
 * @retval translated RTEMS status code
 */
RTEMS_INLINE_ROUTINE rtems_status_code
_Semaphore_Translate_core_mutex_return_code(
  uint32_t   status
)
{
  /*
   *  If this thread is blocking waiting for a result on a remote operation.
   */
  #if defined(RTEMS_MULTIPROCESSING)
    if ( _Thread_Is_proxy_blocking(status) )
      return RTEMS_PROXY_BLOCKING;
  #endif

  /*
   *  Internal consistency check for bad status from SuperCore
   */
  #if defined(RTEMS_DEBUG)
    if ( status > CORE_MUTEX_STATUS_LAST )
      return RTEMS_INTERNAL_ERROR;
  #endif
  return _Semaphore_Translate_core_mutex_return_code_[status];
}

#if defined(RTEMS_SMP)
RTEMS_INLINE_ROUTINE rtems_status_code
_Semaphore_Translate_MRSP_status_code( MRSP_Status mrsp_status )
{
  return (rtems_status_code) mrsp_status;
}
#endif

/**
 * @brief Semaphore Translate Core Semaphore Return Code
 *
 * This function returns a RTEMS status code based on the semaphore
 * status code specified.
 *
 * @param[in] status is the semaphore status code to translate
 *
 * @retval translated RTEMS status code
 */
RTEMS_INLINE_ROUTINE rtems_status_code
_Semaphore_Translate_core_semaphore_return_code(
  uint32_t   status
)
{
  #if defined(RTEMS_MULTIPROCESSING)
    if ( _Thread_Is_proxy_blocking(status) )
      return RTEMS_PROXY_BLOCKING;
  #endif
  /*
   *  Internal consistency check for bad status from SuperCore
   */
  #if defined(RTEMS_DEBUG)
    if ( status > CORE_SEMAPHORE_STATUS_LAST )
      return RTEMS_INTERNAL_ERROR;
  #endif
  return _Semaphore_Translate_core_semaphore_return_code_[status];
}

/**
 *  @brief Allocates a semaphore control block from
 *  the inactive chain of free semaphore control blocks.
 *
 *  This function allocates a semaphore control block from
 *  the inactive chain of free semaphore control blocks.
 */
RTEMS_INLINE_ROUTINE Semaphore_Control *_Semaphore_Allocate( void )
{
  return (Semaphore_Control *) _Objects_Allocate( &_Semaphore_Information );
}

/**
 *  @brief Frees a semaphore control block to the
 *  inactive chain of free semaphore control blocks.
 *
 *  This routine frees a semaphore control block to the
 *  inactive chain of free semaphore control blocks.
 */
RTEMS_INLINE_ROUTINE void _Semaphore_Free (
  Semaphore_Control *the_semaphore
)
{
  _Objects_Free( &_Semaphore_Information, &the_semaphore->Object );
}

RTEMS_INLINE_ROUTINE Semaphore_Control *_Semaphore_Do_get(
  Objects_Id               id,
  Thread_queue_Context    *queue_context
#if defined(RTEMS_MULTIPROCESSING)
  ,
  Thread_queue_MP_callout  mp_callout
#endif
)
{
  _Thread_queue_Context_initialize( queue_context, mp_callout );
  return (Semaphore_Control *) _Objects_Get(
    id,
    &queue_context->Lock_context,
    &_Semaphore_Information
  );
}

#if defined(RTEMS_MULTIPROCESSING)
  #define _Semaphore_Get( id, queue_context, mp_callout ) \
    _Semaphore_Do_get( id, queue_context, mp_callout )
#else
  #define _Semaphore_Get( id, queue_context, mp_callout ) \
    _Semaphore_Do_get( id, queue_context )
#endif

#ifdef __cplusplus
}
#endif

#ifdef RTEMS_MULTIPROCESSING
#include <rtems/rtems/semmp.h>
#endif

#endif
/*  end of include file */
