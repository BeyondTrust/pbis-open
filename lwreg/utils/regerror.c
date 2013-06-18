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
 *        regerror.c
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Error Message API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
PCSTR
RegWin32ExtErrorToName(
    LW_WINERROR winerr
    );

#define STATUS_CODE(status, werror, errno, desc)             \
    {status, werror, errno, #status, #werror, #errno, desc },
#define __ERROR_XMACRO__

struct table_entry
{
    NTSTATUS    ntStatus;
    WINERROR    werror;
    int         uerror;
    PCSTR       pszStatusName;
    PCSTR       pszWinerrName;
    PCSTR       pszUnixErrnoName;
    PCSTR       pszDescription;

} status_table_regerror[] =
{
#ifdef __ERROR_XMACRO__
#define S STATUS_CODE
// List of lw extended errors
S ( LW_STATUS_SUCCESS, ERROR_SUCCESS, 0, "" )
S ( LW_STATUS_BUFFER_TOO_SMALL, ERROR_INSUFFICIENT_BUFFER, -1, "" )
S ( LW_STATUS_INSUFFICIENT_RESOURCES, ERROR_OUTOFMEMORY, -1, "" )
S ( LW_STATUS_NOT_IMPLEMENTED, LWREG_ERROR_NOT_IMPLEMENTED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_REGEX_COMPILE_FAILED, -1, "" )
S ( LW_STATUS_INTERNAL_ERROR, ERROR_INTERNAL_ERROR, -1, "" )
S ( LW_STATUS_INVALID_PARAMETER, ERROR_INVALID_PARAMETER, EINVAL, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, ERROR_INVALID_MESSAGE, -1, "" )
S ( LW_STATUS_OBJECT_NAME_NOT_FOUND, LWREG_ERROR_NO_SUCH_KEY_OR_VALUE, -1, "" )
S ( LW_STATUS_RESOURCE_IN_USE, LWREG_ERROR_KEY_IS_ACTIVE, -1, "" )
S ( LW_STATUS_DUPLICATE_NAME, LWREG_ERROR_DUPLICATE_KEYVALUENAME, -1, "" )
S ( LW_STATUS_KEY_HAS_CHILDREN, LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY, -1, "" )
S ( LW_STATUS_NOT_SUPPORTED, LWREG_ERROR_UNKNOWN_DATA_TYPE, -1, "" )
S ( LW_STATUS_INVALID_BLOCK_LENGTH, LWREG_ERROR_BEYOUND_MAX_KEY_OR_VALUE_LENGTH, -1, "" )
S ( LW_STATUS_NO_MORE_ENTRIES, LWREG_ERROR_NO_MORE_KEYS_OR_VALUES, -1, "" )
S ( LW_STATUS_OBJECT_NAME_INVALID, LWREG_ERROR_INVALID_NAME, -1, "" )
S ( LW_STATUS_ACCESS_DENIED, ERROR_ACCESS_DENIED, EACCES, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_INVALID_CACHE_PATH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_INVALID_PREFIX_PATH, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_REGEX_COMPILE_FAILED, -1, "" )
S ( LW_STATUS_UNHANDLED_EXCEPTION, LWREG_ERROR_NOT_HANDLED, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_UNEXPECTED_TOKEN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_INVALID_LOG_LEVEL, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_UNKNOWN, -1, "" )
S ( LW_STATUS_MESSAGE_NOT_FOUND, LWREG_ERROR_INVALID_CONTEXT, -1, "" )
S ( LW_STATUS_OBJECTID_EXISTS, LWREG_ERROR_KEYNAME_EXIST, -1, "" )
S ( LW_STATUS_NO_SECURITY_ON_OBJECT, LWREG_ERROR_NO_SECURITY_ON_KEY, -1, "")
S ( LW_STATUS_INVALID_SECURITY_DESCR, LWREG_ERROR_INVALID_SECURITY_DESCR, -1, "")
S ( LW_STATUS_NO_TOKEN, LWREG_ERROR_INVALID_ACCESS_TOKEN, -1, "")
S ( LW_STATUS_CANNOT_DELETE, LWREG_ERROR_DELETE_DEFAULT_VALUE_NOT_ALLOWED, -1, "")
S ( LW_STATUS_TOO_MANY_NAMES, LWREG_ERROR_TOO_MANY_ENTRIES, -1, "")
S ( -1, 0, -1, "")
#undef S
#endif
};

#undef STATUS_CODE
#undef __ERROR_XMACRO__

typedef int (*predicate) (struct table_entry* e, void* data);

static
PCSTR
RegErrorToName(
    LW_WINERROR winerr
    );

static int
match_werror(
    struct table_entry* e,
    void *data
    )
{
    return e->werror == *((LW_WINERROR*) data);
}

static int
match_status(
    struct table_entry* e,
    void *data
    )
{
    return e->ntStatus == *((LW_NTSTATUS*) data);
}

static struct table_entry*
find(
    predicate pred,
    void* data
    )
{
    unsigned int i;

    for (i = 0; i < sizeof(status_table_regerror)/sizeof(status_table_regerror[0]); i++)
    {
        if (pred(&status_table_regerror[i], data))
            return &status_table_regerror[i];
    }

    return NULL;
}

static
struct table_entry*
RegNtLookupCode(
    IN NTSTATUS status
    );

static struct
{
    DWORD dwError;
    PCSTR pszMessage;
} gLwRegErrorMap[] =
{
    {
        ERROR_SUCCESS,
        "No error"
    },
    {
        ERROR_INSUFFICIENT_BUFFER,
        "The provided buffer is insufficient"
    },
    {
        ERROR_OUTOFMEMORY,
        "Out of memory"
    },
    {
        LWREG_ERROR_NOT_IMPLEMENTED,
        "The requested feature has not been implemented yet"
    },
    {
        LWREG_ERROR_REGEX_COMPILE_FAILED,
        "Failed to compile regular expression"
    },
    {
    	ERROR_INTERNAL_ERROR,
        "Internal error"
    },
    {
        ERROR_INVALID_PARAMETER,
        "Invalid parameter"
    },
    {
        ERROR_INVALID_MESSAGE,
        "The Inter Process message is invalid"
    },
    {
        LWREG_ERROR_KEY_IS_ACTIVE,
        "The key is still actively in use"
    },
    {
        LWREG_ERROR_DUPLICATE_KEYVALUENAME,
        "Duplicate key/value name is not allowed in registry"
    },
    {
        LWREG_ERROR_FAILED_DELETE_HAS_SUBKEY,
        "Cannot delete key because it still has subkeys (If you want to delete the key, please use delete_tree.)"
    },
    {
    	LWREG_ERROR_NO_SUCH_KEY_OR_VALUE,
        "No such key or value"
    },
    {
        LWREG_ERROR_UNKNOWN_DATA_TYPE,
        "Unsupported registry data type"
    },
    {
    	LWREG_ERROR_BEYOUND_MAX_KEY_OR_VALUE_LENGTH,
        "Key (value) name / Value length is beyond maximum allowed length"
    },
    {
        LWREG_ERROR_NO_MORE_KEYS_OR_VALUES,
        "No more keys/values to enumerate"
    },
    {
        LWREG_ERROR_INVALID_NAME,
        "The registry keyname cannot have '\\'. The registry valuename cannot be NULL"
    },
    {
        ERROR_ACCESS_DENIED,
        "Incorrect access attempt"
    },
    {
        LWREG_ERROR_INVALID_CONTEXT,
        "Invalid command entered"
    },
    {
        LWREG_ERROR_INVALID_LOG_LEVEL,
        "Log level is not supported"
    },
    {
        LWREG_ERROR_INVALID_CACHE_PATH,
        "Invalid cache path"
    },
    {
    	LWREG_ERROR_KEYNAME_EXIST,
        "The key already exists in the registry"
    },
    {
    	LWREG_ERROR_NO_SECURITY_ON_KEY,
        "The key has no security descriptor stored"
    },
    {
    	LWREG_ERROR_INVALID_SECURITY_DESCR,
        "Invalid security descriptor"
    },
    {
    	LWREG_ERROR_INVALID_ACCESS_TOKEN,
    	"Failure creating access token"
    },
    {
        LWREG_ERROR_SYNTAX,
       "Syntax error found while parsing registry file"
    },
    {
       LWREG_ERROR_PARSE,
       "Parse error occurred while paring registry file"
    },
    {
        LWREG_ERROR_DELETE_DEFAULT_VALUE_NOT_ALLOWED,
        "Delete default value is not allowed"
    },
    {
        LWREG_ERROR_TOO_MANY_ENTRIES,
        "Too many subkeys or values for entry"
    },
};

size_t
LwRegGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    )
{
    size_t sErrIndex = 0;
    size_t sRequiredLen = 0;

    if (pszBuffer && stBufSize) {
       memset(pszBuffer, 0, stBufSize);
    }

    for (sErrIndex = 0; sErrIndex < sizeof(gLwRegErrorMap)/sizeof(gLwRegErrorMap[0]); sErrIndex++)
    {
        if (gLwRegErrorMap[sErrIndex].dwError == dwError)
        {
            sRequiredLen = strlen(gLwRegErrorMap[sErrIndex].pszMessage) + 1;
            if (stBufSize >= sRequiredLen)
            {
                strcpy(pszBuffer, gLwRegErrorMap[sErrIndex].pszMessage);
            }
            return sRequiredLen;
        }
    }

    sRequiredLen = strlen("Unknown error") + 1;
    if (stBufSize >= sRequiredLen)
    {
        strcpy(pszBuffer, "Unknown error");
    }
    return sRequiredLen;
}

DWORD
RegGetErrorMessageForLoggingEvent(
    DWORD dwErrCode,
    PSTR* ppszErrorMsg
    )
{
    DWORD dwErrorBufferSize = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszErrorMsg = NULL;
    PSTR  pszErrorBuffer = NULL;

    dwErrorBufferSize = LwRegGetErrorString(dwErrCode, NULL, 0);

    if (!dwErrorBufferSize)
        goto cleanup;

    dwError = LW_RTL_ALLOCATE((PVOID*)&pszErrorBuffer, CHAR,
    		                  sizeof(*pszErrorBuffer) * dwErrorBufferSize);
    BAIL_ON_REG_ERROR(dwError);

    dwLen = LwRegGetErrorString(dwErrCode, pszErrorBuffer, dwErrorBufferSize);

    if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
    {
        dwError = RegCStringAllocatePrintf(
                     &pszErrorMsg,
                     "Error: %s [error code: %d]",
                     pszErrorBuffer,
                     dwErrCode);
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppszErrorMsg = pszErrorMsg;

cleanup:

    LWREG_SAFE_FREE_STRING(pszErrorBuffer);

    return dwError;

error:

    LWREG_SAFE_FREE_STRING(pszErrorMsg);

    *ppszErrorMsg = NULL;

    goto cleanup;
}

VOID
RegPrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    )
{
    PCSTR pszUseErrorPrefix = NULL;
    size_t size = 0;
    PSTR pszErrorString = NULL;
    PCSTR pszWinError = NULL;

    if (dwError)
    {
        pszUseErrorPrefix = pszErrorPrefix;
        if (!pszUseErrorPrefix)
        {
            pszUseErrorPrefix = "LWREG ERROR: ";
        }

        size = LwRegGetErrorString(dwError, NULL, 0);
        if (size)
        {
            pszErrorString = malloc(size);
            if (pszErrorString)
            {
            	LwRegGetErrorString(dwError, pszErrorString, size);
            }
        }

        pszWinError = LW_PRINTF_STRING(RegWin32ExtErrorToName(dwError));

        if (!pszWinError)
        {
            pszWinError = "";
        }

        if (LW_IS_NULL_OR_EMPTY_STR(pszErrorString))
        {
            fprintf(stderr,
                    "%s (error = %u%s%s)\n",
                     pszUseErrorPrefix,
                     dwError,
                     *pszWinError ? " - " : "",
                     pszWinError);
        }
        else
        {
            fprintf(stderr,
                    "%s (error = %u%s%s)\n%s\n",
                    pszUseErrorPrefix,
                    dwError,
                    *pszWinError ? " - " : "",
                    pszWinError,
                    pszErrorString);
        }
    }
    LWREG_SAFE_FREE_STRING(pszErrorString);
}

DWORD
RegMapErrnoToLwRegError(
    DWORD dwErrno
    )
{
    switch(dwErrno)
    {
        case 0:
            return ERROR_SUCCESS;
        case EPERM:
        case EACCES:
            return ERROR_ACCESS_DENIED;
        case ENOENT:
            return ERROR_FILE_NOT_FOUND;
        case EINVAL:
            return ERROR_INVALID_PARAMETER;
        case ENOMEM:
            return ERROR_OUTOFMEMORY;

        default:
            REG_LOG_ERROR("Unable to map errno %d", dwErrno);
            return LWREG_ERROR_UNKNOWN;
    }
}

LW_WINERROR
RegNtStatusToWin32Error(
    LW_NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return e ? e->werror : NtStatusToWin32Error(ntStatus);
}

PCSTR
RegNtStatusToName(
    IN NTSTATUS status
    )
{
    struct table_entry* pEntry = RegNtLookupCode(status);

    if (pEntry && pEntry->pszStatusName)
    {
        return pEntry->pszStatusName;
    }
    else
    {
        return "UNKNOWN";
    }
}

static
PCSTR
RegErrorToName(
    LW_WINERROR winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszWinerrName : NULL;
}

static
PCSTR
RegWin32ExtErrorToName(
    LW_WINERROR winerr
    )
{
	PCSTR pszError = LwWin32ErrorToName(winerr);

	if (!pszError)
	{
		pszError = RegErrorToName(winerr);
	}

	return pszError;
}

static
struct table_entry *
RegNtLookupCode(
    IN NTSTATUS status
    )
{
    ULONG index;

    for (index = 0; index < sizeof(status_table_regerror) / sizeof(*status_table_regerror); index++)
    {
        if (status_table_regerror[index].ntStatus == status)
        {
            return &status_table_regerror[index];
        }
    }

    return NULL;
}
