/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _RDR_CCB
{
    //LIST_ENTRY NextCCB;
    UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;

    int fd;
    char *path;
    int oflags;
    mode_t mode;

} RDR_CCB, *PRDR_CCB;

typedef struct _RDR_IRP_CONTEXT
{
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;

} RDR_IRP_CONTEXT, *PRDR_IRP_CONTEXT;

typedef enum _RDR_SOCKET_STATE
{
    RDR_SOCKET_STATE_NOT_READY,
    RDR_SOCKET_STATE_CONNECTING,
    RDR_SOCKET_STATE_NEGOTIATING,
    RDR_SOCKET_STATE_READY,
    RDR_SOCKET_STATE_TEARDOWN,
    RDR_SOCKET_STATE_ERROR
} RDR_SOCKET_STATE;

typedef struct
{
    pthread_mutex_t mutex;      /* Locks the structure */

    RDR_SOCKET_STATE volatile state;
    NTSTATUS volatile error;
    pthread_cond_t event;           /* Signals waiting threads on state change */
    int32_t volatile refCount;      /* Reference count */
    BOOLEAN volatile bParentLink;   /* Whether socket is linked to by parent (global socket table) */

    time_t  volatile lastActiveTime;     /* Checked by the reaper thread and message
                                            handler threads; set when new data is
                                            received */

    int fd;
    PWSTR pwszHostname;         /* Raw hostname, including channel specifier */
    PWSTR pwszCanonicalName;      /* Canconical hostname for DNS resolution/GSS principal construction */
    
    uint32_t maxBufferSize;     /* Max transmit buffer size */
    uint32_t maxRawSize;        /* Maximum raw buffer size */
    uint32_t sessionKey;        /* Socket session key from NEGOTIATE */
    uint32_t capabilities;      /* Remote server capabilities from NEGOTIATE */
    uint8_t *pSecurityBlob;     /* Security blob from NEGOTIATE */
    uint32_t securityBlobLen;   /* Security blob len from NEGOTIATE */

    PLWIO_PACKET_ALLOCATOR hPacketAllocator;

    SMB_HASH_TABLE *pSessionHashByPrincipal;   /* Dependent sessions */
    SMB_HASH_TABLE *pSessionHashByUID;         /* Dependent sessions */

    PLW_TASK pTask;     /* Single reader */

    PSMB_PACKET    pSessionPacket; /* To store packets without a UID
                                         (Negotiate and Session Setup) */
    BOOLEAN volatile bSessionSetupInProgress;

    uint16_t maxMpxCount;       /* MaxMpxCount from NEGOTIATE */
    LSMB_SEMAPHORE semMpx;

    SMB_SECURITY_MODE securityMode; /* Share or User security */
    BOOLEAN  bPasswordsMustBeEncrypted;
    BOOLEAN  bSignedMessagesSupported;
    BOOLEAN  bSignedMessagesRequired;
    BOOLEAN  bUseSignedMessagesIfSupported;
    BOOLEAN  bIgnoreServerSignatures;
    BOOLEAN  bShutdown;

    PBYTE    pSessionKey;
    DWORD    dwSessionKeyLength;

    DWORD    dwSequence;
    PSMB_PACKET pPacket;

} SMB_SOCKET, *PSMB_SOCKET;

typedef enum _RDR_SESSION_STATE
{
    RDR_SESSION_STATE_NOT_READY,
    RDR_SESSION_STATE_INITIALIZING,
    RDR_SESSION_STATE_READY,
    RDR_SESSION_STATE_TEARDOWN,
    RDR_SESSION_STATE_ERROR
} RDR_SESSION_STATE;

typedef struct
{
    pthread_mutex_t mutex;      /* Locks the structure */

    RDR_SESSION_STATE volatile state;    /* Session state : valid, invalid, etc */
    NTSTATUS volatile error;
    pthread_cond_t event;                /* Signals waiting threads on state change */
    int32_t volatile refCount;           /* Count of state-change waiters and users */
    BOOLEAN volatile bParentLink;        /* Whether session is linked to by parent (socket) */

    time_t volatile lastActiveTime;

    SMB_SOCKET *pSocket;        /* Back pointer to parent socket */

    USHORT uid;

    struct _RDR_SESSION_KEY
    {
        uid_t uid;
        PSTR pszPrincipal;
    } key;

    SMB_HASH_TABLE *pTreeHashByPath;    /* Storage for dependent trees */
    SMB_HASH_TABLE *pTreeHashByTID;     /* Storage for dependent trees */

    uint16_t volatile nextTID;           /* The next free MID to be used.  For now this
                                            is merely a monotonically increasing counter
                                            checked against the hash. In the future it
                                            could be a list of free MIDs from a custom
                                            allocator. */

    PSMB_PACKET  pTreePacket;   /* To store packets without a
                                       TID (Tree Connect) or disconnects */
    BOOLEAN volatile bTreeConnectInProgress;

    PBYTE  pSessionKey;
    DWORD  dwSessionKeyLength;

    /* @todo: store max mux, enforce.  Per session, per tree, or global? */
} SMB_SESSION, *PSMB_SESSION;

typedef enum _RDR_TREE_STATE
{
    RDR_TREE_STATE_NOT_READY,
    RDR_TREE_STATE_INITIALIZING,
    RDR_TREE_STATE_READY,
    RDR_TREE_STATE_TEARDOWN,
    RDR_TREE_STATE_ERROR
} RDR_TREE_STATE;

typedef struct
{
    pthread_mutex_t mutex;      /* Locks both the structure and the hash */
                                /* responses are inserted and removed so often
                                   that a RW lock is probably overkill.*/

    RDR_TREE_STATE volatile state;   /* Tree state: valid, invalid, etc. */
    NTSTATUS volatile error;
    pthread_cond_t event;       /* Signals waiting threads on state change */
    int32_t volatile refCount;           /* Count of state-change waiters and users */
    BOOLEAN volatile bParentLink; /* Whether tree is linked to by parent (session) */

    time_t  volatile lastActiveTime;
    SMB_SESSION *pSession;      /* Back pointer to parent session */
    uint16_t tid;
    PSTR pszPath;               /* For hashing */
    uint16_t mid;

    SMB_HASH_TABLE *pResponseHash; /* Storage for dependent responses */

} SMB_TREE, *PSMB_TREE;

typedef struct
{
    /* In a fully asynchronus daemon, this structure would contain all the
       state required to continue an operation. */
    pthread_mutex_t mutex;      /* Locks the structure */
    SMB_RESOURCE_STATE volatile state;   /* Response state: valid, invalid, etc. */
    NTSTATUS volatile error;
    pthread_cond_t event;

    /* No refcount: the lifetime of a response is always managed by the
       owning thread (for now) */

    PSMB_TREE pTree;            /* Parent tree */
    uint16_t mid;               /* Multiplex ID (MID) for the response */

    PSMB_PACKET pPacket; /* Pointer to response packet; response
                                   owner must free */
} SMB_RESPONSE, *PSMB_RESPONSE;

typedef struct _SMB_CLIENT_FILE_HANDLE
{
    pthread_mutex_t     mutex;
    pthread_mutex_t*    pMutex;

    PWSTR     pwszPath;

    PSMB_TREE pTree;

    USHORT usFileType;
    uint16_t  fid;
    uint64_t  llOffset;
    
    struct
    {
        USHORT         usSearchId;
        USHORT         usSearchCount;
        USHORT         usLastNameOffset;
        USHORT         usEndOfSearch;
        SMB_INFO_LEVEL infoLevel;
        PBYTE          pBuffer;
        PBYTE          pCursor;
        ULONG          ulBufferCapacity;
        ULONG          ulBufferLength;
    } find;
} SMB_CLIENT_FILE_HANDLE, *PSMB_CLIENT_FILE_HANDLE;

typedef struct _RDR_CONFIG
{
    ULONG   ulNumMaxPackets;
    BOOLEAN bSignMessagesIfSupported;
} RDR_CONFIG, *PRDR_CONFIG;

typedef struct _RDR_GLOBAL_RUNTIME
{
    RDR_CONFIG        config;
    SMB_HASH_TABLE   *pSocketHashByName;    /* Socket hash by name */
    pthread_mutex_t   socketHashLock;
    pthread_mutex_t*  pSocketHashLock;
    PLWIO_PACKET_ALLOCATOR hPacketAllocator;
    BOOLEAN volatile bShutdown;
    pthread_mutex_t   reaperMutex;
    pthread_cond_t    reaperEvent;
    pthread_t reaperThread;
    time_t expirationTime;
    time_t volatile nextWakeupTime;
    pid_t SysPid;
    PLW_THREAD_POOL pThreadPool;
    PLW_TASK_GROUP pReaderTaskGroup;
} RDR_GLOBAL_RUNTIME, *PRDR_GLOBAL_RUNTIME;

#endif

