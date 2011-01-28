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
 *        main.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "registryd.h"
#include <lw/svcm.h>

static
DWORD
RegSvcmParseArgs(
    ULONG ArgCount,
    PWSTR* ppArgs,
    PREGSERVERINFO pRegServerInfo
    );

NTSTATUS
RegSvcmInit(
    PCWSTR pServiceName,
    PLW_SVCM_INSTANCE pInstance
    )
{
    return STATUS_SUCCESS;
}

VOID
RegSvcmDestroy(
    PLW_SVCM_INSTANCE pInstance
    )
{
    return;
}


NTSTATUS
RegSvcmStart(
    PLW_SVCM_INSTANCE pInstance,
    ULONG ArgCount,
    PWSTR* ppArgs,
    ULONG FdCount,
    int* pFds
    )
{
    DWORD dwError = 0;

    dwError = RegSrvSetDefaults();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSvcmParseArgs(
        ArgCount,
        ppArgs,
        &gServerInfo);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvInitialize();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvStartListenThread();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return LwWin32ErrorToNtStatus(dwError);

error:

    REG_LOG_ERROR("REG Process exiting due to error [Code:%d]", dwError);

    goto cleanup;
}

NTSTATUS
RegSvcmStop(
    PLW_SVCM_INSTANCE pInstance
    )
{
    REG_LOG_VERBOSE("Reg main cleaning up");

    RegSrvStopListenThread();

    RegSrvApiShutdown();

    REG_LOG_INFO("REG Service exiting...");

    return STATUS_SUCCESS;
}

static LW_SVCM_MODULE gService =
{
    .Size = sizeof(gService),
    .Init = RegSvcmInit,
    .Destroy = RegSvcmDestroy,
    .Start = RegSvcmStart,
    .Stop = RegSvcmStop
};

PLW_SVCM_MODULE
(LW_RTL_SVCM_ENTRY_POINT_NAME)(
    VOID
    )
{
    return &gService;
}

DWORD
RegSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strncpy(gpServerInfo->szCachePath,
            CACHEDIR,
            sizeof(gpServerInfo->szCachePath)-1);

    strncpy(gpServerInfo->szPrefixPath,
            PREFIXDIR,
            sizeof(gpServerInfo->szPrefixPath)-1);

    setlocale(LC_ALL, "");

    return (dwError);
}

DWORD
RegSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = RegInitCacheFolders();
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvApiInit();
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
RegInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = RegSrvGetCachePath(&pszCachePath);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCheckDirectoryExists(
        pszCachePath,
        &bExists);

    BAIL_ON_REG_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = RegCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    LWREG_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

DWORD
RegSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szCachePath))
    {
        dwError = LWREG_ERROR_INVALID_CACHE_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpServerInfo->szCachePath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
RegSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    REG_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath))
    {
        dwError = LWREG_ERROR_INVALID_PREFIX_PATH;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwRtlCStringDuplicate(&pszPath, gpServerInfo->szPrefixPath);
    BAIL_ON_REG_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    REG_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LWREG_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

static
DWORD
RegSvcmParseArgs(
    ULONG ArgCount,
    PWSTR* ppArgs,
    PREGSERVERINFO pRegServerInfo
    )
{
    typedef enum
    {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;
    static const WCHAR paramLogFile[] = {'-','-','l','o','g','f','i','l','e','\0'};
    static const WCHAR paramLogLevel[] = {'-','-','l','o','g','l','e','v','e','l','\0'};
    static const WCHAR paramSyslog[] = {'-','-','s','y','s','l','o','g','\0'};
    static const WCHAR paramError[] = {'e','r','r','o','r','\0'};
    static const WCHAR paramWarning[] = {'w','a','r','n','i','n','g','\0'};
    static const WCHAR paramInfo[] = {'i','n','f','o','\0'};
    static const WCHAR paramVerbose[] = {'v','e','r','b','o','s','e','\0'};
    static const WCHAR paramDebug[] = {'d','e','b','u','g','\0'};
    static const WCHAR paramTrace[] = {'t','r','a','c','e','\0'};
    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 0;
    PWSTR pArg = NULL;
    BOOLEAN bLogTargetSet = FALSE;
    PSTR pConverted = NULL;

    do
    {
        pArg = ppArgs[iArg++];
        if (pArg == NULL || *pArg == '\0')
        {
            break;
        }

        switch(parseMode)
        {

        case PARSE_MODE_OPEN:
            {
                if (LwRtlWC16StringIsEqual(pArg, paramLogFile, TRUE))
                {
                    parseMode = PARSE_MODE_LOGFILE;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramLogLevel, TRUE))
                {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramSyslog, TRUE))
                {
                    gServerInfo.logTarget = REG_LOG_TARGET_SYSLOG;
                    bLogTargetSet = TRUE;
                }
                else
                {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_REG_ERROR(dwError);
                }
                break;
            }

        case PARSE_MODE_LOGFILE:
            {
                dwError = LwNtStatusToWin32Error(
                    LwRtlCStringAllocateFromWC16String(&pConverted, pArg));
                BAIL_ON_REG_ERROR(dwError);

                strncpy(pRegServerInfo->szLogFilePath, pConverted, sizeof(pRegServerInfo->szLogFilePath) - 1);

                RegStripWhitespace(pRegServerInfo->szLogFilePath, TRUE, TRUE);

                if (!strcmp(pRegServerInfo->szLogFilePath, "."))
                {
                    pRegServerInfo->logTarget = REG_LOG_TARGET_CONSOLE;
                }
                else
                {
                    pRegServerInfo->logTarget = REG_LOG_TARGET_FILE;
                }

                bLogTargetSet = TRUE;

                parseMode = PARSE_MODE_OPEN;

                break;
            }

        case PARSE_MODE_LOGLEVEL:
            {
                if (LwRtlWC16StringIsEqual(pArg, paramError, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_ERROR;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramWarning, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_WARNING;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramInfo, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_INFO;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramVerbose, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_VERBOSE;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramDebug, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_DEBUG;
                }
                else if (LwRtlWC16StringIsEqual(pArg, paramTrace, TRUE))
                {
                    pRegServerInfo->maxAllowedLogLevel = REG_LOG_LEVEL_TRACE;
                }
                else
                {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_REG_ERROR(dwError);
                }

                parseMode = PARSE_MODE_OPEN;
                break;
            }
        }
    } while (iArg < ArgCount);

    if (pRegServerInfo->dwStartAsDaemon)
    {
        if (pRegServerInfo->logTarget == REG_LOG_TARGET_CONSOLE)
        {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }
    }
    else
    {
        if (pRegServerInfo->logTarget != REG_LOG_TARGET_FILE &&
            pRegServerInfo->logTarget != REG_LOG_TARGET_SYSLOG)
        {
            pRegServerInfo->logTarget = REG_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}
