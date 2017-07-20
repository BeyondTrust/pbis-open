#include <dce/smb.h>
#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <comsoc_smb.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cnp.h>
#include <npnaf.h>
#include <stddef.h>

#include <lwio/lwio.h>
#include <lw/base.h>

#define SMB_SOCKET_LOCK(sock) (rpc__smb_socket_lock(sock))
#define SMB_SOCKET_UNLOCK(sock) (rpc__smb_socket_unlock(sock))
#define GE(err)                                 \
    do                                          \
    {                                           \
        if ((err))                              \
            goto error;                         \
    } while (0)
#define RETRY_INTERVAL 10


typedef struct rpc_smb_transport_info_s
{
    rpc_access_token_p_t access_token;
    struct
    {
        unsigned16 length;
        unsigned char* data;
    } session_key;
    PIO_CREDS creds;
} rpc_smb_transport_info_t, *rpc_smb_transport_info_p_t;

typedef enum rpc_smb_state_e
{
    SMB_STATE_SEND,
    SMB_STATE_RECV,
    SMB_STATE_LISTEN,
    SMB_STATE_ERROR
} rpc_smb_state_t;

typedef struct rpc_smb_buffer_s
{
    size_t capacity;
    unsigned char* base;
    unsigned char* start_cursor;
    unsigned char* end_cursor;
} rpc_smb_buffer_t, *rpc_smb_buffer_p_t;

typedef struct rpc_smb_close_context_s
{
    IO_STATUS_BLOCK io_status;
    IO_ASYNC_CONTROL_BLOCK io_async;
} rpc_smb_close_context_t, *rpc_smb_close_context_p_t;

typedef struct rpc_smb_socket_s
{
    rpc_smb_state_t volatile state;
    rpc_np_addr_t peeraddr;
    rpc_np_addr_t localaddr;
    rpc_smb_transport_info_t info;
    IO_FILE_NAME filename;
    IO_FILE_HANDLE np;
    rpc_smb_close_context_p_t close_context;
    IO_STATUS_BLOCK io_status;
    IO_ASYNC_CONTROL_BLOCK io_async;
    rpc_timer_t retry_timer;
    unsigned retry_timer_set:1;
    rpc_smb_buffer_t sendbuffer;
    rpc_smb_buffer_t recvbuffer;
    boolean received_last;
    struct
    {
        IO_FILE_HANDLE* queue;
        size_t capacity;
        size_t length;
        int selectfd[2];
    } accept_backlog;
    dcethread* listen_thread;
    dcethread_mutex lock;
    unsigned lock_init:1;
    dcethread_cond event;
    unsigned event_init:1;
} rpc_smb_socket_t, *rpc_smb_socket_p_t;

INTERNAL
void
rpc__smb_socket_destroy(
    rpc_smb_socket_p_t sock
    );

INTERNAL
void
rpc__smb_socket_create_named_pipe_complete(
    void* data
    );

INTERNAL
void
rpc__smb_socket_connect_named_pipe_complete(
    void* data
    );

INTERNAL
int
rpc__smb_socket_accept_async(
    rpc_smb_socket_p_t smb
    );

void
rpc_smb_transport_info_from_lwio_creds(
    void* creds,
    rpc_transport_info_handle_t* info,
    unsigned32* st
    )
{
    rpc_smb_transport_info_p_t smb_info = NULL;

    smb_info = calloc(1, sizeof(*smb_info));
    
    if (!smb_info)
    {
        *st = rpc_s_no_memory;
        goto error;
    }

    if (LwIoCopyCreds(creds, &smb_info->creds) != 0)
    {
        *st = rpc_s_no_memory;
        goto error;
    }

    *info = (rpc_transport_info_handle_t) smb_info;

    *st = rpc_s_ok;

error:

    if (*st != rpc_s_ok && smb_info)
    {
        rpc_smb_transport_info_free((rpc_transport_info_handle_t) smb_info);
    }

    return;
}

INTERNAL
void
rpc__smb_transport_info_destroy(
    rpc_smb_transport_info_p_t smb_info
    )
{
    if (smb_info->creds)
    {
        LwIoDeleteCreds(smb_info->creds);
    }
    
    if (smb_info->session_key.data)
    {
        free(smb_info->session_key.data);
    }

    RtlReleaseAccessToken(&smb_info->access_token);
}

void
rpc_smb_transport_info_free(
    rpc_transport_info_handle_t info
    )
{
    rpc__smb_transport_info_destroy((rpc_smb_transport_info_p_t) info);
    free(info);
}

void
rpc_smb_transport_info_inq_session_key(
    rpc_transport_info_handle_t info,
    unsigned char** sess_key,
    unsigned16* sess_key_len
    )
{
    rpc_smb_transport_info_p_t smb_info = (rpc_smb_transport_info_p_t) info;

    if (sess_key)
    {
        *sess_key = smb_info->session_key.data;
    }

    if (sess_key_len)
    {
        *sess_key_len = (unsigned32) smb_info->session_key.length;
    }
}

INTERNAL
boolean
rpc__smb_transport_info_equal(
    rpc_transport_info_handle_t info1,
    rpc_transport_info_handle_t info2
    )
{
    rpc_smb_transport_info_p_t smb_info1 = (rpc_smb_transport_info_p_t) info1;
    rpc_smb_transport_info_p_t smb_info2 = (rpc_smb_transport_info_p_t) info2;

    return 
        (smb_info2 == NULL
         && smb_info1->creds == NULL) ||
        (smb_info2 != NULL &&
         ((smb_info1->creds == NULL && smb_info2->creds == NULL) ||
          (smb_info1->creds != NULL && smb_info2->creds != NULL &&
           LwIoCompareCreds(smb_info1->creds, smb_info2->creds))));
}

INTERNAL
inline
size_t
rpc__smb_buffer_pending(
    rpc_smb_buffer_p_t buffer
    )
{
    return buffer->end_cursor - buffer->start_cursor;
}

INTERNAL
inline
size_t
rpc__smb_buffer_length(
    rpc_smb_buffer_p_t buffer
    )
{
    return buffer->end_cursor - buffer->base;
}

INTERNAL
inline
size_t
rpc__smb_buffer_available(
    rpc_smb_buffer_p_t buffer
    )
{
    return (buffer->base + buffer->capacity) - buffer->end_cursor;
}

INTERNAL
inline
rpc_socket_error_t
rpc__smb_buffer_ensure_available(
    rpc_smb_buffer_p_t buffer,
    size_t space
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    unsigned char* new_base = NULL;

    if (!buffer->base)
    {
        buffer->capacity = 2048;
        buffer->base = malloc(buffer->capacity);

        if (!buffer->base)
        {
            serr = ENOMEM;
            goto error;
        }
        
        buffer->end_cursor = buffer->start_cursor = buffer->base;
    }
    
    if (space > rpc__smb_buffer_available(buffer))
    {
        while (space > rpc__smb_buffer_available(buffer))
        {
            buffer->capacity *= 2;
        }
            
        new_base = realloc(buffer->base, buffer->capacity);
        
        if (!new_base)
        {
            serr = ENOMEM;
            goto error;
        }
        
        buffer->start_cursor = new_base + (buffer->start_cursor - buffer->base);
        buffer->end_cursor = new_base + (buffer->end_cursor - buffer->base);
        
        buffer->base = new_base;
    }

error:

    return serr;
}

INTERNAL
inline
size_t
rpc__smb_buffer_packet_size(
    rpc_smb_buffer_p_t buffer
    )
{
    rpc_cn_common_hdr_p_t packet = (rpc_cn_common_hdr_p_t) buffer->start_cursor;
    uint16_t result;

    if (rpc__smb_buffer_pending(buffer) < sizeof(*packet))
    {
        return sizeof(*packet);
    }
    else
    {
        int packet_order = ((packet->drep[0] >> 4) & 1);
        int native_order = (NDR_LOCAL_INT_REP == ndr_c_int_big_endian) ? 0 : 1;

        if (packet_order != native_order)
        {
            result = SWAB_16(packet->frag_len);
        }
        else
        {
            result = packet->frag_len;
        }

        return (size_t) result;
    }
}

INTERNAL
inline
boolean
rpc__smb_buffer_packet_is_last(
    rpc_smb_buffer_p_t buffer
    )
{
    rpc_cn_common_hdr_p_t packet = (rpc_cn_common_hdr_p_t) buffer->start_cursor;

    return ((packet->flags & RPC_C_CN_FLAGS_LAST_FRAG) == RPC_C_CN_FLAGS_LAST_FRAG &&
	    packet->ptype != RPC_C_CN_PKT_AUTH3);
}

INTERNAL
inline
rpc_socket_error_t
rpc__smb_buffer_append(
    rpc_smb_buffer_p_t buffer,
    void* data,
    size_t data_size
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
 
    serr = rpc__smb_buffer_ensure_available(buffer, data_size);
    if (serr)
    {
        goto error;
    }

    memcpy(buffer->end_cursor, data, data_size);

    buffer->end_cursor += data_size;

error:
    
    return serr;
}

INTERNAL
inline
void
rpc__smb_buffer_settle(
    rpc_smb_buffer_p_t buffer
    )
{
    size_t filled = buffer->end_cursor - buffer->start_cursor;
    memmove(buffer->base, buffer->start_cursor, filled);
    buffer->start_cursor = buffer->base;
    buffer->end_cursor = buffer->base + filled;
}

/* Advance buffer start_cursor to the end of the next valid fragment */
INTERNAL
inline
boolean
rpc__smb_buffer_advance_cursor(rpc_smb_buffer_p_t buffer, boolean* last)
{
    size_t packet_size;

    if (rpc__smb_buffer_packet_size(buffer) <= rpc__smb_buffer_pending(buffer))
    {
        packet_size = rpc__smb_buffer_packet_size(buffer);

        if (last)
        {
            *last = rpc__smb_buffer_packet_is_last(buffer);
        }

        buffer->start_cursor += packet_size;

        return true;
    }
    else
    {
        return false;
    }
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_create(
    rpc_smb_socket_p_t* out
    )
{
    rpc_smb_socket_p_t sock = NULL;
    int err = 0;

    sock = calloc(1, sizeof(*sock));

    if (!sock)
    {
        err = ENOMEM;
        goto done;
    }

    sock->close_context = calloc(1, sizeof(*sock->close_context));

    if (!sock->close_context)
    {
        err = ENOMEM;
        goto done;
    }
    sock->accept_backlog.selectfd[0] = -1;
    sock->accept_backlog.selectfd[1] = -1;

    /* Set up reasonable default local endpoint */
    sock->localaddr.rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_NP;
    sock->localaddr.len = offsetof(rpc_np_addr_t, remote_host) + sizeof(sock->localaddr.remote_host);
    sock->localaddr.sa.sun_family = AF_UNIX;
    sock->localaddr.sa.sun_path[0] = '\0';
    sock->localaddr.remote_host[0] = '\0';

    DCETHREAD_TRY
    {
        dcethread_mutex_init_throw(&sock->lock, NULL);
        sock->lock_init = TRUE;
        dcethread_cond_init_throw(&sock->event, NULL);
        sock->event_init = TRUE;
    }
    DCETHREAD_CATCH_ALL(e)
    {
        err = ENOMEM;
    }
    DCETHREAD_ENDTRY;

    if (err)
    {
        goto done;
    }

    *out = sock;

done:

    if (err && sock)
    {
        rpc__smb_socket_destroy(sock);
    }

    return err;
}

INTERNAL
void
rpc__smb_close_file_complete(
    void* context
    )
{
    rpc_smb_close_context_p_t close_context = context;

    if (close_context->io_async.AsyncCancelContext)
    {
        LwNtDereferenceAsyncCancelContext(&close_context->io_async.AsyncCancelContext);
    }

    free(close_context);
}

INTERNAL
void
rpc__smb_socket_destroy(
    rpc_smb_socket_p_t sock
    )
{
    size_t i;

    if (sock)
    {
        if (sock->accept_backlog.queue)
        {
            for (i = 0; i < sock->accept_backlog.capacity; i++)
            {
                if (sock->accept_backlog.queue[i])
                {
                    NtCloseFile(sock->accept_backlog.queue[i]);
                }
            }
            
            close(sock->accept_backlog.selectfd[0]);
            close(sock->accept_backlog.selectfd[1]);

            free(sock->accept_backlog.queue);
        }

        if (sock->filename.Name.Buffer)
        {
            LwRtlUnicodeStringFree(&sock->filename.Name);
        }

        if (sock->close_context)
        {
            sock->close_context->io_async.Callback = rpc__smb_close_file_complete;
            sock->close_context->io_async.CallbackContext = sock->close_context;

            if (!sock->np ||
                LwNtAsyncCloseFile(
                    sock->np,
                    &sock->close_context->io_async,
                    &sock->close_context->io_status) != STATUS_PENDING)
            {
                free(sock->close_context);
            }
        }

        if (sock->sendbuffer.base)
        {
            free(sock->sendbuffer.base);
        }

        if (sock->recvbuffer.base)
        {
            free(sock->recvbuffer.base);
        }
        
        if (sock->retry_timer_set)
        {
            rpc__timer_clear(&sock->retry_timer);
        }

        rpc__smb_transport_info_destroy(&sock->info);

        if (sock->lock_init)
        {
            dcethread_mutex_destroy_throw(&sock->lock);
        }

        if (sock->event_init)
        {
            dcethread_cond_destroy_throw(&sock->event);
        }

        free(sock);
    }
    
    return;
}

INTERNAL
inline
void
rpc__smb_socket_lock(
    rpc_smb_socket_p_t sock
    )
{
    dcethread_mutex_lock_throw(&sock->lock);
}

INTERNAL
inline
void
rpc__smb_socket_unlock(
    rpc_smb_socket_p_t sock
    )
{
    dcethread_mutex_unlock_throw(&sock->lock);
}

INTERNAL
inline
void
rpc__smb_socket_change_state(
    rpc_smb_socket_p_t sock,
    rpc_smb_state_t state
    )
{
    sock->state = state;
    dcethread_cond_broadcast_throw(&sock->event);
}

INTERNAL
inline
void
rpc__smb_socket_wait(
    rpc_smb_socket_p_t sock
    )
{
    DCETHREAD_TRY
    {
        dcethread_cond_wait_throw(&sock->event, &sock->lock);
    }
    DCETHREAD_CATCH_ALL(e)
    {
        dcethread_mutex_unlock(&sock->lock);
        DCETHREAD_RAISE(*e);
    }
    DCETHREAD_ENDTRY;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_construct(
    rpc_socket_t sock,
    rpc_protseq_id_t pseq_id ATTRIBUTE_UNUSED,
    rpc_transport_info_handle_t info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb_sock = NULL;
    rpc_smb_transport_info_p_t smb_info = (rpc_smb_transport_info_p_t) info;

    serr = rpc__smb_socket_create(&smb_sock);

    if (serr)
    {
        goto error;
    }

    if (smb_info)
    {
        if (smb_info->creds)
        {
            serr = NtStatusToErrno(LwIoCopyCreds(smb_info->creds, &smb_sock->info.creds));
            if (serr)
            {
                goto error;
            }
        }
    }

    sock->data.pointer = (void*) smb_sock;

done:
    
    return serr;

error:

    if (smb_sock)
    {
        rpc__smb_socket_destroy(smb_sock);
    }

    goto done;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_destruct(
    rpc_socket_t sock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    rpc__smb_socket_destroy((rpc_smb_socket_p_t) sock->data.pointer);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_bind(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_np_addr_p_t npaddr = (rpc_np_addr_p_t) addr;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;

    smb->localaddr = *npaddr;

    return serr;
}

INTERNAL
void
rpc__smb_socket_connect(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_cn_assoc_t *assoc ATTRIBUTE_UNUSED,
    unsigned32 *st
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    char *netaddr = NULL;
    char *endpoint = NULL;
    char *pipename = NULL;
    unsigned32 dbg_status = 0;
    PSTR smbpath = NULL;
    PBYTE sesskey = NULL;
    USHORT sesskeylen = 0;
    IO_STATUS_BLOCK io_status = { 0 };
    static const ACCESS_MASK pipeAccess = 
        READ_CONTROL | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES |
        FILE_WRITE_EA | FILE_READ_EA | FILE_READ_DATA | FILE_APPEND_DATA |
        FILE_WRITE_DATA;

    SMB_SOCKET_LOCK(smb);

    /* Break address into host and endpoint */
    rpc__naf_addr_inq_netaddr (addr,
                               (unsigned_char_t**) &netaddr,
                               &dbg_status);
    rpc__naf_addr_inq_endpoint (addr,
                                (unsigned_char_t**) &endpoint,
                                &dbg_status);
    
    if (!strncmp(endpoint, "\\pipe\\", sizeof("\\pipe\\") - 1) ||
        !strncmp(endpoint, "\\PIPE\\", sizeof("\\PIPE\\") - 1))
    {
        pipename = endpoint + sizeof("\\pipe\\") - 1;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    status = LwRtlCStringAllocatePrintf(
                &smbpath, 
                "/rdr/%s/IPC$/%s",
                (char*) netaddr,
                (char*) pipename);
    if (status)
    {
        goto error;
    }
    
    status = LwRtlUnicodeStringAllocateFromCString(
                &smb->filename.Name,
                smbpath);
    if (status)
    {
        goto error;
    }

    status = NtCreateFile(
                &smb->np,                                /* Created handle */
                NULL,                                    /* Async control block */
                &io_status,                              /* Status block */
                &smb->filename,                          /* Filename */
                NULL,                                    /* Security descriptor */
                NULL,                                    /* Security QOS */
                pipeAccess,                              /* Access mode */
                0,                                       /* Allocation size */
                0,                                       /* File attributes */
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,      /* Sharing mode */
                FILE_OPEN,                               /* Create disposition */
                FILE_NON_DIRECTORY_FILE | FILE_OPEN_NO_RECALL,             /* Create options */
                NULL,                                    /* EA buffer */
                0,                                       /* EA buffer length */
                NULL,                                    /* ECP List */
                smb->info.creds);                       /* Security token */
    if (status)
    {
        goto error;
    }

    status = LwIoGetSessionKey(
                smb->np,
                &sesskeylen,
                &sesskey);
    if (status)
    {
        goto error;
    }

    smb->info.session_key.length = sesskeylen;
    smb->info.session_key.data = malloc(sesskeylen);
    if (!smb->info.session_key.data)
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }
    memcpy(smb->info.session_key.data, sesskey, sesskeylen);

    /* Save address for future inquiries on this socket */
    memcpy(&smb->peeraddr, addr, sizeof(smb->peeraddr));

    /* Since we did a connect, we will be sending first */
    smb->state = SMB_STATE_SEND;

    *st = rpc_s_ok;

done:

    if (sesskey)
    {
        RtlMemoryFree(sesskey);
    }

    if (smbpath)
    {
        RtlMemoryFree(smbpath);
    }

    SMB_SOCKET_UNLOCK(smb);

    // rpc_string_free handles when *ptr is NULL
    rpc_string_free((unsigned_char_t**) &netaddr, &dbg_status);
    rpc_string_free((unsigned_char_t**) &endpoint, &dbg_status);

    return;

error:

    *st = NtStatusToRpcStatus(status);
    goto done;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_accept(
    rpc_socket_t sock,
    rpc_addr_p_t addr,
    rpc_socket_t *newsock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    rpc_socket_t npsock = NULL;
    rpc_smb_socket_p_t npsmb = NULL;
    IO_FILE_HANDLE np = NULL;
    size_t i;
    char c = 0;
    BYTE clientaddr[16] = {0};
    USHORT clientaddrlen = sizeof(clientaddr);
    boolean locked = false;

    *newsock = NULL;

    SMB_SOCKET_LOCK(smb);
    locked = true;

    while (smb->accept_backlog.length == 0)
    {
        if (smb->state == SMB_STATE_ERROR)
        {
            serr = -1;
            goto error;
        }

        rpc__smb_socket_wait(smb);
    }

    for (i = 0; i < smb->accept_backlog.capacity; i++)
    {
        if (smb->accept_backlog.queue[i] != NULL)
        {
            np = smb->accept_backlog.queue[i];
            smb->accept_backlog.queue[i] = NULL;
            smb->accept_backlog.length--;
            if (read(smb->accept_backlog.selectfd[0], &c, sizeof(c)) != sizeof(c))
            {
                serr = errno;
                goto error;
            }
            dcethread_cond_broadcast_throw(&smb->event);
            break;
        }
    }

    SMB_SOCKET_UNLOCK(smb);
    locked = false;

    serr = rpc__socket_open(sock->pseq_id, NULL, &npsock);
    if (serr)
    {
        goto error;
    }

    npsmb = (rpc_smb_socket_p_t) npsock->data.pointer;

    npsmb->np = np;
    np = NULL;
    
    npsmb->state = SMB_STATE_RECV;

    memcpy(&npsmb->localaddr, &smb->localaddr, sizeof(npsmb->localaddr));

    /* Use our address as a template for client address */
    memcpy(&npsmb->peeraddr, &smb->localaddr, sizeof(npsmb->peeraddr));

    /* Query for client address */
    serr = NtStatusToErrno(
        LwIoGetPeerAddress(
            npsmb->np,
            clientaddr,
            &clientaddrlen));
    if (serr)
    {
        goto error;
    }

    switch (clientaddrlen)
    {
    case 0:
        /* No client address */
        break;
    case 4:
        if (inet_ntop(AF_INET, clientaddr, npsmb->peeraddr.remote_host, sizeof(npsmb->peeraddr.remote_host)) == NULL)
        {
            serr = errno;
            goto error;
        }
        break;
#ifdef AF_INET6
    case 16:
        if (inet_ntop(AF_INET6, clientaddr, npsmb->peeraddr.remote_host, sizeof(npsmb->peeraddr.remote_host)) == NULL)
        {
            serr = errno;
            goto error;
        }
        break;
#endif
    default:
        serr = ENOTSUP;
        goto error;
        break;
    }

    if (addr)
    {
        memcpy(addr, &npsmb->peeraddr, sizeof(npsmb->peeraddr));
    }

     serr = NtStatusToErrno(
        LwIoGetPeerAccessToken(
            npsmb->np,
            &npsmb->info.access_token));
    if (serr)
    {
        goto error;
    }

    serr = NtStatusToErrno(
        LwIoGetSessionKey(
            npsmb->np,
            &npsmb->info.session_key.length,
            &npsmb->info.session_key.data));
    if (serr)
    {
        goto error;
    }
    
    *newsock = npsock;
    npsock = NULL;

error:

    if (np)
    {
        NtCloseFile(np);
    }

    if (npsock)
    {
        rpc__socket_close(npsock);
    }

    if (locked)
    {
        SMB_SOCKET_UNLOCK(smb);
    }

    return serr;
}

INTERNAL
void
rpc__smb_socket_retry(void* data)
{
    int serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) data;

    SMB_SOCKET_LOCK(smb);

    rpc__timer_clear(&smb->retry_timer);
    smb->retry_timer_set = false;

    serr = rpc__smb_socket_accept_async(smb);
    GE(serr);

error:

    SMB_SOCKET_UNLOCK(smb);

    return;
}

INTERNAL
void
rpc__smb_socket_retry_in_seconds(rpc_smb_socket_p_t smb, int seconds)
{
    rpc__timer_set(
        &smb->retry_timer,
        rpc__smb_socket_retry,
        smb,
        RPC_CLOCK_SEC(seconds));
    smb->retry_timer_set = true;
}

INTERNAL
int
rpc__smb_socket_accept_async(
    rpc_smb_socket_p_t smb
    )
{
    int serr = RPC_C_SOCKET_OK;
    LONG64 default_timeout = 0;
    NTSTATUS status = STATUS_SUCCESS;

    smb->io_async.Callback = rpc__smb_socket_create_named_pipe_complete;
    smb->io_async.CallbackContext = smb;

    status = LwNtCreateNamedPipeFile(
        &smb->np,                                /* NP handle */
        &smb->io_async,                          /* Async control */
        &smb->io_status,                         /* IO status block */
        &smb->filename,                          /* Filename */
        NULL,                                    /* Security descriptor */
        NULL,                                    /* Security QOS */
        GENERIC_READ | GENERIC_WRITE,            /* Desired access mode */
        FILE_SHARE_READ | FILE_SHARE_WRITE,      /* Share access mode */
        FILE_CREATE,                             /* Create disposition */
        0,                                       /* Create options */
        0,                                       /* Named pipe type */
        0,                                       /* Read mode */
        0,                                       /* Completion mode */
        smb->accept_backlog.capacity,            /* Maximum instances */
        0,                                       /* Inbound quota */
        0,                                       /* Outbound quota */
        &default_timeout);                       /* Default timeout */
    if (status == STATUS_PENDING)
    {
        status = STATUS_SUCCESS;
    }
    serr = NtStatusToErrno(status);
    GE(serr);

error:

    if (serr)
    {
        rpc__smb_socket_retry_in_seconds(smb, RETRY_INTERVAL);
    }

    return serr;
}

INTERNAL
void
rpc__smb_socket_create_named_pipe_complete(void* data)
{
    int serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) data;
    NTSTATUS status = STATUS_SUCCESS;

    SMB_SOCKET_LOCK(smb);

    LwNtDereferenceAsyncCancelContext(&smb->io_async.AsyncCancelContext);

    serr = NtStatusToErrno(smb->io_status.Status);
    GE(serr);
    
    smb->io_async.Callback = rpc__smb_socket_connect_named_pipe_complete;
    smb->io_async.CallbackContext = smb;

    status = LwIoConnectNamedPipe(
        smb->np,
        &smb->io_async,
        &smb->io_status);

    if (status == STATUS_PENDING)
    {
        status = STATUS_SUCCESS;
    }

    serr = NtStatusToErrno(status);
    GE(serr);

error:

    if (serr)
    {
        rpc__smb_socket_retry_in_seconds(smb, RETRY_INTERVAL);
    }

    SMB_SOCKET_UNLOCK(smb);

    return;
}

INTERNAL
void
rpc__smb_socket_connect_named_pipe_complete(void* data)
{
    int serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) data;
    size_t i = 0;
    char c = 0;

    SMB_SOCKET_LOCK(smb);

    LwNtDereferenceAsyncCancelContext(&smb->io_async.AsyncCancelContext);

    serr = NtStatusToErrno(smb->io_status.Status);
    GE(serr);

    /* Wait for a slot to open in the accept queue */
    while (smb->accept_backlog.length == smb->accept_backlog.capacity)
    {
        if (smb->state == SMB_STATE_ERROR)
        {
            goto error;
        }
        
        rpc__smb_socket_wait(smb);
    }
    
    /* Put the handle into the accept queue */
    for (i = 0; i < smb->accept_backlog.capacity; i++)
    {
        if (smb->accept_backlog.queue[i] == NULL)
        {
            smb->accept_backlog.queue[i] = smb->np;
            smb->np = NULL;
            smb->accept_backlog.length++;
            if (write(smb->accept_backlog.selectfd[1], &c, sizeof(c)) != sizeof(c))
            {
                serr = errno;
                goto error;
            }
            dcethread_cond_broadcast_throw(&smb->event);
            break;
        }
    }
    
    /* Do it all again */
    serr = rpc__smb_socket_accept_async(smb);
    GE(serr);

error:

    if (serr)
    {
        rpc__smb_socket_retry_in_seconds(smb, RETRY_INTERVAL);
    }

    SMB_SOCKET_UNLOCK(smb);

    return;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_listen(
    rpc_socket_t sock,
    int backlog
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    char* endpoint = NULL;
    char* pipename = NULL;
    char* smbpath = NULL;
    unsigned32 dbg_status = 0;

    SMB_SOCKET_LOCK(smb);

    smb->accept_backlog.capacity = backlog;
    smb->accept_backlog.length = 0;
    smb->accept_backlog.queue = calloc(backlog, sizeof(*smb->accept_backlog.queue));

    if (!smb->accept_backlog.queue)
    {
        serr = ENOMEM;
        goto error;
    }

    if (pipe(smb->accept_backlog.selectfd) != 0)
    {
        serr = errno;
        goto error;
    }

    /* Extract endpoint */
    rpc__naf_addr_inq_endpoint ((rpc_addr_p_t) &smb->localaddr,
                                (unsigned_char_t**) &endpoint,
                                &dbg_status);
    
    if (!strncmp(endpoint, "\\pipe\\", sizeof("\\pipe\\") - 1) ||
        !strncmp(endpoint, "\\PIPE\\", sizeof("\\PIPE\\") - 1))
    {
        pipename = endpoint + sizeof("\\pipe\\") - 1;
    }
    else
    {
        serr = EINVAL;
        GE(serr);
    }

    serr = NtStatusToErrno(
        LwRtlCStringAllocatePrintf(
            &smbpath, 
            "\\npfs\\%s",
            (char*) pipename));
    GE(serr);
    
    serr = NtStatusToErrno(
        LwRtlUnicodeStringAllocateFromCString(
            &smb->filename.Name,
            smbpath));
    GE(serr);

    smb->state = SMB_STATE_LISTEN;

    serr = rpc__smb_socket_accept_async(smb);
    GE(serr);

error:

    if (smbpath)
    {
        RtlMemoryFree(smbpath);
    }

    // rpc_string_free handles when *ptr is NULL
    rpc_string_free((unsigned_char_t**) &endpoint, &dbg_status);

    SMB_SOCKET_UNLOCK(smb);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_do_send(
    rpc_socket_t sock
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    DWORD bytes_written = 0;
    unsigned char* cursor = smb->sendbuffer.base;
    IO_STATUS_BLOCK io_status = { 0 };

    do
    {
        serr = NtStatusToErrno(
            NtWriteFile(
                smb->np,                               /* File handle */
                NULL,                                  /* Async control block */
                &io_status,                            /* IO status block */
                smb->sendbuffer.base,                  /* Buffer */
                smb->sendbuffer.start_cursor - cursor, /* Length */
                NULL,                                  /* Byte offset */
                NULL                                   /* Key */
                ));
        if (serr)
        {
            goto error;
        }

        bytes_written = io_status.BytesTransferred;
        cursor += bytes_written;
    } while (cursor < smb->sendbuffer.start_cursor);

    /* Settle the remaining data (which hopefully should be zero if
       the runtime calls us with complete packets) to the start of
       the send buffer */
    rpc__smb_buffer_settle(&smb->sendbuffer);

error:
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_do_recv(
    rpc_socket_t sock,
    size_t* count
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    DWORD bytes_requested = 0;
    DWORD bytes_read = 0;
    IO_STATUS_BLOCK io_status = { 0 };
    NTSTATUS status = STATUS_SUCCESS;

    *count = 0;

    do
    {
        /* FIXME: magic number */
        serr = rpc__smb_buffer_ensure_available(&smb->recvbuffer, 8192);
        if (serr)
        {
            goto error;
        }
        
        bytes_read = 0;
        bytes_requested = rpc__smb_buffer_available(&smb->recvbuffer);

        status = NtReadFile(
            smb->np,                      /* File handle */
            NULL,                         /* Async control block */
            &io_status,                   /* IO status block */
            smb->recvbuffer.end_cursor,   /* Buffer */
            bytes_requested,              /* Length */
            NULL,                         /* Byte offset */
            NULL                          /* Key */
            );
        
        if (status == STATUS_END_OF_FILE)
        {
            serr = 0;
            bytes_read = 0;
        }
        else
        {
            serr = NtStatusToErrno(status);
            bytes_read = io_status.BytesTransferred;
            smb->recvbuffer.end_cursor += bytes_read;
        }

        *count += bytes_read;
    } while (status == STATUS_SUCCESS && bytes_read == bytes_requested);

error:
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_sendmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr ATTRIBUTE_UNUSED,
    int *cc
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    int i;
    boolean last = false;

    SMB_SOCKET_LOCK(smb);

    /* Wait until we are in a state where we can send */
    while (smb->state != SMB_STATE_SEND)
    {
        if (smb->state == SMB_STATE_ERROR)
        {
            serr = -1;
            goto error;
        }
        rpc__smb_socket_wait(smb);
    }

    *cc = 0;

    /* Gather vector into a single buffer */
    for (i = 0; i < iov_len; i++)
    {
        serr = rpc__smb_buffer_append(&smb->sendbuffer, iov[i].iov_base, iov[i].iov_len);

        if (serr)
        {
            goto error;
        }

        *cc += iov[i].iov_len;
    }

    /* Send all fragments */
    while (rpc__smb_buffer_advance_cursor(&smb->sendbuffer, &last))
    {
        serr = rpc__smb_socket_do_send(sock);
        if (serr)
        {
            goto error;
        }

        if (last)
        {
            /* Switch into recv mode */
            rpc__smb_socket_change_state(smb, SMB_STATE_RECV);
            break;
        }
    }

cleanup:

    SMB_SOCKET_UNLOCK(smb);

    return serr;

error:

    rpc__smb_socket_change_state(smb, SMB_STATE_ERROR);

    goto cleanup;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_recvfrom(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    byte_p_t buf ATTRIBUTE_UNUSED,
    int len ATTRIBUTE_UNUSED,
    rpc_addr_p_t from ATTRIBUTE_UNUSED,
    int *cc ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported smb socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_recvmsg(
    rpc_socket_t sock,
    rpc_socket_iovec_p_t iov,
    int iov_len,
    rpc_addr_p_t addr,
    int *cc
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    int i;
    size_t pending;
    size_t count;

    SMB_SOCKET_LOCK(smb);

    while (smb->state != SMB_STATE_RECV)
    {
        if (smb->state == SMB_STATE_ERROR)
        {
            serr = -1;
            goto error;
        }
        rpc__smb_socket_wait(smb);
    }

    *cc = 0;

    if (rpc__smb_buffer_length(&smb->recvbuffer) == 0)
    {
        /* Nothing in buffer, read a complete PDU */
        do
        {
            serr = rpc__smb_socket_do_recv(sock, &count);
            if (serr)
            {
                goto error;
            }
            if (count == 0)
            {
                break;
            }
        } while (!rpc__smb_buffer_advance_cursor(&smb->recvbuffer, &smb->received_last));

        /* Reset cursor back to start to begin disperal into scatter buffer */
        smb->recvbuffer.start_cursor = smb->recvbuffer.base;
    }
    
    for (i = 0; i < iov_len; i++)
    {
        pending = rpc__smb_buffer_pending(&smb->recvbuffer);
        if (iov[i].iov_len < pending)
        {
            memcpy(iov[i].iov_base, smb->recvbuffer.start_cursor, iov[i].iov_len);

            smb->recvbuffer.start_cursor += iov[i].iov_len;
            *cc += iov[i].iov_len;
        }
        else
        {
            memcpy(iov[i].iov_base, smb->recvbuffer.start_cursor, pending);
            
            *cc += pending;

            /* Reset buffer because we have emptied it */
            smb->recvbuffer.start_cursor = smb->recvbuffer.end_cursor = smb->recvbuffer.base;
            /* Switch into send mode if this was the last PDU */
            if (smb->received_last)
            {
                rpc__smb_socket_change_state(smb, SMB_STATE_SEND);
            }
        }
    }

    if (addr)
    {
        memcpy(addr, &smb->peeraddr, sizeof(smb->peeraddr));
    }

cleanup:

    SMB_SOCKET_UNLOCK(smb);

    return serr;

error:

    rpc__smb_socket_change_state(smb, SMB_STATE_ERROR);

    goto cleanup;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_inq_endpoint(
    rpc_socket_t sock,
    rpc_addr_p_t addr
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;

    if (addr->len == 0)
    {
        addr->len = sizeof(addr->sa);
    }

    addr->rpc_protseq_id = smb->localaddr.rpc_protseq_id;
    memcpy(&addr->sa, &smb->localaddr.sa, addr->len);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_broadcast(
    rpc_socket_t sock ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_bufs(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    unsigned32 txsize ATTRIBUTE_UNUSED,
    unsigned32 rxsize ATTRIBUTE_UNUSED,
    unsigned32 *ntxsize ATTRIBUTE_UNUSED,
    unsigned32 *nrxsize ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_nbio(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported smb socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_close_on_exec(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_getpeername(
    rpc_socket_t sock,
    rpc_addr_p_t addr
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;

    SMB_SOCKET_LOCK(smb);
    
    if (!smb->np)
    {
        serr = EINVAL;
        goto error;
    }
    
    memcpy(addr, &smb->peeraddr, sizeof(smb->peeraddr));

error:

    SMB_SOCKET_UNLOCK(smb);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_get_if_id(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_network_if_id_t *network_if_id ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    *network_if_id = SOCK_STREAM;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_keepalive(
    rpc_socket_t sock ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_nowriteblock_wait(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;
    
    fprintf(stderr, "WARNING: unsupported smb socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_set_rcvtimeo(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    struct timeval *tmo ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_getpeereid(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    uid_t *euid ATTRIBUTE_UNUSED,
    gid_t *egid ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported smb socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
int
rpc__smb_socket_get_select_desc(
    rpc_socket_t sock
    )
{
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    return smb->accept_backlog.selectfd[0];
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_enum_ifaces(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_socket_enum_iface_fn_p_t efun ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *rpc_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *netmask_addr_vec ATTRIBUTE_UNUSED,
    rpc_addr_vector_p_t *broadcast_addr_vec ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t serr = ENOTSUP;

    fprintf(stderr, "WARNING: unsupported smb socket function %s\n", __FUNCTION__);

    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_inq_transport_info(
    rpc_socket_t sock,
    rpc_transport_info_handle_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_smb_socket_p_t smb = (rpc_smb_socket_p_t) sock->data.pointer;
    rpc_smb_transport_info_p_t smb_info = NULL;

    smb_info = calloc(1, sizeof(*smb_info));
    
    if (!smb_info)
    {
        serr = ENOMEM;
        goto error;
    }

    if (smb->info.creds)
    {
        serr = NtStatusToErrno(LwIoCopyCreds(smb->info.creds, &smb_info->creds));
        if (serr)
        {
            goto error;
        }
    }

    if (smb->info.access_token)
    {
        smb_info->access_token = smb->info.access_token;
        RtlReferenceAccessToken(smb_info->access_token);
    }

    if (smb->info.session_key.data)
    {
        smb_info->session_key.data = malloc(smb->info.session_key.length);
        if (!smb_info->session_key.data)
        {
            serr = ENOMEM;
            goto error;
        }

        memcpy(smb_info->session_key.data, smb->info.session_key.data, smb->info.session_key.length);
        smb_info->session_key.length = smb->info.session_key.length;
    }

    *info = (rpc_transport_info_handle_t) smb_info;

error:

    if (serr)
    {
        *info = NULL;

        if (smb_info)
        {
            rpc_smb_transport_info_free((rpc_transport_info_handle_t) smb_info);
        }
    }
    
    return serr;
}

INTERNAL
rpc_socket_error_t
rpc__smb_socket_transport_inq_access_token(
    rpc_transport_info_handle_t info,
    rpc_access_token_p_t* token
    )
{
    rpc_smb_transport_info_p_t smb_info = (rpc_smb_transport_info_p_t) info;
    
    RtlReferenceAccessToken(smb_info->access_token);
    *token = smb_info->access_token;

    return 0;
}

rpc_socket_vtbl_t rpc_g_smb_socket_vtbl =
{
    .socket_construct = rpc__smb_socket_construct,
    .socket_destruct = rpc__smb_socket_destruct,
    .socket_bind = rpc__smb_socket_bind,
    .socket_connect = rpc__smb_socket_connect,
    .socket_accept = rpc__smb_socket_accept,
    .socket_listen = rpc__smb_socket_listen,
    .socket_sendmsg = rpc__smb_socket_sendmsg,
    .socket_recvfrom = rpc__smb_socket_recvfrom,
    .socket_recvmsg = rpc__smb_socket_recvmsg,
    .socket_inq_endpoint = rpc__smb_socket_inq_endpoint,
    .socket_set_broadcast = rpc__smb_socket_set_broadcast,
    .socket_set_bufs = rpc__smb_socket_set_bufs,
    .socket_set_nbio = rpc__smb_socket_set_nbio,
    .socket_set_close_on_exec = rpc__smb_socket_set_close_on_exec,
    .socket_getpeername = rpc__smb_socket_getpeername,
    .socket_get_if_id = rpc__smb_socket_get_if_id,
    .socket_set_keepalive = rpc__smb_socket_set_keepalive,
    .socket_nowriteblock_wait = rpc__smb_socket_nowriteblock_wait,
    .socket_set_rcvtimeo = rpc__smb_socket_set_rcvtimeo,
    .socket_getpeereid = rpc__smb_socket_getpeereid,
    .socket_get_select_desc = rpc__smb_socket_get_select_desc,
    .socket_enum_ifaces = rpc__smb_socket_enum_ifaces,
    .socket_inq_transport_info = rpc__smb_socket_inq_transport_info,
    .transport_info_free = rpc_smb_transport_info_free,
    .transport_info_equal = rpc__smb_transport_info_equal,
    .transport_inq_access_token = rpc__smb_socket_transport_inq_access_token
};
