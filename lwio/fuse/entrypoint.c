/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

#include "includes.h"

static
void*
LwIoFuseEntrypointInit(
    struct fuse_conn_info* conn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_FUSE_CONTEXT pFuseContext = LwIoFuseGetContext();

    status = LwIoFuseInit(
        pFuseContext,
        conn
        );
    BAIL_ON_NT_STATUS(status);

    return pFuseContext;

error:

    abort();
}

static
int 
LwIoFuseEntrypointGetattr(
    const char *path,
    struct stat *statbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseGetattr(
        path,
        statbuf);
    BAIL_ON_NT_STATUS(status);
    
error:

    return LwIoFuseMapNtStatus(status);
}

static
int 
LwIoFuseEntrypointStatfs(
    const char *path,
    struct statvfs *statbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseStatfs(
        path,
        statbuf);
    BAIL_ON_NT_STATUS(status);
    
error:

    return LwIoFuseMapNtStatus(status);
}


static
int
LwIoFuseEntrypointReaddir(
    const char *path,
    void *buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info *fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;
 
    status = LwIoFuseReaddir(
        path,
        buf,
        filler,
        offset,
        fi);
    BAIL_ON_NT_STATUS(status);

error:

    switch (status)
    {
    case STATUS_BUFFER_TOO_SMALL:
        /* Special status code to indicate to FUSE
           that the provided buffer was too small */
        return 1;
    default:
        return LwIoFuseMapNtStatus(status);
    }
}

static
int
LwIoFuseEntrypointOpen(
    const char* path,
    struct fuse_file_info* fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;
 
    status = LwIoFuseOpen(
        path,
        fi);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointRelease(
    const char* path,
    struct fuse_file_info* fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;
 
    status = LwIoFuseRelease(
        path,
        fi);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointRead(
    const char* path,
    char* buf,
    size_t len,
    off_t off,
    struct fuse_file_info* fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int bytesRead = 0;

    status = LwIoFuseRead(
        path,
        buf,
        len,
        off,
        fi,
        &bytesRead
        );
    BAIL_ON_NT_STATUS(status);

error:

    if (status)
    {
        return LwIoFuseMapNtStatus(status);
    }
    else
    {
        return bytesRead;
    }
}

static
int
LwIoFuseEntrypointWrite(
    const char* path,
    const char* buf,
    size_t len,
    off_t off,
    struct fuse_file_info* fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int bytesWritten = 0;

    status = LwIoFuseWrite(
        path,
        buf,
        len,
        off,
        fi,
        &bytesWritten
        );
    BAIL_ON_NT_STATUS(status);

error:

    if (status)
    {
        return LwIoFuseMapNtStatus(status);
    }
    else
    {
        return bytesWritten;
    }
}

static
int
LwIoFuseEntrypointTruncate(
    const char* path,
    off_t size
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseTruncate(
        path,
        size);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointMknod(
    const char* path,
    mode_t mode,
    dev_t dev
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseMknod(
        path,
        mode,
        dev);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointCreate(
    const char* path,
    mode_t mode,
    struct fuse_file_info* fi
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseCreate(
        path,
        mode,
        fi);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointUnlink(
    const char* path
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseUnlink(path);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointMkdir(
    const char* path,
    mode_t mode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseMkdir(
        path,
        mode
        );
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointRmdir(
    const char* path
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseRmdir(path);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointRename(
    const char* oldpath,
    const char* newpath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseRename(oldpath, newpath);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointChmod(
    const char* path,
    mode_t mode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseChmod(path, mode);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointChown(
    const char* path,
    uid_t uid,
    gid_t gid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseChown(path, uid, gid);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static
int
LwIoFuseEntrypointUtimens(
    const char* path,
    const struct timespec tv[2]
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LwIoFuseUtimens(path, tv);
    BAIL_ON_NT_STATUS(status);

error:

    return LwIoFuseMapNtStatus(status);
}

static struct fuse_operations gLwIoFuseOperations =
{
    .init = LwIoFuseEntrypointInit,
    .getattr = LwIoFuseEntrypointGetattr,
    .statfs = LwIoFuseEntrypointStatfs,
    .readdir = LwIoFuseEntrypointReaddir,
    .open = LwIoFuseEntrypointOpen,
    .release = LwIoFuseEntrypointRelease,
    .read = LwIoFuseEntrypointRead,
    .write = LwIoFuseEntrypointWrite,
    .truncate = LwIoFuseEntrypointTruncate,
    .mknod = LwIoFuseEntrypointMknod,
    .create = LwIoFuseEntrypointCreate,
    .unlink = LwIoFuseEntrypointUnlink,
    .mkdir = LwIoFuseEntrypointMkdir,
    .rmdir = LwIoFuseEntrypointRmdir,
    .rename = LwIoFuseEntrypointRename,
    .chmod = LwIoFuseEntrypointChmod,
    .chown = LwIoFuseEntrypointChown,
    .utimens = LwIoFuseEntrypointUtimens
};

struct fuse_operations*
LwIoFuseGetOperationsTable(
    void
    )
{
    return &gLwIoFuseOperations;
}
