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
 *        rsys-error.c
 *
 * Abstract:
 *
 *        Reaper for syslog
 * 
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen <kstemen@likewise.com>
 */

#include "includes.h"

typedef struct _ERROR_MESSAGE
{
    DWORD dwCode;
    PCSTR pszMessage;
} ERROR_MESSAGE, *PERROR_MESSAGE;

static
const ERROR_MESSAGE gRSysErrorMessages[] = 
{
    {
        RSYS_ERROR_INVALID_CACHE_PATH,
        "An invalid cache path was specified"
    },
    {
        RSYS_ERROR_INVALID_CONFIG_PATH,
        "The path to the configuration file is invalid"
    },
    {
        RSYS_ERROR_INVALID_PREFIX_PATH,
        "The product installation folder could not be determined"
    },
    {
        RSYS_ERROR_INSUFFICIENT_BUFFER,
        "The provided buffer is insufficient"
    },
    {
        RSYS_ERROR_OUT_OF_MEMORY,
        "Out of memory"
    },
    {
        RSYS_ERROR_INVALID_MESSAGE,
        "The Inter Process message is invalid"
    },
    {
        RSYS_ERROR_UNEXPECTED_MESSAGE,
        "An unexpected Inter Process message was received"
    },
    {
        RSYS_ERROR_DATA_ERROR,
        "The cached data is incorrect"
    },
    {
        RSYS_ERROR_NOT_IMPLEMENTED,
        "The requested feature has not been implemented yet"
    },
    {
        RSYS_ERROR_NO_CONTEXT_ITEM,
        "The requested item was not found in context"
    },
    {
        RSYS_ERROR_REGEX_COMPILE_FAILED,
        "Failed to compile regular expression"
    },
    {
        RSYS_ERROR_INTERNAL,
        "Internal error"
    },
    {
        RSYS_ERROR_INVALID_DNS_RESPONSE,
        "Response from DNS is invalid"
    },
    {
        RSYS_ERROR_DNS_RESOLUTION_FAILED,
        "Failed to resolve query using DNS"
    },
    {
        RSYS_ERROR_FAILED_TIME_CONVERSION,
        "Failed to convert the time"
    },
    {
        RSYS_ERROR_INVALID_SID,
        "The security descriptor (SID) is invalid"
    },
    {
        RSYS_ERROR_UNEXPECTED_DB_RESULT,
        "Unexpected cached data found"
    },
    {
        RSYS_ERROR_INVALID_RSYS_CONNECTION,
        "The connection to the authentication service is invalid"
    },
    {
        RSYS_ERROR_INVALID_PARAMETER,
        "Invalid parameter"
    },
    {
        RSYS_ERROR_LDAP_NO_PARENT_DN,
        "No distinguished name found in LDAP for parent of this object"
    },
    {
        RSYS_ERROR_LDAP_ERROR,
        "An Error was encountered when negotiating with LDAP"
    },
    {
        RSYS_ERROR_NO_SUCH_DOMAIN,
        "Unknown Active Directory domain"
    },
    {
        RSYS_ERROR_LDAP_FAILED_GETDN,
        "Failed to find distinguished name using LDAP"
    },
    {
        RSYS_ERROR_DUPLICATE_DOMAINNAME,
        "A duplicate Active Directory domain name was found"
    },
    {
        RSYS_ERROR_FAILED_FIND_DC,
        "Failed to find the domain controller"
    },
    {
        RSYS_ERROR_LDAP_GET_DN_FAILED,
        "Failed to find distinguished name using LDAP"
    },
    {
        RSYS_ERROR_INVALID_SID_REVISION,
        "The security descriptor (SID) has an invalid revision"
    },
    {
        RSYS_ERROR_LOAD_LIBRARY_FAILED,
        "Failed to dynamically load a library"
    },
    {
        RSYS_ERROR_LOOKUP_SYMBOL_FAILED,
        "Failed to lookup a symbol in a dynamic library"
    },
    {
        RSYS_ERROR_INVALID_EVENTLOG,
        "The Eventlog interface is invalid"
    },
    {
        RSYS_ERROR_INVALID_CONFIG,
        "The specified configuration (file) is invalid"
    },
    {
        RSYS_ERROR_UNEXPECTED_TOKEN,
        "An unexpected token was encountered in the configuration"
    },
    {
        RSYS_ERROR_LDAP_NO_RECORDS_FOUND,
        "No records were found in the cache"
    },
    {
        RSYS_ERROR_STRING_CONV_FAILED,
        "Failed to convert string format (wide/ansi)"
    },
    {
        RSYS_ERROR_QUERY_CREATION_FAILED,
        "Failed to create query to examine cache"
    },
    {
        RSYS_ERROR_FAILED_TO_SET_TIME,
        "The system time could not be set"
    },
    {
        RSYS_ERROR_NO_NETBIOS_NAME,
        "Failed to find NetBIOS name for the domain"
    },
    {
        RSYS_ERROR_INVALID_OBJECTGUID,
        "The specified Globally Unique Identifier (GUID) is invalid"
    },
    {
        RSYS_ERROR_INVALID_DOMAIN,
        "The domain name is invalid"
    },
    {
        RSYS_ERROR_NO_DEFAULT_REALM,
        "The kerberos default realm is not set"
    },
    {
        RSYS_ERROR_NOT_SUPPORTED,
        "The request is not supported",  
    },
    {
        RSYS_ERROR_NO_HANDLER,
        "No handler could be found for this message type"
    },
    {
        RSYS_ERROR_NO_MATCHING_CACHE_ENTRY,
        "No matching entry in the Domain Controller Cache could be found"
    },
    {
        RSYS_ERROR_MAC_FLUSH_DS_CACHE_FAILED,
        "Could not find DirectoryService cache utility to call -flushcache with"
    },
};

size_t
RSysGetErrorString(
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
        
    if (RSYS_ERROR_MASK(dwError) != 0)
    {
        stResult = RSysMapErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = RSysGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    
cleanup:
    
    return stResult;
}

size_t
RSysMapErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gRSysErrorMessages)/
        sizeof(gRSysErrorMessages[0]);
    DWORD dwIndex = 0;
    
    for (dwIndex = 0; dwIndex < dwNMessages; dwIndex++)
    {
        if (gRSysErrorMessages[dwIndex].dwCode == dwError)
        {
            PCSTR pszMessage = gRSysErrorMessages[dwIndex].pszMessage;
            DWORD dwRequiredLen = strlen(pszMessage) + 1;
            
            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }
            
            stResult = dwRequiredLen;
           
            goto cleanup;
        }
    }
    
    stResult = RSysGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
RSysGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = RSYS_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = RSysStrError(dwConvertError, pszBuffer, stBufSize);

    while (result)
    {
        if(result == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = RSysGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        RSYS_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = RTL_ALLOCATE(
                        &pszTempBuffer,
                        CHAR,
                        stBufSize);
        BAIL_ON_RSYS_ERROR(dwError);

        result = RSysStrError(dwConvertError, pszTempBuffer, stBufSize);
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

    RSYS_SAFE_FREE_MEMORY(pszTempBuffer);
    
    return stResult;

error:

    stResult = 0;
    
    goto cleanup;
}

size_t
RSysGetUnmappedErrorString(
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
