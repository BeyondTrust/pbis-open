/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        dscache.c
 *
 * Abstract:
 *
 *        Likewise Base Library (LwBase)
 *
 *        Directory Service Cache Exception API
 *        For services that can call into lookup functions like gethostbtname, gethostbyaddr, getpwnam
 *        on Macintosh systems running 10.5+, the DirectoryService can cause complications when lookups
 *        re-enter the DirectoryService /Search plugin. There is a mutex used to serialize access to the
 *        /Search plugin, which can block operations that require the above functions to complete a query
 *        about an object such as a user or group (which needs to fetch data from a remote host). A mechanism
 *        to bypass the /Search plugin is possible by having a given process notify the /Cache plugin (accessed
 *        prior to the /Search plugin) of it's process pid. The /Cache plugin checks the list of exception pids
 *        for the given query to process, and will call directly to the /Local or/or /BSD plugins, thus skipping
 *        past the /Search plugin and avoiding the mutex deadlock potential.
 *
 *        These are helper functions used by daemon startup/shutdown operations to add or remove the process pid
 *        of the daemon to the /Cache DS plugin. The notificatin is done by calling Mac DirectoryService
 *        dsDoPluginCustomCall() with the operation code 10000 (to add) or 100001 (to remove) a pid exception.
 *
 * Authors: Glenn Curtis (glennc@likewise.com)
 *
 */
#include "includes.h"

#if defined(__LWI_DARWIN__)
#include <DirectoryService/DirServices.h>
#include <DirectoryService/DirServicesUtils.h>
#include <Security/Security.h>

typedef struct _dsCacheExceptionRqst {
    AuthorizationExternalForm auth;
    int32_t pid;
} DsCacheExceptionRqst;

// Local helper function to do the request
LW_NTSTATUS SendDSCustomCall(int command, int32_t pid);
#endif

LW_NTSTATUS
LwAddPidExceptionToDSCache(
    int32_t pid
    )
{
#if defined(__LWI_DARWIN__)
    return SendDSCustomCall(10000, pid);
#else
    return LW_STATUS_SUCCESS;
#endif
}

LW_NTSTATUS
LwRemovePidExceptionFromDSCache(
    int32_t pid
    )
{
#if defined(__LWI_DARWIN__)
    return SendDSCustomCall(10001, pid);
#else
    return LW_STATUS_SUCCESS;
#endif
}

#if defined(__LWI_DARWIN__)
LW_NTSTATUS
SendDSCustomCall(
    int command,
    int32_t pid
    )
{
    LW_NTSTATUS ntstatus = LW_STATUS_SUCCESS;
    tDirReference hDirRef = 0;
    tDirNodeReference hNodeRef = 0;
    tDirStatus status = eDSNoErr;
    const char nodeName[] = "/Cache";
    tDataListPtr dirNodeName = NULL;
    tDataBufferPtr pPidRequest = NULL;
    tDataBufferPtr pPidResponse = NULL;
    DsCacheExceptionRqst * pRequest = NULL;

    status = dsOpenDirService(&hDirRef);
    if(status != eDSNoErr)
    {
        ntstatus = LW_STATUS_UNSUCCESSFUL;
        goto errorexit;
    }

    pPidRequest = dsDataBufferAllocate(hDirRef, sizeof(DsCacheExceptionRqst));
    if (pPidRequest == NULL)
    {
        ntstatus = LW_STATUS_UNSUCCESSFUL;
        goto errorexit;
    }

    pPidResponse = dsDataBufferAllocate(hDirRef, sizeof(int32_t));
    if (pPidResponse == NULL)
    {
        ntstatus = LW_STATUS_UNSUCCESSFUL;
        goto errorexit;
    }

    pRequest = (DsCacheExceptionRqst*)pPidRequest->fBufferData;

    memset(&pRequest->auth, 0, sizeof(pRequest->auth));
    pRequest->pid = pid;
    pPidRequest->fBufferLength = sizeof(DsCacheExceptionRqst);

    dirNodeName = dsBuildFromPath(hDirRef, nodeName, "/");
    if (dirNodeName == NULL)
    {
        ntstatus = LW_STATUS_UNSUCCESSFUL;
        goto errorexit;
    }

    status = dsOpenDirNode(hDirRef, dirNodeName, &hNodeRef);
    if (status != eDSNoErr)
    {
        // This can happen on older Tiger Mac OS X 10.4 systems, as there is no /Cache plugin.
        // It is okay to return successful in this scenario.
        goto errorexit;
    }

    status = dsDoPlugInCustomCall(hNodeRef, command, pPidRequest, pPidResponse);
    if (status != eDSNoErr)
    {
        ntstatus = LW_STATUS_UNSUCCESSFUL;
        goto errorexit;
    }

errorexit:

    if (dirNodeName)
        dsDataListDeallocate(hDirRef, dirNodeName);

    if (hNodeRef)
        dsCloseDirNode(hNodeRef);

    if (pPidRequest)
        dsDataBufferDeAllocate(hDirRef, pPidRequest);

    if (pPidResponse)
        dsDataBufferDeAllocate(hDirRef, pPidResponse);

    if (hDirRef)
        dsCloseDirService(hDirRef);

    return ntstatus;
}
#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
