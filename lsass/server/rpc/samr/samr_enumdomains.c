/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_enumdomains.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrEnumDomains function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvEnumDomains(
    IN  handle_t        hBinding,
    IN  CONNECT_HANDLE  hConn,
    IN OUT PDWORD       pdwResume,
    IN  DWORD           dwMaxSize,
    OUT ENTRY_ARRAY   **ppDomains,
    OUT PDWORD          pdwNumEntries
    )
{
    wchar_t wszFilter[] = L"%ws=%u OR %ws=%u";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    DWORD dwObjectClassDomain = DS_OBJECT_CLASS_DOMAIN;
    DWORD dwObjectClassBuiltin = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwEntriesNum = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD dwSize = 0;
    DWORD i = 0;
    DWORD dwResume = 0;
    DWORD dwCount = 0;
    ENTRY_ARRAY *pDomains = NULL;
    ENTRY *pDomain = NULL;

    PWSTR wszAttributes[] = {
        wszAttrCommonName,
        NULL
    };

    BAIL_ON_INVALID_PTR(hBinding);
    BAIL_ON_INVALID_PTR(hConn);
    BAIL_ON_INVALID_PTR(pdwResume);
    BAIL_ON_INVALID_PTR(ppDomains);
    BAIL_ON_INVALID_PTR(pdwNumEntries);

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /* Check access rights required */
    if (!(pConnCtx->dwAccessGranted & SAMR_ACCESS_ENUM_DOMAINS))
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwFilterLen = ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilter) / sizeof(wszFilter[0])) +
                  ((sizeof(wszAttrObjectClass) / sizeof(WCHAR)) - 1) +
                  10;

    dwError = LwAllocateMemory(dwFilterLen * sizeof(pwszFilter[0]),
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilter,
                    &wszAttrObjectClass[0],
                    dwObjectClassDomain,
                    &wszAttrObjectClass[0],
                    dwObjectClassBuiltin) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&pDomains),
                                     sizeof(*pDomains));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    i = (*pdwResume);

    if (i >= dwEntriesNum)
    {
        ntStatus = STATUS_NO_MORE_ENTRIES;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwSize += sizeof(pDomains->dwCount);
    for (; dwSize < dwMaxSize && i < dwEntriesNum; i++)
    {
        pEntry = &(pEntries[i]);

        dwError = DirectoryGetEntryAttributeSingle(pEntry,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr, &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal &&
            pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            dwSize += sizeof(UINT32);
            dwSize += wc16slen(pAttrVal->data.pwszStringValue) * sizeof(WCHAR);
            dwSize += 2 * sizeof(UINT16);

            dwCount++;
        }
        else
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }
    }

    /* At least one domain entry is returned regardless of declared
       max response size */
    dwCount  = (!dwCount) ? 1 : dwCount;
    dwResume = (*pdwResume) + dwCount;

    ntStatus = SamrSrvAllocateMemory(
                          (void**)&pDomains->pEntries,
                          sizeof(pDomains->pEntries[0]) * dwCount);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pDomains->dwCount = dwCount;

    for (i = (*pdwResume); i < dwResume; i++)
    {
        pEntry = &(pEntries[i]);

        dwError = DirectoryGetEntryAttributeSingle(pEntry,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr, &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            pDomain = &(pDomains->pEntries[i - (*pdwResume)]);

            pDomain->dwIndex = i;
            ntStatus = SamrSrvInitUnicodeString(
                                    &pDomain->Name,
                                    pAttrVal->data.pwszStringValue);
        }
        else
        {
            ntStatus = STATUS_INTERNAL_ERROR;
        }
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (dwResume < dwEntriesNum)
    {
        ntStatus = STATUS_MORE_ENTRIES;
    }

    *pdwResume      = dwResume;
    *pdwNumEntries  = dwCount;
    *ppDomains      = pDomains;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pDomains)
    {
        SamrSrvFreeMemory(pDomains);
    }

    *pdwResume      = 0;
    *pdwNumEntries  = 0;
    *ppDomains      = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
