/**
 * @file
 *
 * @ingroup ScoreSchedulerStrongAPA
 *
 * @brief Strong APA Scheduler Implementation
 */

/*
 * Copyright (c) 2013, 2016 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <rtems/score/schedulerstrongapa.h>
#include <rtems/score/schedulerpriorityimpl.h>
#include <rtems/score/schedulersmpimpl.h>

static Scheduler_strong_APA_Context *_Scheduler_strong_APA_Get_self(
  Scheduler_Context *context
)
{
  return (Scheduler_strong_APA_Context *) context;
}

/*
 * This method returns the scheduler node for the specified thread
 * as a scheduler specific type.
 */
static Scheduler_strong_APA_Node *
_Scheduler_strong_APA_Thread_get_node(
  Thread_Control *the_thread
)
{
  return (Scheduler_strong_APA_Node *)
    _Scheduler_Thread_get_node( the_thread );
}

static Scheduler_strong_APA_Node *
_Scheduler_strong_APA_Node_downcast( Scheduler_Node *node )
{
  return (Scheduler_strong_APA_Node *) node;
}

static void _Scheduler_strong_APA_Move_from_scheduled_to_ready(
  Scheduler_Context *context,
  Scheduler_Node    *scheduled_to_ready
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( scheduled_to_ready );

  _Chain_Extract_unprotected( &node->Base.Base.Node );
  _Scheduler_priority_Ready_queue_enqueue_first(
    &node->Base.Base.Node,
    &node->Ready_queue,
    &self->Bit_map
  );
}

static void _Scheduler_strong_APA_Move_from_ready_to_scheduled(
  Scheduler_Context *context,
  Scheduler_Node    *ready_to_scheduled
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( ready_to_scheduled );

  _Scheduler_priority_Ready_queue_extract(
    &node->Base.Base.Node,
    &node->Ready_queue,
    &self->Bit_map
  );
  _Chain_Insert_ordered_unprotected(
    &self->Base.Scheduled,
    &node->Base.Base.Node,
    _Scheduler_SMP_Insert_priority_fifo_order
  );
}

static void _Scheduler_strong_APA_Insert_ready_lifo(
  Scheduler_Context *context,
  Scheduler_Node    *the_thread
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( the_thread );

  _Scheduler_priority_Ready_queue_enqueue(
    &node->Base.Base.Node,
    &node->Ready_queue,
    &self->Bit_map
  );
}

static void _Scheduler_strong_APA_Insert_ready_fifo(
  Scheduler_Context *context,
  Scheduler_Node    *the_thread
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( the_thread );

  _Scheduler_priority_Ready_queue_enqueue_first(
    &node->Base.Base.Node,
    &node->Ready_queue,
    &self->Bit_map
  );
}

static void _Scheduler_strong_APA_Extract_from_ready(
  Scheduler_Context *context,
  Scheduler_Node    *the_thread
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( the_thread );

  _Scheduler_priority_Ready_queue_extract(
    &node->Base.Base.Node,
    &node->Ready_queue,
    &self->Bit_map
  );
}

static void _Scheduler_strong_APA_Do_update(
  Scheduler_Context *context,
  Scheduler_Node *node_to_update,
  Priority_Control new_priority
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Node_downcast( node_to_update );

  _Scheduler_SMP_Node_update_priority( &node->Base, new_priority );
  _Scheduler_priority_Ready_queue_update(
    &node->Ready_queue,
    new_priority,
    &self->Bit_map,
    &self->Ready[ 0 ]
  );
}

static Scheduler_strong_APA_Context *
_Scheduler_strong_APA_Get_context( const Scheduler_Control *scheduler )
{
  return (Scheduler_strong_APA_Context *) _Scheduler_Get_context( scheduler );
}

void _Scheduler_strong_APA_Initialize( const Scheduler_Control *scheduler )
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_context( scheduler );

  _Scheduler_SMP_Initialize( &self->Base );
  _Priority_bit_map_Initialize( &self->Bit_map );
  _Scheduler_priority_Ready_queue_initialize(
    &self->Ready[ 0 ],
    scheduler->maximum_priority
  );
}

void _Scheduler_strong_APA_Node_initialize(
  const Scheduler_Control *scheduler,
  Scheduler_Node          *node,
  Thread_Control          *the_thread,
  Priority_Control         priority
)
{
  Scheduler_Context            *context;
  Scheduler_strong_APA_Context *self;
  Scheduler_strong_APA_Node    *the_node;

  the_node = _Scheduler_strong_APA_Node_downcast( node );
  _Scheduler_SMP_Node_initialize( &the_node->Base, the_thread, priority );

  context = _Scheduler_Get_context( scheduler );
  self = _Scheduler_strong_APA_Get_self( context );
  _Scheduler_priority_Ready_queue_update(
    &the_node->Ready_queue,
    priority,
    &self->Bit_map,
    &self->Ready[ 0 ]
  );
  /*
   *  All we add is affinity information to the basic SMP node.
   */
  node->Affinity     = *_CPU_set_Default();
  node->Affinity.set = &node->Affinity.preallocated;
}

static Scheduler_Node *_Scheduler_strong_APA_Get_highest_ready(
  Scheduler_Context *context,
  Scheduler_Node    *node
)
{
  Scheduler_strong_APA_Context *self =
    _Scheduler_strong_APA_Get_self( context );

  (void) node;

  return (Scheduler_Node *) _Scheduler_priority_Ready_queue_first(
    &self->Bit_map,
    &self->Ready[ 0 ]
  );
}

void _Scheduler_strong_APA_Block(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Block(
    context,
    the_thread,
    _Scheduler_strong_APA_Extract_from_ready,
    _Scheduler_strong_APA_Get_highest_ready,
    _Scheduler_strong_APA_Move_from_ready_to_scheduled,
    _Scheduler_SMP_Allocate_processor_exact
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_ordered(
  Scheduler_Context    *context,
  Scheduler_Node       *node,
  Thread_Control       *needs_help,
  Chain_Node_order      order,
  Scheduler_SMP_Insert  insert_ready,
  Scheduler_SMP_Insert  insert_scheduled
)
{
  return _Scheduler_SMP_Enqueue_ordered(
    context,
    node,
    needs_help,
    order,
    insert_ready,
    insert_scheduled,
    _Scheduler_strong_APA_Move_from_scheduled_to_ready,
    _Scheduler_SMP_Get_lowest_scheduled,
    _Scheduler_SMP_Allocate_processor_exact
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_lifo(
  Scheduler_Context *context,
  Scheduler_Node    *node,
  Thread_Control    *needs_help
)
{
  return _Scheduler_strong_APA_Enqueue_ordered(
    context,
    node,
    needs_help,
    _Scheduler_SMP_Insert_priority_lifo_order,
    _Scheduler_strong_APA_Insert_ready_lifo,
    _Scheduler_SMP_Insert_scheduled_lifo
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_fifo(
  Scheduler_Context *context,
  Scheduler_Node    *node,
  Thread_Control    *needs_help
)
{
  return _Scheduler_strong_APA_Enqueue_ordered(
    context,
    node,
    needs_help,
    _Scheduler_SMP_Insert_priority_fifo_order,
    _Scheduler_strong_APA_Insert_ready_fifo,
    _Scheduler_SMP_Insert_scheduled_fifo
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_scheduled_ordered(
  Scheduler_Context    *context,
  Scheduler_Node       *node,
  Chain_Node_order      order,
  Scheduler_SMP_Insert  insert_ready,
  Scheduler_SMP_Insert  insert_scheduled
)
{
  return _Scheduler_SMP_Enqueue_scheduled_ordered(
    context,
    node,
    order,
    _Scheduler_strong_APA_Extract_from_ready,
    _Scheduler_strong_APA_Get_highest_ready,
    insert_ready,
    insert_scheduled,
    _Scheduler_strong_APA_Move_from_ready_to_scheduled,
    _Scheduler_SMP_Allocate_processor_exact
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_scheduled_lifo(
  Scheduler_Context *context,
  Scheduler_Node    *node
)
{
  return _Scheduler_strong_APA_Enqueue_scheduled_ordered(
    context,
    node,
    _Scheduler_SMP_Insert_priority_lifo_order,
    _Scheduler_strong_APA_Insert_ready_lifo,
    _Scheduler_SMP_Insert_scheduled_lifo
  );
}

static Thread_Control *_Scheduler_strong_APA_Enqueue_scheduled_fifo(
  Scheduler_Context *context,
  Scheduler_Node    *node
)
{
  return _Scheduler_strong_APA_Enqueue_scheduled_ordered(
    context,
    node,
    _Scheduler_SMP_Insert_priority_fifo_order,
    _Scheduler_strong_APA_Insert_ready_fifo,
    _Scheduler_SMP_Insert_scheduled_fifo
  );
}

Thread_Control *_Scheduler_strong_APA_Unblock(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_SMP_Unblock(
    context,
    the_thread,
    _Scheduler_strong_APA_Do_update,
    _Scheduler_strong_APA_Enqueue_fifo
  );
}

Thread_Control *_Scheduler_strong_APA_Update_priority(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_SMP_Update_priority(
    context,
    the_thread,
    _Scheduler_strong_APA_Extract_from_ready,
    _Scheduler_strong_APA_Do_update,
    _Scheduler_strong_APA_Enqueue_fifo,
    _Scheduler_strong_APA_Enqueue_lifo,
    _Scheduler_strong_APA_Enqueue_scheduled_fifo,
    _Scheduler_strong_APA_Enqueue_scheduled_lifo
  );
}

Thread_Control *_Scheduler_strong_APA_Ask_for_help(
  const Scheduler_Control *scheduler,
  Thread_Control          *offers_help,
  Thread_Control          *needs_help
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_SMP_Ask_for_help(
    context,
    offers_help,
    needs_help,
    _Scheduler_strong_APA_Enqueue_fifo
  );
}

Thread_Control *_Scheduler_strong_APA_Yield(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_SMP_Yield(
    context,
    the_thread,
    _Scheduler_strong_APA_Extract_from_ready,
    _Scheduler_strong_APA_Enqueue_fifo,
    _Scheduler_strong_APA_Enqueue_scheduled_fifo
  );
}

/*
 * This is the public scheduler specific Change Priority operation.
 */
bool _Scheduler_strong_APA_Get_affinity(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  size_t                   cpusetsize,
  cpu_set_t               *cpuset
)
{
  Scheduler_strong_APA_Node *node =
    _Scheduler_strong_APA_Thread_get_node( the_thread );

  (void) scheduler;

  if ( node->Affinity.setsize != cpusetsize ) {
    return false;
  }

  CPU_COPY( cpuset, node->Affinity.set );
  return true;
}

bool _Scheduler_strong_APA_Set_affinity(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  size_t                   cpusetsize,
  const cpu_set_t         *cpuset
)
{
  Scheduler_strong_APA_Node				*node;
  States_Control                        current_state;

  /*
   * Validate that the cpset meets basic requirements.
   */
  if ( !_CPU_set_Is_valid( cpuset, cpusetsize ) ) {
    return false;
  }

  node = _Scheduler_strong_APA_Thread_get_node( the_thread );

  /*
   * The old and new set are the same, there is no point in
   * doing anything.
   */
  if ( CPU_EQUAL_S( cpusetsize, cpuset, node->Affinity.set ) )
    return true;

  current_state = the_thread->current_state;

  if ( _States_Is_ready( current_state ) ) {
    _Scheduler_strong_APA_Block( scheduler, the_thread );
  }

  CPU_COPY( node->Affinity.set, cpuset );

  if ( _States_Is_ready( current_state ) ) {
    /*
     * FIXME: Do not ignore threads in need for help.
     */
    (void) _Scheduler_strong_APA_Unblock( scheduler, the_thread );
  }

  return true;
}
