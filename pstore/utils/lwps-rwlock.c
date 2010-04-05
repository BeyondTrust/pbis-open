/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwps-rwlock.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        Reader Writer Lock API
 *
 *        Implemented using fcntl
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */

#include "lwps-utils.h"
#include "lwps-rwlock_p.h"

DWORD
LwpsCreateRWLock(
    PCSTR pszLockFile,
    PHANDLE phLock
    )
{
    DWORD dwError = 0;
    PLWPS_RWLOCK pLock = NULL;

    BAIL_ON_INVALID_POINTER(phLock);
    BAIL_ON_INVALID_STRING(pszLockFile);

    dwError = LwpsAllocateMemory(
                     sizeof(LWPS_RWLOCK),
                     (PVOID*)&pLock);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateString(
                     pszLockFile,
                     &pLock->pszFilename);
    BAIL_ON_LWPS_ERROR(dwError);

    pLock->fd = open(pLock->pszFilename, O_RDWR | O_CREAT, 0600);
    if (pLock->fd < 0) {
       dwError = errno;
       BAIL_ON_LWPS_ERROR(dwError);
    }

    *phLock = (HANDLE)pLock;

cleanup:

    return dwError;

error:

    if (pLock) {
        LwpsFreeRWLock((HANDLE)pLock);
    }

    if (phLock) {
       *phLock = (HANDLE)NULL;
    }

    goto cleanup;
}
 
DWORD
LwpsAcquireReadLock(
    HANDLE hLock
    )
{
    DWORD dwError = 0;
    PLWPS_RWLOCK pLock = (PLWPS_RWLOCK)hLock;
    struct flock lock = { .l_type = F_RDLCK, 
			  .l_start = SEEK_SET, 
			  .l_len = 0,
			  .l_whence = 0,
			  .l_pid = 0 };

    BAIL_ON_INVALID_HANDLE(hLock);
    
    lock.l_pid = getpid();

    if (fcntl(pLock->fd, F_SETLKW, &lock) < 0)
    {
       dwError = errno;
       BAIL_ON_LWPS_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwpsReleaseReadLock(
    HANDLE hLock
    )
{
    DWORD dwError = 0;
    PLWPS_RWLOCK pLock = (PLWPS_RWLOCK)hLock;
    struct flock lock = { .l_type = F_UNLCK, 
			  .l_start = SEEK_SET, 
			  .l_len = 0,
			  .l_whence = 0,
			  .l_pid = 0 };

    BAIL_ON_INVALID_HANDLE(hLock);
    
    lock.l_pid = getpid();

    if (fcntl(pLock->fd, F_SETLKW, &lock) < 0)
    {
       dwError = errno;
       BAIL_ON_LWPS_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwpsAcquireWriteLock(
    HANDLE hLock
    )
{
    DWORD dwError = 0;
    PLWPS_RWLOCK pLock = (PLWPS_RWLOCK)hLock;
    struct flock lock = { .l_type = F_WRLCK, 
			  .l_start = SEEK_SET, 
			  .l_len = 0,
			  .l_whence = 0,
			  .l_pid = 0 };

    BAIL_ON_INVALID_HANDLE(hLock);
    
    lock.l_pid = getpid();

    if (fcntl(pLock->fd, F_SETLKW, &lock) < 0)
    {
       dwError = errno;
       BAIL_ON_LWPS_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LwpsReleaseWriteLock(
    HANDLE hLock
    )
{
    DWORD dwError = 0;
    PLWPS_RWLOCK pLock = (PLWPS_RWLOCK)hLock;
    struct flock lock = { .l_type = F_UNLCK, 
			  .l_start = SEEK_SET, 
			  .l_len = 0,
			  .l_whence = 0,
			  .l_pid = 0 };

    BAIL_ON_INVALID_HANDLE(hLock);
    
    lock.l_pid = getpid();

    if (fcntl(pLock->fd, F_SETLKW, &lock) < 0)
    {
       dwError = errno;
       BAIL_ON_LWPS_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

VOID
LwpsFreeRWLock(
    HANDLE hLock
    )
{
    PLWPS_RWLOCK pLock = (PLWPS_RWLOCK)hLock;

    if (pLock)
    {
       LWPS_SAFE_FREE_STRING(pLock->pszFilename);
       if (pLock->fd >= 0) {
          close(pLock->fd);
       }
       LwpsFreeMemory(pLock);
    }
}

