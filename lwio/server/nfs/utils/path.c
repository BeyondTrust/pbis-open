/*
 * Copyright Likewise Software    2004-2009
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
 *        path.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - NFS
 *
 *        Utilities
 *
 *        Paths
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
NfsBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    )
{
    NTSTATUS  ntStatus       = 0;
    size_t    len_prefix     = 0;
    size_t    len_suffix     = 0;
    size_t    len_separator  = 0;
    PWSTR     pDataCursor    = NULL;
    wchar16_t wszFwdSlash[]  = {'/',  0};
    wchar16_t wszBackSlash[] = {'\\', 0};
    PWSTR     pwszFilename   = NULL;

    if (!pwszSuffix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (!ppwszFilename)
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = pwszPrefix ? wc16slen(pwszPrefix) : 0;
    len_suffix = wc16slen(pwszSuffix);

    if (len_prefix && len_suffix && *pwszSuffix &&
        (*pwszSuffix != wszFwdSlash[0]) && (*pwszSuffix != wszBackSlash[0]))
    {
        len_separator = sizeof(wszBackSlash[0]);
    }

    ntStatus = NfsAllocateMemory(
                    (len_prefix + len_suffix + len_separator + 1 ) * sizeof(wchar16_t),
                    (PVOID*)&pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pwszFilename;
    while (pwszPrefix && *pwszPrefix)
    {
        *pDataCursor++ = *pwszPrefix++;
    }

    if (len_separator)
    {
        *pDataCursor++ = wszBackSlash[0];
    }

    while (pwszSuffix && *pwszSuffix)
    {
        *pDataCursor++ = *pwszSuffix++;
    }

    pDataCursor = pwszFilename;
    while (pDataCursor && *pDataCursor)
    {
        if (*pDataCursor == wszFwdSlash[0])
        {
            *pDataCursor = wszBackSlash[0];
        }
        pDataCursor++;
    }

    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppwszFilename = NULL;

    if (pwszFilename)
    {
        NfsFreeMemory(pwszFilename);
    }

    goto cleanup;
}

NTSTATUS
NfsGetParentPath(
    PWSTR  pwszPath,
    PWSTR* ppwszParentPath
    )
{
    NTSTATUS  ntStatus       = STATUS_SUCCESS;
    PWSTR     pwszParentPath = NULL;
    PWSTR     pwszCursor     = NULL;
    size_t    sLen           = 0;
    wchar16_t wszBackSlash[] = { '\\', 0 };
    wchar16_t wszFwdSlash[]  = { '/',  0 };

    if (!pwszPath ||
        !(sLen = wc16slen(pwszPath)) ||
        ((*pwszPath != wszBackSlash[0]) && (*pwszPath != wszFwdSlash[0])))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszCursor = pwszPath + sLen - 1;

    while (!IsNullOrEmptyString(pwszCursor) && (pwszCursor != pwszPath))
    {
        if ((*pwszCursor == wszBackSlash[0]) || (*pwszCursor == wszFwdSlash[0]))
        {
            ntStatus = NfsAllocateMemory(
                            (pwszCursor - pwszPath + 1) * sizeof(wchar16_t),
                            (PVOID*)&pwszParentPath);
            BAIL_ON_NT_STATUS(ntStatus);

            memcpy( (PBYTE)pwszParentPath,
                    (PBYTE)pwszPath,
                    (pwszCursor - pwszPath) * sizeof(wchar16_t));

            break;
        }

        pwszCursor--;
    }

    if (!pwszParentPath)
    {
        ntStatus = NfsAllocateStringW(&wszBackSlash[0], &pwszParentPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszParentPath = pwszParentPath;

cleanup:

    return ntStatus;

error:

    *ppwszParentPath = NULL;

    NFS_SAFE_FREE_MEMORY(pwszParentPath);

    goto cleanup;
}

NTSTATUS
NfsMatchPathPrefix(
    PWSTR pwszPath,
    ULONG ulPathLength,
    PWSTR pwszPrefix
    )
{
    NTSTATUS ntStatus = STATUS_NO_MATCH;
    ULONG   ulPrefixLength = wc16slen(pwszPrefix);
    PWSTR   pwszTmp = NULL;

    if (ulPathLength >= ulPrefixLength)
    {
        ntStatus = NfsAllocateMemory(
                        (ulPrefixLength + 1) * sizeof(wchar16_t),
                        (PVOID*)&pwszTmp);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy( (PBYTE)pwszTmp,
                (PBYTE)pwszPath,
                ulPrefixLength * sizeof(wchar16_t));

        if (!SMBWc16sCaseCmp(pwszTmp, pwszPrefix))
        {
            ntStatus = STATUS_SUCCESS;
        }
    }

error:

    NFS_SAFE_FREE_MEMORY(pwszTmp);

    return ntStatus;
}
