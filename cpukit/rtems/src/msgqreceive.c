/**
 *  @file
 *
 *  @brief RTEMS Message Queue Receive
 *  @ingroup ClassicMessageQueue
 */

/*
 *  COPYRIGHT (c) 1989-2014.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/messageimpl.h>
#include <rtems/rtems/optionsimpl.h>

THREAD_QUEUE_OBJECT_ASSERT( Message_queue_Control, message_queue.Wait_queue );

rtems_status_code rtems_message_queue_receive(
  rtems_id        id,
  void           *buffer,
  size_t         *size,
  rtems_option    option_set,
  rtems_interval  timeout
)
{
  Message_queue_Control *the_message_queue;
  Thread_queue_Context   queue_context;
  Thread_Control        *executing;

  if ( buffer == NULL ) {
    return RTEMS_INVALID_ADDRESS;
  }

  if ( size == NULL ) {
    return RTEMS_INVALID_ADDRESS;
  }

  the_message_queue = _Message_queue_Get( id, &queue_context, NULL );

  if ( the_message_queue == NULL ) {
#if defined(RTEMS_MULTIPROCESSING)
    return _Message_queue_MP_Receive( id, buffer, size, option_set, timeout );
#else
    return RTEMS_INVALID_ID;
#endif
  }

  _CORE_message_queue_Acquire_critical(
    &the_message_queue->message_queue,
    &queue_context
  );

  executing = _Thread_Executing;
  _CORE_message_queue_Seize(
    &the_message_queue->message_queue,
    executing,
    buffer,
    size,
    !_Options_Is_no_wait( option_set ),
    timeout,
    &queue_context
  );
  return _Message_queue_Translate_core_message_queue_return_code(
    executing->Wait.return_code
  );
}
