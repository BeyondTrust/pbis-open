/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        sharedb.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - NFS
 *
 *        Share API
 *
 *        Mapping functions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NfsShareMapSpecificToWindowsPath(
    IN  PWSTR  pwszFSPrefix,
    IN  PWSTR  pwszRootPath,
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    );

NTSTATUS
NfsShareMapIdToServiceStringW(
    IN  SHARE_SERVICE  service,
    OUT PWSTR*         ppwszService
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszShareType = NULL;
    PSTR  pszShareType = NULL;

    ntStatus = NfsShareMapIdToServiceStringA(service, &pszShareType);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NfsMbsToWc16s(pszShareType, &pwszShareType);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszService = pwszShareType;

cleanup:

    NFS_SAFE_FREE_MEMORY(pszShareType);

    return ntStatus;

error:

    *ppwszService = NULL;

    goto cleanup;
}

NTSTATUS
NfsShareMapIdToServiceStringA(
    IN  SHARE_SERVICE  service,
    OUT PSTR*          ppszService
    )
{
    NTSTATUS ntStatus = 0;
    PCSTR    pszId = NULL;
    PSTR     pszService = NULL;

    switch (service)
    {
        case SHARE_SERVICE_DISK_SHARE:

            pszId = LWIO_NFS_SHARE_STRING_ID_DISK_A;

            break;

        case SHARE_SERVICE_PRINTER:

            pszId = LWIO_NFS_SHARE_STRING_ID_PRINTER_A;

            break;

        case SHARE_SERVICE_COMM_DEVICE:

            pszId = LWIO_NFS_SHARE_STRING_ID_COMM_A;

            break;

        case SHARE_SERVICE_NAMED_PIPE:

            pszId = LWIO_NFS_SHARE_STRING_ID_IPC_A;

            break;

        case SHARE_SERVICE_ANY:

            pszId = LWIO_NFS_SHARE_STRING_ID_ANY_A;

            break;

        default:

            ntStatus = STATUS_NOT_FOUND;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateString(
                    pszId,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszService = pszService;

cleanup:

    return ntStatus;

error:

    *ppszService = NULL;

    goto cleanup;
}

NTSTATUS
NfsShareMapServiceStringToIdA(
    IN     PCSTR          pszService,
    IN OUT SHARE_SERVICE* pService
    )
{
    NTSTATUS ntStatus = 0;
    SHARE_SERVICE service = SHARE_SERVICE_UNKNOWN;

    if (IsNullOrEmptyString(pszService))
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    else if (!strcmp(pszService, LWIO_NFS_SHARE_STRING_ID_IPC_A))
    {
        service = SHARE_SERVICE_NAMED_PIPE;
    }
    else if (!strcmp(pszService, LWIO_NFS_SHARE_STRING_ID_DISK_A))
    {
        service = SHARE_SERVICE_DISK_SHARE;
    }
    else if (!strcmp(pszService, LWIO_NFS_SHARE_STRING_ID_COMM_A))
    {
        service = SHARE_SERVICE_COMM_DEVICE;
    }
    else if (!strcmp(pszService, LWIO_NFS_SHARE_STRING_ID_PRINTER_A))
    {
        service = SHARE_SERVICE_PRINTER;
    }
    else if (!strcmp(pszService, LWIO_NFS_SHARE_STRING_ID_ANY_A))
    {
        service = SHARE_SERVICE_ANY;
    }
    else
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pService = service;

cleanup:

    return ntStatus;

error:

    *pService = SHARE_SERVICE_UNKNOWN;

    goto cleanup;
}

NTSTATUS
NfsShareMapServiceStringToIdW(
    IN     PWSTR          pwszService,
    IN OUT SHARE_SERVICE* pService
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    wchar16_t     wszIpc[]  = LWIO_NFS_SHARE_STRING_ID_IPC_W;
    wchar16_t     wszDisk[] = LWIO_NFS_SHARE_STRING_ID_DISK_W;
    wchar16_t     wszComm[] = LWIO_NFS_SHARE_STRING_ID_COMM_W;
    wchar16_t     wszPtr[]  = LWIO_NFS_SHARE_STRING_ID_PRINTER_W;
    wchar16_t     wszAny[]  = LWIO_NFS_SHARE_STRING_ID_ANY_W;
    SHARE_SERVICE service   = SHARE_SERVICE_UNKNOWN;

    if (IsNullOrEmptyString(pwszService))
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    else if (!wc16scmp(pwszService, &wszIpc[0]))
    {
        service = SHARE_SERVICE_NAMED_PIPE;
    }
    else if (!wc16scmp(pwszService, &wszDisk[0]))
    {
        service = SHARE_SERVICE_DISK_SHARE;
    }
    else if (!wc16scmp(pwszService, &wszComm[0]))
    {
        service = SHARE_SERVICE_COMM_DEVICE;
    }
    else if (!wc16scmp(pwszService, &wszPtr[0]))
    {
        service = SHARE_SERVICE_PRINTER;
    }
    else if (!wc16scmp(pwszService, &wszAny[0]))
    {
        service = SHARE_SERVICE_ANY;
    }
    else
    {
        ntStatus = STATUS_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pService = service;

cleanup:

    return ntStatus;

error:

    *pService = SHARE_SERVICE_UNKNOWN;

    goto cleanup;
}

NTSTATUS
NfsShareMapFromWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    )
{
    NTSTATUS  ntStatus = 0;
    wchar16_t wszBackslash[2] = {'\\', 0};
    wchar16_t wszFwdslash[2] = {'/', 0};
    wchar16_t wszColon[2] = {':', 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 3;
    size_t    sFSRootLen = 0;
    size_t    sRequiredLen = 0;
    wchar16_t wszFileSystemRoot[] = LWIO_NFS_FILE_SYSTEM_ROOT_W;

    if (!pwszInputPath || !*pwszInputPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    sFSRootLen = wc16slen(&wszFileSystemRoot[0]);

    if ((sInputPathLen < sFSPrefixLen) ||
        ((pwszInputPath[1] != wszColon[0]) &&
         !(pwszInputPath[2] == wszBackslash[0] ||
           pwszInputPath[2] == wszFwdslash[0])))
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        sRequiredLen += sFSRootLen * sizeof(wchar16_t);
        pwszPathReadCursor += sFSPrefixLen;
    }

    if (!pwszPathReadCursor || !*pwszPathReadCursor)
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    sRequiredLen += sizeof(wchar16_t); // path delimiter

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    // The rest of the path
    sRequiredLen += wc16slen(pwszPathReadCursor) * sizeof(wchar16_t);
    sRequiredLen += sizeof(wchar16_t);

    ntStatus = NfsAllocateMemory(sRequiredLen, (PVOID*)&pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    pwszPathReadCursor = pwszInputPath;
    pwszPathWriteCursor = pwszPath;

    pwszPathReadCursor += sFSPrefixLen;
    memcpy((PBYTE)pwszPathWriteCursor, (PBYTE)&wszFileSystemRoot[0],
            sFSRootLen * sizeof(wchar16_t));
    pwszPathWriteCursor += sFSRootLen;
    *pwszPathWriteCursor++ = wszBackslash[0];

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    while (pwszPathReadCursor && *pwszPathReadCursor)
    {
        if (*pwszPathReadCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor++ = wszBackslash[0];
        }
        else
        {
            *pwszPathWriteCursor++ = *pwszPathReadCursor;
        }
        pwszPathReadCursor++;
    }

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    NFS_SAFE_FREE_MEMORY(pwszPath);

    goto cleanup;
}

NTSTATUS
NfsShareMapToWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR pwszPath = NULL;
    wchar16_t wszFileSystemPrefix[] = LWIO_NFS_FILE_SYSTEM_PREFIX_W;
    wchar16_t wszFileSystemRoot[] = LWIO_NFS_FILE_SYSTEM_ROOT_W;

    ntStatus = NfsShareMapSpecificToWindowsPath(
                    &wszFileSystemPrefix[0],
                    &wszFileSystemRoot[0],
                    pwszInputPath,
                    &pwszPath);
    if (ntStatus == STATUS_OBJECT_PATH_SYNTAX_BAD)
    {
        wchar16_t wszPipeSystemRoot[] = LWIO_NFS_PIPE_SYSTEM_ROOT_W;

        ntStatus = NfsShareMapSpecificToWindowsPath(
                            &wszFileSystemPrefix[0],
                            &wszPipeSystemRoot[0],
                            pwszInputPath,
                            &pwszPath);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    if (pwszPath)
    {
        NfsFreeMemory(pwszPath);
    }

    goto cleanup;
}

static
NTSTATUS
NfsShareMapSpecificToWindowsPath(
    IN  PWSTR  pwszFSPrefix,
    IN  PWSTR  pwszRootPath,
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    )
{
    NTSTATUS  ntStatus = 0;
    wchar16_t wszBackslash[2] = {'\\', 0};
    wchar16_t wszFwdslash[2] = {'/', 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 0;
    size_t    sFSRootLen = 0;
    size_t    sRequiredLen = 0;

    if (!pwszInputPath || !*pwszInputPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    sFSPrefixLen = wc16slen(pwszFSPrefix);
    sFSRootLen = wc16slen(pwszRootPath);

    if ((sInputPathLen < sFSRootLen) ||
        memcmp((PBYTE)pwszPathReadCursor,
               (PBYTE)pwszRootPath,
               sFSRootLen * sizeof(wchar16_t)))
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        sRequiredLen += sFSPrefixLen * sizeof(wchar16_t);
        pwszPathReadCursor += sFSRootLen;
    }

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    // The rest of the path
    sRequiredLen += wc16slen(pwszPathReadCursor) * sizeof(wchar16_t);
    sRequiredLen += sizeof(wchar16_t);

    ntStatus = NfsAllocateMemory(sRequiredLen, (PVOID*)&pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    pwszPathReadCursor = pwszInputPath;
    pwszPathWriteCursor = pwszPath;

    pwszPathReadCursor += sFSRootLen;
    memcpy((PBYTE)pwszPathWriteCursor, (PBYTE)pwszFSPrefix,
            sFSPrefixLen * sizeof(wchar16_t));
    pwszPathWriteCursor += sFSPrefixLen;

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) ||
            (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    while (pwszPathReadCursor && *pwszPathReadCursor)
    {
        if (*pwszPathReadCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor++ = wszBackslash[0];
        }
        else
        {
            *pwszPathWriteCursor++ = *pwszPathReadCursor;
        }
        pwszPathReadCursor++;
    }

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    NFS_SAFE_FREE_MEMORY(pwszPath);

    goto cleanup;
}
