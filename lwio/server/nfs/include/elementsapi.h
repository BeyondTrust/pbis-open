/*
 * Copyright Likewise Software    2004-2009
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        elementsapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Elements API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __ELEMENTSAPI_H__
#define __ELEMENTSAPI_H__

#define NFS_LRU_CAPACITY             64

#define SMB_FIND_CLOSE_AFTER_REQUEST 0x1
#define SMB_FIND_CLOSE_IF_EOS        0x2
#define SMB_FIND_RETURN_RESUME_KEYS  0x4
#define SMB_FIND_CONTINUE_SEARCH     0x8
#define SMB_FIND_WITH_BACKUP_INTENT  0x10

typedef UCHAR SMB_OPLOCK_LEVEL;

#define SMB_OPLOCK_LEVEL_NONE  0x00
#define SMB_OPLOCK_LEVEL_I     0x01
#define SMB_OPLOCK_LEVEL_BATCH 0x02
#define SMB_OPLOCK_LEVEL_II    0x03

#define SMB_CN_MAX_BUFFER_SIZE 0x00010000

typedef VOID (*PFN_LWIO_NFS_FREE_OPLOCK_STATE)(HANDLE hOplockState);
typedef VOID (*PFN_LWIO_NFS_FREE_BRL_STATE_LIST)(HANDLE hBRLStateList);
typedef VOID (*PFN_LWIO_NFS_CANCEL_ASYNC_STATE)(HANDLE hAsyncState);
typedef VOID (*PFN_LWIO_NFS_FREE_ASYNC_STATE)(HANDLE hAsyncState);

typedef struct __SMB2_FID
{
    ULONG64 ullPersistentId;
    ULONG64 ullVolatileId;
} __attribute__((__packed__)) SMB2_FID, *PSMB2_FID;

typedef struct _LWIO_ASYNC_STATE
{
    pthread_rwlock_t               mutex;
    pthread_rwlock_t*              pMutex;

    LONG                           refcount;

    ULONG64                        ullAsyncId;
    USHORT                         usCommand;

    HANDLE                         hAsyncState;

    PFN_LWIO_NFS_FREE_ASYNC_STATE   pfnFreeAsyncState;
    PFN_LWIO_NFS_CANCEL_ASYNC_STATE pfnCancelAsyncState;

} LWIO_ASYNC_STATE, *PLWIO_ASYNC_STATE;

typedef struct _LWIO_NFS_FILE
{
    pthread_rwlock_t        mutex;
    pthread_rwlock_t*       pMutex;

    LONG                    refcount;

    USHORT                  fid;

    IO_FILE_HANDLE          hFile;
    PIO_FILE_NAME           pFilename; // physical path on server
    PWSTR                   pwszFilename; // requested path
    ACCESS_MASK             desiredAccess;
    LONG64                  allocationSize;
    FILE_ATTRIBUTES         fileAttributes;
    FILE_SHARE_FLAGS        shareAccess;
    FILE_CREATE_DISPOSITION createDisposition;
    FILE_CREATE_OPTIONS     createOptions;

    UCHAR                          ucCurrentOplockLevel;

    HANDLE                         hOplockState;
    PFN_LWIO_NFS_FREE_OPLOCK_STATE pfnFreeOplockState;

    HANDLE                           hCancellableBRLStateList;
    PFN_LWIO_NFS_FREE_BRL_STATE_LIST pfnFreeBRLStateList;

    ULONG64                        ullLastFailedLockOffset;

} LWIO_NFS_FILE, *PLWIO_NFS_FILE;

typedef struct _LWIO_NFS_SEARCH_SPACE_2
{
    UCHAR                  ucInfoClass;
    UCHAR                  ucSearchFlags;
    ULONG                  ulFileIndex;
    PWSTR                  pwszSearchPatternRaw;
    PWSTR                  pwszSearchPatternRef;
    ULONG                  ulSearchPatternLength;
    PWSTR                  pwszSearchPattern;
    PBYTE                  pFileInfo;
    PBYTE                  pFileInfoCursor;
    USHORT                 usFileInfoLen;
    BOOLEAN                bUseLongFilenames;

} LWIO_NFS_SEARCH_SPACE_2, *PLWIO_NFS_SEARCH_SPACE_2;

typedef struct _LWIO_NFS_FILE_2
{
    pthread_rwlock_t        mutex;
    pthread_rwlock_t*       pMutex;

    LONG                    refcount;

    SMB2_FID                fid;

    IO_FILE_HANDLE          hFile;
    PIO_FILE_NAME           pFilename; // physical path on server
    PWSTR                   pwszFilename; // requested path
    ACCESS_MASK             desiredAccess;
    LONG64                  allocationSize;
    FILE_ATTRIBUTES         fileAttributes;
    FILE_SHARE_FLAGS        shareAccess;
    FILE_CREATE_DISPOSITION createDisposition;
    FILE_CREATE_OPTIONS     createOptions;

    LWIO_NFS_SEARCH_SPACE_2  searchSpace;
    PLWIO_NFS_SEARCH_SPACE_2 pSearchSpace;

    UCHAR                          ucCurrentOplockLevel;

    HANDLE                         hOplockState;
    PFN_LWIO_NFS_FREE_OPLOCK_STATE pfnFreeOplockState;

} LWIO_NFS_FILE_2, *PLWIO_NFS_FILE_2;

typedef struct _LWIO_NFS_TREE
{
    LONG              refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    USHORT            tid;

    PNFS_SHARE_INFO   pShareInfo;

    IO_FILE_HANDLE    hFile;

    PLWIO_NFS_FILE    lruFile[NFS_LRU_CAPACITY];

    PLWRTL_RB_TREE    pFileCollection;

    PLWRTL_RB_TREE    pAsyncStateCollection;

    USHORT            nextAvailableFid;

} LWIO_NFS_TREE, *PLWIO_NFS_TREE;

typedef struct _LWIO_NFS_TREE_2
{
    LONG              refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    ULONG             ulTid;

    PNFS_SHARE_INFO   pShareInfo;

    IO_FILE_HANDLE    hFile;

    PLWIO_NFS_FILE_2  lruFile[NFS_LRU_CAPACITY];

    PLWRTL_RB_TREE    pFileCollection;

    ULONG64           ullNextAvailableFid;

} LWIO_NFS_TREE_2, *PLWIO_NFS_TREE_2;

typedef struct _LWIO_NFS_SESSION
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    USHORT            uid;

    PLWIO_NFS_TREE    lruTree[NFS_LRU_CAPACITY];

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    USHORT            nextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_NFS_SESSION, *PLWIO_NFS_SESSION;

typedef struct _LWIO_NFS_SESSION_2
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    ULONG64           ullUid;

    PLWIO_NFS_TREE_2  lruTree[NFS_LRU_CAPACITY];

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    ULONG             ulNextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_NFS_SESSION_2, *PLWIO_NFS_SESSION_2;

typedef enum
{
    LWIO_NFS_CONN_STATE_INITIAL = 0,
    LWIO_NFS_CONN_STATE_NEGOTIATE,
    LWIO_NFS_CONN_STATE_READY,
    LWIO_NFS_CONN_STATE_INVALID
} LWIO_NFS_CONN_STATE;

typedef struct _NFS_PROPERTIES
{
    USHORT  preferredSecurityMode;
    BOOLEAN bEncryptPasswords;
    BOOLEAN bEnableSecuritySignatures;
    BOOLEAN bRequireSecuritySignatures;
    USHORT  MaxMpxCount;
    USHORT  MaxNumberVCs;
    ULONG   MaxBufferSize;
    ULONG   MaxRawSize;
    ULONG   Capabilities;
    uuid_t  GUID;

} NFS_PROPERTIES, *PNFS_PROPERTIES;

typedef struct _NFS_CLIENT_PROPERITES
{

    USHORT MaxBufferSize;
    USHORT MaxMpxCount;
    USHORT VcNumber;
    ULONG  SessionKey;
    ULONG  Capabilities;
    PWSTR  pwszNativeOS;
    PWSTR  pwszNativeLanMan;
    PWSTR  pwszNativeDomain;

} NFS_CLIENT_PROPERTIES, *PNFS_CLIENT_PROPERTIES;

typedef struct _NFS_SOCKET *PLWIO_NFS_SOCKET;
typedef VOID (*PFN_NFS_SOCKET_FREE)(PLWIO_NFS_SOCKET pSocket);
typedef VOID (*PFN_NFS_SOCKET_DISCONNECT)(PLWIO_NFS_SOCKET pSocket);
typedef NTSTATUS (*PFN_NFS_SOCKET_GET_ADDRESS_BYTES)(PLWIO_NFS_SOCKET pSocket, PVOID* ppAddr, PULONG pulAddrLength);

typedef struct _NFS_CONNECTION_SOCKET_DISPATCH {
    PFN_NFS_SOCKET_FREE pfnFree;
    PFN_NFS_SOCKET_DISCONNECT pfnDisconnect;
    PFN_NFS_SOCKET_GET_ADDRESS_BYTES pfnGetAddressBytes;
} NFS_CONNECTION_SOCKET_DISPATCH, *PNFS_CONNECTION_SOCKET_DISPATCH;

typedef struct _NFS_CONNECTION
{
    LONG                refCount;

    pthread_rwlock_t     mutex;
    pthread_rwlock_t*    pMutex;

    LWIO_NFS_CONN_STATE  state;

    // Immutable for lifetime of connection.
    PLWIO_NFS_SOCKET pSocket;
    PNFS_CONNECTION_SOCKET_DISPATCH pSocketDispatch;

    NFS_PROPERTIES        serverProperties;
    NFS_CLIENT_PROPERTIES clientProperties;

    SMB_PROTOCOL_VERSION protocolVer;

    ULONG           ulSequence;

    union
    {
        USHORT          usNextAvailableUid;
        ULONG64         ullNextAvailableUid;
    };

    // Invariant
    // Not owned
    HANDLE              hPacketAllocator;

    struct
    {
        BOOLEAN         bNeedHeader;
        size_t          sNumBytesToRead;
        size_t          sOffset;
        PSMB_PACKET     pRequestPacket;

    } readerState;

    PBYTE               pSessionKey;
    ULONG               ulSessionKeyLength;

    // Server-wide state
    PNFS_HOST_INFO             pHostinfo;
    PLWIO_NFS_SHARE_ENTRY_LIST pShareList;
    PVOID                      pProtocolTransportDriverContext;

    HANDLE              hGssContext;

    pthread_mutex_t     mutexGssNegotiate;
    pthread_mutex_t*    pMutexGssNegotiate;

    HANDLE              hGssNegotiate;

    union
    {
        PLWIO_NFS_SESSION   lruSession[NFS_LRU_CAPACITY];
        PLWIO_NFS_SESSION_2 lruSession2[NFS_LRU_CAPACITY];
    };

    PLWRTL_RB_TREE      pSessionCollection;

    ULONG64             ullNextAvailableAsyncId;

    PLWRTL_RB_TREE      pAsyncStateCollection;

} LWIO_NFS_CONNECTION, *PLWIO_NFS_CONNECTION;

typedef struct _NFS_FINDER_REPOSITORY
{
    LONG             refCount;

    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PLWRTL_RB_TREE   pSearchSpaceCollection;

    USHORT           usNextSearchId;

} NFS_FINDER_REPOSITORY, *PNFS_FINDER_REPOSITORY;

typedef struct _NFS_SEARCH_SPACE
{
    LONG             refCount;

    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    USHORT           usSearchId;

    IO_FILE_HANDLE   hFile;
    PWSTR            pwszSearchPattern;
    USHORT           usSearchAttrs;
    ULONG            ulSearchStorageType;
    PBYTE            pFileInfo;
    PBYTE            pFileInfoCursor;
    USHORT           usFileInfoLen;
    SMB_INFO_LEVEL   infoLevel;
    BOOLEAN          bUseLongFilenames;

} NFS_SEARCH_SPACE, *PNFS_SEARCH_SPACE;

typedef struct _NFS_TIMER_REQUEST* PNFS_TIMER_REQUEST;

typedef VOID (*PFN_NFS_TIMER_CALLBACK)(
                    PNFS_TIMER_REQUEST pTimerRequest,
                    PVOID              pUserData
                    );

typedef struct _SMB_FS_VOLUME_INFO_HEADER
{
    LONG64  llVolumeCreationTime;
    ULONG   ulVolumeSerialNumber;
    ULONG   ulVolumeLabelLength;
    BOOLEAN bSupportsObjects;
    UCHAR   pad;
} __attribute__((__packed__)) SMB_FS_VOLUME_INFO_HEADER,
                             *PSMB_FS_VOLUME_INFO_HEADER;

typedef struct _SMB_FS_ATTRIBUTE_INFO_HEADER
{
    ULONG ulFSAttributes;
    LONG  lMaxFilenameLen;
    ULONG ulFileSystemNameLen;
} __attribute__((__packed__)) SMB_FS_ATTRIBUTE_INFO_HEADER,
                             *PSMB_FS_ATTRIBUTE_INFO_HEADER;

typedef struct _SMB_FS_INFO_VOLUME_HEADER
{
    ULONG   ulVolumeSerialNumber;
    UCHAR   ucNumLabelChars;

    /* PWSTR pwszLabel; */

} __attribute__((__packed__)) SMB_FS_INFO_VOLUME_HEADER,
                             *PSMB_FS_INFO_VOLUME_HEADER;

typedef struct _SMB_FS_FULL_INFO_HEADER
{
    ULONG64 ullTotalAllocationUnits;
    ULONG64 ullCallerAvailableAllocationUnits;
    ULONG64 ullAvailableAllocationUnits;
    ULONG   ulSectorsPerAllocationUnit;
    ULONG   ulBytesPerSector;

} __attribute__((__packed__)) SMB_FS_FULL_INFO_HEADER,
                             *PSMB_FS_FULL_INFO_HEADER;

typedef struct _SMB_FILE_INTERNAL_INFO_HEADER
{
    ULONG64 ullIndex;

} __attribute__((__packed__)) SMB_FILE_INTERNAL_INFO_HEADER,
                             *PSMB_FILE_INTERNAL_INFO_HEADER;

typedef struct _SMB_FILE_EA_INFO_HEADER
{
    ULONG ulEaSize;

} __attribute__((__packed__)) SMB_FILE_EA_INFO_HEADER,
                             *PSMB_FILE_EA_INFO_HEADER;

typedef struct _NFS_PROTOCOL_EXEC_CONTEXT* PNFS_PROTOCOL_EXEC_CONTEXT;

typedef VOID (*PFN_NFS_PROTOCOL_FREE_EXEC_CONTEXT)(
                        PNFS_PROTOCOL_EXEC_CONTEXT pContext
                        );

typedef struct _NFS_EXEC_CONTEXT
{
    LONG                               refCount;

    pthread_mutex_t                    mutex;
    pthread_mutex_t*                   pMutex;

    PLWIO_NFS_CONNECTION               pConnection;
    PSMB_PACKET                        pSmbRequest;

    PNFS_PROTOCOL_EXEC_CONTEXT         pProtocolContext;
    PFN_NFS_PROTOCOL_FREE_EXEC_CONTEXT pfnFreeContext;

    PSMB_PACKET                        pSmbResponse;
    ULONG                              ulNumDuplicates;

    PSMB_PACKET                        pInterimResponse;

    BOOLEAN                            bInternal;

    ULONG64                            ullAsyncId;

} NFS_EXEC_CONTEXT, *PNFS_EXEC_CONTEXT;

typedef struct _NFS_ELEMENTS_STATISTICS
{
    LONG64 llNumConnections;
    LONG64 llMaxNumConnections;

    LONG64 llNumSessions;
    LONG64 llMaxNumSessions;

    LONG64 llNumTreeConnects;
    LONG64 llMaxNumTreeConnects;

    LONG64 llNumOpenFiles;
    LONG64 llMaxNumOpenFiles;

} NFS_ELEMENTS_STATISTICS, *PNFS_ELEMENTS_STATISTICS;

NTSTATUS
NfsElementsInit(
    VOID
    );

NTSTATUS
NfsTimerPostRequest(
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_NFS_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PNFS_TIMER_REQUEST*    ppTimerRequest
    );

NTSTATUS
NfsTimerCancelRequest(
    IN  PNFS_TIMER_REQUEST pTimerRequest,
    OUT PVOID*             ppUserData
    );

NTSTATUS
NfsTimerRelease(
    IN  PNFS_TIMER_REQUEST pTimerRequest
    );

NTSTATUS
NfsGssAcquireContext(
    PNFS_HOST_INFO pHostinfo,
    HANDLE         hGssOrig,
    PHANDLE        phGssNew
    );

BOOLEAN
NfsGssNegotiateIsComplete(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

NTSTATUS
NfsGssGetSessionDetails(
    HANDLE hGss,
    HANDLE hGssNegotiate,
    PBYTE* ppSessionKey,
    PULONG pulSessionKeyLength,
    PSTR* ppszClientPrincipalName,
    LW_MAP_SECURITY_GSS_CONTEXT* pContextHandle
    );

NTSTATUS
NfsGssBeginNegotiate(
    HANDLE  hGss,
    PHANDLE phGssNegotiate
    );

NTSTATUS
NfsGssNegotiate(
    HANDLE  hGss,
    HANDLE  hGssResume,
    PBYTE   pSecurityInputBlob,
    ULONG   ulSecurityInputBlobLen,
    PBYTE*  ppSecurityOutputBlob,
    ULONG*  pulSecurityOutputBloblen
    );

VOID
NfsGssEndNegotiate(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

VOID
NfsGssReleaseContext(
    HANDLE hGss
    );

NTSTATUS
NfsGssNegHints(
    HANDLE hGssContext,
    PBYTE *ppNegHints,
    ULONG *pulNegHintsLength
    );

ULONG64
NfsAsyncStateBuildId(
    ULONG  ulPid,
    USHORT usMid
    );

NTSTATUS
NfsAsyncStateCreate(
    ULONG64                         ullAsyncId,
    USHORT                          usCommand,
    HANDLE                          hAsyncState,
    PFN_LWIO_NFS_CANCEL_ASYNC_STATE pfnCancelAsyncState,
    PFN_LWIO_NFS_FREE_ASYNC_STATE   pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*              ppAsyncState
    );

VOID
NfsAsyncStateCancel(
    PLWIO_ASYNC_STATE pAsyncState
    );

PLWIO_ASYNC_STATE
NfsAsyncStateAcquire(
    PLWIO_ASYNC_STATE pAsyncState
    );

VOID
NfsAsyncStateRelease(
    PLWIO_ASYNC_STATE pAsyncState
    );

NTSTATUS
NfsConnectionCreate(
    PLWIO_NFS_SOCKET                pSocket,
    HANDLE                          hPacketAllocator,
    HANDLE                          hGssContext,
    PLWIO_NFS_SHARE_ENTRY_LIST      pShareList,
    PNFS_PROPERTIES                 pServerProperties,
    PNFS_HOST_INFO                  pHostinfo,
    PNFS_CONNECTION_SOCKET_DISPATCH pSocketDispatch,
    PLWIO_NFS_CONNECTION*           ppConnection
    );

SMB_PROTOCOL_VERSION
NfsConnectionGetProtocolVersion(
    PLWIO_NFS_CONNECTION pConnection
    );

NTSTATUS
NfsConnectionSetProtocolVersion(
    PLWIO_NFS_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    );

NTSTATUS
NfsConnectionSetProtocolVersion_inlock(
    PLWIO_NFS_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    );

NTSTATUS
NfsConnectionCreateSession(
    PLWIO_NFS_CONNECTION pConnection,
    PLWIO_NFS_SESSION* ppSession
    );

NTSTATUS
NfsConnection2CreateSession(
    PLWIO_NFS_CONNECTION pConnection,
    PLWIO_NFS_SESSION_2* ppSession
    );

NTSTATUS
NfsConnectionFindSession(
    PLWIO_NFS_CONNECTION pConnection,
    USHORT               uid,
    PLWIO_NFS_SESSION*   ppSession
    );

NTSTATUS
NfsConnection2FindSession(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_NFS_SESSION_2* ppSession
    );

NTSTATUS
NfsConnection2CreateAsyncState(
    PLWIO_NFS_CONNECTION            pConnection,
    USHORT                          usCommand,
    PFN_LWIO_NFS_CANCEL_ASYNC_STATE pfnCancelAsyncState,
    PFN_LWIO_NFS_FREE_ASYNC_STATE   pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*              ppAsyncState
    );

NTSTATUS
NfsConnection2FindAsyncState(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullAsyncId,
    PLWIO_ASYNC_STATE*   ppAsyncState
    );

NTSTATUS
NfsConnection2RemoveAsyncState(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullAsyncId
    );

NTSTATUS
NfsConnectionGetNamedPipeClientAddress(
    PLWIO_NFS_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

NTSTATUS
NfsConnectionGetNamedPipeSessionKey(
    PLWIO_NFS_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

LWIO_NFS_CONN_STATE
NfsConnectionGetState(
    PLWIO_NFS_CONNECTION pConnection
    );

BOOLEAN
NfsConnectionIsInvalid(
    PLWIO_NFS_CONNECTION pConnection
    );

PLWIO_NFS_CONNECTION
NfsConnectionAcquire(
    PLWIO_NFS_CONNECTION pConnection
    );

VOID
NfsConnectionRelease(
    PLWIO_NFS_CONNECTION pConnection
    );

NTSTATUS
NfsConnectionRemoveSession(
    PLWIO_NFS_CONNECTION pConnection,
    USHORT              uid
    );

NTSTATUS
NfsConnection2RemoveSession(
    PLWIO_NFS_CONNECTION pConnection,
    ULONG64              ullUid
    );

VOID
NfsConnectionSetInvalid(
    PLWIO_NFS_CONNECTION pConnection
    );

VOID
NfsConnectionSetState(
    PLWIO_NFS_CONNECTION pConnection,
    LWIO_NFS_CONN_STATE  connState
    );

NTSTATUS
NfsSessionCreate(
    USHORT            uid,
    PLWIO_NFS_SESSION* ppSession
    );

NTSTATUS
NfsSessionFindTree(
    PLWIO_NFS_SESSION pSession,
    USHORT           tid,
    PLWIO_NFS_TREE*   ppTree
    );

NTSTATUS
NfsSessionRemoveTree(
    PLWIO_NFS_SESSION pSession,
    USHORT           tid
    );

NTSTATUS
NfsSessionCreateTree(
    PLWIO_NFS_SESSION pSession,
    PNFS_SHARE_INFO   pShareInfo,
    PLWIO_NFS_TREE*   ppTree
    );

PLWIO_NFS_SESSION
NfsSessionAcquire(
    PLWIO_NFS_SESSION pSession
    );

VOID
NfsSessionRelease(
    PLWIO_NFS_SESSION pSession
    );

VOID
NfsSessionRundown(
    PLWIO_NFS_SESSION pSession
    );

NTSTATUS
NfsSession2Create(
    ULONG64              ullUid,
    PLWIO_NFS_SESSION_2* ppSession
    );

NTSTATUS
NfsSession2FindTree(
    PLWIO_NFS_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_NFS_TREE_2*   ppTree
    );

NTSTATUS
NfsSession2RemoveTree(
    PLWIO_NFS_SESSION_2 pSession,
    ULONG               ulTid
    );

NTSTATUS
NfsSession2CreateTree(
    PLWIO_NFS_SESSION_2 pSession,
    PNFS_SHARE_INFO     pShareInfo,
    PLWIO_NFS_TREE_2*   ppTree
    );

PLWIO_NFS_SESSION_2
NfsSession2Acquire(
    PLWIO_NFS_SESSION_2 pSession
    );

VOID
NfsSession2Release(
    PLWIO_NFS_SESSION_2 pSession
    );

VOID
NfsSession2Rundown(
    PLWIO_NFS_SESSION_2 pSession
    );

NTSTATUS
NfsTreeCreate(
    USHORT            tid,
    PNFS_SHARE_INFO   pShareInfo,
    PLWIO_NFS_TREE*    ppTree
    );

NTSTATUS
NfsTreeFindFile(
    PLWIO_NFS_TREE  pTree,
    USHORT         fid,
    PLWIO_NFS_FILE* ppFile
    );

NTSTATUS
NfsTreeCreateFile(
    PLWIO_NFS_TREE           pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE*          ppFile
    );

NTSTATUS
NfsTreeRemoveFile(
    PLWIO_NFS_TREE pTree,
    USHORT        fid
    );

NTSTATUS
NfsTreeAddAsyncState(
    PLWIO_NFS_TREE    pTree,
    PLWIO_ASYNC_STATE pAsyncState
    );

NTSTATUS
NfsTreeFindAsyncState(
    PLWIO_NFS_TREE     pTree,
    ULONG64            ullAsyncId,
    PLWIO_ASYNC_STATE* ppAsyncState
    );

NTSTATUS
NfsTreeRemoveAsyncState(
    PLWIO_NFS_TREE pTree,
    ULONG64        ullAsyncId
    );

BOOLEAN
NfsTreeIsNamedPipe(
    PLWIO_NFS_TREE pTree
    );

NTSTATUS
NfsGetTreeRelativePath(
    PWSTR  pwszOriginalPath,
    PWSTR* ppwszSpecificPath
    );

PLWIO_NFS_TREE
NfsTreeAcquire(
    PLWIO_NFS_TREE pTree
    );

VOID
NfsTreeRelease(
    PLWIO_NFS_TREE pTree
    );

VOID
NfsTreeRundown(
    PLWIO_NFS_TREE pTree
    );

NTSTATUS
NfsTree2Create(
    ULONG             ulTid,
    PNFS_SHARE_INFO   pShareInfo,
    PLWIO_NFS_TREE_2* ppTree
    );

NTSTATUS
NfsTree2FindFile(
    PLWIO_NFS_TREE_2  pTree,
    PSMB2_FID         pFid,
    PLWIO_NFS_FILE_2* ppFile
    );

NTSTATUS
NfsTree2CreateFile(
    PLWIO_NFS_TREE_2        pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE_2*       ppFile
    );

NTSTATUS
NfsTree2RemoveFile(
    PLWIO_NFS_TREE_2 pTree,
    PSMB2_FID        pFid
    );

BOOLEAN
NfsTree2IsNamedPipe(
    PLWIO_NFS_TREE_2 pTree
    );

PLWIO_NFS_TREE_2
NfsTree2Acquire(
    PLWIO_NFS_TREE_2 pTree
    );

VOID
NfsTree2Release(
    PLWIO_NFS_TREE_2 pTree
    );

VOID
NfsTree2Rundown(
    PLWIO_NFS_TREE_2 pTree
    );

NTSTATUS
NfsIoCreateFile(
    IN PNFS_SHARE_INFO pShareInfo,
    OUT PIO_FILE_HANDLE pFileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PIO_CREATE_SECURITY_CONTEXT pSecurityContext,
    IN PIO_FILE_NAME pFileName,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OPTIONAL PVOID pSecurityQualityOfService,
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID pEaBuffer,
    IN ULONG EaLength,
    IN OUT PIO_ECP_LIST* ppEcpList
    );

NTSTATUS
NfsFileCreate(
    USHORT                  fid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE*          ppFile
    );

NTSTATUS
NfsFileSetOplockState(
    PLWIO_NFS_FILE                 pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_NFS_FREE_OPLOCK_STATE pfnReleaseOplockState
    );

HANDLE
NfsFileRemoveOplockState(
    PLWIO_NFS_FILE pFile
    );

VOID
NfsFileResetOplockState(
    PLWIO_NFS_FILE pFile
    );

VOID
NfsFileSetOplockLevel(
    PLWIO_NFS_FILE pFile,
    UCHAR          ucOplockLevel
    );

UCHAR
NfsFileGetOplockLevel(
    PLWIO_NFS_FILE pFile
    );

VOID
NfsFileSetLastFailedLockOffset(
    PLWIO_NFS_FILE pFile,
    ULONG64        ullLastFailedLockOffset
    );

ULONG64
NfsFileGetLastFailedLockOffset(
    PLWIO_NFS_FILE pFile
    );

PLWIO_NFS_FILE
NfsFileAcquire(
    PLWIO_NFS_FILE pFile
    );

VOID
NfsFileRelease(
    PLWIO_NFS_FILE pFile
    );

VOID
NfsFileRundown(
    PLWIO_NFS_FILE pFile
    );

NTSTATUS
NfsFile2Create(
    PSMB2_FID               pFid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_NFS_FILE_2*       ppFile
    );

NTSTATUS
NfsFile2SetOplockState(
    PLWIO_NFS_FILE_2               pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_NFS_FREE_OPLOCK_STATE pfnReleaseOplockState
    );

HANDLE
NfsFile2RemoveOplockState(
    PLWIO_NFS_FILE_2 pFile
    );

VOID
NfsFile2ResetOplockState(
    PLWIO_NFS_FILE_2 pFile
    );

VOID
NfsFile2SetOplockLevel(
    PLWIO_NFS_FILE_2 pFile,
    UCHAR            ucOplockLevel
    );

UCHAR
NfsFile2GetOplockLevel(
    PLWIO_NFS_FILE_2 pFile
    );

PLWIO_NFS_FILE_2
NfsFile2Acquire(
    PLWIO_NFS_FILE_2 pFile
    );

VOID
NfsFile2Release(
    PLWIO_NFS_FILE_2 pFile
    );

VOID
NfsFile2Rundown(
    PLWIO_NFS_FILE_2 pFile
    );

NTSTATUS
NfsFinderCreateRepository(
    OUT PHANDLE phFinderRepository
    );

NTSTATUS
NfsFinderBuildSearchPath(
    IN              PWSTR    pwszPath,
    IN              PWSTR    pwszSearchPattern,
       OUT          PWSTR*   ppwszFilesystemPath,
       OUT          PWSTR*   ppwszSearchPattern,
    IN OUT OPTIONAL PBOOLEAN pbPathHasWildCards
    );

NTSTATUS
NfsFinderCreateSearchSpace(
    IN  PNFS_SHARE_INFO pShareInfo,
    IN  PIO_CREATE_SECURITY_CONTEXT pIoSecurityContext,
    IN  HANDLE         hFinderRepository,
    IN  PWSTR          pwszFilesystemPath,
    IN  PWSTR          pwszSearchPattern,
    IN  USHORT         usSearchAttrs,
    IN  ULONG          ulSearchStorageType,
    IN  SMB_INFO_LEVEL infoLevel,
    IN  BOOLEAN        bUseLongFilenames,
    IN  ACCESS_MASK    accessMask,
    OUT PHANDLE        phFinder,
    OUT PUSHORT        pusSearchId
    );

NTSTATUS
NfsFinderGetSearchSpace(
    IN  HANDLE  hFinderRepository,
    IN  USHORT  usSearchId,
    OUT PHANDLE phFinder
    );

VOID
NfsFinderReleaseSearchSpace(
    IN HANDLE hFinder
    );

NTSTATUS
NfsFinderCloseSearchSpace(
    IN HANDLE hFinderRepository,
    IN USHORT usSearchId
    );

VOID
NfsFinderCloseRepository(
    IN HANDLE hFinderRepository
    );

NTSTATUS
NfsBuildExecContext(
   IN  PLWIO_NFS_CONNECTION pConnection,
   IN  PSMB_PACKET          pSmbRequest,
   IN  BOOLEAN              bInternal,
   OUT PNFS_EXEC_CONTEXT*   ppContext
   );

NTSTATUS
NfsBuildEmptyExecContext(
   OUT PNFS_EXEC_CONTEXT* ppContext
   );

BOOLEAN
NfsIsValidExecContext(
   IN PNFS_EXEC_CONTEXT pExecContext
   );

VOID
NfsReleaseExecContextHandle(
   IN HANDLE hExecContext
   );

PNFS_EXEC_CONTEXT
NfsAcquireExecContext(
   PNFS_EXEC_CONTEXT pContext
   );

VOID
NfsReleaseExecContext(
   IN PNFS_EXEC_CONTEXT pContext
   );

NTSTATUS
NfsElementsGetBootTime(
    PULONG64 pullBootTime
    );

BOOLEAN
NfsElementsGetShareNameEcpEnabled(
    VOID
    );

NTSTATUS
NfsElementsGetStats(
    PNFS_ELEMENTS_STATISTICS pStats
    );

NTSTATUS
NfsElementsResetStats(
    VOID
    );

NTSTATUS
NfsElementsShutdown(
    VOID
    );

#endif /* __ELEMENTSAPI_H__ */
