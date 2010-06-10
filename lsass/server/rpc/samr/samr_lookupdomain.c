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
 *        samr_lookupdomain.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrLookupDomain function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
SamrSrvLookupDomain(
    /* [in] */ handle_t hBinding,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ UNICODE_STRING *domain_name,
    /* [out] */ SID **ppSid
    )
{
    CHAR szDnToken[] = "DC";
    wchar_t wszFilter[] = L"%ws=%u AND %ws=\'%ws\'";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDomain[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrObjectSID[] = DS_ATTR_OBJECT_SID;
    DWORD dwObjectClass = DS_OBJECT_CLASS_DOMAIN;
    WCHAR wszBuiltinDomainName[] = SAMR_BUILTIN_DOMAIN_NAME;
    PWSTR pwszDomainName = NULL;
    DWORD dwBaseLen = 0;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[2];
    PDIRECTORY_ENTRY pEntries = NULL;
    DWORD dwCount = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PSID pDomainSid = NULL;

    BAIL_ON_INVALID_PARAMETER(ppSid);
    BAIL_ON_INVALID_PARAMETER(domain_name);

    memset(wszAttributes, 0, sizeof(wszAttributes));

    pConnCtx = (PCONNECT_CONTEXT)hConn;

    if (pConnCtx == NULL || pConnCtx->Type != SamrContextConnect)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = SamrSrvGetFromUnicodeString(&pwszDomainName,
                                           domain_name);
    BAIL_ON_NO_MEMORY(pwszDomainName);

    if (!wc16scasecmp(pwszDomainName, wszBuiltinDomainName))
    {
        dwObjectClass = DS_OBJECT_CLASS_BUILTIN_DOMAIN;
    }

    dwBaseLen = domain_name->MaximumLength +
                ((sizeof(szDnToken) + 2) * sizeof(WCHAR));

    dwFilterLen = (sizeof(wszAttrObjectClass) - 1) +
                  10 +
                  (sizeof(wszAttrDomain) - 1) +
                  domain_name->Length +
                  sizeof(wszFilter);

    ntStatus = SamrSrvAllocateMemory(OUT_PPVOID(&pwszFilter),
                                     dwFilterLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (sw16printfw(pwszFilter, dwFilterLen/sizeof(WCHAR), wszFilter,
                    wszAttrObjectClass, dwObjectClass,
                    wszAttrDomain, pwszDomainName) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    wszAttributes[0] = wszAttrObjectSID;
    wszAttributes[1] = NULL;

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwCount == 1) {
        dwError = DirectoryGetEntryAttributeSingle(pEntries,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr, &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {
            ntStatus = SamrSrvAllocateSidFromWC16String(
                            &pDomainSid,
                            pAttrVal->data.pwszStringValue);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

        } else {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

    } else if (dwCount == 0) {
        ntStatus = STATUS_NO_SUCH_DOMAIN;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

    } else {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppSid = pDomainSid;

cleanup:
    if (pwszBase) {
        SamrSrvFreeMemory(pwszBase);
    }

    if (pwszDomainName) {
        SamrSrvFreeMemory(pwszDomainName);
    }

    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwCount);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pDomainSid) {
        SamrSrvFreeMemory(&pDomainSid);
    }

    *ppSid = NULL;
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
