/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        diradd.c
 *
 * Abstract:
 *
 *
 *      BeyondTrust Directory Wrapper Interface
 *      DirectoryAddObject Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

DWORD
DirectoryAddObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD attributes[]
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PSTR pszObjectDn = NULL;
    PCSTR filterFormat = "%s=%Q";
    PWSTR pwszFilter = NULL;
    CHAR szAttrDn[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDn[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwObjectClass = 0;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrObjectClass,
        NULL
    };

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pwszObjectDN, &pszObjectDn);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryAllocateWC16StringFilterPrintf(
                              &pwszFilter,
                              filterFormat,
                              szAttrDn, pszObjectDn);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectorySearch(hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (dwNumEntries == 1)
    {
        pEntry = &(pEntries[0]);

        dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectClass,
                              DIRECTORY_ATTR_TYPE_INTEGER,
                              &dwObjectClass);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        switch (dwObjectClass)
        {
        case DIR_OBJECT_CLASS_USER:
            dwError = ERROR_USER_EXISTS;
            break;

        case DIR_OBJECT_CLASS_LOCAL_GROUP:
            dwError = ERROR_ALIAS_EXISTS;
            break;

        default:
            dwError = ERROR_ALREADY_EXISTS;
            break;
        }
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }
    else if (dwNumEntries > 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryAdd(
                    pContext->hBindHandle,
                    pwszObjectDN,
                    attributes);

error:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    DIRECTORY_FREE_MEMORY(pszObjectDn);
    DIRECTORY_FREE_MEMORY(pwszFilter);

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
