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
 *        Reaper for syslog
 *
 *        Log Info Settings (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
RSysSrvSetLogByConfig(
    )
{
    DWORD dwError = 0;
    PRSYS_LOG pLog = NULL;

    switch (gpAPIConfig->pLogInfo->logTarget)
    {
        case RSYS_LOG_TARGET_DISABLED:
            break;

        case RSYS_LOG_TARGET_CONSOLE:
            dwError = RSysOpenSplitFileLog(
                            "/dev/stdout",
                            "/dev/stderr",
                            &pLog);
            BAIL_ON_RSYS_ERROR(dwError);
            break;

        case RSYS_LOG_TARGET_FILE:
            dwError = RSysOpenFileLog(
                            gpAPIConfig->pLogInfo->pszPath,
                            &pLog);
            BAIL_ON_RSYS_ERROR(dwError);
            break;

        case RSYS_LOG_TARGET_SYSLOG:
            dwError = RSysOpenSyslog(
                            "reapsysl",
                            0,
                            LOG_DAEMON,
                            &pLog);
            BAIL_ON_RSYS_ERROR(dwError);
            break;
    }
    dwError = RSysSetGlobalLog(
                    pLog,
                    gpAPIConfig->pLogInfo->maxAllowedLogLevel);
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    return dwError;

error:
    if (pLog)
    {
        RSysCloseLog(pLog);
    }

    goto cleanup;
}

DWORD
RSysSrvGetLogInfo(
    HANDLE hServer,
    PRSYS_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PRSYS_LOG_INFO pLogInfo = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;

    BAIL_ON_INVALID_POINTER(ppLogInfo);

    pthread_rwlock_rdlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = RSysSrvDupLogInfo(
                    &pLogInfo,
                    gpAPIConfig->pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);

    *ppLogInfo = pLogInfo;

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }

    return dwError;

error:

    *ppLogInfo = NULL;

    if (pLogInfo)
    {
        RSysFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}

DWORD
RSysSrvDupLogInfo(
    PRSYS_LOG_INFO* ppDest,
    PRSYS_LOG_INFO pSrc
    )
{
    DWORD dwError = 0;
    PRSYS_LOG_INFO pNewLogInfo = NULL;

    dwError = RTL_ALLOCATE(
                    &pNewLogInfo,
                    RSYS_LOG_INFO,
                    sizeof(*pNewLogInfo));
    BAIL_ON_RSYS_ERROR(dwError);

    pNewLogInfo->maxAllowedLogLevel = pSrc->maxAllowedLogLevel;
    pNewLogInfo->logTarget = pSrc->logTarget;
    if (pSrc->pszPath)
    {
        dwError = RtlCStringDuplicate(
                        &pNewLogInfo->pszPath,
                        pSrc->pszPath);
        BAIL_ON_RSYS_ERROR(dwError);
    }

    *ppDest = pNewLogInfo;

cleanup:
    return dwError;

error:
    *ppDest = NULL;
    if (pNewLogInfo != NULL)
    {
        RSysFreeLogInfo(pNewLogInfo);
    }

    goto cleanup;
}

DWORD
RSysSrvSetLogInfo(
    HANDLE hServer,
    PRSYS_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    PRSYS_SRV_API_STATE pServerState = (PRSYS_SRV_API_STATE)hServer;
    BOOLEAN bUnlockConfigLock = FALSE;
    PRSYS_LOG_INFO pNewLogInfo = NULL;

    BAIL_ON_INVALID_POINTER(pLogInfo);

    if (pServerState && pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_RSYS_ERROR(dwError);
    }

    pthread_rwlock_wrlock(&gRSysConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = RSysSrvDupLogInfo(
                    &pNewLogInfo,
                    pLogInfo);
    BAIL_ON_RSYS_ERROR(dwError);

    RSysFreeLogInfo(gpAPIConfig->pLogInfo);
    gpAPIConfig->pLogInfo = pNewLogInfo;
    pNewLogInfo = NULL;

    dwError = RSysSrvSetLogByConfig();
    BAIL_ON_RSYS_ERROR(dwError);

cleanup:
    if (bUnlockConfigLock)
    {
        pthread_rwlock_unlock(&gRSysConfigLock);
    }
    if (pNewLogInfo != NULL)
    {
        RSysFreeLogInfo(pNewLogInfo);
    }

    return dwError;

error:

    goto cleanup;
}
