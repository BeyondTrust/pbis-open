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
 *        nfsutils.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Utility Functions
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __NFS_UTILS_H__
#define __NFS_UTILS_H__

#define NFS_SAFE_FREE_MEMORY(pMemory) \
    if (pMemory) { NfsFreeMemory(pMemory); }

#define NFS_SAFE_FREE_MEMORY_AND_RESET(pMemory) \
    if (pMemory) { NfsFreeMemory(pMemory); (pMemory) = NULL; }

typedef VOID (*PFN_PROD_CONS_QUEUE_FREE_ITEM)(PVOID pItem);

typedef struct _SMB_PROD_CONS_QUEUE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    LWIO_QUEUE       queue;

    ULONG           ulNumMaxItems;
    ULONG           ulNumItems;

    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem;

    pthread_cond_t  event;
    pthread_cond_t* pEvent;

} SMB_PROD_CONS_QUEUE, *PSMB_PROD_CONS_QUEUE;


typedef struct _NFS_HOST_INFO
{
    LONG  refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PSTR  pszHostname;
    PSTR  pszDomain;

    BOOLEAN bIsJoined;

} NFS_HOST_INFO, *PNFS_HOST_INFO;

NTSTATUS
NfsAllocateMemory(
    IN  size_t size,
    OUT PVOID* ppMemory
    );

NTSTATUS
NfsReallocMemory(
    IN  PVOID  pMemory,
    IN  size_t size,
    OUT PVOID* ppNewMemory
    );

VOID
NfsFreeMemory(
    IN PVOID pMemory
    );

NTSTATUS
NfsAcquireHostInfo(
    PNFS_HOST_INFO  pOrigHostInfo,
    PNFS_HOST_INFO* ppNewHostInfo
    );

VOID
NfsReleaseHostInfo(
    PNFS_HOST_INFO pHostinfo
    );

NTSTATUS
NfsBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    );

NTSTATUS
NfsGetParentPath(
    PWSTR  pwszPath,
    PWSTR* ppwszParentPath
    );

NTSTATUS
NfsMatchPathPrefix(
    PWSTR pwszPath,
    ULONG ulPathLength,
    PWSTR pwszPrefix
    );

NTSTATUS
NfsProdConsInit(
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PSMB_PROD_CONS_QUEUE*         ppQueue
    );

NTSTATUS
NfsProdConsInitContents(
    PSMB_PROD_CONS_QUEUE          pQueue,
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
    );

NTSTATUS
NfsProdConsEnqueue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    );

NTSTATUS
NfsProdConsDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    );

NTSTATUS
NfsProdConsTimedDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    struct timespec*     pTimespec,
    PVOID*               ppItem
    );

VOID
NfsProdConsFree(
    PSMB_PROD_CONS_QUEUE pQueue
    );
NTSTATUS
NfsMbsToWc16s(
    IN  PCSTR  pszString,
    OUT PWSTR* ppwszString
    );

NTSTATUS
NfsWc16sToMbs(
    IN  PCWSTR pwszString,
    OUT PSTR*  ppszString
    );

NTSTATUS
NfsAllocateStringW(
    IN  PWSTR  pwszInputString,
    OUT PWSTR* ppwszOutputString
    );

NTSTATUS
NfsAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

VOID
NfsProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

#endif /* __NFS_UTILS_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
