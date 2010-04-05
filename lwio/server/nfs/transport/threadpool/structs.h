#ifndef __STRUCTS_H__
#define __STRUCTS_H__

// Owned by listener
typedef struct _NFS_TRANSPORT_LISTENER {
    NFS_TRANSPORT_HANDLE pTransport;
    PLW_THREAD_POOL pPool;
    PLW_TASK pTask;
    PLW_TASK_GROUP pTaskGroup;
    int ListenFd;
} NFS_TRANSPORT_LISTENER, *PNFS_TRANSPORT_LISTENER;

// Top-level structure
typedef struct _NFS_TRANSPORT_HANDLE_DATA {
    NFS_TRANSPORT_PROTOCOL_DISPATCH Dispatch;
    PNFS_PROTOCOL_TRANSPORT_CONTEXT pContext;
    NFS_TRANSPORT_LISTENER Listener;
} NFS_TRANSPORT_HANDLE_DATA, *PNFS_TRANSPORT_HANDLE_DATA;

typedef ULONG NFS_SOCKET_STATE_MASK, *PNFS_SOCKET_STATE_MASK;

#define NFS_SOCKET_STATE_CLOSED         0x00000001
#define NFS_SOCKET_STATE_FD_WRITABLE    0x00000002
#define NFS_SOCKET_STATE_FD_READABLE    0x00000004
#define NFS_SOCKET_STATE_DISCONNECTED   0x00000008

// Abstraction for socket address to handle different address types.
typedef union {
    struct sockaddr Generic;
    struct sockaddr_in Ip;
#ifdef LW_USE_INET6
    struct sockaddr_in6 Ip6;
#endif
} NFS_SOCKET_ADDRESS, *PNFS_SOCKET_ADDRESS;

// Transport abstraction for a connection.
typedef struct _NFS_SOCKET
{
    LONG RefCount;
    LW_RTL_MUTEX Mutex;

    // Back reference.
    PNFS_TRANSPORT_LISTENER pListener;

    // Protocol connection context.
    PNFS_CONNECTION pConnection;

    // Socket-specific information.

    // Immutable for life of the task.
    int fd;
    NFS_SOCKET_ADDRESS ClientAddress;
    SOCKLEN_T ClientAddressLength;
    CHAR AddressStringBuffer[NFS_SOCKET_ADDRESS_STRING_MAX_SIZE];

    PLW_TASK pTask;
    NFS_SOCKET_STATE_MASK StateMask;
    NTSTATUS DoneStatus;
    // Buffer information
    PVOID pBuffer;
    ULONG Size;
    ULONG Minimum;
    ULONG Offset;
    // Send queue - (NFS_SEND_ITEM.SendLinks)
    LW_LIST_LINKS SendHead;
} NFS_SOCKET;

typedef struct _NFS_SEND_ITEM
{
    LW_LIST_LINKS SendLinks;
    PNFS_SEND_CONTEXT pSendContext;
    PLW_ZCT_VECTOR pZct;
    PVOID pBuffer;
    ULONG Length;
    ULONG Offset;
} NFS_SEND_ITEM, *PNFS_SEND_ITEM;

typedef struct _LWIO_NFS_LISTENER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PLW_THREAD_POOL pPool;
    PLW_TASK pTask;
    PLW_TASK_GROUP pTaskGroup;
    int listenFd;
 
    NFS_PROPERTIES serverProperties;

    // Invariant
    // Not owned
    HANDLE                     hPacketAllocator;
    HANDLE                     hGssContext;
    PLWIO_NFS_SHARE_ENTRY_LIST pShareList;
} LWIO_NFS_LISTENER_CONTEXT, *PLWIO_NFS_LISTENER_CONTEXT;

typedef struct _LWIO_NFS_LISTENER
{
    LWIO_NFS_LISTENER_CONTEXT context;

} LWIO_NFS_LISTENER, *PLWIO_NFS_LISTENER;

typedef struct _LWIO_NFS_THREADPOOL_TRANSPORT_CONFIG
{
    BOOLEAN bEnableSigning;
    BOOLEAN bRequireSigning;

} LWIO_NFS_THREADPOOL_TRANSPORT_CONFIG, *PLWIO_NFS_THREADPOOL_TRANSPORT_CONFIG;

typedef struct _LWIO_NFS_THREADPOOL_TRANSPORT_GLOBALS
{
    pthread_mutex_t              mutex;

    PSMB_PROD_CONS_QUEUE         pWorkQueue;

    LWIO_NFS_LISTENER            listener;

    PLWIO_PACKET_ALLOCATOR       hPacketAllocator;

    PLWIO_NFS_SHARE_ENTRY_LIST   pShareList;

} LWIO_NFS_THREADPOOL_TRANSPORT_GLOBALS, *PLWIO_NFS_THREADPOOL_TRANSPORT_GLOBALS;

#endif /* __STRUCTS_H__ */
