/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwldap.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        Mac Directory Service Cache Exception API
 *
 * Authors: Glenn Curtis (glennc@likewise.com)
 */
#include "includes.h"

#ifdef HAVE_DIRECTORYSERVICE_DIRSERVICES_H
#include <DirectoryService/DirServices.h>
#include <DirectoryService/DirServicesUtils.h>
#include <Security/Security.h>
#endif

#ifdef HAVE_DSOPENDIRSERVICE
typedef struct _dsCacheExceptionRqst {
    AuthorizationExternalForm auth;
    pid_t pid;
} DsCacheExceptionRqst;

// Local helper function to do the request
DWORD LwDsSendCustomCall(int command, pid_t pid);
#endif

DWORD
LwDsCacheAddPidException(
    IN pid_t pid
    )
{
#ifdef HAVE_DSOPENDIRSERVICE
    return LwDsSendCustomCall(10000, pid);
#else
    return LW_ERROR_SUCCESS;
#endif
}

DWORD
LwDsCacheRemovePidException(
    IN pid_t pid
    )
{
#ifdef HAVE_DSOPENDIRSERVICE
    return LwDsSendCustomCall(10001, pid);
#else
    return LW_ERROR_SUCCESS;
#endif
}

#ifdef HAVE_DSOPENDIRSERVICE
DWORD
LwDsSendCustomCall(
    int command,
    pid_t pid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
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
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        goto errorexit;
    }

    pPidRequest = dsDataBufferAllocate(hDirRef, sizeof(DsCacheExceptionRqst));
    if (pPidRequest == NULL)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        goto errorexit;
    }

    pPidResponse = dsDataBufferAllocate(hDirRef, sizeof(int32_t));
    if (pPidResponse == NULL)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        goto errorexit;
    }

    pRequest = (DsCacheExceptionRqst*)pPidRequest->fBufferData;

    memset(&pRequest->auth, 0, sizeof(pRequest->auth));
    pRequest->pid = pid;
    pPidRequest->fBufferLength = sizeof(DsCacheExceptionRqst);

    dirNodeName = dsBuildFromPath(hDirRef, nodeName, "/");
    if (dirNodeName == NULL)
    {
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
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
        dwError = LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
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

    return dwError;
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
