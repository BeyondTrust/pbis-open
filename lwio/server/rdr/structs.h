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

#include <lwnet.h>
#include "lwlist.h"

#define SECURITY_MODE_SIGNED_MESSAGES_SUPPORTED 0x4
#define SECURITY_MODE_SIGNED_MESSAGES_REQUIRED 0x8
#define SMB_SHARE_IS_IN_DFS 0x2

#define RDR_OBJECT_PROTOCOL(pObject) (*(SMB_PROTOCOL_VERSION*)(pObject))

typedef struct _RDR_OP_CONTEXT
{
    PIRP pIrp;
    SMB_PACKET Packet;
    BOOLEAN (*Continue) (
        struct _RDR_OP_CONTEXT* pContext,
        NTSTATUS status,
        PVOID pParam
        );
    LW_LIST_LINKS Link;
    union
    {
        struct
        {
            struct _RDR_SOCKET* pSocket;
        } Echo;
        struct 
        {
            LONG64 llByteOffset;
            LONG64 llTotalBytesRead;
            USHORT usReadLen;
        } Read;
        struct
        {
            USHORT usOpCount;
            /* Protected by file mutex */
            USHORT usComplete;
            NTSTATUS Status;
        } Read2;
        struct
        {
            USHORT usIndex;
            ULONG ulChunkOffset;
            ULONG ulChunkLength;
            ULONG ulDataLength;
            NTSTATUS Status;
        } Read2Chunk;
        struct 
        {
            USHORT usOpIndex;
            LONG64 llByteOffset;
            LONG64 llTotalBytesWritten;
        } Write;
        struct
        {
            USHORT usOpCount;
            /* Protected by file mutex */
            USHORT usComplete;
            NTSTATUS Status;
        } Write2;
        struct
        {
            USHORT usIndex;
            ULONG ulChunkLength;
            ULONG ulDataLength;
            NTSTATUS Status;
        } Write2Chunk;
        struct
        {
            union
            {
                struct _RDR_CCB* pFile;
                struct _RDR_CCB2* pFile2;
            };
            PWSTR pwszFilename;
            PWSTR pwszCanonicalPath;
        } Create;
        struct
        {
            union
            {
                struct _RDR_TREE* pTree;
                struct _RDR_TREE2* pTree2;
                struct _RDR_SESSION* pSession;
                struct _RDR_SESSION2* pSession2;
                struct _RDR_SOCKET* pSocket;
            };
            PWSTR pwszSharename;
            BOOLEAN bStopOnDfs;
            PIO_CREDS pCreds;
            uid_t Uid;
            PSMB_PACKET pPacket;
            PSTR pszCachePath;
            HANDLE hGssContext;
            struct _RDR_OP_CONTEXT* pContinue;
        } TreeConnect;
        struct
        {
            PVOID pInformation;
            ULONG ulLength;
            unsigned bRestart:1;
        } QueryDirectory;
        struct
        {
            PIO_CREDS pCreds;
            uid_t Uid;
            IO_CREDS AnonCreds;
            PUNICODE_STRING pPath;
            PWSTR* ppwszFilePath;
            PWSTR* ppwszCanonicalPath;
            PWSTR pwszNamespace;
            NTSTATUS OrigStatus;
            PUSHORT pusTry;
            struct _RDR_OP_CONTEXT* pContinue;
        } DfsConnect;
    } State;
    USHORT usMid;
    /* Retry count */
    USHORT usTry;
} RDR_OP_CONTEXT, *PRDR_OP_CONTEXT;

typedef enum _RDR_SOCKET_STATE
{
    RDR_SOCKET_STATE_NOT_READY,
    RDR_SOCKET_STATE_CONNECTING,
    RDR_SOCKET_STATE_NEGOTIATING,
    RDR_SOCKET_STATE_READY,
    RDR_SOCKET_STATE_ERROR
} RDR_SOCKET_STATE;

typedef struct _RDR_SOCKET
{
    pthread_mutex_t mutex;
    RDR_SOCKET_STATE volatile state;
    NTSTATUS volatile error;
    /* Protocol */
    SMB_PROTOCOL_VERSION version;
    /* Reference count */
    LONG volatile refCount;
    /* Whether socket is linked to by parent (global socket table) */
    BOOLEAN volatile bParentLink;
    int fd;
    /* Raw hostname, including channel specifier */
    PWSTR pwszHostname;
    /* Canconical hostname, address list from netlogon */
    PWSTR pwszCanonicalName;
    PLWNET_RESOLVE_ADDR* ppAddressList;
    ULONG AddressListCount;
    /* Next address to attempt */
    ULONG AddressIndex;
    /* Max transmit buffer size */
    ULONG ulMaxTransactSize;
    /* Max read size */
    ULONG ulMaxReadSize;
    /* Max write size */
    ULONG ulMaxWriteSize;
    /* Maximum raw buffer size */
    ULONG maxRawSize;
    /* Socket session key from NEGOTIATE */
    ULONG sessionKey;
    /* Remote server capabilities from NEGOTIATE */
    ULONG capabilities;
    /* Security blob from NEGOTIATE */
    PBYTE pSecurityBlob;
    /* Security blob len from NEGOTIATE */
    ULONG securityBlobLen;
    /* Dependent sessions */
    SMB_HASH_TABLE *pSessionHashByPrincipal;
    /* Dependent sessions */
    SMB_HASH_TABLE *pSessionHashByUID;
    PLW_TASK pTask;
    PLW_TASK pTimeout;
    /* MaxMpxCount from NEGOTIATE */
    USHORT usMaxSlots;
    USHORT usUsedSlots;
    BYTE ucSecurityMode;
    unsigned bIgnoreServerSignatures:1;
    PBYTE pSessionKey;
    DWORD dwSessionKeyLength;
    DWORD dwSequence;
    /* Incoming packet */
    PSMB_PACKET pPacket;
    /* Outgoing packet */
    PSMB_PACKET pOutgoing;
    size_t OutgoingWritten;
    /* List of RDR_OP_CONTEXTs with packets that need to be sent */
    LW_LIST_LINKS PendingSend;
    /* List of RDR_OP_CONTEXTs waiting for response packets */
    LW_LIST_LINKS PendingResponse;
    /* List of RDR_OP_CONTEXTs waiting for the socket to change state */
    LW_LIST_LINKS StateWaiters;
    ULONG64 ullNextMid;
    unsigned volatile bReadBlocked:1;
    unsigned volatile bWriteBlocked:1;
    unsigned volatile bEcho:1;
} RDR_SOCKET, *PRDR_SOCKET;

typedef enum _RDR_SESSION_STATE
{
    RDR_SESSION_STATE_NOT_READY,
    RDR_SESSION_STATE_INITIALIZING,
    RDR_SESSION_STATE_READY,
    RDR_SESSION_STATE_ERROR
} RDR_SESSION_STATE;

typedef struct _RDR_SESSION
{
    pthread_mutex_t mutex;
    RDR_SESSION_STATE volatile state;
    NTSTATUS volatile error;
    LONG volatile refCount;
    /* Whether session is linked to by parent (socket) */
    BOOLEAN volatile bParentLink;
    /* Back pointer to parent socket */
    RDR_SOCKET *pSocket;
    USHORT uid;
    struct _RDR_SESSION_KEY
    {
        uid_t uid;
        PSTR pszPrincipal;
        ULONG VerifierLength;
        PBYTE pVerifier;
    } key;
    SMB_HASH_TABLE *pTreeHashByPath;
    SMB_HASH_TABLE *pTreeHashByTID;
    PBYTE  pSessionKey;
    DWORD  dwSessionKeyLength;
    PLW_TASK pTimeout;
    LW_LIST_LINKS StateWaiters;
    PRDR_OP_CONTEXT pLogoffContext;
} RDR_SESSION, *PRDR_SESSION;

typedef struct _RDR_SESSION2
{
    pthread_mutex_t mutex;
    RDR_SESSION_STATE volatile state;
    NTSTATUS volatile error;
    LONG volatile refCount;
    /* Whether session is linked to by parent (socket) */
    BOOLEAN volatile bParentLink;
    /* Back pointer to parent socket */
    RDR_SOCKET *pSocket;
    ULONG64 ullSessionId;
    struct _RDR_SESSION_KEY key;
    SMB_HASH_TABLE *pTreeHashByPath;
    SMB_HASH_TABLE *pTreeHashById;
    PBYTE pSessionKey;
    DWORD dwSessionKeyLength;
    PLW_TASK pTimeout;
    LW_LIST_LINKS StateWaiters;
    PRDR_OP_CONTEXT pLogoffContext;
} RDR_SESSION2, *PRDR_SESSION2;

typedef enum _RDR_TREE_STATE
{
    RDR_TREE_STATE_NOT_READY,
    RDR_TREE_STATE_INITIALIZING,
    RDR_TREE_STATE_READY,
    RDR_TREE_STATE_ERROR
} RDR_TREE_STATE;

typedef struct _RDR_TREE
{
    SMB_PROTOCOL_VERSION version;
    pthread_mutex_t mutex;
    RDR_TREE_STATE volatile state;
    NTSTATUS volatile error;
    LONG volatile refCount;
    /* Whether tree is linked to by parent (session) */
    BOOLEAN volatile bParentLink;
    /* Back pointer to parent session */
    RDR_SESSION *pSession;
    USHORT tid;
    USHORT usSupportFlags;
    PWSTR pwszPath;
    PLW_TASK pTimeout;
    LW_LIST_LINKS StateWaiters;
    PRDR_OP_CONTEXT pDisconnectContext;
} RDR_TREE, *PRDR_TREE;

typedef struct _RDR_TREE2
{
    SMB_PROTOCOL_VERSION version;
    pthread_mutex_t mutex;
    RDR_TREE_STATE volatile state;
    NTSTATUS volatile error;
    LONG volatile refCount;
    /* Whether tree is linked to by parent (session) */
    BOOLEAN volatile bParentLink;
    /* Back pointer to parent session */
    RDR_SESSION2 *pSession;
    ULONG ulTid;
    PWSTR pwszPath;
    ULONG ulCapabilities;
    PLW_TASK pTimeout;
    LW_LIST_LINKS StateWaiters;
    PRDR_OP_CONTEXT pDisconnectContext;
} RDR_TREE2, *PRDR_TREE2;

typedef struct _RDR_CCB
{
    SMB_PROTOCOL_VERSION version;
    pthread_mutex_t mutex;
    unsigned bMutexInitialized:1;
    PWSTR pwszPath;
    PWSTR pwszCanonicalPath;
    PRDR_TREE pTree;
    USHORT usFileType;
    USHORT fid;
    uint64_t llOffset; 
    struct
    {
        USHORT usSearchId;
        USHORT usSearchCount;
        USHORT usLastNameOffset;
        USHORT usEndOfSearch;
        SMB_INFO_LEVEL infoLevel;
        PBYTE pBuffer;
        PBYTE pCursor;
        ULONG ulBufferCapacity;
        ULONG ulBufferLength;
        unsigned bInProgress:1;
    } find;
    struct
    {
        FILE_CREATE_OPTIONS CreateOptions;
    } Params;
} RDR_CCB, *PRDR_CCB;

typedef struct _RDR_SMB2_FID
{
    ULONG64 ullPersistentId;
    ULONG64 ullVolatileId;
} __attribute__((__packed__))
RDR_SMB2_FID, *PRDR_SMB2_FID;

typedef struct _RDR_CCB2
{
    SMB_PROTOCOL_VERSION version;
    pthread_mutex_t mutex;
    unsigned bMutexInitialized:1;
    PWSTR pwszPath;
    PWSTR pwszCanonicalPath;
    PRDR_TREE2 pTree;
    RDR_SMB2_FID Fid;
    LONG64 llOffset;
    /* File enumeration state */
    struct
    {
        /* Last query response */
        PSMB_PACKET pPacket;
        /* Current position in packet */
        PBYTE pCursor;
        /* Remaining data */
        ULONG ulRemaining;
        /* FIXME: use this */
        unsigned bInProgress:1;
    } Enum;
} RDR_CCB2, *PRDR_CCB2;

typedef struct _RDR_ROOT_CCB
{
    SMB_PROTOCOL_VERSION version;
    BOOLEAN bIsLocalSystem;
} RDR_ROOT_CCB, *PRDR_ROOT_CCB;

typedef struct _RDR_CONFIG
{
    BOOLEAN bSmb2Enabled;
    BOOLEAN bSigningEnabled;
    BOOLEAN bSigningRequired;
    USHORT usIdleTimeout;
    USHORT usResponseTimeout;
    USHORT usEchoTimeout;
    USHORT usEchoInterval;
    USHORT usConnectTimeout;
    USHORT usMinCreditReserve;
} RDR_CONFIG, *PRDR_CONFIG;

typedef struct _RDR_GLOBAL_RUNTIME
{
    RDR_CONFIG config;
    SMB_HASH_TABLE *pSocketHashByName;
    pthread_mutex_t Lock;
    unsigned bLockConstructed:1;
    pid_t SysPid;
    PLW_THREAD_POOL pThreadPool;
    PLW_TASK_GROUP pSocketTaskGroup;
    PLW_TASK_GROUP pSocketTimerGroup;
    PLW_TASK_GROUP pSessionTimerGroup;
    PLW_TASK_GROUP pTreeTimerGroup;
    PLW_HASHMAP pDomainHints;
    BOOLEAN bShutdown;
} RDR_GLOBAL_RUNTIME, *PRDR_GLOBAL_RUNTIME;

#endif

