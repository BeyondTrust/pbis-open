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
 *        lwps-error.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lwps-utils.h"
#include "lwps-error_p.h"

static
const char* gLwpsErrorMessages[] = 
{
    // LWPS_ERROR_INVALID_CACHE_PATH                              : 16384
    "An invalid cache path was specified",
    // LWPS_ERROR_INVALID_CONFIG_PATH                             : 16385
    "The path to the configuration file is invalid",
    // LWPS_ERROR_INVALID_PREFIX_PATH                             : 16386
    "The product installation folder could not be determined",
    // LWPS_ERROR_INSUFFICIENT_BUFFER                             : 16387
    "The provided buffer is insufficient",
    // LWPS_ERROR_OUT_OF_MEMORY                                   : 16388
    "Out of memory",
    // LWPS_ERROR_DATA_ERROR                                      : 16389
    "The cached data is incorrect",
    // LWPS_ERROR_NOT_IMPLEMENTED                                 : 16390
    "The requested feature has not been implemented yet",
    // LWPS_ERROR_REGEX_COMPILE_FAILED                            : 16391
    "Failed to compile regular expression",
    // LWPS_ERROR_INTERNAL                                        : 16392
    "Internal error",
    // LWPS_ERROR_UNEXPECTED_DB_RESULT                            : 16393
    "Unexpected cached data found",
    // LWPS_ERROR_INVALID_PARAMETER                               : 16394
    "Invalid parameter",
    // LWPS_ERROR_INVALID_SID_REVISION                            : 16395
    "The security descriptor (SID) has an invalid revision",
    // LWPS_ERROR_LOAD_LIBRARY_FAILED                             : 16396
    "Failed to dynamically load a library",
    // LWPS_ERROR_LOOKUP_SYMBOL_FAILED                            : 16397
    "Failed to lookup a symbol in a dynamic library",
    // LWPS_ERROR_INVALID_CONFIG                                  : 16398
    "The specified configuration (file) is invalid",
    // LWPS_ERROR_UNEXPECTED_TOKEN                                : 16399
    "An unexpected token was encountered in the configuration",
    // LWPS_ERROR_STRING_CONV_FAILED                              : 16400
    "Failed to convert string format (wide/ansi)",
    // LWPS_ERROR_QUERY_CREATION_FAILED                           : 16401
    "Failed to create query to examine cache",
    //  LWPS_ERROR_NOT_SUPPORTED                                  : 16402
    "The request is not supported",
    //  LWPS_ERROR_NO_SUCH_PROVIDER                               : 16403
    "The requested storage provider could not be located",
    //  LWPS_ERROR_INVALID_PROVIDER                               : 16404
    "Encountered an invalid storage provider",
    //  LWPS_ERROR_INVALID_SID                                    : 16405
    "The security identifier is invalid",
    //  LWPS_ERROR_INVALID_ACCOUNT                                : 16406
    "The user or machine account is invalid",
    //  LWPS_ERROR_INVALID_HANDLE                                 : 16407
    "The handle does not represent a currently open database instance",
    //  LWPS_ERROR_DB_RECORD_NOT_FOUND                            : 16408
    "The requested database record was not found",
    //  LWPS_ERROR_INVALID_MESSAGE                                : 16409
    "The Inter Process message is invalid",
    //  LWPS_ERROR_ACCESS_DENIED                                  : 16410
    "Incorrect access attempt"
};

BOOLEAN
LwpsIsLwpsError(
    DWORD dwError
    )
{
    return (LWPS_ERROR_MASK(dwError) != 0);
}

size_t
LwpsGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    
    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }
    
    if (!dwError)
    {
        // No error string for success
        goto cleanup;
    }
        
    if (LWPS_ERROR_MASK(dwError) != 0)
    {
        stResult = LwpsMapLwpsErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = LwpsGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    
cleanup:
    
    return stResult;
}

size_t
LwpsMapLwpsErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gLwpsErrorMessages)/sizeof(PCSTR);
    
    if ((dwError >= LWPS_ERROR_INVALID_CACHE_PATH) &&
        (dwError < LWPS_ERROR_SENTINEL))
    {
        DWORD dwErrorOffset = dwError - 0x4000;
        
        if (dwErrorOffset < dwNMessages)
        {
            PCSTR pszMessage = gLwpsErrorMessages[dwErrorOffset];
            DWORD dwRequiredLen = strlen(pszMessage) + 1;
            
            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }
            
            stResult = dwRequiredLen;
           
            goto cleanup;
        }
    }              
    
    stResult = LwpsGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
LwpsGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = LWPS_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;
    size_t stTempSize = stBufSize;

    int error = LwpsStrError(dwConvertError, pszBuffer, stBufSize);

    while (error)
    {
        if (error == ERANGE)
        {
            // Guess
            stTempSize = stTempSize * 2 + 10;
        }
        else
        {
            stResult = LwpsGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        LWPS_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = LwpsAllocateMemory(
                        stTempSize,
                        (PVOID*)&pszTempBuffer);
        BAIL_ON_LWPS_ERROR(dwError);

        error = LwpsStrError(dwConvertError, pszTempBuffer, stTempSize);
    }

    if (pszTempBuffer != NULL)
    {
        stResult = strlen(pszTempBuffer) + 1;
    }
    else
    {
        stResult = strlen(pszBuffer) + 1;
    }

cleanup:

    LWPS_SAFE_FREE_MEMORY(pszTempBuffer);
    
    return stResult;

error:

    stResult = 0;
    
    goto cleanup;
}

size_t
LwpsGetUnmappedErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    CHAR  szBuf[128] = "";
    DWORD dwRequiredLen = 0;
                        
    dwRequiredLen = sprintf(szBuf, "Error [code=%d] occurred.", dwError) + 1;
                        
    if (stBufSize >= dwRequiredLen) {
        memcpy(pszBuffer, szBuf, dwRequiredLen);
    }
    
    stResult = dwRequiredLen;
   
    return stResult;
}
