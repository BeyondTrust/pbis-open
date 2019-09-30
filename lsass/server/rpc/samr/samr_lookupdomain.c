/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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
    PCSTR filterFormat = "%s=%u AND %s=%Q";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    CHAR szAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    CHAR szAttrDomain[] = DS_ATTR_DOMAIN;
    WCHAR wszAttrObjectSID[] = DS_ATTR_OBJECT_SID;
    DWORD dwObjectClass = DS_OBJECT_CLASS_DOMAIN;
    WCHAR wszBuiltinDomainName[] = SAMR_BUILTIN_DOMAIN_NAME;
    PWSTR pwszDomainName = NULL;
    PSTR pszDomainName = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
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

    dwError = LwWc16sToMbs(pwszDomainName, &pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                            &pwszFilter,
                            filterFormat,
                            szAttrObjectClass, dwObjectClass,
                            szAttrDomain, pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

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

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwCount);
    }

    LW_SAFE_FREE_MEMORY(pszDomainName);
    LW_SAFE_FREE_MEMORY(pwszFilter);

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
