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
 *        samdbstructs.h
 *
 * Abstract:
 *
 *
 *      BeyondTrust SAM Database Provider
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
