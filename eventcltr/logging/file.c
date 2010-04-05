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
 *        file.c
 *
 * Abstract:
 *
 *        Event Collector
 *        File Log Backend
 *
 * Authors: Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

DWORD CltrLogToFile(
    IN PCLTR_LOG pThis,
    IN CltrLogLevel level,
    IN PSTR pszMessage
    )
{
    PCLTR_FILE_LOG pFileThis = (PCLTR_FILE_LOG)pThis;
    PSTR pszEntryType = NULL;
    time_t currentTime;
    struct tm tmp = {0};
    char timeBuf[128];
    FILE* pTarget = NULL;
    DWORD dwError = 0;
    
    switch (level)
    {
        case CLTR_LOG_LEVEL_ALWAYS:
        {
            pszEntryType = CLTR_INFO_TAG;
            pTarget = pFileThis->pInfoFile;
            break;
        }
        case CLTR_LOG_LEVEL_ERROR:
        {
            pszEntryType = CLTR_ERROR_TAG;
            pTarget = pFileThis->pErrorFile;
            break;
        }

        case CLTR_LOG_LEVEL_WARNING:
        {
            pszEntryType = CLTR_WARN_TAG;
            pTarget = pFileThis->pErrorFile;
            break;
        }

        case CLTR_LOG_LEVEL_INFO:
        {
            pszEntryType = CLTR_INFO_TAG;
            pTarget = pFileThis->pInfoFile;
            break;
        }

        case CLTR_LOG_LEVEL_DEBUG:
        {
            pszEntryType = CLTR_DEBUG_TAG;
            pTarget = pFileThis->pInfoFile;
            break;
        }

        case CLTR_LOG_LEVEL_VERBOSE:
        default:
        {
            pszEntryType = CLTR_VERBOSE_TAG;
            pTarget = pFileThis->pInfoFile;
            break;
        }
    }

    currentTime = time(NULL);
#ifdef _WIN32
    localtime_s(&tmp, &currentTime);
#else
    localtime_r(&currentTime, &tmp);
#endif

    strftime(timeBuf, sizeof(timeBuf), CLTR_LOG_TIME_FORMAT, &tmp);

    if (fprintf(pTarget, "%s:%s:%s\n", timeBuf, pszEntryType, pszMessage) < 0)
    {
        dwError = errno;
        // Can't use bail in here since it could cause an infinite loop
        goto error;
    }
    if (fflush(pTarget) < 0)
    {
        dwError = errno;
        // Can't use bail in here since it could cause an infinite loop
        goto error;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD CltrCloseFileLog(
    IN PCLTR_LOG pThis
    )
{
    DWORD dwError = 0;
    PCLTR_FILE_LOG pFileThis = (PCLTR_FILE_LOG)pThis;

    if (pFileThis != NULL)
    {
        if (fclose(pFileThis->pInfoFile) < 0)
        {
            dwError = errno;
        }
        if (fclose(pFileThis->pErrorFile) < 0 && dwError == 0)
        {
            dwError = errno;
        }
        LwRtlMemoryFree(pFileThis);
    }

    return dwError;
}

DWORD
CltrOpenSplitFileLog(
    IN PCSTR       pszInfoPath,
    IN PCSTR       pszErrorPath,
    OUT PCLTR_LOG* ppLog
    )
{
    DWORD dwError = 0;
    FILE *pInfoFile = NULL;
    FILE *pErrorFile = NULL;
    
    BAIL_ON_INVALID_STRING(pszInfoPath);
    BAIL_ON_INVALID_STRING(pszErrorPath);
    
    pInfoFile = fopen(pszInfoPath, "a");
    if (!pInfoFile)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }
    pErrorFile = fopen(pszErrorPath, "a");
    if (!pErrorFile)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = CltrOpenSplitFileLogEx(
                    pInfoFile,
                    pErrorFile,
                    ppLog);
    BAIL_ON_CLTR_ERROR(dwError);
    
cleanup:
    return dwError;

error:
    *ppLog = NULL;
    
    if (pInfoFile)
    {
        fclose(pInfoFile);
    }
    if (pErrorFile)
    {
        fclose(pErrorFile);
    }
    goto cleanup;
}

DWORD
CltrOpenSplitFileLogEx(
    IN FILE *pInfoFile,
    IN FILE *pErrorFile,
    OUT PCLTR_LOG* ppLog
    )
{
    DWORD dwError = 0;
    CLTR_FILE_LOG *pLog = NULL;

    pLog = LwRtlMemoryAllocate(sizeof(*pLog));
    if (pLog == NULL)
    {
        dwError = ENOMEM;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pLog->pInfoFile = pInfoFile;
    pLog->pErrorFile = pErrorFile;
    pLog->generic.pfnWrite = CltrLogToFile;
    pLog->generic.pfnClose = CltrCloseFileLog;
    
    *ppLog = &pLog->generic;

cleanup:
    return dwError;

error:
    LW_RTL_FREE(&pLog);
    goto cleanup;
}

DWORD
CltrOpenSingleFileLogEx(
    IN FILE *pFile,
    OUT PCLTR_LOG* ppLog
    )
{
    DWORD dwError = 0;
    FILE *pErrorFile = NULL;
    int iDuppedFd = -1;

    iDuppedFd = dup(fileno(pFile));
    if (iDuppedFd < 0)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    pErrorFile = fdopen(iDuppedFd, "a");
    if (pErrorFile == NULL)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }
    // pErrorFile now owns the fd
    iDuppedFd = -1;
    
    dwError = CltrOpenSplitFileLogEx(
                    pFile,
                    pErrorFile,
                    ppLog);
    BAIL_ON_CLTR_ERROR(dwError);

cleanup:
    return dwError;

error:
    if (pErrorFile != NULL)
    {
        fclose(pErrorFile);
    }
    if (iDuppedFd != -1)
    {
        close(iDuppedFd);
    }
    *ppLog = NULL;
    goto cleanup;
}

DWORD
CltrOpenFileLog(
    IN PCSTR       pszFilePath,
    OUT PCLTR_LOG* ppLog
    )
{
    DWORD dwError = 0;
    FILE *pFile = NULL;
    
    BAIL_ON_INVALID_STRING(pszFilePath);
    
    pFile = fopen(pszFilePath, "a");
    if (!pFile)
    {
        dwError = errno;
        BAIL_ON_CLTR_ERROR(dwError);
    }

    dwError = CltrOpenSingleFileLogEx(
                    pFile,
                    ppLog);
    BAIL_ON_CLTR_ERROR(dwError);
    
cleanup:
    return dwError;

error:
    *ppLog = NULL;
    
    if (pFile)
    {
        fclose(pFile);
    }
    goto cleanup;
}
