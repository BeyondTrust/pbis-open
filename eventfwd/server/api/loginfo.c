/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        loginfo.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
 *
 *        Log Info Settings (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
EfdSrvSetLogByConfig(
    )
{
    DWORD dwError = 0;
    PEFD_LOG pLog = NULL;

    switch (gpAPIConfig->pLogInfo->logTarget)
    {
        case EFD_LOG_TARGET_DISABLED:
            break;

        case EFD_LOG_TARGET_CONSOLE:
            dwError = EfdOpenSplitFileLog(
                            "/dev/stdout",
                            "/dev/stderr",
                            &pLog);
            BAIL_ON_EFD_ERROR(dwError);
            break;

        case EFD_LOG_TARGET_FILE:
            dwError = EfdOpenFileLog(
                            gpAPIConfig->pLogInfo->pszPath,
                            &pLog);
            BAIL_ON_EFD_ERROR(dwError);
            break;

        case EFD_LOG_TARGET_SYSLOG:
            dwError = EfdOpenSyslog(
                            "evtfwd",
                            0,
                            LOG_DAEMON,
                            &pLog);
            BAIL_ON_EFD_ERROR(dwError);
            break;
    }
    dwError = EfdSetGlobalLog(
                    pLog,
                    gpAPIConfig->pLogInfo->maxAllowedLogLevel);
    BAIL_ON_EFD_ERROR(dwError);

cleanup:
    return dwError;

error:
    if (pLog)
    {
        EfdCloseLog(pLog);
    }

    goto cleanup;
}

DWORD
EfdSrvGetLogInfo(
    HANDLE hServer,
    PEFD_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PEFD_LOG_INFO pLogInfo = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppLogInfo);

    pthread_rwlock_rdlock(&gEfdConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = EfdSrvDupLogInfo(
                    &pLogInfo,
                    gpAPIConfig->pLogInfo);
    BAIL_ON_EFD_ERROR(dwError);

    *ppLogInfo = pLogInfo;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gEfdConfigLock);
    }

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        EfdFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
EfdSrvDupLogInfo(
    PEFD_LOG_INFO* ppDest,
    PEFD_LOG_INFO pSrc
    )
{
    DWORD dwError = 0;
    PEFD_LOG_INFO pNewLogInfo = NULL;

    dwError = RTL_ALLOCATE(
                    &pNewLogInfo,
                    EFD_LOG_INFO,
                    sizeof(*pNewLogInfo));
    BAIL_ON_EFD_ERROR(dwError);

    pNewLogInfo->maxAllowedLogLevel = pSrc->maxAllowedLogLevel;
    pNewLogInfo->logTarget = pSrc->logTarget;
    if (pSrc->pszPath)
    {
        dwError = RtlCStringDuplicate(
                        &pNewLogInfo->pszPath,
                        pSrc->pszPath);
        BAIL_ON_EFD_ERROR(dwError);
    }

    *ppDest = pNewLogInfo;

cleanup:
    return dwError;

error:
    *ppDest = NULL;
    if (pNewLogInfo != NULL)
    {
        EfdFreeLogInfo(pNewLogInfo);
    }

    goto cleanup;
}

DWORD
EfdSrvSetLogInfo(
    HANDLE hServer,
    PEFD_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    PEFD_SRV_API_STATE pServerState = (PEFD_SRV_API_STATE)hServer;
    BOOLEAN bUnlockConfigLock = FALSE;
    PEFD_LOG_INFO pNewLogInfo = NULL;

    BAIL_ON_INVALID_POINTER(pLogInfo);

    if (pServerState && pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_EFD_ERROR(dwError);
    }

    pthread_rwlock_wrlock(&gEfdConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = EfdSrvDupLogInfo(
                    &pNewLogInfo,
                    pLogInfo);
    BAIL_ON_EFD_ERROR(dwError);

    EfdFreeLogInfo(gpAPIConfig->pLogInfo);
    gpAPIConfig->pLogInfo = pNewLogInfo;
    pNewLogInfo = NULL;

    dwError = EfdSrvSetLogByConfig();
    BAIL_ON_EFD_ERROR(dwError);

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gEfdConfigLock);
    }
    if (pNewLogInfo != NULL)
    {
        EfdFreeLogInfo(pNewLogInfo);
    }

    return dwError;

error:

    goto cleanup;
}
