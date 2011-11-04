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
 *        samdbmodify.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects modification routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

typedef DWORD (*PFN_SAMDB_ATTRIBUTE_MODIFIER)(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pModifications,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    );

typedef struct _SAMDB_ATTRIBUTE_MODIFIER
{
    PSTR                          pszDbColName;
    PFN_SAMDB_ATTRIBUTE_MODIFIER  pfnAttrModifier;

} SAMDB_ATTRIBUTE_MODIFIER, *PSAMDB_ATTRIBUTE_MODIFIER;

typedef struct _SAMDB_OBJECTCLASS_MODIFIER
{
    PSAMDB_ATTRIBUTE_MODIFIER   pModifiers;

} SAMDB_OBJECTCLASS_MODIFIER, *PSAMDB_OBJECTCLASS_MODIFIER;


static
DWORD
SamDbModifyDomainName(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pModification,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    );


static
DWORD
SamDbModifyMachineDC(
    PCSTR  pszDN,
    PCSTR  pszMachineName,
    PCSTR  pszNewMachineName,
    PSTR  *ppszNewDN
    );


static
DWORD
SamDbModifyDomainSid(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDomainDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pDomainObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    );


static
DWORD
SamDbModifyObjectSid(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    );


static
DWORD
SamDbModifyAttributes(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pModification,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    );


static SAMDB_ATTRIBUTE_MODIFIER gSamDbUnknownObjectModifiers[] =
{
    { NULL,                     NULL }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbDomainObjectModifiers[] =
{
    { SAM_DB_COL_DOMAIN,        SamDbModifyDomainName },
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyDomainSid },
    { NULL,                     SamDbModifyAttributes }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbBuiltinDomainObjectModifiers[] =
{
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyObjectSid },
    { NULL,                     SamDbModifyAttributes }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbContainerObjectModifiers[] =
{
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyObjectSid },
    { NULL,                     SamDbModifyAttributes }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbLocalGroupObjectModifiers[] =
{
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyObjectSid },
    { NULL,                     SamDbModifyAttributes }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbUserObjectModifiers[] =
{
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyObjectSid },
    { NULL,                     SamDbModifyAttributes }
};

static SAMDB_ATTRIBUTE_MODIFIER gSamDbLocalGroupMemberObjectModifiers[] =
{
    { SAM_DB_COL_OBJECT_SID,    SamDbModifyObjectSid },
    { NULL,                     SamDbModifyAttributes }
};

static PSAMDB_ATTRIBUTE_MODIFIER gSamDbObjectClassModifiers[] =
{
    /* SAMDB_OBJECT_CLASS_UNKNOWN */
    &gSamDbUnknownObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_DOMAIN */
    &gSamDbDomainObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN */
    &gSamDbBuiltinDomainObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_CONTAINER */
    &gSamDbContainerObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_LOCAL_GROUP */
    &gSamDbLocalGroupObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_USER */
    &gSamDbUserObjectModifiers[0],

    /* SAMDB_OBJECT_CLASS_LOCALGRP_MEMBER */
    &gSamDbLocalGroupMemberObjectModifiers[0]
};


static
DWORD
SamDbUpdateObjectInDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    );

static
DWORD
SamDbUpdateBuildObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAM_DB_DN                          pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbUpdateBuildColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    );

static
DWORD
SamDbUpdateBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    );

DWORD
SamDbModifyObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    LONG64 llObjectRecordId = 0;
    PSTR   pszObjectDN = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;

    dwError = LwWc16sToMbs(
                    pwszObjectDN,
                    &pszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbGetObjectRecordInfo(
                    pDirectoryContext,
                    pszObjectDN,
                    &llObjectRecordId,
                    &objectClass);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSchemaModifyValidateDirMods(
                    pDirectoryContext,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateObjectInDatabase(
                    pDirectoryContext,
                    pwszObjectDN,
                    objectClass,
                    modifications);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbIncrementSequenceNumber(
                    pDirectoryContext);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);

   return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbUpdateObjectInDatabase(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PWSTR                  pwszObjectDN,
    SAMDB_OBJECT_CLASS     objectClass,
    DIRECTORY_MOD          modifications[]
    )
{
    DWORD dwError = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo = NULL;
    PSAM_DB_DN pDN = NULL;
    DWORD dwModCount = 0;
    DWORD iMod = 0;
    PDIRECTORY_MOD pMods = NULL;
    PDIRECTORY_MOD pModsApplied = NULL;
    DWORD dwNumModsApplied = 0;
    DWORD iModifier = 0;
    PSAMDB_ATTRIBUTE_MODIFIER pModifiers = NULL;
    PWSTR pwszAttrName = NULL;
    DWORD iModApplied = 0;
    DWORD iModCurrent = 0;
    DWORD dwOffset = 0;

    if (!pDirectoryContext || !pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbParseDN(
                pwszObjectDN,
                &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbFindObjectClassMapInfo(
                    objectClass,
                    pDirectoryContext->pObjectClassAttrMaps,
                    pDirectoryContext->dwNumObjectClassAttrMaps,
                    &pObjectClassMapInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    /*
     * Allocate the same amount of space for pMods as calculated
     * from modifications. pMods is going to be used as a working
     * copy.
     */
    while (modifications[dwModCount].pwszAttrName)
    {
        dwModCount++;
    }

    dwError = DirectoryAllocateMemory(sizeof(pMods[0]) * (dwModCount + 1),
                                      OUT_PPVOID(&pMods));
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iMod = 0; iMod < dwModCount; iMod++)
    {
        pMods[iMod] = modifications[iMod];
    }

    /*
     * Get modifier functions table for given object class
     */
    pModifiers = gSamDbObjectClassModifiers[objectClass];

    for (iModifier = 0;
         pModifiers[iModifier].pszDbColName != NULL;
         iModifier++)
    {
        dwError = LwMbsToWc16s(pModifiers[iModifier].pszDbColName,
                                &pwszAttrName);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (iMod = 0; iMod < dwModCount; iMod++)
        {
            if (wc16scasecmp(pMods[iMod].pwszAttrName, pwszAttrName))
            {
                continue;
            }

            dwError = pModifiers[iModifier].pfnAttrModifier(
                                               pDirectoryContext,
                                               pDN,
                                               pObjectClassMapInfo,
                                               pMods,
                                               &pModsApplied,
                                               &dwNumModsApplied);
            BAIL_ON_SAMDB_ERROR(dwError);

            /*
             * Remove mods that have been applied from pMods, so they
             * are no longer used if more than one modification is needed
             * (i.e. there are other attributes to be changed)
             */
            if (dwNumModsApplied > 0)
            {
                for (iModApplied = 0;
                     iModApplied < dwNumModsApplied;
                     iModApplied++)
                {
                    /* Count until dwModCount + 1 to reach the null
                       mod termination */
                    for (iModCurrent = 0;
                         iModCurrent < dwModCount + 1;
                         iModCurrent++)
                    {
                        if (pModsApplied[iModApplied].pwszAttrName ==
                            pMods[iModCurrent].pwszAttrName)
                        {
                            dwOffset++;
                            dwModCount--;
                        }

                        if (dwOffset)
                        {
                            pMods[iModCurrent] = pMods[iModCurrent + dwOffset];
                        }
                    }

                    dwOffset = 0;
                }
            }

            DIRECTORY_FREE_MEMORY(pModsApplied);

            pModsApplied     = NULL;
            dwNumModsApplied = 0;
        }

        DIRECTORY_FREE_MEMORY(pwszAttrName);
        pwszAttrName = NULL;
    }

    if (dwModCount > 0 &&
        pMods[0].pwszAttrName != NULL &&
        pModifiers[iModifier].pfnAttrModifier)
    {
        dwError = pModifiers[iModifier].pfnAttrModifier(
                                           pDirectoryContext,
                                           pDN,
                                           pObjectClassMapInfo,
                                           pMods,
                                           &pModsApplied,
                                           &dwNumModsApplied);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

cleanup:
    DIRECTORY_FREE_MEMORY(pMods);
    DIRECTORY_FREE_MEMORY(pwszAttrName);
    DIRECTORY_FREE_MEMORY(pModsApplied);

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbModifyDomainName(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDomainDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pDomainObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const wchar_t wszAccountFilterFmt[] = L"%ws='%ws' AND (%ws=%d OR %ws=%d)";
    const wchar_t wszAnyObjectFilterFmt[] = L"%ws>0";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszDomainDN = NULL;
    PSTR pszDomainQuery = NULL;
    BOOLEAN bTxStarted = FALSE;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DB_COLUMN_VALUE pDomainColumnValueList = NULL;
    DWORD dwNumModified = 0;
    PDIRECTORY_MOD pModified = NULL;
    DWORD iModified = 0;
    DWORD iMod = 0;
    DWORD dwNumMods = 0;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszDomainFilter = NULL;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD iAttr = 0;
    PWSTR pwszOldDomainName = NULL;
    PWSTR pwszNewDomainName = NULL;
    PWSTR pwszCmpDomainName = NULL;
    HANDLE hDirectory = (HANDLE)pDirectoryContext;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    DWORD dwAccountFilterLen = 0;
    PWSTR pwszAccountFilter = NULL;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pAccountEntries = NULL;
    PDIRECTORY_ENTRY pAccountEntry = NULL;
    DWORD dwNumAccountEntries = 0;
    DWORD iEntry = 0;
    PSAM_DB_DN pAccountDN = NULL;
    PWSTR pwszAccountDN = NULL;
    PSTR pszAccountDN = NULL;
    DWORD dwAccountObjectClass = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pAccountClassMapInfo = NULL;
    PSTR pszAccountQuery = NULL;
    PSAM_DB_COLUMN_VALUE pAccountColumnValueList = NULL;
    DWORD dwAnyObjectFilterLen = 0;
    PWSTR pwszAnyObjectFilter = NULL;
    PDIRECTORY_ENTRY pObjectEntries = NULL;
    PDIRECTORY_ENTRY pObjectEntry = NULL;
    DWORD dwNumObjectEntries = 0;
    PWSTR pwszObjectDN = NULL;
    PWSTR pwszParentDN = NULL;
    DWORD dwObjectClass = 0;
    PSTR pszOldDomainName = NULL;
    PSTR pszNewDomainName = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszNewObjectDN = NULL;
    PSTR pszParentDN = NULL;
    PSTR pszNewParentDN = NULL;
    PSAM_DB_DN pObjectDN = NULL;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo = NULL;
    PSTR pszObjectQuery = NULL;
    PSAM_DB_COLUMN_VALUE pObjectColumnValueList = NULL;

    WCHAR wszAttrRecordId[] = SAM_DB_DIR_ATTR_RECORD_ID;
    WCHAR wszAttrObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrParentDN[] = SAM_DB_DIR_ATTR_PARENT_DN;
    WCHAR wszAttrDomainName[] = SAM_DB_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNetbiosName[] = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectClass[0],
        &wszAttrObjectDN[0],
        &wszAttrParentDN[0],
        &wszAttrDomainName[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_DOMAIN_NAME  = 0,
        ATTR_IDX_NETBIOS_NAME,
        ATTR_IDX_COMMON_NAME,
        ATTR_IDX_SAM_ACCOUNT_NAME,
        ATTR_IDX_OBJECT_DN,
        ATTR_IDX_PARENT_DN,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_IDX_DOMAIN_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_NETBIOS_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_COMMON_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_SAM_ACCOUNT_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_IDX_OBJECT_DN */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        },
        {   /* ATTR_IDX_PARENT_DN */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        }
    };

    DIRECTORY_MOD modDomainName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrDomainName[0],
        1,
        &AttrValues[ATTR_IDX_DOMAIN_NAME]
    };

    DIRECTORY_MOD modNetbiosName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrNetbiosName[0],
        1,
        &AttrValues[ATTR_IDX_NETBIOS_NAME]
    };

    DIRECTORY_MOD modCommonName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrCommonName[0],
        1,
        &AttrValues[ATTR_IDX_COMMON_NAME]
    };

    DIRECTORY_MOD modSamAccountName = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrSamAccountName[0],
        1,
        &AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD modObjectDN = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectDN[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_DN]
    };

    DIRECTORY_MOD modParentDN = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrParentDN[0],
        1,
        &AttrValues[ATTR_IDX_PARENT_DN]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    /*
     * Get new domain name from the mods
     */
    while (pMods[iMod].pwszAttrName)
    {
        if (pMods[iMod].pAttrValues == NULL ||
            pMods[iMod].ulNumValues == 0 ||
            pMods[iMod].ulOperationFlags != DIR_MOD_FLAGS_REPLACE)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (pwszNewDomainName == NULL &&
            (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrDomainName) == 0 ||
             wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrNetbiosName) == 0 ||
             wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrCommonName) == 0 ||
             wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrSamAccountName) == 0))
        {
            switch (pMods[iMod].pAttrValues[0].Type)
            {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                dwError = LwAllocateWc16String(
                               &pwszNewDomainName,
                               pMods[iMod].pAttrValues[0].data.pwszStringValue);
                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                dwError = LwMbsToWc16s(
                               pMods[iMod].pAttrValues[0].data.pszStringValue,
                               &pwszNewDomainName);
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        iMod++;
    }

    dwNumMods = iMod;

    /*
     * Validate the mods - Domain, Netbios, Common and SamAccount names
     * (if passed) have to be set to the same value
     */

    for (iMod = 0; iMod < dwNumMods; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrDomainName) == 0 ||
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrNetbiosName) == 0 ||
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrCommonName) == 0 ||
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrSamAccountName) == 0)
        {
            switch (pMods[iMod].pAttrValues[0].Type)
            {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                dwError = LwAllocateWc16String(
                               &pwszCmpDomainName,
                               pMods[iMod].pAttrValues[0].data.pwszStringValue);
                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                dwError = LwMbsToWc16s(
                               pMods[iMod].pAttrValues[0].data.pszStringValue,
                               &pwszCmpDomainName);
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_SAMDB_ERROR(dwError);

            if (wc16scasecmp(pwszCmpDomainName,
                             pwszNewDomainName))
            {
                /*
                 * Inconsistency found - one of requested domain name attributes
                 * is not the same as the others
                 */
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
            }

            DIRECTORY_FREE_MEMORY(pwszCmpDomainName);
            pwszCmpDomainName = NULL;
        }
    }

    /*
     * Search for existing domain object first to get the old name
     */

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszDomainFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszDomainFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pDomainEntries,
                                &dwNumDomainEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomainEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pDomainEntry = &(pDomainEntries[0]);

    for (iAttr = 0; iAttr < pDomainEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttribute = &(pDomainEntry->pAttributes[iAttr]);

        if (wc16scasecmp(pAttribute->pwszName, wszAttrDomainName) == 0)
        {
            pwszOldDomainName = pAttribute->pValues[0].data.pwszStringValue;
        }
    }

    /*
     * Modify the domain object
     */

    iMod = 0;
    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_DOMAIN_NAME].data.pwszStringValue  = pwszNewDomainName;
    AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue = pwszNewDomainName;
    AttrValues[ATTR_IDX_COMMON_NAME].data.pwszStringValue  = pwszNewDomainName;
    AttrValues[ATTR_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszNewDomainName;

    mods[iMod++] = modDomainName;
    mods[iMod++] = modNetbiosName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modSamAccountName;

    dwError = LwWc16sToMbs(
                   pDomainDN->pwszDN,
                   &pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateBuildObjectQuery(
                    pDirectoryContext,
                    pDomainDN,
                    pDomainObjectClassMapInfo,
                    mods,
                    &pszDomainQuery,
                    &pDomainColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszDomainQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = SamDbUpdateBindValues(
                    pDirectoryContext,
                    pszDomainDN,
                    pDomainColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
        pSqlStatement = NULL;
    }

    /*
     * Prepare the applied mods to return to the caller
     */
    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrDomainName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrNetbiosName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrCommonName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrSamAccountName))
        {
            continue;
        }

        dwNumModified++;
    }

    dwError = DirectoryAllocateMemory(sizeof(pModified[0]) * dwNumModified,
                                      OUT_PPVOID(&pModified));
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrDomainName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrNetbiosName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrCommonName) &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrSamAccountName))
        {
            continue;
        }

        pModified[iModified++] = pMods[iMod];
    }

    /*
     * Search and modify local domain accounts
     */

    dwAccountFilterLen = ((sizeof(wszAttrDomainName)/sizeof(WCHAR) - 1) +
                          wc16slen(pwszOldDomainName) +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAccountFilterFmt)/
                           sizeof(wszAccountFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwAccountFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszAccountFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszAccountFilter, dwAccountFilterLen, wszAccountFilterFmt,
                    &wszAttrDomainName[0],
                    pwszOldDomainName,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_USER,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_LOCAL_GROUP) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszAccountFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pAccountEntries,
                                &dwNumAccountEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumAccountEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumAccountEntries; iEntry++)
    {
        pAccountEntry = &(pAccountEntries[iEntry]);

        for (iAttr = 0; iAttr < pAccountEntry->ulNumAttributes; iAttr++)
        {
            PDIRECTORY_ATTRIBUTE pAttribute = &(pAccountEntry->pAttributes[iAttr]);

            if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectDN) == 0)
            {
                pwszAccountDN = pAttribute->pValues[0].data.pwszStringValue;
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectClass) == 0)
            {
                dwAccountObjectClass = pAttribute->pValues[0].data.ulValue;
            }
        }

        iMod = 0;
        memset(&mods[0], 0, sizeof(mods));

        AttrValues[ATTR_IDX_DOMAIN_NAME].data.pwszStringValue
            = pwszNewDomainName;
        AttrValues[ATTR_IDX_NETBIOS_NAME].data.pwszStringValue
            = pwszNewDomainName;

        mods[iMod++] = modDomainName;
        mods[iMod++] = modNetbiosName;

        dwError = SamDbParseDN(pwszAccountDN,
                               &pAccountDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszAccountDN,
                                &pszAccountDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbFindObjectClassMapInfo(
                        dwAccountObjectClass,
                        pDirectoryContext->pObjectClassAttrMaps,
                        pDirectoryContext->dwNumObjectClassAttrMaps,
                        &pAccountClassMapInfo);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbUpdateBuildObjectQuery(
                        pDirectoryContext,
                        pAccountDN,
                        pAccountClassMapInfo,
                        mods,
                        &pszAccountQuery,
                        &pAccountColumnValueList);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszAccountQuery,
                        -1,
                        &pSqlStatement,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError,
                                      pDirectoryContext->pDbContext->pDbHandle);

        dwError = SamDbUpdateBindValues(
                        pDirectoryContext,
                        pszAccountDN,
                        pAccountColumnValueList,
                        pSqlStatement);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_step(pSqlStatement);
        if (dwError == SQLITE_DONE)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        if (pSqlStatement)
        {
            sqlite3_finalize(pSqlStatement);
            pSqlStatement = NULL;
        }

        if (pAccountDN)
        {
            SamDbFreeDN(pAccountDN);
            pAccountDN = NULL;
        }

        if (pAccountColumnValueList)
        {
            SamDbFreeColumnValueList(pAccountColumnValueList);
            pAccountColumnValueList = NULL;
        }

        DIRECTORY_FREE_STRING(pszAccountDN);
        DIRECTORY_FREE_STRING(pszAccountQuery);

        pszAccountDN        = NULL;
        pszAccountQuery     = NULL;
    }
    
    /*
     * Get all objects and update DN wherever it includes "DC=MACHINE"
     */

    dwAnyObjectFilterLen = ((sizeof(wszAttrRecordId)/sizeof(WCHAR) - 1) +
                            dwInt32StrSize +
                            (sizeof(wszAnyObjectFilterFmt)/
                             sizeof(wszAnyObjectFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwAnyObjectFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszAnyObjectFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszAnyObjectFilter, dwAnyObjectFilterLen,
                    wszAnyObjectFilterFmt,
                    &wszAttrRecordId[0]) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszAnyObjectFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pObjectEntries,
                                &dwNumObjectEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumObjectEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumObjectEntries; iEntry++)
    {
        pObjectEntry = &(pObjectEntries[iEntry]);

        for (iAttr = 0; iAttr < pObjectEntry->ulNumAttributes; iAttr++)
        {
            PDIRECTORY_ATTRIBUTE pAttribute = &(pObjectEntry->pAttributes[iAttr]);

            if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectDN) == 0)
            {
                pwszObjectDN = pAttribute->pValues[0].data.pwszStringValue;
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrParentDN) == 0)
            {
                if (pAttribute->ulNumValues > 0 &&
                    pAttribute->pValues)
                {
                    pwszParentDN = pAttribute->pValues[0].data.pwszStringValue;
                }
                else
                {
                    pwszParentDN = NULL;
                }
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectClass) == 0)
            {
                dwObjectClass = pAttribute->pValues[0].data.ulValue;
            }
        }

        dwError = LwWc16sToMbs(pwszOldDomainName,
                                &pszOldDomainName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszNewDomainName,
                                &pszNewDomainName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszObjectDN, &pszObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbModifyMachineDC(pszObjectDN,
                                       pszOldDomainName,
                                       pszNewDomainName,
                                       &pszNewObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (pwszParentDN)
        {
            dwError = LwWc16sToMbs(pwszParentDN, &pszParentDN);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbModifyMachineDC(pszParentDN,
                                           pszOldDomainName,
                                           pszNewDomainName,
                                           &pszNewParentDN);
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (pszNewObjectDN || pszNewParentDN)
        {
            iMod = 0;
            memset(&mods[0], 0, sizeof(mods));

            AttrValues[ATTR_IDX_OBJECT_DN].data.pszStringValue = pszNewObjectDN;
            AttrValues[ATTR_IDX_PARENT_DN].data.pszStringValue = pszNewParentDN;

            if (pszNewObjectDN)
            {
                mods[iMod++] = modObjectDN;
            }
            if (pszNewParentDN)
            {
                mods[iMod++] = modParentDN;
            }

            dwError = SamDbParseDN(pwszObjectDN,
                                   &pObjectDN);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbFindObjectClassMapInfo(
                            dwObjectClass,
                            pDirectoryContext->pObjectClassAttrMaps,
                            pDirectoryContext->dwNumObjectClassAttrMaps,
                            &pObjectClassMapInfo);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = SamDbUpdateBuildObjectQuery(
                            pDirectoryContext,
                            pObjectDN,
                            pObjectClassMapInfo,
                            mods,
                            &pszObjectQuery,
                            &pObjectColumnValueList);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = sqlite3_prepare_v2(
                            pDirectoryContext->pDbContext->pDbHandle,
                            pszObjectQuery,
                            -1,
                            &pSqlStatement,
                            NULL);
            BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError,
                                          pDirectoryContext->pDbContext->pDbHandle);

            dwError = SamDbUpdateBindValues(
                            pDirectoryContext,
                            pszObjectDN,
                            pObjectColumnValueList,
                            pSqlStatement);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = sqlite3_step(pSqlStatement);
            if (dwError == SQLITE_DONE)
            {
                dwError = LW_ERROR_SUCCESS;
            }
            BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

            if (pSqlStatement)
            {
                sqlite3_finalize(pSqlStatement);
                pSqlStatement = NULL;
            }

            if (pObjectDN)
            {
                SamDbFreeDN(pObjectDN);
                pObjectDN = NULL;
            }

            if (pObjectColumnValueList)
            {
                SamDbFreeColumnValueList(pObjectColumnValueList);
                pObjectColumnValueList = NULL;
            }

            DIRECTORY_FREE_STRING(pszObjectQuery);
            pszObjectQuery = NULL;
        }

        DIRECTORY_FREE_STRING(pszOldDomainName);
        DIRECTORY_FREE_STRING(pszNewDomainName);
        DIRECTORY_FREE_STRING(pszObjectDN);
        DIRECTORY_FREE_STRING(pszNewObjectDN);
        DIRECTORY_FREE_STRING(pszParentDN);
        DIRECTORY_FREE_STRING(pszNewParentDN);

        pszOldDomainName = NULL;
        pszNewDomainName = NULL;
        pszObjectDN = NULL;
        pszNewObjectDN = NULL;
        pszParentDN = NULL;
        pszNewParentDN = NULL;
    }

    *ppModified     = pModified;
    *pdwNumModified = dwNumModified;

cleanup:
    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    if (pDomainColumnValueList)
    {
        SamDbFreeColumnValueList(pDomainColumnValueList);
    }

    if (pAccountColumnValueList)
    {
        SamDbFreeColumnValueList(pAccountColumnValueList);
    }

    if (pObjectColumnValueList)
    {
        SamDbFreeColumnValueList(pObjectColumnValueList);
    }

    if (pAccountDN)
    {
        SamDbFreeDN(pAccountDN);
    }

    if (pObjectDN)
    {
        SamDbFreeDN(pObjectDN);
    }

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pAccountEntries)
    {
        DirectoryFreeEntries(pAccountEntries,
                             dwNumAccountEntries);
    }

    if (pObjectEntries)
    {
        DirectoryFreeEntries(pObjectEntries,
                             dwNumObjectEntries);
    }

    DIRECTORY_FREE_MEMORY(pwszNewDomainName);
    DIRECTORY_FREE_MEMORY(pwszCmpDomainName);
    DIRECTORY_FREE_MEMORY(pwszDomainFilter);
    DIRECTORY_FREE_MEMORY(pwszAccountFilter);
    DIRECTORY_FREE_MEMORY(pwszAnyObjectFilter);
    DIRECTORY_FREE_STRING(pszDomainQuery);
    DIRECTORY_FREE_STRING(pszDomainDN);
    DIRECTORY_FREE_STRING(pszAccountQuery);
    DIRECTORY_FREE_STRING(pszAccountDN);
    DIRECTORY_FREE_STRING(pszOldDomainName);
    DIRECTORY_FREE_STRING(pszNewDomainName);
    DIRECTORY_FREE_STRING(pszObjectQuery);
    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_STRING(pszNewObjectDN);
    DIRECTORY_FREE_STRING(pszParentDN);
    DIRECTORY_FREE_STRING(pszNewParentDN);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamDbModifyMachineDC(
    PCSTR  pszDN,
    PCSTR  pszMachineName,
    PCSTR  pszNewMachineName,
    PSTR  *ppszNewDN
    )
{
    DWORD dwError = 0;
    PSTR pszToken = NULL;
    PSTR pszPreToken = NULL;
    PSTR pszPostToken = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszNewObjectDN = NULL;
    DWORD dwTokenLen = 0;
    DWORD dwPreTokenLen = 0;

    dwError = LwStrDupOrNull(pszDN, &pszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    LwStrStr(pszObjectDN, pszMachineName, &pszToken);

    if (pszToken)
    {
        pszToken[0]   = '\0';
        pszPreToken   = pszObjectDN;
        dwPreTokenLen = strlen(pszPreToken);
        dwTokenLen    = strlen(pszMachineName);
        pszPostToken  = &pszObjectDN[dwPreTokenLen + dwTokenLen];

        dwError = LwAllocateStringPrintf(&pszNewObjectDN,
                                         "%s%s%s",
                                         pszPreToken,
                                         pszNewMachineName,
                                         pszPostToken);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppszNewDN = pszNewObjectDN;

cleanup:
    DIRECTORY_FREE_STRING(pszObjectDN);

    return dwError;

error:
    DIRECTORY_FREE_STRING(pszNewObjectDN);

    *ppszNewDN = NULL;

    goto cleanup;
}


static
DWORD
SamDbModifyDomainSid(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDomainDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pDomainObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const wchar_t wszAccountFilterFmt[] = L"%ws='%ws' AND (%ws=%d OR %ws=%d)";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTxStarted = FALSE;
    sqlite3_stmt* pSqlStatement = NULL;
    DWORD iModified = 0;
    DWORD iMod = 0;
    DWORD dwNumMods = 0;
    PWSTR pwszNewDomainSid = NULL;
    PSID pNewDomainSid = NULL;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszDomainFilter = NULL;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD iAttr = 0;
    PWSTR pwszOldDomainSid = NULL;
    PWSTR pwszDomainName = NULL;
    PSID pOldDomainSid = NULL;
    PSTR pszDomainDN = NULL;
    PSTR pszDomainQuery = NULL;
    DWORD dwNumModified = 0;
    PDIRECTORY_MOD pModified = NULL;
    PSAM_DB_COLUMN_VALUE pDomainColumnValueList = NULL;
    HANDLE hDirectory = (HANDLE)pDirectoryContext;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    DWORD dwAccountFilterLen = 0;
    PWSTR pwszAccountFilter = NULL;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pAccountEntries = NULL;
    PDIRECTORY_ENTRY pAccountEntry = NULL;
    DWORD dwNumAccountEntries = 0;
    DWORD iEntry = 0;
    PWSTR pwszAccountSid = NULL;
    PWSTR pwszAccountDN = NULL;
    PSID pOldAccountSid = NULL;
    DWORD dwSidLength = 0;
    PSID pNewAccountSid = NULL;
    DWORD dwRid = 0;
    PWSTR pwszNewAccountSid = NULL;
    PSAM_DB_DN pAccountDN = NULL;
    PSTR pszAccountDN = NULL;
    DWORD dwAccountObjectClass = 0;
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pAccountClassMapInfo = NULL;
    PSTR pszAccountQuery = NULL;
    PSAM_DB_COLUMN_VALUE pAccountColumnValueList = NULL;

    WCHAR wszAttrObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSid[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrDomainName[] = DIRECTORY_ATTR_DOMAIN_NAME;

    PWSTR wszAttributes[] = {
        &wszAttrObjectClass[0],
        &wszAttrObjectDN[0],
        &wszAttrObjectSid[0],
        &wszAttrDomainName[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_OBJECT_SID  = 0,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_IDX_OBJECT_SID */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectSid = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectSid[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_SID]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    /*
     * Get new domain SID from the mods
     */
    while (pMods[iMod].pwszAttrName)
    {
        if (pMods[iMod].pAttrValues == NULL ||
            pMods[iMod].ulNumValues == 0 ||
            pMods[iMod].ulOperationFlags != DIR_MOD_FLAGS_REPLACE)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (pwszNewDomainSid == NULL &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid) == 0)
        {
            switch (pMods[iMod].pAttrValues[0].Type)
            {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                dwError = LwAllocateWc16String(
                               &pwszNewDomainSid,
                               pMods[iMod].pAttrValues[0].data.pwszStringValue);
                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                dwError = LwMbsToWc16s(
                               pMods[iMod].pAttrValues[0].data.pszStringValue,
                               &pwszNewDomainSid);
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        iMod++;
    }

    dwNumMods = iMod;

    /*
     * Validate the SID
     */

    ntStatus = RtlAllocateSidFromWC16String(&pNewDomainSid,
                                            pwszNewDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }

    /*
     * Search for existing domain object first to get the old SID and name
     */

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszDomainFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszDomainFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pDomainEntries,
                                &dwNumDomainEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomainEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pDomainEntry = &(pDomainEntries[0]);

    for (iAttr = 0; iAttr < pDomainEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttribute = &(pDomainEntry->pAttributes[iAttr]);

        if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectSid) == 0)
        {
            pwszOldDomainSid = pAttribute->pValues[0].data.pwszStringValue;
        }
        else if (wc16scasecmp(pAttribute->pwszName, wszAttrDomainName) == 0)
        {
            pwszDomainName = pAttribute->pValues[0].data.pwszStringValue;
        }
    }

    ntStatus = RtlAllocateSidFromWC16String(&pOldDomainSid,
                                            pwszOldDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /*
     * Modify the domain object
     */

    iMod = 0;
    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewDomainSid;

    mods[iMod++] = modObjectSid;

    dwError = LwWc16sToMbs(
                   pDomainDN->pwszDN,
                   &pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateBuildObjectQuery(
                    pDirectoryContext,
                    pDomainDN,
                    pDomainObjectClassMapInfo,
                    mods,
                    &pszDomainQuery,
                    &pDomainColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszDomainQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = SamDbUpdateBindValues(
                    pDirectoryContext,
                    pszDomainDN,
                    pDomainColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
        pSqlStatement = NULL;
    }

    /*
     * Prepare the applied mods to return to the caller
     */
    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid))
        {
            continue;
        }

        dwNumModified++;
    }

    dwError = DirectoryAllocateMemory(sizeof(pModified[0]) * dwNumModified,
                                      OUT_PPVOID(&pModified));
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid))
        {
            continue;
        }

        pModified[iModified++] = pMods[iMod];
    }

    /*
     * Search and modify local domain accounts
     */

    dwAccountFilterLen = ((sizeof(wszAttrDomainName)/sizeof(WCHAR) - 1) +
                          wc16slen(pwszDomainName) +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                          dwInt32StrSize +
                          (sizeof(wszAccountFilterFmt)/
                           sizeof(wszAccountFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwAccountFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszAccountFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszAccountFilter, dwAccountFilterLen, wszAccountFilterFmt,
                    &wszAttrDomainName[0],
                    pwszDomainName,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_USER,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_LOCAL_GROUP) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszAccountFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pAccountEntries,
                                &dwNumAccountEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumAccountEntries == 0)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    for (iEntry = 0; iEntry < dwNumAccountEntries; iEntry++)
    {
        pAccountEntry = &(pAccountEntries[iEntry]);

        for (iAttr = 0; iAttr < pAccountEntry->ulNumAttributes; iAttr++)
        {
            PDIRECTORY_ATTRIBUTE pAttribute = &(pAccountEntry->pAttributes[iAttr]);

            if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectSid) == 0)
            {
                pwszAccountSid = pAttribute->pValues[0].data.pwszStringValue;
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectDN) == 0)
            {
                pwszAccountDN = pAttribute->pValues[0].data.pwszStringValue;
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectClass) == 0)
            {
                dwAccountObjectClass = pAttribute->pValues[0].data.ulValue;
            }
        }

        /* Account SID has to be valid ... */
        ntStatus = RtlAllocateSidFromWC16String(&pOldAccountSid,
                                                pwszAccountSid);
        if (ntStatus != STATUS_SUCCESS)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        /* ... and it has to be in the same domain as machine SID */
        if (!RtlIsPrefixSid(pOldDomainSid,
                            pOldAccountSid))
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwSidLength = RtlLengthSid(pOldAccountSid);
        dwError = DirectoryAllocateMemory(dwSidLength,
                                          (PVOID*)&pNewAccountSid);
        BAIL_ON_SAMDB_ERROR(dwError);

        ntStatus = RtlCopySid(dwSidLength,
                              pNewAccountSid,
                              pNewDomainSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwRid = pOldAccountSid->SubAuthority[
                                 pOldAccountSid->SubAuthorityCount - 1];
        ntStatus = RtlAppendRidSid(dwSidLength,
                                   pNewAccountSid,
                                   dwRid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        ntStatus = RtlAllocateWC16StringFromSid(&pwszNewAccountSid,
                                                pNewAccountSid);
        if (ntStatus)
        {
            dwError = LW_ERROR_SAM_DATABASE_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        memset(&mods[0], 0, sizeof(mods));

        AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewAccountSid;
        mods[0] = modObjectSid;

        dwError = SamDbParseDN(pwszAccountDN,
                               &pAccountDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszAccountDN,
                                &pszAccountDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbFindObjectClassMapInfo(
                        dwAccountObjectClass,
                        pDirectoryContext->pObjectClassAttrMaps,
                        pDirectoryContext->dwNumObjectClassAttrMaps,
                        &pAccountClassMapInfo);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbUpdateBuildObjectQuery(
                        pDirectoryContext,
                        pAccountDN,
                        pAccountClassMapInfo,
                        mods,
                        &pszAccountQuery,
                        &pAccountColumnValueList);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_prepare_v2(
                        pDirectoryContext->pDbContext->pDbHandle,
                        pszAccountQuery,
                        -1,
                        &pSqlStatement,
                        NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError,
                                      pDirectoryContext->pDbContext->pDbHandle);

        dwError = SamDbUpdateBindValues(
                        pDirectoryContext,
                        pszAccountDN,
                        pAccountColumnValueList,
                        pSqlStatement);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_step(pSqlStatement);
        if (dwError == SQLITE_DONE)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

        if (pSqlStatement)
        {
            sqlite3_finalize(pSqlStatement);
            pSqlStatement = NULL;
        }

        if (pAccountDN)
        {
            SamDbFreeDN(pAccountDN);
            pAccountDN = NULL;
        }

        if (pAccountColumnValueList)
        {
            SamDbFreeColumnValueList(pAccountColumnValueList);
            pAccountColumnValueList = NULL;
        }

        DIRECTORY_FREE_STRING(pszAccountDN);
        DIRECTORY_FREE_STRING(pszAccountQuery);
        RTL_FREE(&pOldAccountSid);
        DIRECTORY_FREE_MEMORY(pNewAccountSid);
        RTL_FREE(&pwszNewAccountSid);

        pszAccountDN     = NULL;
        pszAccountQuery  = NULL;
        pNewAccountSid   = NULL;
    }

    *ppModified     = pModified;
    *pdwNumModified = dwNumModified;

cleanup:
    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    if (pDomainColumnValueList)
    {
        SamDbFreeColumnValueList(pDomainColumnValueList);
    }

    if (pAccountColumnValueList)
    {
        SamDbFreeColumnValueList(pAccountColumnValueList);
    }

    if (pAccountDN)
    {
        SamDbFreeDN(pAccountDN);
    }

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pAccountEntries)
    {
        DirectoryFreeEntries(pAccountEntries,
                             dwNumAccountEntries);
    }

    DIRECTORY_FREE_MEMORY(pwszNewDomainSid);
    DIRECTORY_FREE_MEMORY(pwszDomainFilter);
    DIRECTORY_FREE_MEMORY(pwszAccountFilter);
    DIRECTORY_FREE_STRING(pszDomainQuery);
    RTL_FREE(&pNewDomainSid);
    RTL_FREE(&pOldDomainSid);
    DIRECTORY_FREE_STRING(pszDomainDN);
    DIRECTORY_FREE_STRING(pszAccountQuery);
    DIRECTORY_FREE_STRING(pszAccountDN);
    RTL_FREE(&pOldAccountSid);
    DIRECTORY_FREE_MEMORY(pNewAccountSid);
    RTL_FREE(&pwszNewAccountSid);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamDbModifyObjectSid(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    )
{
    const wchar_t wszDomainFilterFmt[] = L"%ws=%d";
    const wchar_t wszObjectSidFilterFmt[] = L"%ws='%ws'";
    const DWORD dwInt32StrSize = 10;

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTxStarted = FALSE;
    sqlite3_stmt* pSqlStatement = NULL;
    DWORD dwObjectClass = 0;
    PWSTR pwszNewObjectSid = NULL;
    PSID pNewObjectSid = NULL;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    DWORD dwDomainFilterLen = 0;
    PWSTR pwszDomainFilter = NULL;
    HANDLE hDirectory = (HANDLE)pDirectoryContext;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    DWORD dwObjectFilterLen = 0;
    PWSTR pwszObjectFilter = NULL;
    ULONG ulAttributesOnly = 0;
    PDIRECTORY_ENTRY pDomainEntries = NULL;
    PDIRECTORY_ENTRY pDomainEntry = NULL;
    DWORD dwNumDomainEntries = 0;
    DWORD iAttr = 0;
    PWSTR pwszDomainSid = NULL;
    PSID pDomainSid = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszObjectDN = NULL;
    PSTR pszDN = NULL;
    PSTR pszQuery = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    DWORD iModified = 0;
    PDIRECTORY_MOD pModified = NULL;
    DWORD dwNumModified = 0;

    WCHAR wszAttrObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSid[] = SAM_DB_DIR_ATTR_OBJECT_SID;

    PWSTR wszAttributes[] = {
        &wszAttrObjectClass[0],
        &wszAttrObjectDN[0],
        &wszAttrObjectSid[0],
        NULL
    };

    enum AttrValueIndex {
        ATTR_IDX_OBJECT_SID  = 0,
        ATTR_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_IDX_OBJECT_SID */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectSid = {
        DIR_MOD_FLAGS_REPLACE,
        &wszAttrObjectSid[0],
        1,
        &AttrValues[ATTR_IDX_OBJECT_SID]
    };

    DIRECTORY_MOD mods[ATTR_IDX_SENTINEL + 1];

    dwObjectClass = pObjectClassMapInfo->objectClass;

    if (!(dwObjectClass == SAMDB_OBJECT_CLASS_LOCAL_GROUP ||
          dwObjectClass == SAMDB_OBJECT_CLASS_USER))
    {
        dwError = EACCES;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    /*
     * Get new domain SID from the mods
     */
    while (pMods[iMod].pwszAttrName)
    {
        if (pMods[iMod].pAttrValues == NULL ||
            pMods[iMod].ulNumValues == 0 ||
            pMods[iMod].ulOperationFlags != DIR_MOD_FLAGS_REPLACE)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (pwszNewObjectSid == NULL &&
            wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid) == 0)
        {
            switch (pMods[iMod].pAttrValues[0].Type)
            {
            case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                dwError = LwAllocateWc16String(
                               &pwszNewObjectSid,
                               pMods[iMod].pAttrValues[0].data.pwszStringValue);
                break;

            case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                dwError = LwMbsToWc16s(
                               pMods[iMod].pAttrValues[0].data.pszStringValue,
                               &pwszNewObjectSid);
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        iMod++;
    }

    dwNumMods = iMod;

    /*
     * Validate the SID
     */

    ntStatus = RtlAllocateSidFromWC16String(&pNewObjectSid,
                                            pwszNewObjectSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
    }

    /*
     * Search for existing domain object first to get current domain SID
     */

    dwDomainFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR) - 1) +
                         dwInt32StrSize +
                         (sizeof(wszDomainFilterFmt)/
                          sizeof(wszDomainFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwDomainFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszDomainFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszDomainFilter, dwDomainFilterLen, wszDomainFilterFmt,
                    &wszAttrObjectClass[0],
                    DIR_OBJECT_CLASS_DOMAIN) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszDomainFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pDomainEntries,
                                &dwNumDomainEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomainEntries != 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pDomainEntry = &(pDomainEntries[0]);

    for (iAttr = 0; iAttr < pDomainEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttribute = &(pDomainEntry->pAttributes[iAttr]);

        if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectSid) == 0)
        {
            pwszDomainSid = pAttribute->pValues[0].data.pwszStringValue;
        }
    }

    ntStatus = RtlAllocateSidFromWC16String(&pDomainSid,
                                            pwszDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /*
     * New SID must be prefixed with the domain SID (must be in
     * the local domain)
     */
    if ((!RtlIsPrefixSid(pDomainSid, pNewObjectSid)) ||
        (pNewObjectSid->SubAuthorityCount != pDomainSid->SubAuthorityCount + 1))
        
    {
        dwError = LW_ERROR_INVALID_SID;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /*
     * Search for existing object with the same SID
     */

    dwObjectFilterLen = ((sizeof(wszAttrObjectSid)/sizeof(WCHAR) - 1) +
                         wc16slen(pwszNewObjectSid) +
                         (sizeof(wszObjectSidFilterFmt)/
                          sizeof(wszObjectSidFilterFmt[0])));
    dwError = DirectoryAllocateMemory(dwObjectFilterLen * sizeof(WCHAR),
                                      (PVOID*)&pwszObjectFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszObjectFilter, dwObjectFilterLen, wszObjectSidFilterFmt,
                    &wszAttrObjectSid[0],
                    pwszNewObjectSid) < 0)
    {
        dwError = errno;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject_inlock(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszObjectFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pEntries,
                                &dwNumEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumEntries)
    {
        pEntry = &(pEntries[0]);

        for (iAttr = 0; iAttr < pEntry->ulNumAttributes; iAttr++)
        {
            PDIRECTORY_ATTRIBUTE pAttribute = &(pEntry->pAttributes[iAttr]);

            if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectClass) == 0)
            {
                dwObjectClass = pAttribute->pValues[0].data.ulValue;
            }
            else if (wc16scasecmp(pAttribute->pwszName, wszAttrObjectDN) == 0)
            {
                pwszObjectDN = pAttribute->pValues[0].data.pwszStringValue;
            }
        }

        if (wc16scasecmp(pwszObjectDN, pDN->pwszDN))
        {
            /*
             * Such SID already exists in the database (under different DN)
             * so check whether it's a user or local group and return
             * the respective error code.
             */
            switch (dwObjectClass)
            {
            case SAMDB_OBJECT_CLASS_LOCAL_GROUP:
                dwError = LW_ERROR_GROUP_EXISTS;
                break;

            case SAMDB_OBJECT_CLASS_USER:
                dwError = LW_ERROR_USER_EXISTS;
            }
            BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

    iMod = 0;
    memset(&mods[0], 0, sizeof(mods));

    AttrValues[ATTR_IDX_OBJECT_SID].data.pwszStringValue = pwszNewObjectSid;

    mods[iMod++] = modObjectSid;

    dwError = LwWc16sToMbs(
                   pDN->pwszDN,
                   &pszDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateBuildObjectQuery(
                    pDirectoryContext,
                    pDN,
                    pObjectClassMapInfo,
                    mods,
                    &pszQuery,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = SamDbUpdateBindValues(
                    pDirectoryContext,
                    pszDN,
                    pColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    /*
     * Prepare the applied mods to return to the caller
     */
    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid))
        {
            continue;
        }

        dwNumModified++;
    }

    dwError = DirectoryAllocateMemory(sizeof(pModified[0]) * dwNumModified,
                                      OUT_PPVOID(&pModified));
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iMod = 0; pMods[iMod].pwszAttrName != NULL; iMod++)
    {
        if (wc16scasecmp(pMods[iMod].pwszAttrName, wszAttrObjectSid))
        {
            continue;
        }

        pModified[iModified++] = pMods[iMod];
    }

    *ppModified     = pModified;
    *pdwNumModified = dwNumModified;

cleanup:
    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    if (pDomainEntries)
    {
        DirectoryFreeEntries(pDomainEntries,
                             dwNumDomainEntries);
    }

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries,
                             dwNumEntries);
    }

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    DIRECTORY_FREE_MEMORY(pwszNewObjectSid);
    RTL_FREE(&pNewObjectSid);
    DIRECTORY_FREE_MEMORY(pwszDomainFilter);
    RTL_FREE(&pDomainSid);
    DIRECTORY_FREE_MEMORY(pwszObjectFilter);
    DIRECTORY_FREE_MEMORY(pszDN);
    DIRECTORY_FREE_STRING(pszQuery);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamDbModifyAttributes(
    PSAM_DIRECTORY_CONTEXT                pDirectoryContext,
    PSAM_DB_DN                            pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO   pObjectClassMapInfo,
    PDIRECTORY_MOD                        pMods,
    PDIRECTORY_MOD                       *ppModified,
    PDWORD                                pdwNumModified
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszDN = NULL;
    PSTR pszQuery = NULL;
    BOOLEAN bTxStarted = FALSE;
    sqlite3_stmt* pSqlStatement = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    DWORD dwNumModified = 0;
    PDIRECTORY_MOD pModified = NULL;
    DWORD iMod = 0;

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = LwWc16sToMbs(
                pDN->pwszDN,
                &pszDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbUpdateBuildObjectQuery(
                    pDirectoryContext,
                    pDN,
                    pObjectClassMapInfo,
                    pMods,
                    &pszQuery,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, pDirectoryContext);

    dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszQuery,
                    -1,
                    &pSqlStatement,
                    NULL);
    BAIL_ON_SAMDB_SQLITE_ERROR_DB(dwError, pDirectoryContext->pDbContext->pDbHandle);

    dwError = SamDbUpdateBindValues(
                    pDirectoryContext,
                    pszDN,
                    pColumnValueList,
                    pSqlStatement);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_step(pSqlStatement);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

    while (pMods[dwNumModified].pwszAttrName)
    {
        dwNumModified++;
    }

    dwError = DirectoryAllocateMemory(sizeof(pModified[0]) * dwNumModified,
                                      OUT_PPVOID(&pModified));
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iMod = 0; iMod < dwNumModified; iMod++)
    {
        pModified[iMod] = pMods[iMod];
    }

    *ppModified     = pModified;
    *pdwNumModified = dwNumModified;

cleanup:
    SAM_DB_END_TRANSACTION(bTxStarted, dwError, pDirectoryContext);

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    if (pSqlStatement)
    {
        sqlite3_finalize(pSqlStatement);
    }

    DIRECTORY_FREE_STRING(pszQuery);
    DIRECTORY_FREE_STRING(pszDN);

    return dwError;

error:
    goto cleanup;
}


#define SAMDB_UPDATE_OBJECT_QUERY_PREFIX    "UPDATE " SAM_DB_OBJECTS_TABLE
#define SAMDB_UPDATE_OBJECT_QUERY_SET       " SET "
#define SAMDB_UPDATE_OBJECT_QUERY_EQUALS    " = "
#define SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR ","
#define SAMDB_UPDATE_OBJECT_QUERY_WHERE     " WHERE "
#define SAMDB_UPDATE_OBJECT_QUERY_SUFFIX    ";"

static
DWORD
SamDbUpdateBuildObjectQuery(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAM_DB_DN                          pDN,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSTR*                               ppszQuery,
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszQueryCursor = NULL;
    PSTR  pszCursor = 0;
    DWORD iCol = 0;
    DWORD dwQueryLen = 0;
    DWORD dwColNamesLen = 0;
    CHAR  szBuf[32];
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pIter = NULL;

    dwError = SamDbUpdateBuildColumnValueList(
                    pDirectoryContext,
                    pObjectClassMapInfo,
                    modifications,
                    &pColumnValueList);
    BAIL_ON_SAMDB_ERROR(dwError);

    // We are building a query which will look like
    // UPDATE samdbobjects
    //    SET col1 = ?1,
    //        col2 = ?2
    //  WHERE DistinguishedName = ?3;
    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR) - 1;
        }
        else
        {
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SET) - 1;
        }

        dwColNamesLen += strlen(&pIter->pAttrMap->szDbColumnName[0]);

        dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS) - 1;

        sprintf(szBuf, "\?%u", ++iCol);

        dwColNamesLen += strlen(szBuf);
    }

    dwQueryLen = sizeof(SAMDB_UPDATE_OBJECT_QUERY_PREFIX) - 1;
    dwQueryLen += dwColNamesLen;
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_WHERE) - 1;

    dwQueryLen += sizeof(SAM_DB_COL_DISTINGUISHED_NAME) - 1;
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS) - 1;
    sprintf(szBuf, "\?%u", ++iCol);
    dwQueryLen += strlen(szBuf);
    dwQueryLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SUFFIX) - 1;
    dwQueryLen++;

    dwError = DirectoryAllocateMemory(
                    dwQueryLen,
                    (PVOID*)&pszQuery);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQueryCursor = pszQuery;
    iCol = 0;
    dwColNamesLen = 0;

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_PREFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
    }

    for (pIter = pColumnValueList; pIter; pIter = pIter->pNext)
    {
        if (dwColNamesLen)
        {
            pszCursor = SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_FIELD_SEPARATOR)  - 1;
        }
        else
        {
            pszCursor = SAMDB_UPDATE_OBJECT_QUERY_SET;
            while (pszCursor && *pszCursor)
            {
                *pszQueryCursor++ = *pszCursor++;
            }
            dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_SET)  - 1;
        }

        pszCursor = &pIter->pAttrMap->szDbColumnName[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }

        pszCursor = SAMDB_UPDATE_OBJECT_QUERY_EQUALS;
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
        }
        dwColNamesLen += sizeof(SAMDB_UPDATE_OBJECT_QUERY_EQUALS)  - 1;

        sprintf(szBuf, "\?%u", ++iCol);
        pszCursor = &szBuf[0];
        while (pszCursor && *pszCursor)
        {
            *pszQueryCursor++ = *pszCursor++;
            dwColNamesLen++;
        }
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_WHERE;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAM_DB_COL_DISTINGUISHED_NAME;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_EQUALS;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    sprintf(szBuf, "\?%u", ++iCol);
    pszCursor = &szBuf[0];
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    pszCursor = SAMDB_UPDATE_OBJECT_QUERY_SUFFIX;
    while (pszCursor && *pszCursor)
    {
        *pszQueryCursor++ = *pszCursor++;
        dwColNamesLen++;
    }

    *ppszQuery = pszQuery;
    *ppColumnValueList = pColumnValueList;

cleanup:

    return dwError;

error:

    *ppszQuery = NULL;

    DIRECTORY_FREE_STRING(pszQuery);

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }

    goto cleanup;
}

static
DWORD
SamDbUpdateBuildColumnValueList(
    PSAM_DIRECTORY_CONTEXT              pDirectoryContext,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassMapInfo,
    DIRECTORY_MOD                       modifications[],
    PSAM_DB_COLUMN_VALUE*               ppColumnValueList
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pColumnValueList = NULL;
    PSAM_DB_COLUMN_VALUE pColumnValue = NULL;
    DWORD dwNumMods = 0;
    DWORD iMap = 0;

    //
    // Build column values for the attributes specified by the user
    //
    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_COLUMN_VALUE),
                        (PVOID*)&pColumnValue);
        BAIL_ON_SAMDB_ERROR(dwError);

        pColumnValue->pDirMod = &modifications[dwNumMods];

        dwError = SamDbAttributeLookupByName(
                        pDirectoryContext->pAttrLookup,
                        pColumnValue->pDirMod->pwszAttrName,
                        &pColumnValue->pAttrMap);
        BAIL_ON_SAMDB_ERROR(dwError);

        for (iMap = 0; iMap < pObjectClassMapInfo->dwNumMaps; iMap++)
        {
            PSAMDB_ATTRIBUTE_MAP_INFO pMapInfo = NULL;

            pMapInfo = &pObjectClassMapInfo->pAttributeMaps[iMap];

            if (!wc16scasecmp(&pMapInfo->wszAttributeName[0],
                              modifications[dwNumMods].pwszAttrName))
            {
                pColumnValue->pAttrMapInfo = pMapInfo;

                break;
            }
        }
        assert(pColumnValue->pAttrMapInfo != NULL);

        pColumnValue->pNext = pColumnValueList;
        pColumnValueList = pColumnValue;
        pColumnValue = NULL;

        dwNumMods++;
    }

    *ppColumnValueList = SamDbReverseColumnValueList(pColumnValueList);

cleanup:

    return dwError;

error:

    *ppColumnValueList = NULL;

    if (pColumnValueList)
    {
        SamDbFreeColumnValueList(pColumnValueList);
    }
    if (pColumnValue)
    {
        SamDbFreeColumnValueList(pColumnValue);
    }

    goto cleanup;
}

static
DWORD
SamDbUpdateBindValues(
    PSAM_DIRECTORY_CONTEXT pDirectoryContext,
    PCSTR                  pszObjectDN,
    PSAM_DB_COLUMN_VALUE   pColumnValueList,
    sqlite3_stmt*          pSqlStatement
    )
{
    DWORD dwError = 0;
    PSAM_DB_COLUMN_VALUE pIter = pColumnValueList;
    DWORD iParam = 0;
    PSTR pszValue = NULL;
    PWSTR pwszValue = NULL;

    for (; pIter; pIter = pIter->pNext)
    {
        if (pIter->pAttrValues)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
        }
        else if (pIter->pDirMod)
        {
            if (pIter->ulNumValues > 1)
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
            }
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        switch (pIter->pAttrMap->attributeType)
        {
            case SAMDB_ATTR_TYPE_TEXT:
            {
                PATTRIBUTE_VALUE pAttrValue = NULL;

                if (pIter->pAttrValues)
                {
                    pAttrValue = pIter->pAttrValues;
                }
                else if (pIter->pDirMod)
                {
                    pAttrValue = pIter->pDirMod->pAttrValues;
                }

                switch (pAttrValue->Type)
                {
                case DIRECTORY_ATTR_TYPE_UNICODE_STRING:
                    pwszValue = pAttrValue->data.pwszStringValue;
                    break;

                case DIRECTORY_ATTR_TYPE_ANSI_STRING:
                    pszValue = pAttrValue->data.pszStringValue;
                    break;
                }

                if (pwszValue)
                {
                    dwError = LwWc16sToMbs(pwszValue, &pszValue);
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                if (pszValue)
                {
                    dwError = sqlite3_bind_text(
                                    pSqlStatement,
                                    ++iParam,
                                    pszValue,
                                    -1,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                /* If both pointers are set then it means pszValue was
                   allocated from pwszValue */
                if (pwszValue && pszValue)
                {
                    LW_SAFE_FREE_STRING(pszValue);
                }

                pszValue = NULL;
                pwszValue = NULL;
                break;
            }

            case SAMDB_ATTR_TYPE_INT32:
            case SAMDB_ATTR_TYPE_BOOLEAN:
            case SAMDB_ATTR_TYPE_DATETIME:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.ulValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.ulValue);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;

            case SAMDB_ATTR_TYPE_INT64:

                if (pIter->pAttrValues)
                {
                    dwError = sqlite3_bind_int64(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pAttrValues[0].data.llValue);
                }
                else
                if (pIter->pDirMod)
                {
                    dwError = sqlite3_bind_int64(
                                    pSqlStatement,
                                    ++iParam,
                                    pIter->pDirMod->pAttrValues[0].data.llValue);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;

            case SAMDB_ATTR_TYPE_BLOB:
            case SAMDB_ATTR_TYPE_SECURITY_DESCRIPTOR:
            {
                POCTET_STRING pOctetString = NULL;

                if (pIter->pAttrValues)
                {
                    pOctetString = pIter->pAttrValues[0].data.pOctetString;
                }
                else
                if (pIter->pDirMod)
                {
                    pOctetString = pIter->pDirMod->pAttrValues[0].data.pOctetString;
                }

                if (pOctetString)
                {
                    dwError = sqlite3_bind_blob(
                                    pSqlStatement,
                                    ++iParam,
                                    pOctetString->pBytes,
                                    pOctetString->ulNumBytes,
                                    SQLITE_TRANSIENT);
                }
                else
                {
                    dwError = sqlite3_bind_null(pSqlStatement, ++iParam);
                }
                BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

                break;
            }

            default:

                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

    dwError = sqlite3_bind_text(
                    pSqlStatement,
                    ++iParam,
                    pszObjectDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(dwError, pSqlStatement);

cleanup:
    /* If both pointers are set then it means pszValue was
       allocated from pwszValue */
    if (pwszValue && pszValue)
    {
        LW_SAFE_FREE_STRING(pszValue);
    }

    return dwError;

error:

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
