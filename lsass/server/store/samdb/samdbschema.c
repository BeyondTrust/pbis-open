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
 *        samdbschema.c
 *
 * Abstract:
 *
 *        Likewise SAM DB
 *
 *        Schema Validation
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
BOOLEAN
SamDbIsMatchedType(
    DIRECTORY_ATTR_TYPE dirAttrType,
    SAMDB_ATTR_TYPE     dbAttrType
    );

DWORD
SamDbSchemaAddValidateDirMods(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          mods[]
    )
{
    DWORD dwError = 0;
    DWORD iAttrMap = 0;
    PSTR  pszAttrName = NULL;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pMapInfo = NULL;

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    //
    // For each mandatory attribute in the
    // schema for a  given object type
    // ensure that the DirModification has the appropriate
    // attribute, type and value
    for (; iAttrMap < pMapInfo->dwNumMaps; iAttrMap++)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo = NULL;

        pAttrMapInfo = &pMapInfo->pAttributeMaps[iAttrMap];

        if (pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_MANDATORY)
        {
            PSAM_DB_ATTRIBUTE_MAP pAttrMap = NULL;
            PDIRECTORY_MOD pMod = NULL;
            DWORD dwNumMods = 0;
            DWORD iValue = 0;

            // Find expected properties of this attribute
            dwError = SamDbAttributeLookupByName(
                            pDirectoryContext->pAttrLookup,
                            &pAttrMapInfo->wszAttributeName[0],
                            &pAttrMap);
            BAIL_ON_SAMDB_ERROR(dwError);

            // Ensure the incoming mods have only one instance of this attribute
            while (mods[dwNumMods].pwszAttrName)
            {
                if (!wc16scasecmp(mods[dwNumMods].pwszAttrName,
                                  &pAttrMapInfo->wszAttributeName[0]))
                {
                    if (!pAttrMap->bIsMultiValued && pMod)
                    {
                        DWORD dwError2 = 0;

                        LW_SAFE_FREE_STRING(pszAttrName);

                        dwError2 = LwWc16sToMbs(
                                        &pAttrMapInfo->wszAttributeName[0],
                                        &pszAttrName);
                        if (dwError2 == LW_ERROR_SUCCESS)
                        {
                            SAMDB_LOG_ERROR(
                                    "Found multi-valued attribute [%s]."
                                    "Expected single value",
                                    pszAttrName);
                        }

                        dwError = LW_ERROR_INVALID_PARAMETER;
                        BAIL_ON_SAMDB_ERROR(dwError);
                    }

                    pMod = &mods[dwNumMods];
                }

                dwNumMods++;
            }

            // We could have an attribute that is always provided by the db
            // For instance, the rowid. The user will not specify these values.
            // However, these values are mandatory for the object to exist.
            if ((!pMod || (pMod && !pMod->ulNumValues)) &&
                !(pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED) &&
                !(pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATE_ALWAYS) &&
                !(pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB))
            {
                DWORD dwError2 = 0;

                LW_SAFE_FREE_STRING(pszAttrName);

                dwError2 = LwWc16sToMbs(
                                &pAttrMapInfo->wszAttributeName[0],
                                &pszAttrName);
                if (dwError2 == LW_ERROR_SUCCESS)
                {
                    SAMDB_LOG_ERROR(
                            "Mandatory attribute [%s] was not specified.",
                            pszAttrName);
                }

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            // Match Types
            // TODO: Match multi-valued types against expectations
            for (; pMod && iValue < pMod->ulNumValues; iValue++)
            {
                PATTRIBUTE_VALUE pAttrValue = &pMod->pAttrValues[iValue];

                if (!SamDbIsMatchedType(
                        pAttrValue->Type,
                        pAttrMap->attributeType))
                {
                    DWORD dwError2 = 0;

                    LW_SAFE_FREE_STRING(pszAttrName);

                    dwError2 = LwWc16sToMbs(
                                    &pAttrMapInfo->wszAttributeName[0],
                                    &pszAttrName);
                    if (dwError2 == LW_ERROR_SUCCESS)
                    {
                        SAMDB_LOG_ERROR(
                                "Mismatched type specified for attribute [%s]."
                                "Expected [%d] Found [%d]",
                                pszAttrName,
                                pAttrValue->Type,
                                pAttrMap->attributeType);
                    }

                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                if (((pAttrValue->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) &&
                     !pAttrValue->data.pwszStringValue) ||
                    ((pAttrValue->Type == DIRECTORY_ATTR_TYPE_ANSI_STRING) &&
                     !pAttrValue->data.pszStringValue))
                {
                    DWORD dwError2 = 0;

                    LW_SAFE_FREE_STRING(pszAttrName);

                    dwError2 = LwWc16sToMbs(
                                    &pAttrMapInfo->wszAttributeName[0],
                                    &pszAttrName);
                    if (dwError2 == LW_ERROR_SUCCESS)
                    {
                        SAMDB_LOG_ERROR(
                                "An invalid string value was specified for"
                                " attribute [%s].",
                                pszAttrName);
                    }

                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }
            }
        }
    }

cleanup:

    LW_SAFE_FREE_STRING(pszAttrName);

    return dwError;

error:

    goto cleanup;
}

DWORD
SamDbSchemaModifyValidateDirMods(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          mods[]
    )
{
    DWORD dwError = 0;
    DWORD dwNumMods = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pMapInfo = NULL;

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    // Make sure we are not trying to set read-only attributes
    while (mods[dwNumMods].pwszAttrName && mods[dwNumMods].pAttrValues)
    {
        PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo = NULL;
        DWORD iAttrMap = 0;

        for (; iAttrMap < pMapInfo->dwNumMaps; iAttrMap++)
        {
            PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfoIter = NULL;

            pAttrMapInfoIter = &pMapInfo->pAttributeMaps[iAttrMap];

            if (!wc16scasecmp(mods[dwNumMods].pwszAttrName,
                              &pAttrMapInfoIter->wszAttributeName[0]))
            {
                pAttrMapInfo = pAttrMapInfoIter;

                break;
            }
        }

        if (!pAttrMapInfo ||
            (pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_READONLY) ||
            (pAttrMapInfo->dwAttributeFlags & SAM_DB_ATTR_FLAGS_GENERATED_BY_DB))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwNumMods++;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
BOOLEAN
SamDbIsMatchedType(
    DIRECTORY_ATTR_TYPE dirAttrType,
    SAMDB_ATTR_TYPE     dbAttrType
    )
{
    BOOLEAN bResult = FALSE;

    switch (dirAttrType)
    {
        case DIRECTORY_ATTR_TYPE_BOOLEAN :
        case DIRECTORY_ATTR_TYPE_INTEGER :

            if (dbAttrType == SAMDB_ATTR_TYPE_INT32)
            {
                bResult = TRUE;
            }

            break;

        case DIRECTORY_ATTR_TYPE_LARGE_INTEGER :

            if (dbAttrType == SAMDB_ATTR_TYPE_INT64)
            {
                bResult = TRUE;
            }

            break;

        case DIRECTORY_ATTR_TYPE_OCTET_STREAM :
            if (dbAttrType == SAMDB_ATTR_TYPE_BLOB)
            {
                bResult = TRUE;
            }

            break;

        case DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR:
            if (dbAttrType == SAMDB_ATTR_TYPE_SECURITY_DESCRIPTOR)
            {
                bResult = TRUE;
            }

            break;

        case DIRECTORY_ATTR_TYPE_UNICODE_STRING :
        case DIRECTORY_ATTR_TYPE_ANSI_STRING :

            if (dbAttrType == SAMDB_ATTR_TYPE_TEXT)
            {
                bResult = TRUE;
            }

            break;

        default:

            break;
    }

    return bResult;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
