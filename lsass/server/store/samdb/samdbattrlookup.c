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
 *        samdbattrlookup.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
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
