/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        peer-private.h
 *
 * Abstract:
 *
 *        Multi-threaded peer API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PEER_PRIVATE_H__
#define __LWMSG_PEER_PRIVATE_H__

#include <lwmsg/peer.h>
#include <lwmsg/protocol.h>
#include <lwmsg/connection.h>
#include <lwmsg/assoc.h>
#include <lwmsg/session.h>
#include "context-private.h"
#include "util-private.h"
#include "call-private.h"
#include "session-private.h"
#include "task-private.h"

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

typedef enum PeerAssocTaskType
{
    PEER_TASK_BEGIN_ACCEPT,
    PEER_TASK_FINISH_ACCEPT,
    PEER_TASK_BEGIN_CONNECT,
    PEER_TASK_FINISH_CONNECT,
    PEER_TASK_DISPATCH,
    PEER_TASK_BEGIN_CLOSE,
    PEER_TASK_FINISH_CLOSE,
    PEER_TASK_BEGIN_RESET,
    PEER_TASK_FINISH_RESET,
    PEER_TASK_DROP
} PeerAssocTaskType;

typedef struct PeerCall
{
    LWMsgCall base;
    LWMsgRing hash_ring;
    LWMsgRing queue_ring;
    struct PeerAssocTask* task;
    enum
    {
        PEER_CALL_NONE = 0x0,
        PEER_CALL_DISPATCHED = 0x1,
        PEER_CALL_PENDED = 0x2,
        PEER_CALL_COMPLETED = 0x4,
        PEER_CALL_CANCELLED = 0x8,
        PEER_CALL_RELEASED = 0x10
    } volatile state;

    LWMsgCookie cookie;
    LWMsgStatus volatile status;

    union
    {
        struct
        {
            LWMsgCancelFunction cancel;
            void* cancel_data;
            LWMsgDispatchSpec* spec;
            void* dispatch_data;
            LWMsgParams in;
            LWMsgParams out;
        } incoming;
        struct
        {
            LWMsgCompleteFunction complete;
            void* complete_data;
            const LWMsgParams* in;
            LWMsgParams* out;
        } outgoing;
    } params;
} PeerCall;

typedef struct PeerListenTask
{
    LWMsgPeer* peer;
    LWMsgTask* event_task;
    LWMsgEndpointType type;
    char* endpoint;
    mode_t perms;
    int fd;
} PeerListenTask;

typedef struct PeerAssocTask
{
    LWMsgPeer* peer;
    LWMsgTask* event_task;
    PeerAssocTaskType type;
    LWMsgAssoc* assoc;
    LWMsgSession* session;
    LWMsgHashTable incoming_calls;
    LWMsgHashTable outgoing_calls;
    LWMsgRing active_incoming_calls;
    LWMsgRing active_outgoing_calls;
    LWMsgMessage incoming_message;
    LWMsgMessage outgoing_message;
    unsigned incoming:1;
    unsigned outgoing:1;
    unsigned destroy_outgoing:1;
    LWMsgCookie volatile next_cookie;
    unsigned int volatile refcount;
    LWMsgStatus volatile status;
    pthread_mutex_t call_lock;
    pthread_cond_t call_event;
} PeerAssocTask;

typedef enum PeerState
{
    PEER_STATE_STOPPED = 0,
    PEER_STATE_STARTING,
    PEER_STATE_STARTED,
    PEER_STATE_STOPPING,
    PEER_STATE_ERROR
} PeerState;

typedef struct PeerEndpoint
{
    LWMsgEndpointType type;
    char* endpoint;
    mode_t permissions;
    int fd;
    LWMsgRing ring;
} PeerEndpoint;

struct LWMsgPeer
{    
    LWMsgErrorContext error;
    const LWMsgContext* context;
    LWMsgProtocol* protocol;
    LWMsgSessionManager* session_manager;
    LWMsgTaskManager* task_manager;
    size_t max_clients;
    size_t max_dispatch;
    size_t max_backlog;
    struct
    {
        LWMsgTime message;
        LWMsgTime establish;
        LWMsgTime idle;
    } timeout;
    void* dispatch_data;
    
    LWMsgSessionConstructFunction session_construct;
    LWMsgSessionDestructFunction session_destruct;
    void* session_construct_data;

    LWMsgPeerExceptionFunction except;
    void* except_data;

    struct
    {
        LWMsgDispatchSpec** vector;
        size_t vector_length;
    } dispatch;

    /* Listen task group  */
    LWMsgTaskGroup* listen_tasks;
    /* Connect task group */
    LWMsgTaskGroup* connect_tasks;

    LWMsgRing listen_endpoints;

    LWMsgRing connect_endpoints;
    PeerAssocTask* connect_task;
    LWMsgSession* connect_session;
    PeerState connect_state;
    LWMsgStatus connect_status;

    /* Total number of connected clients */
    size_t num_clients;

    pthread_mutex_t lock;
    pthread_cond_t event;
    PeerState state;
    LWMsgStatus status;
};

#define PEER_RAISE_ERROR(_peer_, _status_, ...) \
    RAISE_ERROR((_peer_), (_status_), __VA_ARGS__)

#define PEER_LOCK(_peer_, _lock_)           \
    do                                          \
    {                                           \
        if (!(_lock_))                          \
        {                                       \
            (_lock_) = 1;                       \
            lwmsg_peer_lock((_peer_));      \
        }                                       \
    } while (0)

#define PEER_UNLOCK(_peer_, _lock_)         \
    do                                          \
    {                                           \
        if ((_lock_))                           \
        {                                       \
            (_lock_) = 0;                       \
            lwmsg_peer_unlock((_peer_));    \
        }                                       \
    } while (0)


#define PEER_CALL(h) ((PeerCall*) (h))

void
lwmsg_peer_lock(
    LWMsgPeer* peer
    );

void
lwmsg_peer_unlock(
    LWMsgPeer* peer
    );

LWMsgStatus
lwmsg_peer_assoc_task_new_connect(
    LWMsgPeer* peer,
    LWMsgAssoc* assoc,
    LWMsgSession* session,
    PeerAssocTask** task
    );

LWMsgStatus
lwmsg_peer_listen_task_new(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t perms,
    int fd,
    PeerListenTask** task
    );

void
lwmsg_peer_listen_task_delete(
    PeerListenTask* task
    );

void
lwmsg_peer_task_cancel_and_unref(
    PeerAssocTask* task
    );

void
lwmsg_peer_task_unref(
    PeerAssocTask* task
    );

void
lwmsg_peer_task_ref(
    PeerAssocTask* task
    );

LWMsgStatus
lwmsg_peer_task_perform(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    LWMsgBool shutdown,
    LWMsgTime* current_time,
    LWMsgTime* next_deadline
    );

LWMsgStatus
lwmsg_peer_task_prepare_select(
    LWMsgPeer* peer,
    PeerAssocTask* task,
    int* nfds,
    fd_set* readset,
    fd_set* writeset
    );

LWMsgBool
lwmsg_peer_acquire_client_slot(
    LWMsgPeer* peer
    );

void
lwmsg_peer_release_client_slot(
    LWMsgPeer* peer
    );

size_t
lwmsg_peer_get_num_clients(
    LWMsgPeer* peer
    );

void
lwmsg_peer_call_delete(
    PeerCall* call
    );

LWMsgStatus
lwmsg_peer_call_new(
    PeerAssocTask* task,
    PeerCall** call
    );

LWMsgStatus
lwmsg_peer_call_dispatch_incoming(
    PeerCall* call,
    LWMsgDispatchSpec* spec,
    void* dispatch_data,
    LWMsgMessage* incoming_message
    );

LWMsgStatus
lwmsg_peer_call_complete_outgoing(
    PeerCall* call,
    LWMsgMessage* incoming_message
    );

LWMsgStatus
lwmsg_peer_call_cancel_incoming(
    PeerCall* call
    );

#endif
