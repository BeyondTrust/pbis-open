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
 *        samdbstructs.h
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM database structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __SAMDBSTRUCTS_H__
#define __SAMDBSTRUCTS_H__

typedef struct _SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO
{
    SAMDB_OBJECT_CLASS        objectClass;
    PSAMDB_ATTRIBUTE_MAP_INFO pAttributeMaps;
    DWORD                     dwNumMaps;

} SAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO, *PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO;

typedef struct _SAM_DB_CONTEXT
{
    sqlite3* pDbHandle;

    sqlite3_stmt* pDelObjectStmt;
    sqlite3_stmt* pQueryObjectCountStmt;
    sqlite3_stmt* pQueryObjectRecordInfoStmt;

    struct _SAM_DB_CONTEXT* pNext;

} SAM_DB_CONTEXT, *PSAM_DB_CONTEXT;

typedef struct _SAM_DB_ATTR_LOOKUP
{
    PLWRTL_RB_TREE pLookupTable;

} SAM_DB_ATTR_LOOKUP, *PSAM_DB_ATTR_LOOKUP;

typedef struct _SAM_DIRECTORY_CONTEXT
{
    PWSTR    pwszDistinguishedName;
    PWSTR    pwszCredential;
    ULONG    ulMethod;

    PSAM_DB_CONTEXT         pDbContext;

    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps;
    DWORD                               dwNumObjectClassAttrMaps;
    PSAM_DB_ATTR_LOOKUP   pAttrLookup;

} SAM_DIRECTORY_CONTEXT, *PSAM_DIRECTORY_CONTEXT;

typedef struct _SAM_GLOBALS
{
    pthread_mutex_t mutex;

    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps;
    DWORD                               dwNumObjectClassAttrMaps;

    PSAM_DB_ATTRIBUTE_MAP pAttrMaps;
    DWORD                 dwNumMaps;
    SAM_DB_ATTR_LOOKUP    attrLookup;

    PSTR            pszProviderName;

    DIRECTORY_PROVIDER_FUNCTION_TABLE providerFunctionTable;

    pthread_rwlock_t  rwLock;
    pthread_rwlock_t* pRwLock;

    PSAM_DB_CONTEXT pDbContextList;
    DWORD           dwNumDbContexts;
    DWORD           dwNumMaxDbContexts;

} SAM_GLOBALS, *PSAM_GLOBALS;

typedef struct _SAM_DB_DOMAIN_INFO
{
    ULONG ulDomainRecordId;

    PWSTR pwszDomainName;
    PWSTR pwszNetBIOSName;
    PWSTR pwszDomainSID;

} SAM_DB_DOMAIN_INFO, *PSAM_DB_DOMAIN_INFO;

typedef struct _SAMDB_DN_TOKEN
{
    SAMDB_DN_TOKEN_TYPE tokenType;
    PWSTR               pwszDN;
    PWSTR               pwszToken;
    DWORD               dwLen;

    struct _SAMDB_DN_TOKEN * pNext;

} SAMDB_DN_TOKEN, *PSAMDB_DN_TOKEN;

typedef struct _SAM_DB_DN
{
    PWSTR pwszDN;

    PSAMDB_DN_TOKEN pTokenList;

} SAM_DB_DN, *PSAM_DB_DN;

typedef struct _SAM_DB_COLUMN_VALUE
{
    PSAMDB_ATTRIBUTE_MAP_INFO pAttrMapInfo;
    PSAM_DB_ATTRIBUTE_MAP     pAttrMap;

    PDIRECTORY_MOD            pDirMod;

    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

    struct _SAM_DB_COLUMN_VALUE* pNext;

} SAM_DB_COLUMN_VALUE, *PSAM_DB_COLUMN_VALUE;

#endif /* __SAMDBSTRUCTS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
