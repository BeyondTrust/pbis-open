/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        diradd.c
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
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
    wchar_t wszFilterFmt[] = L"%ws='%ws'";
    size_t sDnLen = 0;
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
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

    dwError = LwWc16sLen(pwszObjectDN, &sDnLen);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwFilterLen = ((sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0])) - 1) +
                   sizeof(wszAttrDn) +
                   (sizeof(WCHAR) * sDnLen);

    dwError = DirectoryAllocateMemory(dwFilterLen,
                                      OUT_PPVOID(&pwszFilter));
    BAIL_ON_DIRECTORY_ERROR(dwError);

    if (sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                    wszAttrDn,
                    pwszObjectDN) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

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
