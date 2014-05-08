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
    PEER_TASK_DROP,
    PEER_TASK_DONE
} PeerAssocTaskType;

typedef struct PeerHandleKey
{
    LWMsgHandleType type;
    LWMsgHandleID id;
} PeerHandleKey;

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
        PEER_CALL_RELEASED = 0x10,
        PEER_CALL_WAITING = 0x20
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
    LWMsgTask* event_task;
    PeerAssocTaskType type;
    LWMsgAssoc* assoc;
    struct PeerSession* session;
    LWMsgHashTable incoming_calls;
    LWMsgHashTable outgoing_calls;
    LWMsgRing active_incoming_calls;
    LWMsgRing active_outgoing_calls;
    LWMsgMessage incoming_message;
    LWMsgMessage outgoing_message;
    unsigned incoming:1;
    unsigned outgoing:1;
    unsigned send_blocked:1;
    unsigned recv_blocked:1;
    unsigned recv_partial:1;
    unsigned destroy_outgoing:1;
    LWMsgCookie volatile next_cookie;
    uint32_t volatile refs;
    LWMsgStatus volatile status;
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

typedef struct DirectEndpoint
{
    const char* name;
    struct LWMsgPeer* server;
    LWMsgRing sessions;
    LWMsgRing ring;
} DirectEndpoint;

typedef struct DirectSession
{
    /* Base */
    LWMsgSession base;
    /* Actual session */
    struct PeerSession* session;
    /* Outstanding calls */
    LWMsgRing calls;
    LWMsgRing ring;
    DirectEndpoint* endpoint;
    void* data;
    LWMsgSecurityToken* token;
    uint32_t volatile refs;
} DirectSession;

typedef struct DirectCall
{
    LWMsgCall caller;
    LWMsgCall callee;
    DirectSession* session;
    enum
    {
        DIRECT_CALL_DISPATCHED = 0x1,
        DIRECT_CALL_CANCELED = 0x2,
        DIRECT_CALL_COMPLETED = 0x4,
        DIRECT_CALL_WAITING = 0x8
    } state;

    const LWMsgParams* in;
    LWMsgParams* out;
    LWMsgStatus status;
    LWMsgCompleteFunction complete;
    void* complete_data;
    LWMsgCancelFunction cancel;
    void* cancel_data;
    LWMsgRing ring;
    uint8_t volatile refs;
    unsigned is_callback:1;
} DirectCall;

typedef struct PeerSession
{
    /* Base session struct */
    LWMsgSession base;
    /* Session identifier */
    LWMsgSessionID id;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t volatile refs;
    /* Hash of handles by id */
    LWMsgHashTable handle_by_id;
    /* Lock */
    pthread_mutex_t lock;
    unsigned lock_destroy:1;
    /* Event */
    pthread_cond_t event;
    unsigned event_destroy:1;
    /* Next handle ID */
    unsigned long volatile next_hid;
    /* User data pointer */
    void* data;
    LWMsgSessionDestructFunction destruct;
    /* Owning peer */
    struct LWMsgPeer* peer;
    /* Direct session */
    DirectSession* direct_session;
    /* Assoc session */
    PeerAssocTask* assoc_session;
    /* Explicit endpoint to connect to in lieu of list in peer */
    PeerEndpoint* endpoint;
    /* String representation of connected endpoint */
    char* endpoint_str;
    /* Is session outgoing? */
    unsigned is_outgoing:1;
} PeerSession;

struct LWMsgHandle
{
    /* Key */
    PeerHandleKey key;
    /* Validity bit */
    LWMsgBool volatile valid;
    /* Reference count */
    size_t volatile refs;
    /* Handle type */
    const char* type;
    /* Handle pointer */
    void* pointer;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Link in hash table by id */
    LWMsgRing id_ring;
};


struct LWMsgPeer
{
    const LWMsgContext* context;
    LWMsgProtocol* protocol;
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

    LWMsgPeerTraceFunction trace_begin;
    LWMsgPeerTraceFunction trace_end;
    void* trace_data;

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
    PeerSession* connect_session;
    LWMsgStatus connect_status;

    /* Total number of connected clients */
    size_t num_clients;

    pthread_mutex_t lock;
    unsigned lock_init:1;
    pthread_cond_t event;
    unsigned event_init:1;
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
lwmsg_peer_session_new(
    LWMsgPeer* peer,
    PeerSession** out_session
    );

void
lwmsg_peer_session_reset(
    PeerSession* session
    );

void
lwmsg_peer_session_release(
    PeerSession* session
    );

void
lwmsg_peer_session_retain(
    PeerSession* session
    );

void
lwmsg_peer_session_disconnect(
    PeerSession* session
    );

LWMsgStatus
lwmsg_peer_session_acquire_call(
    PeerSession* session,
    LWMsgCall** call
    );

LWMsgStatus
lwmsg_peer_assoc_task_new_connect(
    PeerSession* session,
    LWMsgAssoc* assoc,
    PeerAssocTask** task
    );

LWMsgStatus
lwmsg_peer_assoc_task_new_accept(
    PeerSession* session,
    LWMsgAssoc* assoc,
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
lwmsg_peer_task_release(
    PeerAssocTask* task
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

LWMsgStatus
lwmsg_peer_accept_fd_internal(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd,
    const char* endpoint
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

LWMsgStatus
lwmsg_direct_session_new(
    PeerSession* session,
    DirectSession** direct_session
    );

void
lwmsg_direct_session_release(
    DirectSession* session
    );

LWMsgStatus
lwmsg_direct_connect(
    const char* name,
    DirectSession* session
    );

void
lwmsg_direct_disconnect(
    DirectSession* session
    );

LWMsgStatus
lwmsg_direct_listen(
    const char* name,
    LWMsgPeer* server
    );

void
lwmsg_direct_shutdown(
    const char* name,
    LWMsgPeer* server
    );

LWMsgStatus
lwmsg_direct_call_new(
    DirectSession* session,
    LWMsgBool is_callback,
    DirectCall** call
    );

void
lwmsg_peer_session_string_for_session(
    LWMsgSession* session,
    LWMsgSessionString string
    );

LWMsgStatus
lwmsg_peer_log_message(
    PeerAssocTask* task,
    LWMsgMessage* message,
    LWMsgBool is_outgoing
    );

LWMsgStatus
lwmsg_peer_log_accept(
    PeerAssocTask* task
    );

LWMsgStatus
lwmsg_peer_log_connect(
    PeerAssocTask* task
    );

LWMsgStatus
lwmsg_peer_log_state(
    PeerAssocTask* task
    );

#endif
