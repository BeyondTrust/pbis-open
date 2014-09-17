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
 *        dirdel.c
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *      DirectoryDeleteObject Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"


DWORD
DirectoryDeleteObject(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;
    PDIRECTORY_PROVIDER pProvider = (pContext ? pContext->pProvider : NULL);
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PSTR pszObjectDn = NULL;
    PCSTR filterFormat = "%s=%Q";
    PWSTR pwszFilter = NULL;
    CHAR szAttrDn[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrDn[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectClass[] = DIRECTORY_ATTR_OBJECT_CLASS;
    WCHAR wszAttrGid[] = DIRECTORY_ATTR_GID;
    WCHAR wszAttrPrimaryGroup[] = DIRECTORY_ATTR_PRIMARY_GROUP;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwObjectClass = 0;
    DWORD dwGid = 0;
    PDIRECTORY_ENTRY pMemberEntries = NULL;
    DWORD dwNumMembers = 0;
    DWORD iMember = 0;
    PDIRECTORY_ENTRY pMember = NULL;
    DWORD dwPrimaryGroup = 0;
    PDIRECTORY_ENTRY pDelMemEntries = NULL;
    DWORD dwDelEntriesNum = 0;
    PWSTR pwszMemberDn = NULL;
    PDIRECTORY_ATTRIBUTE pAttribute = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PDIRECTORY_ENTRY pMembershipEntries = NULL;
    DWORD dwNumGroups = 0;
    DWORD iGroup = 0;
    PDIRECTORY_ENTRY pGroup = NULL;
    PWSTR pwszGroupDn = NULL;

    PWSTR wszAttributes[] = {
        wszAttrDn,
        wszAttrObjectClass,
        wszAttrGid,
        NULL
    };

    PWSTR wszMemberAttributes[] = {
        wszAttrDn,
        wszAttrPrimaryGroup,
        NULL
    };

    PWSTR wszMembershipAttributes[] = {
        wszAttrDn,
        NULL
    };

    if (!pContext || !pProvider)
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

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }
    else if (dwNumEntries == 1)
    {
        pEntry = &(pEntries[0]);

        dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectClass,
                              DIRECTORY_ATTR_TYPE_INTEGER,
                              &dwObjectClass);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrGid,
                              DIRECTORY_ATTR_TYPE_INTEGER,
                              &dwGid);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        /*
         * If an object being removed is a group remove any
         * existing members from this group.
         */
        if (dwObjectClass == DIR_OBJECT_CLASS_LOCAL_GROUP)
        {
            dwError = DirectoryGetGroupMembers(
                                  hDirectory,
                                  pwszObjectDN,
                                  wszMemberAttributes,
                                  &pMemberEntries,
                                  &dwNumMembers);
            BAIL_ON_DIRECTORY_ERROR(dwError);

            if (dwNumMembers)
            {
                /*
                 * Allocate space for entries to be deleted and copy the DNs
                 * because pMemberEntries is not NULL-terminated and
                 * DirectoryRemoveFromGroup requires that
                 */
                dwDelEntriesNum = dwNumMembers + 1;

                dwError = DirectoryAllocateMemory(
                                 sizeof(pDelMemEntries[0]) * dwDelEntriesNum,
                                 OUT_PPVOID(&pDelMemEntries));
                BAIL_ON_DIRECTORY_ERROR(dwError);

                for (iMember = 0; iMember < dwNumMembers; iMember++)
                {
                    pMember    = &(pMemberEntries[iMember]);

                    dwError = DirectoryGetEntryAttrValueByName(
                                          pMember,
                                          wszAttrPrimaryGroup,
                                          DIRECTORY_ATTR_TYPE_INTEGER,
                                          &dwPrimaryGroup);
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    if (dwGid == dwPrimaryGroup)
                    {
                        dwError = ERROR_MEMBERS_PRIMARY_GROUP;
                        BAIL_ON_DIRECTORY_ERROR(dwError);
                    }

                    dwError = DirectoryGetEntryAttrValueByName(
                                          pMember,
                                          wszAttrDn,
                                          DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                          &pwszMemberDn);
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    dwError = DirectoryAllocateMemory(
                                          sizeof(DIRECTORY_ATTRIBUTE),
                                          OUT_PPVOID(&pAttribute));
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    dwError = DirectoryAllocateStringW(
                                          wszAttrDn,
                                          &pAttribute[0].pwszName);
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    dwError = DirectoryAllocateMemory(
                                          sizeof(ATTRIBUTE_VALUE),
                                          OUT_PPVOID(&pAttrValue));
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    pAttrValue[0].Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;

                    dwError = DirectoryAllocateStringW(
                                          pwszMemberDn,
                                          &pAttrValue[0].data.pwszStringValue);
                    BAIL_ON_DIRECTORY_ERROR(dwError);

                    pAttribute[0].ulNumValues = 1;
                    pAttribute[0].pValues     = pAttrValue;

                    pDelMemEntries[iMember].ulNumAttributes = 1;
                    pDelMemEntries[iMember].pAttributes     = pAttribute;

                    pAttribute = NULL;
                    pAttrValue = NULL;
                }

                /*
                 * Provider function can be called directly here since
                 * we've done all validity checking already (members being
                 * removed are in this group for sure and it is no one's
                 * primary group)
                 */
                dwError = pProvider->pProviderFnTbl->pfnDirectoryRemoveFromGroup(
                                      pContext->hBindHandle,
                                      pwszObjectDN,
                                      pDelMemEntries);
                BAIL_ON_DIRECTORY_ERROR(dwError);
            }
        }
        /*
         * If an object being removed is a user remove it
         * from all groups it is member of
         */
        else if (dwObjectClass == DIR_OBJECT_CLASS_USER)
        {
            dwError = DirectoryGetMemberships(
                                  hDirectory,
                                  pwszObjectDN,
                                  wszMembershipAttributes,
                                  &pMembershipEntries,
                                  &dwNumGroups);
            BAIL_ON_DIRECTORY_ERROR(dwError);

            for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
            {
                pGroup = &(pMembershipEntries[iGroup]);

                dwError = DirectoryGetEntryAttrValueByName(
                                      pGroup,
                                      wszAttrDn,
                                      DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                      &pwszGroupDn);
                BAIL_ON_DIRECTORY_ERROR(dwError);

                /*
                 * Provider function can be called directly here since
                 * we've done all validity checking already (a member being
                 * removed is in this group for sure)
                 */
                dwError = pProvider->pProviderFnTbl->pfnDirectoryRemoveFromGroup(
                                      pContext->hBindHandle,
                                      pwszGroupDn,
                                      pEntry);
                BAIL_ON_DIRECTORY_ERROR(dwError);
            }
        }
    }
    else
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pProvider->pProviderFnTbl->pfnDirectoryDelete(
                    pContext->hBindHandle,
                    pwszObjectDN);

error:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (pMemberEntries)
    {
        DirectoryFreeEntries(pMemberEntries, dwNumMembers);
    }

    if (pMembershipEntries)
    {
        DirectoryFreeEntries(pMembershipEntries, dwNumGroups);
    }

    if (pDelMemEntries)
    {
        DirectoryFreeEntries(pDelMemEntries, dwDelEntriesNum);
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
