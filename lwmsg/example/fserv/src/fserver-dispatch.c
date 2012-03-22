#include "protocol-server.h"
#include "fserver.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)

static
int
fserv_check_permissions(LWMsgSession* session, const char* path, OpenMode mode)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int ret = 0;
    LWMsgSecurityToken* token = NULL;
    uid_t euid;
    gid_t egid;
    struct stat statbuf;

    /* Extract security token */
    token = lwmsg_session_get_peer_security_token(session);
    
    /* Check that session is authenticated and that the token type is correct */
    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        LOG("Unsupported authentication type on session %p\n", session);
        ret = -1;
        goto error;
    }

    /* Extract uid and gid of the caller */
    status = lwmsg_local_token_get_eid(token, &euid, &egid);
    if (status)
    {
        ret = -1;
        goto error;
    }

    if (stat(path, &statbuf) == -1)
    {
        ret = errno;
        goto error;
    }

    if ((mode & OPEN_MODE_READ))
    {
        int can_read =
            ((statbuf.st_uid == euid && statbuf.st_mode & S_IRUSR) ||
             (statbuf.st_gid == egid && statbuf.st_mode & S_IRGRP) ||
             (statbuf.st_mode & S_IROTH));

        if (!can_read)
        {
            LOG("Permission denied for (uid = %i, gid = %i) to read %s\n",
                (int) euid,
                (int) egid,
                path);
            ret = EPERM;
            goto error;
        }
    }

    if ((mode & OPEN_MODE_WRITE))
    {
        int can_write =
            ((statbuf.st_uid == euid && statbuf.st_mode & S_IWUSR) ||
             (statbuf.st_gid == egid && statbuf.st_mode & S_IWGRP) ||
             (statbuf.st_mode & S_IWOTH));

        if (!can_write)
        {
            LOG("Permission denied for (uid = %i, gid = %i) to write %s\n",
                (int) euid,
                (int) egid,
                path);
            ret = EPERM;
            goto error;
        }
    }

    LOG("Permission granted for (uid = %i, gid = %i) to open %s\n",
        (int) euid,
        (int) egid,
        path);

error:
    
    return ret;
}

static
void
fserv_free_handle(
    void* _handle
    )
{
    FileHandle* handle = _handle;

    if (handle->fd >= 0)
    {
        close(handle->fd);
    }

    free(handle);
}

/* Implementation of fserv_open() */
static LWMsgStatus
fserv_open_srv(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    OpenRequest* req = (OpenRequest*) in->data;
    FileHandle* handle = NULL;
    StatusReply* sreply = NULL;
    LWMsgSession* session = lwmsg_call_get_session(call);
    int flags = 0;
    int fd = -1;
    int ret;
    LWMsgHandle* lwmsg_handle = NULL;
    
    LOG("fserv_open() of %s on session %p\n", req->path, session);

    /* Check permissions */
    ret = fserv_check_permissions(session, req->path, req->mode);

    if (ret)
    {
        /* Allocate status reply */
        sreply = malloc(sizeof(*sreply));
        
        /* Bail out on allocation failure */
        if (!handle)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }

        /* Set output parameters */
        sreply->err = ret;   
        out->tag = FSERV_ERROR_RES;
        out->data = (void*) sreply;
    }
    else
    {
        if ((req->mode & OPEN_MODE_READ) && !(req->mode & OPEN_MODE_WRITE))
        {
            flags |= O_RDONLY;
        }
        
        if ((req->mode & OPEN_MODE_WRITE) && !(req->mode & OPEN_MODE_READ))
        {
            flags |= O_WRONLY;
        }
        
        if ((req->mode & OPEN_MODE_WRITE) && (req->mode & OPEN_MODE_READ))
        {
            flags |= O_RDWR;
        }
        
        if ((req->mode & OPEN_MODE_APPEND))
        {
            flags |= O_APPEND;
        }
        
        /* Open file */
        fd = open(req->path, flags);
        
        if (fd >= 0)
        {
            /* Create handle structure */
            handle = malloc(sizeof(*handle));
            
            if (!handle)
            {
                status = LWMSG_STATUS_MEMORY;
                goto error;
            }
            
            /* Fill in handle */
            handle->fd = fd;
            handle->mode = req->mode;
            
            /* Register handle */
            status = lwmsg_session_register_handle(
                session,
                "FileHandle",
                handle,
                fserv_free_handle,
                &lwmsg_handle);
            if (status)
            {
                goto error;
            }
            
            /* Set output parameters */
            out->tag = FSERV_OPEN_RES;
            out->data = lwmsg_handle;
            handle = NULL;

            /* Retain handle */
            lwmsg_session_retain_handle(session, lwmsg_handle);

            LOG("Successfully opened %s as fd %i for session %p\n",
                req->path, fd, session);
        }
        else
        {
            sreply = malloc(sizeof(*sreply));
            
            if (!handle)
            {
                status = LWMSG_STATUS_MEMORY;
                goto error;
            }
            
            sreply->err = errno;
            out->tag = FSERV_ERROR_RES;
            out->data = (void*) sreply;
        }
    }

error:

    if (handle)
    {
        fserv_free_handle(handle);
    }

    return status;
}

static LWMsgStatus
fserv_write_srv(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    WriteRequest* req = (WriteRequest*) in->data;
    StatusReply* sreply = NULL;
    FileHandle* handle = NULL;
    int fd = -1;
    LWMsgSession* session = lwmsg_call_get_session(call);
    
    /* Get our internal handle from the lwmsg handle */
    status = lwmsg_session_get_handle_data(
        session,
        req->handle,
        (void**)(void*) &handle);
    if (status)
    {
        goto error;
    }

    fd = handle->fd;

    LOG("fserv_write() of %lu bytes to fd %i on session %p\n",
        (unsigned long) req->size, fd, session);

    if (write(fd, req->data, req->size) == -1)
    {
        sreply = malloc(sizeof(*sreply));
        
        if (!sreply)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }

        sreply->err = errno;
        out->tag = FSERV_ERROR_RES;
        out->data = sreply;
    }
    else
    {
        out->tag = FSERV_VOID_RES;
        out->data = NULL;
    }

    out->data = (void*) sreply;

error:
    
    return status;
}

static LWMsgStatus
fserv_read_srv(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ReadRequest* req = (ReadRequest*) in->data;
    StatusReply* sreply = NULL;
    ReadReply* rreply = NULL;
    FileHandle* handle = NULL;
    int fd = -1;
    int ret = 0;
    LWMsgSession* session = lwmsg_call_get_session(call);
    
    /* Get our internal handle from the lwmsg handle */
    status = lwmsg_session_get_handle_data(
        session,
        req->handle,
        (void**)(void*) &handle);
    if (status)
    {
        goto error;
    }

    fd = handle->fd;

    LOG("fserv_read() of %lu bytes from fd %i on session %p\n",
        (unsigned long) req->size, fd, session);

    rreply = malloc(sizeof(*rreply));

    if (!rreply)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }

    rreply->data = malloc(req->size);
    
    if (!rreply->data)
    {
        status = LWMSG_STATUS_MEMORY;
        goto error;
    }

    ret = read(fd, rreply->data, req->size);

    if (ret == -1)
    {
        int err = errno;

        free(rreply->data);
        free(rreply);

        sreply = malloc(sizeof(*sreply));

        if (!sreply)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }

        sreply->err = err;
        out->tag = FSERV_ERROR_RES;
        out->data = (void*) sreply;
    }
    else if (ret == 0)
    {
        free(rreply->data);
        rreply->data = NULL;
        rreply->size = 0;
        out->tag = FSERV_READ_RES;
        out->data = (void*) rreply;
    }
    else
    {
        rreply->size = ret;
        out->tag = FSERV_READ_RES;
        out->data = (void*) rreply;
    }

error:
    
    return status;
}

static LWMsgStatus
fserv_close_srv(
    LWMsgCall* call,
    const LWMsgParams* in,
    LWMsgParams* out,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSession* session = lwmsg_call_get_session(call);
    FileHandle* handle = NULL;
    StatusReply* sreply = NULL;

    LOG("fserv_close() on fd %i for session %p\n", handle->fd, session);

    /* Get our internal handle from the lwmsg handle */
    status = lwmsg_session_get_handle_data(
        session,
        (LWMsgHandle*) in->data,
        (void**)(void*) &handle);
    if (status)
    {
        goto error;
    }

    /* Unregister the handle no matter what */
    status = lwmsg_session_unregister_handle(session, (LWMsgHandle*) in->data);
    if (status)
    {
        goto error;
    }
    
    if (close(handle->fd) == -1)
    {
        sreply = malloc(sizeof(*sreply));
        
        if (!sreply)
        {
            status = LWMSG_STATUS_MEMORY;
            goto error;
        }

        sreply->err = errno;
        out->tag = FSERV_ERROR_RES;
        out->data = sreply;
    }
    else
    {
        out->tag = FSERV_VOID_RES;
        out->data = NULL;
    }

error:

    return status;
}

/* Dispatch spec */
static LWMsgDispatchSpec fserv_dispatch[] =
{
    LWMSG_DISPATCH_BLOCK(FSERV_OPEN_REQ, fserv_open_srv),
    LWMSG_DISPATCH_BLOCK(FSERV_WRITE_REQ, fserv_write_srv),
    LWMSG_DISPATCH_BLOCK(FSERV_READ_REQ, fserv_read_srv),
    LWMSG_DISPATCH_BLOCK(FSERV_CLOSE_REQ, fserv_close_srv),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
fserver_get_dispatch(void)
{
    return fserv_dispatch;
}
