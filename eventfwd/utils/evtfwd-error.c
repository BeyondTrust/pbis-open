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
 *        evtfwd-error.c
 *
 * Abstract:
 *
 *        Event forwarder from eventlogd to collector service
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
const ERROR_MESSAGE gEfdErrorMessages[] = 
{
    {
        EFD_ERROR_INVALID_CACHE_PATH,
        "An invalid cache path was specified"
    },
    {
        EFD_ERROR_INVALID_CONFIG_PATH,
        "The path to the configuration file is invalid"
    },
    {
        EFD_ERROR_INVALID_PREFIX_PATH,
        "The product installation folder could not be determined"
    },
    {
        EFD_ERROR_INSUFFICIENT_BUFFER,
        "The provided buffer is insufficient"
    },
    {
        EFD_ERROR_OUT_OF_MEMORY,
        "Out of memory"
    },
    {
        EFD_ERROR_INVALID_MESSAGE,
        "The Inter Process message is invalid"
    },
    {
        EFD_ERROR_UNEXPECTED_MESSAGE,
        "An unexpected Inter Process message was received"
    },
    {
        EFD_ERROR_DATA_ERROR,
        "The cached data is incorrect"
    },
    {
        EFD_ERROR_NOT_IMPLEMENTED,
        "The requested feature has not been implemented yet"
    },
    {
        EFD_ERROR_NO_CONTEXT_ITEM,
        "The requested item was not found in context"
    },
    {
        EFD_ERROR_REGEX_COMPILE_FAILED,
        "Failed to compile regular expression"
    },
    {
        EFD_ERROR_INTERNAL,
        "Internal error"
    },
    {
        EFD_ERROR_INVALID_DNS_RESPONSE,
        "Response from DNS is invalid"
    },
    {
        EFD_ERROR_DNS_RESOLUTION_FAILED,
        "Failed to resolve query using DNS"
    },
    {
        EFD_ERROR_FAILED_TIME_CONVERSION,
        "Failed to convert the time"
    },
    {
        EFD_ERROR_INVALID_SID,
        "The security descriptor (SID) is invalid"
    },
    {
        EFD_ERROR_UNEXPECTED_DB_RESULT,
        "Unexpected cached data found"
    },
    {
        EFD_ERROR_INVALID_EFD_CONNECTION,
        "The connection to the authentication service is invalid"
    },
    {
        EFD_ERROR_INVALID_PARAMETER,
        "Invalid parameter"
    },
    {
        EFD_ERROR_LDAP_NO_PARENT_DN,
        "No distinguished name found in LDAP for parent of this object"
    },
    {
        EFD_ERROR_LDAP_ERROR,
        "An Error was encountered when negotiating with LDAP"
    },
    {
        EFD_ERROR_NO_SUCH_DOMAIN,
        "Unknown Active Directory domain"
    },
    {
        EFD_ERROR_LDAP_FAILED_GETDN,
        "Failed to find distinguished name using LDAP"
    },
    {
        EFD_ERROR_DUPLICATE_DOMAINNAME,
        "A duplicate Active Directory domain name was found"
    },
    {
        EFD_ERROR_FAILED_FIND_DC,
        "Failed to find the domain controller"
    },
    {
        EFD_ERROR_LDAP_GET_DN_FAILED,
        "Failed to find distinguished name using LDAP"
    },
    {
        EFD_ERROR_INVALID_SID_REVISION,
        "The security descriptor (SID) has an invalid revision"
    },
    {
        EFD_ERROR_LOAD_LIBRARY_FAILED,
        "Failed to dynamically load a library"
    },
    {
        EFD_ERROR_LOOKUP_SYMBOL_FAILED,
        "Failed to lookup a symbol in a dynamic library"
    },
    {
        EFD_ERROR_INVALID_EVENTLOG,
        "The Eventlog interface is invalid"
    },
    {
        EFD_ERROR_INVALID_CONFIG,
        "The specified configuration (file) is invalid"
    },
    {
        EFD_ERROR_UNEXPECTED_TOKEN,
        "An unexpected token was encountered in the configuration"
    },
    {
        EFD_ERROR_LDAP_NO_RECORDS_FOUND,
        "No records were found in the cache"
    },
    {
        EFD_ERROR_STRING_CONV_FAILED,
        "Failed to convert string format (wide/ansi)"
    },
    {
        EFD_ERROR_QUERY_CREATION_FAILED,
        "Failed to create query to examine cache"
    },
    {
        EFD_ERROR_NOT_JOINED_TO_AD,
        "This machine is not currently joined to Active Directory"
    },
    {
        EFD_ERROR_FAILED_TO_SET_TIME,
        "The system time could not be set"
    },
    {
        EFD_ERROR_NO_NETBIOS_NAME,
        "Failed to find NetBIOS name for the domain"
    },
    {
        EFD_ERROR_INVALID_OBJECTGUID,
        "The specified Globally Unique Identifier (GUID) is invalid"
    },
    {
        EFD_ERROR_INVALID_DOMAIN,
        "The domain name is invalid"
    },
    {
        EFD_ERROR_NO_DEFAULT_REALM,
        "The kerberos default realm is not set"
    },
    {
        EFD_ERROR_NOT_SUPPORTED,
        "The request is not supported",  
    },
    {
        EFD_ERROR_NO_HANDLER,
        "No handler could be found for this message type"
    },
    {
        EFD_ERROR_NO_MATCHING_CACHE_ENTRY,
        "No matching entry in the Domain Controller Cache could be found"
    },
    {
        EFD_ERROR_MAC_FLUSH_DS_CACHE_FAILED,
        "Could not find DirectoryService cache utility to call -flushcache with"
    },
};

size_t
EfdGetErrorString(
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
        
    if (EFD_ERROR_MASK(dwError) != 0)
    {
        stResult = EfdMapErrorToString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    else
    {
        stResult = EfdGetSystemErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);
    }
    
cleanup:
    
    return stResult;
}

size_t
EfdMapErrorToString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t stResult = 0;
    DWORD dwNMessages = sizeof(gEfdErrorMessages)/
        sizeof(gEfdErrorMessages[0]);
    DWORD dwIndex = 0;
    
    for (dwIndex = 0; dwIndex < dwNMessages; dwIndex++)
    {
        if (gEfdErrorMessages[dwIndex].dwCode == dwError)
        {
            PCSTR pszMessage = gEfdErrorMessages[dwIndex].pszMessage;
            DWORD dwRequiredLen = strlen(pszMessage) + 1;
            
            if (stBufSize >= dwRequiredLen) {
                memcpy(pszBuffer, pszMessage, dwRequiredLen);
            }
            
            stResult = dwRequiredLen;
           
            goto cleanup;
        }
    }
    
    stResult = EfdGetUnmappedErrorString(
                        dwError,
                        pszBuffer,
                        stBufSize);

cleanup:

    return stResult;
}

size_t
EfdGetSystemErrorString(
    DWORD  dwConvertError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    DWORD  dwError = EFD_ERROR_SUCCESS;
    size_t stResult = 0;
    PSTR   pszTempBuffer = NULL;

    int result = EfdStrError(dwConvertError, pszBuffer, stBufSize);

    while (result)
    {
        if(result == ERANGE)
        {
            // Guess
            stBufSize = stBufSize * 2 + 10;
        }
        else
        {
            stResult = EfdGetUnmappedErrorString(
                            dwConvertError,
                            pszBuffer,
                            stBufSize);
            goto cleanup;
        }
        EFD_SAFE_FREE_MEMORY(pszTempBuffer);

        dwError = RTL_ALLOCATE(
                        &pszTempBuffer,
                        CHAR,
                        stBufSize);
        BAIL_ON_EFD_ERROR(dwError);

        result = EfdStrError(dwConvertError, pszTempBuffer, stBufSize);
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

    EFD_SAFE_FREE_MEMORY(pszTempBuffer);
    
    return stResult;

error:

    stResult = 0;
    
    goto cleanup;
}

size_t
EfdGetUnmappedErrorString(
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
