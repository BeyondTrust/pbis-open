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
 *        samdbinit.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Database initialisation routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"

#define SAMDB_BUILTIN_TAG    "BUILTIN"
#define SAMDB_BUILTIN_SID    "S-1-5-32"

static
DWORD
SamDbCreateTables(
    PSAM_DB_CONTEXT pDbContext
    );

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    );

static
DWORD
SamDbAddBuiltin(
    HANDLE hDirectory,
    PCSTR  pszDomainDN
    );

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PSID   *ppMachineSid
    );

static
DWORD
SamDbAddContainer(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN,
    PCSTR              pszNetBIOSName,
    PCSTR              pszGroupName,
    PCSTR              pszGroupSID,
    SAMDB_OBJECT_CLASS objectClass
    );

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    );

static
DWORD
SamDbAddBuiltinAccounts(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN
    );

static
DWORD
SamDbAddLocalAccounts(
    HANDLE    hDirectory,
    PCSTR     pszDomainDN,
    PCSTR     pszNetBIOSName,
    PSID      pMachineSid
    );

static
DWORD
SamDbSetupLocalGroupMemberships(
    HANDLE hDirectory
    );

static
DWORD
SamDbFixAcls(
    HANDLE hDirectory
    );

static
DWORD
SamDbFixLocalAccounts(
    HANDLE hDirectory
    );

static
VOID
SamDbFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

DWORD
DirectoryInitializeProvider(
    PSTR* ppszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    DIRECTORY_PROVIDER_FUNCTION_TABLE providerAPITable =
        {
                .pfnDirectoryOpen            = &SamDbOpen,
                .pfnDirectoryBind            = &SamDbBind,
                .pfnDirectoryAdd             = &SamDbAddObject,
                .pfnDirectoryModify          = &SamDbModifyObject,
                .pfnDirectorySetPassword     = &SamDbSetPassword,
                .pfnDirectoryChangePassword  = &SamDbChangePassword,
                .pfnDirectoryVerifyPassword  = &SamDbVerifyPassword,
                .pfnDirectoryGetGroupMembers = &SamDbGetGroupMembers,
                .pfnDirectoryGetMemberships  = &SamDbGetUserMemberships,
                .pfnDirectoryAddToGroup      = &SamDbAddToGroup,
                .pfnDirectoryRemoveFromGroup = &SamDbRemoveFromGroup,
                .pfnDirectoryDelete          = &SamDbDeleteObject,
                .pfnDirectorySearch          = &SamDbSearchObject,
                .pfnDirectoryGetUserCount    = &SamDbGetUserCount,
                .pfnDirectoryGetGroupCount   = &SamDbGetGroupCount,
                .pfnDirectoryClose           = &SamDbClose
        };

    gSamGlobals.pszProviderName = "Likewise SAM Local Database";
    gSamGlobals.providerFunctionTable = providerAPITable;

    pthread_rwlock_init(&gSamGlobals.rwLock, NULL);
    gSamGlobals.pRwLock = &gSamGlobals.rwLock;

    dwError = SamDbAttributeLookupInitContents(
                &gSamGlobals.attrLookup,
                gSamGlobals.pAttrMaps,
                gSamGlobals.dwNumMaps);
    BAIL_ON_SAMDB_ERROR(dwError);

    gSamGlobals.dwNumMaxDbContexts = SAM_DB_CONTEXT_POOL_MAX_ENTRIES;

    dwError = SamDbInit();
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszProviderName = gSamGlobals.pszProviderName;
    *ppFnTable = &gSamGlobals.providerFunctionTable;

cleanup:

    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

DWORD
DirectoryShutdownProvider(
    PSTR pszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_lock(&gSamGlobals.mutex);

    SamDbAttributeLookupFreeContents(&gSamGlobals.attrLookup);

    while (gSamGlobals.pDbContextList)
    {
        PSAM_DB_CONTEXT pDbContext = gSamGlobals.pDbContextList;

        gSamGlobals.pDbContextList = gSamGlobals.pDbContextList->pNext;

        SamDbFreeDbContext(pDbContext);
    }

    // Set this so that any further pending contexts will get freed upon release
    gSamGlobals.dwNumDbContexts = gSamGlobals.dwNumMaxDbContexts;

    pthread_mutex_unlock(&gSamGlobals.mutex);

    if (gSamGlobals.pRwLock)
    {
        pthread_rwlock_destroy(&gSamGlobals.rwLock);
        gSamGlobals.pRwLock = NULL;
    }

    return dwError;
}

DWORD
SamDbInit(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory1 = (HANDLE)NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSAM_DIRECTORY_CONTEXT pDirectory = NULL;
    PCSTR  pszDbDirPath = SAM_DB_DIR;
    PCSTR  pszDbPath = SAM_DB;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckFileExists(
                    pszDbPath,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
        dwError = SamDbOpen(&hDirectory1);
        BAIL_ON_SAMDB_ERROR(dwError);

        //Fix ACLs
        dwError = SamDbFixAcls(hDirectory1);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = SamDbFixLocalAccounts(hDirectory1);
        BAIL_ON_SAMDB_ERROR(dwError);

        goto cleanup;
    }

    dwError = LsaCheckDirectoryExists(
                    pszDbDirPath,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!bExists)
    {
        mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        dwError = LsaCreateDirectory(pszDbDirPath, mode);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(
                    pszDbDirPath,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbOpen(&hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    pDirectory = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbCreateTables(pDirectory->pDbContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddDefaultEntries(pDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(
                    pszDbPath,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    if (hDirectory1)
    {
        SamDbClose(hDirectory1);
    }

    if (hDirectory)
    {
        SamDbClose(hDirectory);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbCreateTables(
    PSAM_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;
    PCSTR pszQuery = SAM_DB_QUERY_CREATE_TABLES;

    SAMDB_LOG_DEBUG("Query used to create tables [%s]", pszQuery);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %u): %s",
                dwError,
                LSA_SAFE_LOG_STRING(pszError));

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    )
{
    DWORD  dwError = 0;
    PSTR   pszHostname = NULL;
    size_t sHostnameLen = 0;
    PSTR   pszHostnameLower = NULL;
    DWORD  dwHostnameHash = 0;
    PSTR   pszHashStr = NULL;
    size_t sHashStrLen = 0;
    PSTR   pszDomainDN = NULL;
    CHAR   szDomainName[16] = {0};
    PSID   pMachineSid = NULL;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_SAMDB_ERROR(dwError);

    LwStrToUpper(pszHostname);
    sHostnameLen = strlen(pszHostname);

    if (sHostnameLen < 16)
    {
        strncpy(szDomainName, pszHostname, sHostnameLen);
    }
    else
    {
        /*
         * The hostname is too long for NetBIOS max size
         * so format the machine domain name based on the hostname
         * and its hash (separated with '-').
         */
        dwError = LwAllocateString(pszHostname, &pszHostnameLower);
        BAIL_ON_SAMDB_ERROR(dwError);

        LwStrToLower(pszHostnameLower);

        dwError = LsaStrHash(pszHostnameLower, &dwHostnameHash);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaHashToStr(dwHostnameHash, &pszHashStr);
        BAIL_ON_SAMDB_ERROR(dwError);

        sHashStrLen = strlen(pszHashStr);

        strncpy(szDomainName,
                pszHostname,
                sizeof(szDomainName) - sHashStrLen - 1);
        szDomainName[sizeof(szDomainName) - sHashStrLen - 2] = '-';
        strncpy(&szDomainName[sizeof(szDomainName) - sHashStrLen - 1],
                pszHashStr,
                sHashStrLen);
    }

    dwError = SamDbInitConfig(hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszDomainDN,
                    "DC=%s",
                    &szDomainName[0]);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddMachineDomain(
                    hDirectory,
                    pszDomainDN,
                    &szDomainName[0],
                    &szDomainName[0],
                    &pMachineSid);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBuiltin(
                    hDirectory,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBuiltinAccounts(
                    hDirectory,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddLocalAccounts(
                    hDirectory,
                    pszDomainDN,
                    &szDomainName[0],
                    pMachineSid);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbSetupLocalGroupMemberships(
                    hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:
    DIRECTORY_FREE_STRING(pszHostname);
    DIRECTORY_FREE_STRING(pszDomainDN);
    DIRECTORY_FREE_STRING(pszHashStr);
    DIRECTORY_FREE_STRING(pszHostnameLower);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddBuiltin(
    HANDLE hDirectory,
    PCSTR  pszDomainDN
    )
{
    return SamDbAddContainer(
                    hDirectory,
                    pszDomainDN,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_SID,
                    SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN);
}

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PSID   *ppMachineSid
    )
{
    const ULONG ulSubAuthCount = 4;
    DWORD  dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG  ulSidLength = 0;
    PSTR   pszMachineSID = NULL;
    ULONG  ulSubAuth[3];
    PBYTE  pSubAuth = NULL;
    uuid_t GUID;
    PSID   pMachineSid = NULL;
    SID_IDENTIFIER_AUTHORITY AuthId = { SECURITY_NT_AUTHORITY };

    uuid_generate(GUID);

    pSubAuth = (PBYTE)GUID;
    ulSubAuth[0] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[1] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[2] = *((PULONG)pSubAuth);

    ulSidLength = RtlLengthRequiredSid(ulSubAuthCount);

    dwError = LwAllocateMemory(ulSidLength,
                                (void**)&pMachineSid);
    BAIL_ON_SAMDB_ERROR(dwError);

    status = RtlInitializeSid(pMachineSid,
                              &AuthId,
                              ulSubAuthCount);
    if (status != 0) {
        dwError = LW_ERROR_SAM_INIT_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pMachineSid->SubAuthority[0] = 21;
    pMachineSid->SubAuthority[1] = ulSubAuth[0];
    pMachineSid->SubAuthority[2] = ulSubAuth[1];
    pMachineSid->SubAuthority[3] = ulSubAuth[2];

    dwError = LwAllocateStringPrintf(
                    &pszMachineSID,
                    "S-1-5-21-%lu-%lu-%lu",
                    ulSubAuth[0],
                    ulSubAuth[1],
                    ulSubAuth[2]);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddLocalDomain(
                    hDirectory,
                    pszDomainDN,
                    pszDomainName,
                    pszNetBIOSName,
                    pszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppMachineSid = pMachineSid;

cleanup:

    if (pszMachineSID)
    {
        LwFreeMemory(pszMachineSID);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrNameNetBIOSName[]    = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameDomain[]         = SAM_DB_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrNameMinPwdAge[]      = SAM_DB_DIR_ATTR_MIN_PWD_AGE;
    WCHAR wszAttrNameMaxPwdAge[]      = SAM_DB_DIR_ATTR_MAX_PWD_AGE;
    WCHAR wszAttrNameMinPwdLength[]   = SAM_DB_DIR_ATTR_MIN_PWD_LENGTH;
    WCHAR wszAttrNamePwdChangeTime[]  = SAM_DB_DIR_ATTR_PWD_PROMPT_TIME;
    WCHAR wszAttrNameLockoutThreshold[] = SAM_DB_DIR_ATTR_LOCKOUT_THRESHOLD;
    WCHAR wszAttrNameLockoutDuration[] = SAM_DB_DIR_ATTR_LOCKOUT_DURATION;
    WCHAR wszAttrNameLockoutWindow[]  = SAM_DB_DIR_ATTR_LOCKOUT_WINDOW;
    WCHAR wszAttrNameSecDesc[]        = SAM_DB_DIR_ATTR_SECURITY_DESCRIPTOR;
    PWSTR pwszObjectDN    = NULL;
    PWSTR pwszMachineSID  = NULL;
    PWSTR pwszDomainName  = NULL;
    PWSTR pwszNetBIOSName = NULL;
    PSID pMachineSID = NULL;
    LONG64 llMinPwdAge = LW_WINTIME_TO_NTTIME_REL(SAMDB_MIN_PWD_AGE);
    LONG64 llMaxPwdAge = LW_WINTIME_TO_NTTIME_REL(SAMDB_MAX_PWD_AGE);
    DWORD dwMinPwdLength = 0;
    LONG64 llPwdPromptTime = LW_WINTIME_TO_NTTIME_REL(SAMDB_PWD_PROMPT_TIME);
    DWORD dwLockoutThreshold = SAMDB_LOCKOUT_THRESHOLD;
    LONG64 llLockoutDuration = LW_WINTIME_TO_NTTIME_REL(SAMDB_LOCKOUT_DURATION);
    LONG64 llLockoutWindow = LW_WINTIME_TO_NTTIME_REL(SAMDB_LOCKOUT_WINDOW);
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    OCTET_STRING SecDescBlob = {0};
    ULONG ulSecDescLen = 0;
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avMachineSID  = {0};
    ATTRIBUTE_VALUE avDomainName  = {0};
    ATTRIBUTE_VALUE avCommonName = {0};
    ATTRIBUTE_VALUE avSamAccountName = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    ATTRIBUTE_VALUE avMinPwdAge = {0};
    ATTRIBUTE_VALUE avMaxPwdAge = {0};
    ATTRIBUTE_VALUE avMinPwdLength = {0};
    ATTRIBUTE_VALUE avPwdChangeTime = {0};
    ATTRIBUTE_VALUE avLockoutThreshold = {0};
    ATTRIBUTE_VALUE avLockoutDuration = {0};
    ATTRIBUTE_VALUE avLockoutWindow = {0};
    ATTRIBUTE_VALUE avSecDesc = {0};
    DIRECTORY_MOD mods[16];
    ULONG iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LwMbsToWc16s(
                    pszDomainDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectClass[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avObjectClass.data.ulValue = SAMDB_OBJECT_CLASS_DOMAIN;
    mods[iMod].pAttrValues = &avObjectClass;

    dwError = LwMbsToWc16s(
                    pszMachineSID,
                    &pwszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMachineSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avMachineSID.data.pwszStringValue = pwszMachineSID;
    mods[iMod].pAttrValues = &avMachineSID;

    dwError = LwMbsToWc16s(
                    pszDomainName,
                    &pwszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameDomain[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avDomainName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avDomainName;

    mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avCommonName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avCommonName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avCommonName;

    mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avSamAccountName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avSamAccountName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avSamAccountName;

    dwError = LwMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.data.pwszStringValue = pwszNetBIOSName;
    mods[iMod].pAttrValues = &avNetBIOSName;

    mods[++iMod].pwszAttrName = &wszAttrNameMinPwdAge[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMinPwdAge.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avMinPwdAge.data.llValue = llMinPwdAge;
    mods[iMod].pAttrValues = &avMinPwdAge;

    mods[++iMod].pwszAttrName = &wszAttrNameMaxPwdAge[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMaxPwdAge.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avMaxPwdAge.data.llValue = llMaxPwdAge;
    mods[iMod].pAttrValues = &avMaxPwdAge;

    mods[++iMod].pwszAttrName = &wszAttrNameMinPwdLength[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMinPwdLength.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avMinPwdLength.data.ulValue = dwMinPwdLength;
    mods[iMod].pAttrValues = &avMinPwdLength;

    mods[++iMod].pwszAttrName = &wszAttrNamePwdChangeTime[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avPwdChangeTime.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avPwdChangeTime.data.llValue = llPwdPromptTime;
    mods[iMod].pAttrValues = &avPwdChangeTime;

    mods[++iMod].pwszAttrName = &wszAttrNameLockoutThreshold[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avLockoutThreshold.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avLockoutThreshold.data.ulValue = dwLockoutThreshold;
    mods[iMod].pAttrValues = &avLockoutThreshold;

    mods[++iMod].pwszAttrName = &wszAttrNameLockoutDuration[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avLockoutDuration.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avLockoutDuration.data.llValue = llLockoutDuration;
    mods[iMod].pAttrValues = &avLockoutDuration;

    mods[++iMod].pwszAttrName = &wszAttrNameLockoutWindow[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avLockoutWindow.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avLockoutWindow.data.llValue = llLockoutWindow;
    mods[iMod].pAttrValues = &avLockoutWindow;

    ntStatus = RtlAllocateSidFromWC16String(&pMachineSID,
                                            pwszMachineSID);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SamDbCreateLocalDomainSecDesc(pMachineSID,
                                            &pSecDesc,
                                            &ulSecDescLen);
    BAIL_ON_SAMDB_ERROR(dwError);

    SecDescBlob.ulNumBytes = ulSecDescLen;
    SecDescBlob.pBytes     = (PBYTE)pSecDesc;

    mods[++iMod].pwszAttrName = &wszAttrNameSecDesc[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avSecDesc.Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
    avSecDesc.data.pOctetString = &SecDescBlob;
    mods[iMod].pAttrValues = &avSecDesc;

    mods[++iMod].pwszAttrName = NULL;
    mods[iMod].ulNumValues = 0;
    mods[iMod].pAttrValues = NULL;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszMachineSID);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSName);
    RTL_FREE(&pMachineSID);
    DIRECTORY_FREE_MEMORY(pSecDesc);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddContainer(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN,
    PCSTR              pszNetBIOSName,
    PCSTR              pszName,
    PCSTR              pszSID,
    SAMDB_OBJECT_CLASS objectClass
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrNameContainerName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrNameDomainName[] = SAM_DB_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSName[] = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrNameSecDesc[] = SAM_DB_DIR_ATTR_SECURITY_DESCRIPTOR;
    PSTR pszObjectDN = NULL;
    PWSTR pwszObjectDN = NULL;
    PWSTR pwszSID = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszNetBIOSName = NULL;
    PSID pBuiltinSID = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    OCTET_STRING SecDescBlob = {0};
    ULONG ulSecDescLen = 0;
    ATTRIBUTE_VALUE avContainerName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDomainName = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    ATTRIBUTE_VALUE avSecDesc = {0};
    DIRECTORY_MOD mods[8];
    ULONG iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LwAllocateStringPrintf(
                    &pszObjectDN,
                    "CN=%s,%s",
                    pszName,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszObjectDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszName,
                    &pwszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszSID,
                    &pwszSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avSID.data.pwszStringValue = pwszSID;
    mods[iMod].pAttrValues = &avSID;

    mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avObjectClass.data.ulValue = objectClass;
    mods[iMod].pAttrValues = &avObjectClass;

    mods[++iMod].pwszAttrName = &wszAttrNameDomainName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avDomainName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avDomainName;

    mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.data.pwszStringValue = pwszNetBIOSName;
    mods[iMod].pAttrValues = &avNetBIOSName;

    mods[++iMod].pwszAttrName = &wszAttrNameContainerName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avContainerName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avContainerName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avContainerName;

    mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    mods[iMod].pAttrValues = &avContainerName;

    ntStatus = RtlAllocateSidFromWC16String(&pBuiltinSID,
                                            pwszSID);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SamDbCreateBuiltinDomainSecDesc(pBuiltinSID,
                                              &pSecDesc,
                                              &ulSecDescLen);
    BAIL_ON_SAMDB_ERROR(dwError);

    SecDescBlob.ulNumBytes = ulSecDescLen;
    SecDescBlob.pBytes     = (PBYTE)pSecDesc;

    mods[++iMod].pwszAttrName = &wszAttrNameSecDesc[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avSecDesc.Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
    avSecDesc.data.pOctetString = &SecDescBlob;
    mods[iMod].pAttrValues = &avSecDesc;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSID);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSName);
    RTL_FREE(&pBuiltinSID);
    DIRECTORY_FREE_MEMORY(pSecDesc)

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbAddBuiltinAccounts(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN
    )
{
    struct builtin_account {
        PCSTR               pszName;
        PCSTR               pszSID;
        DWORD               dwGID;
        PCSTR               pszDescription;
        PCSTR               pszDomainName;
        PCSTR               pszNetBIOSDomain;
        SAMDB_ACB           flags;
        SAMDB_OBJECT_CLASS  objectClass;
    } BuiltinAccounts[] = {
        {
            .pszName        = "Administrators",
            .pszSID         = "S-1-5-32-544",
            .dwGID          = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_ADMINS),
            .pszDescription = "Administrators have complete and unrestricted "
                              "access to the computer/domain",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_LOCAL_GROUP
        },
        {
            .pszName        = "Users",
            .pszSID         = "S-1-5-32-545",
            .dwGID          = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_USERS),
            .pszDescription = "Users are prevented from making accidental "
                              "or intentional system-wide changes. Thus, "
                              "users can run certified applications, but not "
                              "most legacy applications",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_LOCAL_GROUP
        },
        {
            .pszName        = "Guests",
            .pszSID         = "S-1-5-32-546",
            .dwGID          = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_GUESTS),
            .pszDescription = "Guests have the same access as members of the "
                              "Users group by default, except for the Guest "
                              "account which is further restricted",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_LOCAL_GROUP
        },
        {
            .pszName        = "Backup Operators",
            .pszSID         = "S-1-5-32-551",
            .dwGID          = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_BACKUP_OPS),
            .pszDescription = "Backup Operators have the privilege to do "
                              "backup/restore operations",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_LOCAL_GROUP
        },
    };

    DWORD dwError = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrNameGID[]            = SAM_DB_DIR_ATTR_GID;
    WCHAR wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrNameDomainName[]     = SAM_DB_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[]  = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameDescription[]    = SAM_DB_DIR_ATTR_DESCRIPTION;
    WCHAR wszAttrAccountFlags[]       = SAM_DB_DIR_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNameSecDesc[]        = SAM_DB_DIR_ATTR_SECURITY_DESCRIPTOR;
    PCSTR pszName = NULL;
    PCSTR pszSID = NULL;
    PSID pAccountSid = NULL;
    PCSTR pszDescription = NULL;
    PCSTR pszDomainName = NULL;
    PCSTR pszNetBIOSDomain = NULL;
    SAMDB_ACB AccountFlags;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PSTR pszObjectDN = NULL;
    PWSTR pwszObjectDN = NULL;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszSID = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszNetBIOSDomain = NULL;
    DWORD dwGID = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    OCTET_STRING SecDescBlob = {0};
    ULONG ulSecDescLen = 0;
    ATTRIBUTE_VALUE avGroupName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avGID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDomainName = {0};
    ATTRIBUTE_VALUE avNetBIOSDomain = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    ATTRIBUTE_VALUE avAccountFlags = {0};
    ATTRIBUTE_VALUE avSecDesc = {0};
    DIRECTORY_MOD mods[11];
    ULONG     iMod = 0;
    DWORD     i = 0;

    for (i = 0; i < sizeof(BuiltinAccounts)/sizeof(BuiltinAccounts[0]); i++)
    {
        pszName          = BuiltinAccounts[i].pszName;
        pszSID           = BuiltinAccounts[i].pszSID;
        dwGID            = BuiltinAccounts[i].dwGID;
        pszDescription   = BuiltinAccounts[i].pszDescription;
        pszDomainName    = BuiltinAccounts[i].pszDomainName;
        pszNetBIOSDomain = BuiltinAccounts[i].pszNetBIOSDomain;
        AccountFlags     = BuiltinAccounts[i].flags;
        objectClass      = BuiltinAccounts[i].objectClass;

        iMod = 0;
        memset(mods, 0, sizeof(mods));

        dwError = LwAllocateStringPrintf(
                        &pszObjectDN,
                        "CN=%s,CN=Builtin,%s",
                        pszName,
                        pszDomainDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszObjectDN,
                        &pwszObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszName,
                        &pwszSamAccountName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszSID,
                        &pwszSID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszDomainName,
                        &pwszDomainName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszNetBIOSDomain,
                        &pwszNetBIOSDomain);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszDescription,
                        &pwszDescription);
        BAIL_ON_SAMDB_ERROR(dwError);

        ntStatus = RtlAllocateSidFromWC16String(&pAccountSid,
                                                pwszSID);
        BAIL_ON_NT_STATUS(ntStatus);

        dwError = SamDbCreateBuiltinGroupSecDesc(pAccountSid,
                                                 &pSecDesc,
                                                 &ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        SecDescBlob.pBytes     = (PBYTE)pSecDesc;
        SecDescBlob.ulNumBytes = ulSecDescLen;

        mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avSID.data.pwszStringValue = pwszSID;
        mods[iMod].pAttrValues = &avSID;

        mods[++iMod].pwszAttrName = &wszAttrNameGID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avGID.data.ulValue = dwGID;
        mods[iMod].pAttrValues = &avGID;

        mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avObjectClass.data.ulValue = objectClass;
        mods[iMod].pAttrValues = &avObjectClass;

        mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGroupName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avGroupName.data.pwszStringValue = pwszSamAccountName;
        mods[iMod].pAttrValues = &avGroupName;

        mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avGroupName;

        avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDomainName.data.pwszStringValue = pwszDomainName;
        mods[++iMod].pwszAttrName = &wszAttrNameDomainName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avDomainName;

        avNetBIOSDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avNetBIOSDomain.data.pwszStringValue = pwszNetBIOSDomain;
        mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avNetBIOSDomain;

        avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDescription.data.pwszStringValue = pwszDescription;
        mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avDescription;

        if (AccountFlags) {
            avAccountFlags.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avAccountFlags.data.ulValue = AccountFlags;
            mods[++iMod].pwszAttrName = &wszAttrAccountFlags[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            mods[iMod].pAttrValues = &avAccountFlags;
        }

        avSecDesc.Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
        avSecDesc.data.pOctetString = &SecDescBlob;
        mods[++iMod].pwszAttrName = &wszAttrNameSecDesc[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avSecDesc;

        mods[++iMod].pwszAttrName = NULL;
        mods[iMod].pAttrValues = NULL;

        dwError = SamDbAddObject(
                        hDirectory,
                        pwszObjectDN,
                        mods);
        BAIL_ON_SAMDB_ERROR(dwError);

        DIRECTORY_FREE_STRING_AND_RESET(pszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszSamAccountName);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszSID);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDomainName);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszNetBIOSDomain);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDescription);
        DIRECTORY_FREE_MEMORY_AND_RESET(pSecDesc);

        RTL_FREE(&pAccountSid);
    }

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    DIRECTORY_FREE_MEMORY(pwszSID);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSDomain);
    DIRECTORY_FREE_MEMORY(pwszDescription);
    DIRECTORY_FREE_MEMORY(pSecDesc);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddLocalAccounts(
    HANDLE    hDirectory,
    PCSTR     pszDomainDN,
    PCSTR     pszNetBIOSName,
    PSID      pMachineSid
    )
{
    struct local_account {
        PCSTR               pszName;
        DWORD               dwUid;
        DWORD               dwGid;
        DWORD               dwRid;
        PCSTR               pszDescription;
        PCSTR               pszShell;
        PCSTR               pszHomedir;
        SAMDB_ACB           flags;
        SAMDB_OBJECT_CLASS  objectClass;
    } LocalAccounts[] = {
        {
            .pszName          = "Administrator",
            .dwUid            = SAM_DB_UID_FROM_RID(DOMAIN_USER_RID_ADMIN),
            .dwGid            = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_LW_USERS),
            .dwRid            = DOMAIN_USER_RID_ADMIN,
            .pszDescription   = "Built-in account for administering the "
                                "computer/domain",
            .pszShell         = SAM_DB_DEFAULT_ADMINISTRATOR_SHELL,
            .pszHomedir       = SAM_DB_DEFAULT_ADMINISTRATOR_HOMEDIR,
            .flags            = SAMDB_ACB_NORMAL | SAMDB_ACB_DISABLED,
            .objectClass      = SAMDB_OBJECT_CLASS_USER
        },
        {
            .pszName          = "Guest",
            .dwUid            = SAM_DB_UID_FROM_RID(DOMAIN_USER_RID_GUEST),
            .dwGid            = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_LW_USERS),
            .dwRid            = DOMAIN_USER_RID_GUEST,
            .pszDescription   = "Built-in account for guest access to the "
                                "computer/domain",
            .pszShell         = SAM_DB_DEFAULT_GUEST_SHELL,
            .pszHomedir       = SAM_DB_DEFAULT_GUEST_HOMEDIR,
            .flags            = SAMDB_ACB_NORMAL | SAMDB_ACB_DISABLED,
            .objectClass      = SAMDB_OBJECT_CLASS_USER
        },
        {
            .pszName          = "Likewise Users",
            .dwUid            = 0,
            .dwGid            = SAM_DB_GID_FROM_RID(DOMAIN_ALIAS_RID_LW_USERS),
            .dwRid            = DOMAIN_ALIAS_RID_LW_USERS,
            .pszDescription   = "Built-in group of Likewise Users",
            .pszShell         = NULL,
            .pszHomedir       = NULL,
            .flags            = 0,
            .objectClass      = SAMDB_OBJECT_CLASS_LOCAL_GROUP
        }
    };

    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    WCHAR wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrNameUID[]            = SAM_DB_DIR_ATTR_UID;
    WCHAR wszAttrNameGID[]            = SAM_DB_DIR_ATTR_GID;
    WCHAR wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrNameDescription[]    = SAM_DB_DIR_ATTR_DESCRIPTION;
    WCHAR wszAttrNameShell[]          = SAM_DB_DIR_ATTR_SHELL;
    WCHAR wszAttrNameHomedir[]        = SAM_DB_DIR_ATTR_HOME_DIR;
    WCHAR wszAttrAccountFlags[]       = SAM_DB_DIR_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNamePrimaryGroup[]   = SAM_DB_DIR_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrNameDomain[]         = SAM_DB_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[]  = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameSecDesc[]        = SAM_DB_DIR_ATTR_SECURITY_DESCRIPTOR;
    PCSTR pszName = NULL;
    DWORD dwUID = 0;
    DWORD dwGID = 0;
    DWORD dwRid = 0;
    PCSTR pszDescription = NULL;
    PCSTR pszShell = NULL;
    PCSTR pszHomedir = NULL;
    PCSTR pszDomain = NULL;
    PCSTR pszNetBIOSDomain = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    SAMDB_ACB AccountFlags = 0;
    PSTR pszObjectDN = NULL;
    PSID pAccountSid = NULL;
    ULONG ulAccountSidLength = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszObjectDN = NULL;
    PWSTR pwszSID = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszShell = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszNetBIOSDomain = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    OCTET_STRING SecDescBlob = {0};
    ULONG ulSecDescLen = 0;
    ATTRIBUTE_VALUE avUserName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avUID = {0};
    ATTRIBUTE_VALUE avGID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    ATTRIBUTE_VALUE avAccountFlags = {0};
    ATTRIBUTE_VALUE avShell = {0};
    ATTRIBUTE_VALUE avHomedir = {0};
    ATTRIBUTE_VALUE avDomain = {0};
    ATTRIBUTE_VALUE avNetBIOSDomain = {0};
    ATTRIBUTE_VALUE avSecDesc = {0};
    DIRECTORY_MOD mods[14];
    ULONG iMod = 0;
    DWORD i = 0;

    for (i = 0; i < sizeof(LocalAccounts)/sizeof(LocalAccounts[0]); i++) {

        pszName          = LocalAccounts[i].pszName;
        dwUID            = LocalAccounts[i].dwUid;
        dwGID            = LocalAccounts[i].dwGid;
        dwRid            = LocalAccounts[i].dwRid;
        pszDescription   = LocalAccounts[i].pszDescription;
        AccountFlags     = LocalAccounts[i].flags;
        objectClass      = LocalAccounts[i].objectClass;
        pszShell         = LocalAccounts[i].pszShell;
        pszHomedir       = LocalAccounts[i].pszHomedir;
        pszDomain        = pszNetBIOSName;
        pszNetBIOSDomain = pszNetBIOSName;

        iMod    = 0;
        memset(mods, 0, sizeof(mods));

        ulAccountSidLength = RtlLengthRequiredSid(
                                  pMachineSid->SubAuthorityCount + 1);
        dwError = LwAllocateMemory(ulAccountSidLength, (void**)&pAccountSid);
        BAIL_ON_SAMDB_ERROR(dwError);

        status = RtlCopySid(ulAccountSidLength, pAccountSid, pMachineSid);
        if (status != 0) {
            dwError = LW_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        status = RtlAppendRidSid(ulAccountSidLength, pAccountSid, dwRid);
        if (status != 0) {
            dwError = LW_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = LwAllocateStringPrintf(
                        &pszObjectDN,
                        "CN=%s,CN=Users,%s",
                        pszName,
                        pszDomainDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszObjectDN,
                        &pwszObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        status = RtlAllocateWC16StringFromSid(
                        &pwszSID,
                        pAccountSid);
        if (status != 0) {
            dwError = LW_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = LwMbsToWc16s(
                        pszName,
                        &pwszSamAccountName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszDomain,
                        &pwszDomain);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszNetBIOSDomain,
                        &pwszNetBIOSDomain);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwMbsToWc16s(
                        pszDescription,
                        &pwszDescription);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (pszShell) {
            dwError = LwMbsToWc16s(
                            pszShell,
                            &pwszShell);
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (pszHomedir) {
            dwError = LwMbsToWc16s(
                            pszHomedir,
                            &pwszHomedir);
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        if (objectClass == SAMDB_OBJECT_CLASS_USER)
        {
            dwError = SamDbCreateLocalUserSecDesc(pAccountSid,
                                                  &pSecDesc,
                                                  &ulSecDescLen);
        }
        else if (objectClass == SAMDB_OBJECT_CLASS_LOCAL_GROUP)
        {
            dwError = SamDbCreateLocalGroupSecDesc(pAccountSid,
                                                   &pSecDesc,
                                                   &ulSecDescLen);
        }
        BAIL_ON_SAMDB_ERROR(dwError);

        SecDescBlob.pBytes     = (PBYTE)pSecDesc;
        SecDescBlob.ulNumBytes = ulSecDescLen;

        mods[iMod].pwszAttrName = &wszAttrNameObjectClass[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avObjectClass.data.ulValue = objectClass;
        mods[iMod].pAttrValues = &avObjectClass;

        mods[++iMod].pwszAttrName = &wszAttrNameObjectSID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avSID.data.pwszStringValue = pwszSID;
        mods[iMod].pAttrValues = &avSID;

        if (dwUID) {
            mods[++iMod].pwszAttrName = &wszAttrNameUID[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avUID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avUID.data.ulValue = dwUID;
            mods[iMod].pAttrValues = &avUID;
        }

        if (dwGID && objectClass == SAMDB_OBJECT_CLASS_USER) {
            mods[++iMod].pwszAttrName = &wszAttrNamePrimaryGroup[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avGID.data.ulValue = dwGID;
            mods[iMod].pAttrValues = &avGID;

        } else if (dwGID && objectClass == SAMDB_OBJECT_CLASS_LOCAL_GROUP) {
            mods[++iMod].pwszAttrName = &wszAttrNameGID[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avGID.data.ulValue = dwGID;
            mods[iMod].pAttrValues = &avGID;

        }

        mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avUserName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avUserName.data.pwszStringValue = pwszSamAccountName;
        mods[iMod].pAttrValues = &avUserName;

        mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avUserName;

        if (AccountFlags) {
            mods[++iMod].pwszAttrName = &wszAttrAccountFlags[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avAccountFlags.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avAccountFlags.data.ulValue = AccountFlags;
            mods[iMod].pAttrValues = &avAccountFlags;
        }

        if (pwszDescription) {
            mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
            avDescription.data.pwszStringValue = pwszDescription;
            mods[iMod].pAttrValues = &avDescription;
        }

        if (pwszShell) {
            mods[++iMod].pwszAttrName = &wszAttrNameShell[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avShell.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
            avShell.data.pwszStringValue = pwszShell;
            mods[iMod].pAttrValues = &avShell;
        }

        if (pwszHomedir) {
            mods[++iMod].pwszAttrName = &wszAttrNameHomedir[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            avHomedir.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
            avHomedir.data.pwszStringValue = pwszHomedir;
            mods[iMod].pAttrValues = &avHomedir;
        }

        mods[++iMod].pwszAttrName = &wszAttrNameDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDomain.data.pwszStringValue = pwszDomain;
        mods[iMod].pAttrValues = &avDomain;

        mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avNetBIOSDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avNetBIOSDomain.data.pwszStringValue = pwszNetBIOSDomain;
        mods[iMod].pAttrValues = &avNetBIOSDomain;

        mods[++iMod].pwszAttrName = &wszAttrNameSecDesc[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avSecDesc.Type = DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR;
        avSecDesc.data.pOctetString = &SecDescBlob;
        mods[iMod].pAttrValues = &avSecDesc;

        mods[++iMod].pwszAttrName = NULL;
        mods[iMod].pAttrValues = NULL;

        dwError = SamDbAddObject(
                        hDirectory,
                        pwszObjectDN,
                        mods);
        BAIL_ON_SAMDB_ERROR(dwError);

        DIRECTORY_FREE_STRING_AND_RESET(pszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszSamAccountName);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDescription);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszShell);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszHomedir);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDomain);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszNetBIOSDomain);
        DIRECTORY_FREE_MEMORY_AND_RESET(pSecDesc);
        DIRECTORY_FREE_MEMORY_AND_RESET(pAccountSid);
        RTL_FREE(&pwszSID);
    }

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    DIRECTORY_FREE_MEMORY(pwszDescription);
    DIRECTORY_FREE_MEMORY(pwszShell);
    DIRECTORY_FREE_MEMORY(pwszHomedir);
    DIRECTORY_FREE_MEMORY(pwszDomain);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSDomain);
    DIRECTORY_FREE_MEMORY(pSecDesc);
    DIRECTORY_FREE_MEMORY(pAccountSid);
    RTL_FREE(&pwszSID);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbSetupLocalGroupMemberships(
    HANDLE hDirectory
    )
{
    wchar_t wszGroupFilterFmt[] = L"%ws='%ws'";
    wchar_t wszMemberFilterFmt[] = L"%ws='%ws'";
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD iGroup = 0;
    DWORD iMember = 0;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    PWSTR pwszGroupFilter = NULL;
    DWORD dwGroupFilterLen = 0;
    WCHAR wszAttrDn[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrObjectSid[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PWSTR pwszGroupName = NULL;
    size_t dwGroupNameLen = 0;
    PDIRECTORY_ENTRY pGroupEntries = NULL;
    DWORD dwGroupEntriesNum = 0;
    PWSTR pwszMemberName = NULL;
    size_t dwMemberNameLen = 0;
    PWSTR pwszGroupDn = NULL;
    PWSTR pwszMemberFilter = NULL;
    DWORD dwMemberFilterLen = 0;
    PDIRECTORY_ENTRY pMemberEntries = NULL;
    DWORD dwMemberEntriesNum = 0;

    PWSTR wszGroupAttributes[] = {
        wszAttrDn,
        NULL
    };

    PWSTR wszMemberAttributes[] = {
        wszAttrDn,
        wszAttrObjectSid,
        NULL
    };

    PCSTR ppszAdministratorsMembers[] = {
        "Administrator",
        NULL
    };

    PCSTR ppszGuestsMembers[] = {
        "Guest",
        NULL
    };

    PCSTR ppszLikewiseUsersMembers[] =  {
        "Administrator",
        "Guest",
        NULL
    };
    
    struct group_members {
        PCSTR pszGroupName;
        PCSTR *ppszMembers;
    } LocalGroupMembers[] = {
        {
            .pszGroupName = "Administrators",
            .ppszMembers  = ppszAdministratorsMembers
        },
        {
            .pszGroupName = "Guests",
            .ppszMembers  = ppszGuestsMembers
        },
        {
            .pszGroupName = "Likewise Users",
            .ppszMembers  = ppszLikewiseUsersMembers
        }
    };

    for (iGroup = 0;
         iGroup < sizeof(LocalGroupMembers)/sizeof(LocalGroupMembers[0]);
         iGroup++)
    {
        PCSTR pszLocalGroupName = LocalGroupMembers[iGroup].pszGroupName;

        dwError = LwMbsToWc16s(pszLocalGroupName,
                               &pwszGroupName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LwWc16sLen(pwszGroupName, &dwGroupNameLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwGroupFilterLen = ((sizeof(wszAttrSamAccountName)/sizeof(WCHAR)) - 1) +
                           dwGroupNameLen +
                           (sizeof(wszGroupFilterFmt)/
                            sizeof(wszGroupFilterFmt[0]));

        dwError = LwAllocateMemory(sizeof(WCHAR) * dwGroupFilterLen,
                                   OUT_PPVOID(&pwszGroupFilter));
        BAIL_ON_SAMDB_ERROR(dwError);

        sw16printfw(pwszGroupFilter, dwGroupFilterLen, wszGroupFilterFmt,
                    wszAttrSamAccountName,
                    pwszGroupName);

        dwError = SamDbSearchObject(hDirectory,
                                    pwszBase,
                                    ulScope,
                                    pwszGroupFilter,
                                    wszGroupAttributes,
                                    FALSE,
                                    &pGroupEntries,
                                    &dwGroupEntriesNum);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (dwGroupEntriesNum)
        {
            PCSTR *ppszMembers = LocalGroupMembers[iGroup].ppszMembers;
            PDIRECTORY_ATTRIBUTE pAttr = &(pGroupEntries[0].pAttributes[0]);

            pwszGroupDn = pAttr[0].pValues[0].data.pwszStringValue;

            for (iMember = 0; ppszMembers[iMember]; iMember++)
            {
                dwError = LwMbsToWc16s(ppszMembers[iMember],
                                       &pwszMemberName);
                BAIL_ON_SAMDB_ERROR(dwError);

                dwError = LwWc16sLen(pwszMemberName, &dwMemberNameLen);
                BAIL_ON_SAMDB_ERROR(dwError);

                dwMemberFilterLen = ((sizeof(wszAttrSamAccountName)/
                                      sizeof(WCHAR)) -1 ) +
                                    dwMemberNameLen +
                                    (sizeof(wszMemberFilterFmt)/
                                     sizeof(wszMemberFilterFmt[0]));

                dwError = LwAllocateMemory(sizeof(WCHAR) * dwMemberFilterLen,
                                           OUT_PPVOID(&pwszMemberFilter));
                BAIL_ON_SAMDB_ERROR(dwError);

                if (sw16printfw(pwszMemberFilter, dwMemberFilterLen,
                                wszMemberFilterFmt,
                                wszAttrSamAccountName,
                                pwszMemberName) < 0)
                {
                    dwError = LwErrnoToWin32Error(errno);
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = SamDbSearchObject(hDirectory,
                                            pwszBase,
                                            ulScope,
                                            pwszMemberFilter,
                                            wszMemberAttributes,
                                            FALSE,
                                            &pMemberEntries,
                                            &dwMemberEntriesNum);
                BAIL_ON_SAMDB_ERROR(dwError);

                if (dwMemberEntriesNum)
                {
                    PDIRECTORY_ENTRY pMemberEntry = &(pMemberEntries[0]);

                    dwError = SamDbAddToGroup(hDirectory,
                                              pwszGroupDn,
                                              pMemberEntry);
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                if (pMemberEntries)
                {
                    for (i = 0; i < dwMemberEntriesNum; i++)
                    {
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                              .pValues[0].data.pwszStringValue);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                              .pValues[0].data.pwszStringValue);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                              .pValues);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                              .pValues);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                              .pwszName);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                              .pwszName);
                        DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes);
                    }

                    DIRECTORY_FREE_MEMORY(pMemberEntries);
                    pMemberEntries = NULL;
                }

                LW_SAFE_FREE_MEMORY(pwszMemberFilter);
                LW_SAFE_FREE_MEMORY(pwszMemberName);
            }
        }
                                                       
        if (pGroupEntries)
        {
            for (i = 0; i < dwGroupEntriesNum; i++)
            {
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pValues[0].data.pwszStringValue);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pValues);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pwszName);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes);
            }

            DIRECTORY_FREE_MEMORY(pGroupEntries);
            pGroupEntries = NULL;
        }

        LW_SAFE_FREE_MEMORY(pwszGroupName);
        LW_SAFE_FREE_MEMORY(pwszGroupFilter);
    }

cleanup:
    if (pGroupEntries)
    {
        for (i = 0; i < dwGroupEntriesNum; i++)
        {
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pValues[0].data.pwszStringValue);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pValues);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes[0]
                                      .pwszName);
                DIRECTORY_FREE_MEMORY(pGroupEntries[i].pAttributes);
        }

        DIRECTORY_FREE_MEMORY(pGroupEntries);
    }

    if (pMemberEntries)
    {
        for (i = 0; i < dwMemberEntriesNum; i++)
        {
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                  .pValues[0].data.pwszStringValue);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                  .pValues[0].data.pwszStringValue);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                  .pValues);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                  .pValues);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[0]
                                  .pwszName);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes[1]
                                  .pwszName);
            DIRECTORY_FREE_MEMORY(pMemberEntries[i].pAttributes);
        }

        DIRECTORY_FREE_MEMORY(pMemberEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupName);
    LW_SAFE_FREE_MEMORY(pwszGroupFilter);
    LW_SAFE_FREE_MEMORY(pwszMemberFilter);
    LW_SAFE_FREE_MEMORY(pwszMemberName);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamDbFixAcls(
    HANDLE hDirectory
    )
{

    DWORD dwError = 0;
    NTSTATUS status = 0;
    const wchar_t wszAnyObjectFilterFmt[] = L"%ws>0";
    const DWORD dwInt32StrSize = 10;
    WCHAR wszAttrRecordId[] = DIRECTORY_ATTR_RECORD_ID;
    WCHAR wszAttrObjectDN[] = DIRECTORY_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrSecurityDescriptor[] = DIRECTORY_ATTR_SECURITY_DESCRIPTOR;
    DWORD dwAnyObjectFilterLen = 0;
    PWSTR pwszAnyObjectFilter = NULL;
    ULONG ulScope = 0;
    DWORD iEntry = 0;
    ULONG ulAttributesOnly = 0;
    PWSTR pwszBase = NULL;
    PWSTR wszAttributes[] = {
        &wszAttrObjectDN[0],
        &wszAttrSecurityDescriptor[0],
        NULL
    };
    PDIRECTORY_ENTRY pObjectEntries = NULL;
    DWORD dwNumObjectEntries = 0;
    PDIRECTORY_ENTRY pObjectEntry = NULL;
    PWSTR pwszAccountObjectDN = NULL;
    POCTET_STRING pSecDescBlob = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pCurrentSecDesc = NULL;

    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    ULONG ulSecDescAbsLen = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelNew = NULL;
    ULONG ulSecDescRelNewLen = 1024;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;
    PSID pSamGroup = NULL;
    BOOLEAN bIsGroupDefaulted = FALSE;
    PSID pGroupSid = NULL;


    enum AttrValueIndex {
        ATTR_VAL_IDX_SEC_DESC = 0,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_SEC_DESC */
            .Type = DIRECTORY_ATTR_TYPE_OCTET_STREAM,
            .data.pOctetString = NULL
        }
    };

    DIRECTORY_MOD ModSecDesc = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSecurityDescriptor,
        1,
        &AttrValues[ATTR_VAL_IDX_SEC_DESC]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    OCTET_STRING NewSecDescBlob = {0};
    DWORD iMod = 0;


    dwAnyObjectFilterLen = ((sizeof(wszAttrRecordId)/sizeof(WCHAR) - 1) +
                            dwInt32StrSize +
                            (sizeof(wszAnyObjectFilterFmt)/
                             sizeof(wszAnyObjectFilterFmt[0])));
    dwError = LwAllocateMemory(dwAnyObjectFilterLen * sizeof(WCHAR),
                                (PVOID*)&pwszAnyObjectFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    sw16printfw(pwszAnyObjectFilter, dwAnyObjectFilterLen,
                wszAnyObjectFilterFmt,
                &wszAttrRecordId[0]);

    dwError = SamDbSearchObject(hDirectory,
                               pwszBase,
                               ulScope,
                               pwszAnyObjectFilter,
                               wszAttributes,
                               ulAttributesOnly,
                               &pObjectEntries,
                               &dwNumObjectEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iEntry = 0; iEntry < dwNumObjectEntries; iEntry++)
    {
        pObjectEntry = &(pObjectEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pObjectEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszAccountObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                  pObjectEntry,
                                  wszAttrSecurityDescriptor,
                                  DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
                                  &pSecDescBlob);
        BAIL_ON_SAMDB_ERROR(dwError);

        pCurrentSecDesc  = (PSECURITY_DESCRIPTOR_RELATIVE)pSecDescBlob->pBytes;

        if (!pCurrentSecDesc)
            continue;

        // Get sizes
        status = RtlSelfRelativeToAbsoluteSD(pCurrentSecDesc,
                                             pSecDescAbs, &ulSecDescAbsLen,
                                             pDacl, &ulDaclLen,
                                             pSacl, &ulSaclLen,
                                             pOwner, &ulOwnerLen,
                                             pGroup, &ulGroupLen);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            status = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(status);

        status = LW_RTL_ALLOCATE(&pSecDescAbs, VOID, ulSecDescAbsLen);
        BAIL_ON_NT_STATUS(status);

        if (ulOwnerLen)
        {
            status = LW_RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulGroupLen)
        {
            status = LW_RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulDaclLen)
        {
            status = LW_RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulSaclLen)
        {
            status = LW_RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
            BAIL_ON_NT_STATUS(status);
        }

        status = RtlSelfRelativeToAbsoluteSD(pCurrentSecDesc,
                                             pSecDescAbs, &ulSecDescAbsLen,
                                             pDacl, &ulDaclLen,
                                             pSacl, &ulSaclLen,
                                             pOwner, &ulOwnerLen,
                                             pGroup, &ulGroupLen);
        BAIL_ON_NT_STATUS(status);

        // Check whether group part is set, if NOT set, set it
        status = RtlGetGroupSecurityDescriptor(pSecDescAbs,
                                               &pSamGroup,
                                               &bIsGroupDefaulted);
        BAIL_ON_NT_STATUS(status);

        if (!pSamGroup)
        {

            status = RtlAllocateSidFromCString(&pGroupSid, "S-1-5-32-544");
            BAIL_ON_NT_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecDescAbs,
                         pGroupSid,
                         FALSE);
            BAIL_ON_NT_STATUS(status);

            // convert absolute back to relative before write to SamDb
            do
            {
                status = LwReallocMemory(pSecDescRelNew,
                                            (PVOID*)&pSecDescRelNew,
                                            ulSecDescRelNewLen);
                BAIL_ON_NT_STATUS(status);

                memset(pSecDescRelNew, 0, ulSecDescRelNewLen);

                status = RtlAbsoluteToSelfRelativeSD(pSecDescAbs,
                                                     pSecDescRelNew,
                                                     &ulSecDescRelNewLen);
                if (STATUS_BUFFER_TOO_SMALL  == status)
                {
                    ulSecDescRelNewLen *= 2;
                }
                else
                {
                    BAIL_ON_NT_STATUS(status);
                }
            }
            while((status != STATUS_SUCCESS) &&
                  (ulSecDescRelNewLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));

            // Write the fixed ACL back to registry
            NewSecDescBlob.pBytes     = (PBYTE)pSecDescRelNew;
            NewSecDescBlob.ulNumBytes = ulSecDescRelNewLen;

            AttrValues[ATTR_VAL_IDX_SEC_DESC].data.pOctetString = &NewSecDescBlob;
            Mods[iMod++] = ModSecDesc;

            dwError = SamDbModifyObject(hDirectory,
                                        pwszAccountObjectDN,
                                        Mods);
            BAIL_ON_SAMDB_ERROR(dwError);
        }
        BAIL_ON_NT_STATUS(status);

        iMod = 0;
        memset(&Mods, 0, sizeof(Mods));
        memset(&NewSecDescBlob, 0, sizeof(NewSecDescBlob));
        LW_SAFE_FREE_MEMORY(pSecDescRelNew);
        ulSecDescRelNewLen = 1024;
        SamDbFreeAbsoluteSecurityDescriptor(&pSecDescAbs);
        ulSecDescAbsLen = 0;
        ulDaclLen = 0;
        ulSaclLen = 0;
        ulOwnerLen = 0;
        ulGroupLen = 0;
    }

cleanup:

    if (pObjectEntries)
    {
        DirectoryFreeEntries(pObjectEntries, dwNumObjectEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszAnyObjectFilter);
    LW_SAFE_FREE_MEMORY(pSecDescRelNew);
    SamDbFreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    if (status != STATUS_SUCCESS &&
        dwError == ERROR_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(status);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbFixLocalAccounts(
    HANDLE hDirectory
    )
{

    DWORD dwError = 0;
    const wchar_t wszUserObjectFilterFmt[] = L"%ws = %u";
    const DWORD dwInt32StrSize = 10;
    WCHAR wszAttrObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDN[] = SAM_DB_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrAccountFlags[] = SAM_DB_DIR_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNtHash[] = SAM_DB_DIR_ATTR_NT_HASH;
    DWORD dwUserObjectFilterLen = 0;
    PWSTR pwszUserObjectFilter = NULL;
    ULONG ulScope = 0;
    ULONG ulAttributesOnly = 0;
    PWSTR pwszBase = NULL;
    PWSTR wszAttributes[] = {
        &wszAttrObjectDN[0],
        &wszAttrAccountFlags[0],
        &wszAttrNtHash[0],
        NULL
    };

    PDIRECTORY_ENTRY pUserEntries = NULL;
    DWORD dwNumUserEntries = 0;
    PDIRECTORY_ENTRY pUserEntry = NULL;
    DWORD iEntry = 0;
    PWSTR pwszUserObjectDN = NULL;
    DWORD dwAccountFlags = 0;
    POCTET_STRING pNtHash = NULL;
    DWORD iMod = 0;

    enum AttrValueIndex {
        ATTR_VAL_IDX_ACCOUNT_FLAGS = 0,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_ACCOUNT_FLAGS */
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.ulValue = 0
        }
    };

    DIRECTORY_MOD ModAccountFlags = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAccountFlags,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];
    memset(&Mods, 0, sizeof(Mods));

    dwUserObjectFilterLen = (sizeof(wszAttrObjectClass)/sizeof(wszAttrObjectClass[0]) +
                             dwInt32StrSize +
                             sizeof(wszUserObjectFilterFmt));
    dwError = LwAllocateMemory(dwUserObjectFilterLen * sizeof(WCHAR),
                               OUT_PPVOID(&pwszUserObjectFilter));
    BAIL_ON_SAMDB_ERROR(dwError);

    if (sw16printfw(pwszUserObjectFilter, dwUserObjectFilterLen,
                    wszUserObjectFilterFmt,
                    &wszAttrObjectClass[0], SAMDB_OBJECT_CLASS_USER) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbSearchObject(hDirectory,
                                pwszBase,
                                ulScope,
                                pwszUserObjectFilter,
                                wszAttributes,
                                ulAttributesOnly,
                                &pUserEntries,
                                &dwNumUserEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (iEntry = 0; iEntry < dwNumUserEntries; iEntry++)
    {
        pUserEntry = &(pUserEntries[iEntry]);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pUserEntry,
                                    wszAttrObjectDN,
                                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                    &pwszUserObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pUserEntry,
                                    wszAttrAccountFlags,
                                    DIRECTORY_ATTR_TYPE_INTEGER,
                                    &dwAccountFlags);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = DirectoryGetEntryAttrValueByName(
                                    pUserEntry,
                                    wszAttrNtHash,
                                    DIRECTORY_ATTR_TYPE_OCTET_STREAM,
                                    &pNtHash);
        BAIL_ON_SAMDB_ERROR(dwError);

        if ((pNtHash == NULL || pNtHash->ulNumBytes == 0) &&
            !(dwAccountFlags & SAMDB_ACB_DISABLED))
        {
            dwAccountFlags |= SAMDB_ACB_DISABLED;

            AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS].data.ulValue = dwAccountFlags;

            Mods[iMod++] = ModAccountFlags;

            dwError = SamDbModifyObject(hDirectory,
                                        pwszUserObjectDN,
                                        Mods);
            BAIL_ON_SAMDB_ERROR(dwError);

            iMod = 0;
            memset(&Mods, 0, sizeof(Mods));
        }
    }

cleanup:
    if (pUserEntries)
    {
        DirectoryFreeEntries(pUserEntries, dwNumUserEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszUserObjectFilter);

    return dwError;

error:
    goto cleanup;
}


static
VOID
SamDbFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL)) {
        return;
    }

    pSecDesc = *ppSecDesc;

    RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);
    RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    LW_RTL_FREE(&pSecDesc);
    LW_RTL_FREE(&pOwner);
    LW_RTL_FREE(&pGroup);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
