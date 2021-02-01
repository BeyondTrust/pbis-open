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
 *        samdbattrlookup.c
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
 *
 *      SAM object attributes lookup routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

static
int
SamDbCompareAttributeLookupKeys(
    PVOID pKey1,
    PVOID pKey2
    );

DWORD
SamDbAttributeLookupInitContents(
    PSAM_DB_ATTR_LOOKUP   pAttrLookup,
    PSAM_DB_ATTRIBUTE_MAP pAttrMap,
    DWORD                 dwNumMaps
    )
{
    DWORD dwError = 0;
    PLWRTL_RB_TREE pLookupTable = NULL;
    DWORD iAttr = 0;

    memset(pAttrLookup, 0, sizeof(SAM_DB_ATTR_LOOKUP));

    dwError = LwRtlRBTreeCreate(
                    &SamDbCompareAttributeLookupKeys,
                    NULL,
                    NULL,
                    &pLookupTable);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; iAttr < dwNumMaps; iAttr++)
    {
        PSAM_DB_ATTRIBUTE_MAP pMap = &pAttrMap[iAttr];

        dwError = LwRtlRBTreeAdd(
                       pLookupTable,
                       pMap->wszDirectoryAttribute,
                       pMap);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pAttrLookup->pLookupTable = pLookupTable;

cleanup:

    return dwError;

error:

    if (pLookupTable)
    {
        LwRtlRBTreeFree(pLookupTable);
    }

    goto cleanup;
}

DWORD
SamDbAttributeLookupByName(
    PSAM_DB_ATTR_LOOKUP    pAttrLookup,
    PWSTR                  pwszAttrName,
    PSAM_DB_ATTRIBUTE_MAP* ppAttrMap
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;

    ntStatus = LwRtlRBTreeFind(
                    pAttrLookup->pLookupTable,
                    pwszAttrName,
                    (PVOID*)&pAttrMap);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        dwError = LW_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppAttrMap = pAttrMap;

cleanup:

    return dwError;

error:

    *ppAttrMap = NULL;

    goto cleanup;
}

VOID
SamDbAttributeLookupFreeContents(
    PSAM_DB_ATTR_LOOKUP pAttrLookup
    )
{
    if (pAttrLookup->pLookupTable)
    {
        LwRtlRBTreeFree(pAttrLookup->pLookupTable);
        pAttrLookup->pLookupTable = NULL;
    }
}

static
int
SamDbCompareAttributeLookupKeys(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PWSTR pwszKey1 = (PWSTR)pKey1;
    PWSTR pwszKey2 = (PWSTR)pKey2;

    return wc16scasecmp(pwszKey1, pwszKey2);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
