/**
 * @file
 *
 * @brief rtems_semaphore_create
 * @ingroup ClassicSem Semaphores
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

#include <rtems/system.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/support.h>
#include <rtems/rtems/attrimpl.h>
#include <rtems/score/isr.h>
#include <rtems/rtems/options.h>
#include <rtems/rtems/semimpl.h>
#include <rtems/rtems/tasksimpl.h>
#include <rtems/score/coremuteximpl.h>
#include <rtems/score/coresemimpl.h>
#include <rtems/score/threaddispatch.h>
#include <rtems/score/sysstate.h>

#include <rtems/score/interr.h>

/*
 *  rtems_semaphore_create
 *
 *  This directive creates a semaphore and sets the initial value based
 *  on the given count.  A semaphore id is returned.
 *
 *  Input parameters:
 *    name             - user defined semaphore name
 *    count            - initial count of semaphore
 *    attribute_set    - semaphore attributes
 *    priority_ceiling - semaphore's ceiling priority
 *    id               - pointer to semaphore id
 *
 *  Output parameters:
 *    id       - semaphore id
 *    RTEMS_SUCCESSFUL - if successful
 *    error code - if unsuccessful
 */

rtems_status_code rtems_semaphore_create(
  rtems_name           name,
  uint32_t             count,
  rtems_attribute      attribute_set,
  rtems_task_priority  priority_ceiling,
  rtems_id            *id
)
{
  Semaphore_Control          *the_semaphore;
  CORE_mutex_Attributes       the_mutex_attr;
  CORE_semaphore_Disciplines  semaphore_discipline;
  CORE_mutex_Status           mutex_status;

  if ( !rtems_is_name_valid( name ) )
    return RTEMS_INVALID_NAME;

  if ( !id )
    return RTEMS_INVALID_ADDRESS;

#if defined(RTEMS_MULTIPROCESSING)
  if ( _Attributes_Is_global( attribute_set ) ) {

    if ( !_System_state_Is_multiprocessing )
      return RTEMS_MP_NOT_CONFIGURED;

    if ( _Attributes_Is_inherit_priority( attribute_set ) ||
         _Attributes_Is_priority_ceiling( attribute_set ) ||
         _Attributes_Is_multiprocessor_resource_sharing( attribute_set ) )
      return RTEMS_NOT_DEFINED;

  } else
#endif

  if ( _Attributes_Is_multiprocessor_resource_sharing( attribute_set ) &&
       !( _Attributes_Is_binary_semaphore( attribute_set ) &&
         !_Attributes_Is_priority( attribute_set ) ) ) {
    return RTEMS_NOT_DEFINED;
  }

  if ( _Attributes_Is_inherit_priority( attribute_set ) ||
              _Attributes_Is_priority_ceiling( attribute_set ) ) {

    if ( ! (_Attributes_Is_binary_semaphore( attribute_set ) &&
            _Attributes_Is_priority( attribute_set ) ) )
      return RTEMS_NOT_DEFINED;

  }

  if ( !_Attributes_Has_at_most_one_protocol( attribute_set ) )
    return RTEMS_NOT_DEFINED;

  if ( !_Attributes_Is_counting_semaphore( attribute_set ) && ( count > 1 ) )
    return RTEMS_INVALID_NUMBER;

#if !defined(RTEMS_SMP)
  /*
   * On uni-processor configurations the Multiprocessor Resource Sharing
   * Protocol is equivalent to the Priority Ceiling Protocol.
   */
  if ( _Attributes_Is_multiprocessor_resource_sharing( attribute_set ) ) {
    attribute_set |= RTEMS_PRIORITY_CEILING | RTEMS_PRIORITY;
  }
#endif

  the_semaphore = _Semaphore_Allocate();

  if ( !the_semaphore ) {
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }

#if defined(RTEMS_MULTIPROCESSING)
  if ( _Attributes_Is_global( attribute_set ) &&
       ! ( _Objects_MP_Allocate_and_open( &_Semaphore_Information, name,
                            the_semaphore->Object.id, false ) ) ) {
    _Semaphore_Free( the_semaphore );
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }
#endif

  the_semaphore->attribute_set = attribute_set;

  /*
   *  Initialize it as a counting semaphore.
   */
  if ( _Attributes_Is_counting_semaphore( attribute_set ) ) {
    if ( _Attributes_Is_priority( attribute_set ) )
      semaphore_discipline = CORE_SEMAPHORE_DISCIPLINES_PRIORITY;
    else
      semaphore_discipline = CORE_SEMAPHORE_DISCIPLINES_FIFO;

    /*
     *  The following are just to make Purify happy.
     */
    the_mutex_attr.lock_nesting_behavior = CORE_MUTEX_NESTING_ACQUIRES;
    the_mutex_attr.priority_ceiling = PRIORITY_MINIMUM;

    _CORE_semaphore_Initialize(
      &the_semaphore->Core_control.semaphore,
      semaphore_discipline,
      count
    );
#if defined(RTEMS_SMP)
  } else if ( _Attributes_Is_multiprocessor_resource_sharing( attribute_set ) ) {
    MRSP_Status mrsp_status = _MRSP_Initialize(
      &the_semaphore->Core_control.mrsp,
      priority_ceiling,
      _Thread_Get_executing(),
      count != 1
    );

    if ( mrsp_status != MRSP_SUCCESSFUL ) {
      _Semaphore_Free( the_semaphore );
      _Objects_Allocator_unlock();

      return _Semaphore_Translate_MRSP_status_code( mrsp_status );
    }
#endif
  } else {
    /*
     *  It is either simple binary semaphore or a more powerful mutex
     *  style binary semaphore.  This is the mutex style.
     */
    if ( _Attributes_Is_priority( attribute_set ) )
      the_mutex_attr.discipline = CORE_MUTEX_DISCIPLINES_PRIORITY;
    else
      the_mutex_attr.discipline = CORE_MUTEX_DISCIPLINES_FIFO;

    if ( _Attributes_Is_binary_semaphore( attribute_set ) ) {
      the_mutex_attr.priority_ceiling      = _RTEMS_tasks_Priority_to_Core(
                                               priority_ceiling
                                             );
      the_mutex_attr.lock_nesting_behavior = CORE_MUTEX_NESTING_ACQUIRES;
      the_mutex_attr.only_owner_release    = false;

      if ( the_mutex_attr.discipline == CORE_MUTEX_DISCIPLINES_PRIORITY ) {
        if ( _Attributes_Is_inherit_priority( attribute_set ) ) {
          the_mutex_attr.discipline = CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT;
          the_mutex_attr.only_owner_release = true;
        } else if ( _Attributes_Is_priority_ceiling( attribute_set ) ) {
          the_mutex_attr.discipline = CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING;
          the_mutex_attr.only_owner_release = true;
        }
      }
    } else /* must be simple binary semaphore */ {
      the_mutex_attr.lock_nesting_behavior = CORE_MUTEX_NESTING_BLOCKS;
      the_mutex_attr.only_owner_release = false;
    }

    mutex_status = _CORE_mutex_Initialize(
      &the_semaphore->Core_control.mutex,
      _Thread_Get_executing(),
      &the_mutex_attr,
      count != 1
    );

    if ( mutex_status == CORE_MUTEX_STATUS_CEILING_VIOLATED ) {
      _Semaphore_Free( the_semaphore );
      _Objects_Allocator_unlock();
      return RTEMS_INVALID_PRIORITY;
    }
  }

  /*
   *  Whether we initialized it as a mutex or counting semaphore, it is
   *  now ready to be "offered" for use as a Classic API Semaphore.
   */
  _Objects_Open(
    &_Semaphore_Information,
    &the_semaphore->Object,
    (Objects_Name) name
  );

  *id = the_semaphore->Object.id;

#if defined(RTEMS_MULTIPROCESSING)
  if ( _Attributes_Is_global( attribute_set ) )
    _Semaphore_MP_Send_process_packet(
      SEMAPHORE_MP_ANNOUNCE_CREATE,
      the_semaphore->Object.id,
      name,
      0                          /* Not used */
    );
#endif
  _Objects_Allocator_unlock();
  return RTEMS_SUCCESSFUL;
}
