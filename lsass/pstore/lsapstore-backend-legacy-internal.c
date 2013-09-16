/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *  
 * Module Name:
 *
 *     lsapstore-backend-legacy-internal.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Legacy Backend Internals Implementation
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *          Arlene Berry (aberry@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-backend-legacy.h"

#include <reg/reg.h>
#include <reg/regutil.h>
#include <lw/base.h>
#include <lwmem.h>
#include <lwstr.h>
// Work around bug in lwstr.h:
#include <wc16str.h>
#include "lsapstore-includes.h"
#include <assert.h>

#define PSTOREDB_REGISTRY_AD_KEY \
    "Services\\lsass\\Parameters\\Providers\\ActiveDirectory\\DomainJoin"
#define PSTOREDB_REGISTRY_PSTORE_SUBKEY \
    "Pstore"
#define PSTOREDB_REGISTRY_PASSWORD_INFO_SUBKEY \
    "PasswordInfo"
#define PSTOREDB_REGISTRY_DEFAULT_VALUE \
    "Default"


// Under PSTOREDB_REGISTRY_PSTORE_SUBKEY:
#define LWPS_REG_DNS_DOMAIN_NAME        "DnsDomainName"
#define LWPS_REG_NETBIOS_DOMAIN_NAME    "NetbiosDomainName"
#define LWPS_REG_DOMAIN_SID             "DomainSid"
#define LWPS_REG_SAM_ACCOUNT_NAME       "SamAccountName"
#define LWPS_REG_ACCOUNT_FLAGS          "AccountFlags"
#define LWPS_REG_KEY_VERSION_NUMBER     "KeyVersionNumber"
#define LWPS_REG_FQDN                   "Fqdn"
#define LWPS_REG_UNIX_LAST_CHANGE_TIME  "UnixLastChangeTime"
// Under PSTOREDB_REGISTRY_PASSWORD_INFO_SUBKEY:
#define LWPS_REG_PASSWORD               "Password"


struct _LWPS_LEGACY_STATE {
    HANDLE hReg;
    PSECURITY_DESCRIPTOR_ABSOLUTE pPasswordSecurityDescriptor;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAccountSecurityDescriptor;
};

//
// Prototypes
//

static
DWORD
LwpsLegacyCreateSecurityDescriptor(
    IN BOOLEAN bIsWorldReadable,
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    );

static
VOID
LwpsLegacyFreeSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    );

static
DWORD
LwpsConvertTimeWindowsToUnix(
    IN LONG64 WindowsTime,
    OUT time_t* pUnixTime
    );

static
DWORD
LwpsConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    );

//
// Functions
//

DWORD
LwpsLegacyOpenProvider(
    OUT PLWPS_LEGACY_STATE* ppContext
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLWPS_LEGACY_STATE pContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegOpenServer(&pContext->hReg);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyCreateSecurityDescriptor(
                    FALSE,
                    &pContext->pPasswordSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyCreateSecurityDescriptor(
                    TRUE,
                    &pContext->pAccountSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        if (pContext)
        {
            LwpsLegacyCloseProvider(pContext);
            pContext = NULL;
        }
    }

    *ppContext = pContext;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LwpsLegacyCloseProvider(
    IN PLWPS_LEGACY_STATE pContext
    )
{
    if (pContext)
    {
        if (pContext->hReg)
        {
            RegCloseServer(pContext->hReg);
        }
        LwpsLegacyFreeSecurityDescriptor(&pContext->pPasswordSecurityDescriptor);
        LwpsLegacyFreeSecurityDescriptor(&pContext->pAccountSecurityDescriptor);
        LwFreeMemory(pContext);
    }
}

DWORD
LwpsLegacyReadPassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDnsDomainName,
    OUT OPTIONAL PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;
    HKEY rootKeyHandle = NULL;
    HKEY accountKeyHandle = NULL;
    HKEY passwordKeyHandle = NULL;
    PSTR pszRegistryPath = NULL;
    PSTR pszDefaultDomain = NULL;
    DWORD unixLastChangeTime = 0;

    //
    // Compute registry path
    //

    dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pszDnsDomainName,
                  PSTOREDB_REGISTRY_PSTORE_SUBKEY);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Open keys
    //

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &rootKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    rootKeyHandle,
                    pszRegistryPath,
                    0,
                    KEY_READ,
                    &accountKeyHandle);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    accountKeyHandle,
                    PSTOREDB_REGISTRY_PASSWORD_INFO_SUBKEY,
                    0,
                    KEY_READ,
                    &passwordKeyHandle);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Allocate info structure
    //

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&pPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Read account portion
    //

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_DNS_DOMAIN_NAME,
                    &pPasswordInfo->Account.DnsDomainName);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_NETBIOS_DOMAIN_NAME,
                    &pPasswordInfo->Account.NetbiosDomainName);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_DOMAIN_SID,
                    &pPasswordInfo->Account.DomainSid);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_SAM_ACCOUNT_NAME,
                    &pPasswordInfo->Account.SamAccountName);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_ACCOUNT_FLAGS,
                    &pPasswordInfo->Account.AccountFlags);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_KEY_VERSION_NUMBER,
                    &pPasswordInfo->Account.KeyVersionNumber);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_FQDN,
                    &pPasswordInfo->Account.Fqdn);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_UNIX_LAST_CHANGE_TIME,
                    &unixLastChangeTime);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsConvertTimeUnixToWindows(
                    (time_t) unixLastChangeTime,
                    &pPasswordInfo->Account.LastChangeTime);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Read password portion
    //

    dwError = LsaPstorepRegGetStringA(
                    pContext->hReg,
                    passwordKeyHandle,
                    LWPS_REG_PASSWORD,
                    &pPasswordInfo->Password);
    if (LWREG_ERROR_NO_SUCH_KEY_OR_VALUE == dwError)
    {
        dwError = NERR_SetupNotJoined;
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&pPasswordInfo);
    }

    if (passwordKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, passwordKeyHandle);
    }
    if (accountKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, accountKeyHandle);
    }
    if (rootKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, rootKeyHandle);
    }

    LW_SAFE_FREE_MEMORY(pszRegistryPath);
    LW_SAFE_FREE_MEMORY(pszDefaultDomain);

    if (ppPasswordInfo)
    {
        *ppPasswordInfo = pPasswordInfo;
    }
    else
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&pPasswordInfo);
    }

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyWritePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    HKEY rootKeyHandle = NULL;
    HKEY accountKeyHandle = NULL;
    HKEY passwordKeyHandle = NULL;
    PSTR pszRegistryPath = NULL;
    PSTR pszDefaultDomain = NULL;
    time_t unixLastChangeTime = 0;

    //
    // Convert data
    //

    if (0 == pPasswordInfo->Account.LastChangeTime)
    {
        // Treat 0 as "now".
        unixLastChangeTime = time(NULL);
    }
    else
    {
        dwError = LwpsConvertTimeWindowsToUnix(
                        pPasswordInfo->Account.LastChangeTime,
                        &unixLastChangeTime);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    if ((unixLastChangeTime < 0) || (unixLastChangeTime > MAXDWORD))
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    //
    // Compute registry paths
    //

    dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pPasswordInfo->Account.DnsDomainName,
                  PSTOREDB_REGISTRY_PSTORE_SUBKEY);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Create keys
    //

    dwError = RegUtilAddKeySecDesc(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  NULL,
                  KEY_ALL_ACCESS,
                  pContext->pAccountSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = RegUtilAddKeySecDesc(
                  pContext->hReg,
                  NULL,
                  pszRegistryPath,
                  PSTOREDB_REGISTRY_PASSWORD_INFO_SUBKEY,
                  KEY_ALL_ACCESS,
                  pContext->pPasswordSecurityDescriptor);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Open keys
    //

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &rootKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    rootKeyHandle,
                    pszRegistryPath,
                    0,
                    KEY_WRITE,
                    &accountKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    pContext->hReg,
                    accountKeyHandle,
                    PSTOREDB_REGISTRY_PASSWORD_INFO_SUBKEY,
                    0,
                    KEY_WRITE,
                    &passwordKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Write account portion
    //

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_DNS_DOMAIN_NAME,
                    pPasswordInfo->Account.DnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_NETBIOS_DOMAIN_NAME,
                    pPasswordInfo->Account.NetbiosDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_DOMAIN_SID,
                    pPasswordInfo->Account.DomainSid);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_SAM_ACCOUNT_NAME,
                    pPasswordInfo->Account.SamAccountName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_ACCOUNT_FLAGS,
                    pPasswordInfo->Account.AccountFlags);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_KEY_VERSION_NUMBER,
                    pPasswordInfo->Account.KeyVersionNumber);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_FQDN,
                    pPasswordInfo->Account.Fqdn);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegSetDword(
                    pContext->hReg,
                    accountKeyHandle,
                    LWPS_REG_UNIX_LAST_CHANGE_TIME,
                    (DWORD) unixLastChangeTime);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    //
    // Write password portion
    //

    dwError = LsaPstorepRegSetStringA(
                    pContext->hReg,
                    passwordKeyHandle,
                    LWPS_REG_PASSWORD,
                    pPasswordInfo->Password);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (passwordKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, passwordKeyHandle);
    }
    if (accountKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, accountKeyHandle);
    }
    if (rootKeyHandle)
    {
        LwRegCloseKey(pContext->hReg, rootKeyHandle);
    }

    LW_SAFE_FREE_MEMORY(pszRegistryPath);
    LW_SAFE_FREE_MEMORY(pszDefaultDomain);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyDeletePassword(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    DWORD dwSubKeysCount = 0;
    DWORD dwValuesCount = 0;

    dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pszDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    RegUtilDeleteTree(
        pContext->hReg,
        NULL,
        pszRegistryPath,
        PSTOREDB_REGISTRY_PSTORE_SUBKEY);
    // TODO-What if there is an error?

    /* Delete domain key only if empty */
    dwError = RegUtilGetKeyObjectCounts(
                  pContext->hReg,
                  HKEY_THIS_MACHINE,
                  pszRegistryPath,
                  NULL,
                  &dwSubKeysCount,
                  &dwValuesCount);
    if (dwError)
    {
        dwError = 0;
    }
    else if (!dwSubKeysCount && !dwValuesCount)
    {
        RegUtilDeleteKey(
            pContext->hReg,
            HKEY_THIS_MACHINE,
            pszRegistryPath,
            NULL);
        // TODO-What if there is an error?
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszRegistryPath);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyGetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR* ppszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszDomainName = NULL;
    DWORD dwValueLen = 0;

    dwError = RegUtilGetValue(
                  pContext->hReg,
                  HKEY_THIS_MACHINE,
                  PSTOREDB_REGISTRY_AD_KEY,
                  NULL,
                  PSTOREDB_REGISTRY_DEFAULT_VALUE,
                  NULL,
                  (PVOID) &pszDomainName,
                  &dwValueLen);
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        // Treat as no default domain set
        assert(!pszDomainName);
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwpsLegacyReadPassword(
                    pContext,
                    pszDomainName,
                    NULL);
    if (dwError == NERR_SetupNotJoined)
    {
        // TODO-INCONSISTENT-FREE-FUNCTION!!!
        LW_SAFE_FREE_MEMORY(pszDomainName);
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        // TODO-INCONSISTENT-FREE-FUNCTION!!!
        LW_SAFE_FREE_MEMORY(pszDomainName);
    }

    *ppszDomainName = pszDomainName;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}


DWORD
LwpsLegacySetDefaultJoinedDomain(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;

    if (pszDomainName)
    {
        dwError = LwpsLegacyReadPassword(
                        pContext,
                        pszDomainName,
                        NULL);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = RegUtilAddKey(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = RegUtilSetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULT_VALUE,
                      REG_SZ,
                      (PVOID)pszDomainName,
                      pszDomainName ? strlen(pszDomainName) : 0);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = RegUtilDeleteValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      PSTOREDB_REGISTRY_AD_KEY,
                      NULL,
                      PSTOREDB_REGISTRY_DEFAULT_VALUE);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

cleanup:
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyGetJoinedDomains(
    IN PLWPS_LEGACY_STATE pContext,
    OUT PSTR** pppszDomainList,
    OUT PDWORD pdwDomainCount
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PWSTR* ppwszSubKeys = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszSubKeyPtr = NULL;
    PSTR* ppszDomainList = NULL;
    DWORD dwSubKeysLen = 0;
    DWORD dwIndexKeys = 0;
    DWORD dwIndexDomains = 0;
    DWORD dwDomainCount = 0;

    dwError = RegUtilIsValidKey(
                    pContext->hReg,
                    HKEY_THIS_MACHINE,
                    PSTOREDB_REGISTRY_AD_KEY);
    if (dwError)
    {
        dwError = 0;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = RegUtilGetKeys(
                    pContext->hReg,
                    HKEY_THIS_MACHINE,
                    PSTOREDB_REGISTRY_AD_KEY,
                    NULL,
                    &ppwszSubKeys,
                    &dwSubKeysLen);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (dwSubKeysLen)
    {
        dwError = LwAllocateMemory(
                        sizeof(*ppszDomainList) * dwSubKeysLen,
                        (PVOID*)&ppszDomainList);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        for (dwIndexKeys = 0, dwIndexDomains = 0 ; dwIndexKeys < dwSubKeysLen; dwIndexKeys++)
        {
            dwError = LwWc16sToMbs(ppwszSubKeys[dwIndexKeys], &pszSubKey);
            GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

            pszSubKeyPtr = strrchr(pszSubKey, '\\');
            if (pszSubKeyPtr)
            {
                dwError = LwAllocateString(
                              ++pszSubKeyPtr,
                              &ppszDomainList[dwIndexDomains]);
                GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

                LW_SAFE_FREE_STRING(pszSubKey);
            }
            else
            {
                ppszDomainList[dwIndexDomains] = pszSubKey;
                pszSubKey = NULL;
            }

            dwError = LwpsLegacyReadPassword(
                            pContext,
                            ppszDomainList[dwIndexDomains],
                            NULL);
            if (dwError == NERR_SetupNotJoined)
            {
                // The domain is not joined.
                dwError = 0;
                LW_SAFE_FREE_STRING(ppszDomainList[dwIndexDomains]);
            }
            else
            {
                dwIndexDomains++;
            }
        }
    }

    dwDomainCount = dwIndexDomains;

cleanup:
    if (dwError)
    {
        if (ppszDomainList)
        {
            LwpsLegacyFreeStringArray(ppszDomainList, dwSubKeysLen);
            ppszDomainList = NULL;
        }
        dwDomainCount = 0;
    }

    for (dwIndexKeys = 0; dwIndexKeys < dwSubKeysLen; dwIndexKeys++)
    {
        LW_SAFE_FREE_MEMORY(ppwszSubKeys[dwIndexKeys]);
    }
    LW_SAFE_FREE_MEMORY(ppwszSubKeys);

    LW_SAFE_FREE_STRING(pszSubKey);

    *pppszDomainList = ppszDomainList;
    *pdwDomainCount = dwDomainCount;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

VOID
LwpsLegacyFreeStringArray(
    IN PSTR* ppszDomainList,
    IN DWORD dwCount
    )
{
    if (ppszDomainList)
    {
        LwFreeStringArray(ppszDomainList, dwCount);
    }
}

static
DWORD
LwpsLegacyBuildRestrictedDaclForKey(
    IN PSID pAllAccessSid,
    IN OPTIONAL PSID pReadAccessSid,
    OUT PACL *ppDacl
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    DWORD dwSidCount = 0;
    DWORD dwTotalSidSize = 0;
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;
    DWORD dwError = 0;

    dwSidCount++;
    dwTotalSidSize = RtlLengthSid(pAllAccessSid);

    if (pReadAccessSid)
    {
        dwSidCount++;
        dwTotalSidSize += RtlLengthSid(pReadAccessSid);
    }

    dwDaclSize = ACL_HEADER_SIZE +
        dwSidCount * LW_FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
        dwTotalSidSize;

    status = LW_RTL_ALLOCATE(&pDacl, VOID, dwDaclSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlAddAccessAllowedAceEx(pDacl,
                                      ACL_REVISION,
                                      0,
                                      KEY_ALL_ACCESS,
                                      pAllAccessSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (pReadAccessSid)
    {
        status = RtlAddAccessAllowedAceEx(pDacl,
                                          ACL_REVISION,
                                          0,
                                          KEY_READ,
                                          pReadAccessSid);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        LW_RTL_FREE(&pDacl);
    }

    *ppDacl = pDacl;

    dwError = LwNtStatusToWin32Error(status);
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
DWORD
LwpsLegacyCreateSecurityDescriptor(
    IN BOOLEAN bIsWorldReadable,
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PACL pDacl = NULL;
    PSID pRootSid = NULL;
    PSID pAdminsSid = NULL;
    PSID pEveryoneSid = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    DWORD dwError = 0;

    status = RtlAllocateWellKnownSid(WinLocalSystemSid, NULL, &pRootSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlAllocateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &pAdminsSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (bIsWorldReadable)
    {
        status = RtlAllocateWellKnownSid(WinWorldSid, NULL, &pEveryoneSid);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = LW_RTL_ALLOCATE(
                    &pSecDesc,
                    VOID,
                    SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlCreateSecurityDescriptorAbsolute(
                    pSecDesc,
                    SECURITY_DESCRIPTOR_REVISION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Owner: Root

    status = RtlDuplicateSid(&pOwnerSid, pRootSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetOwnerSecurityDescriptor(
                    pSecDesc,
                    pOwnerSid,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pOwnerSid = NULL;

    // Group: Administrators

    status = RtlDuplicateSid(&pGroupSid, pAdminsSid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetGroupSecurityDescriptor(
                    pSecDesc,
                    pGroupSid,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pGroupSid = NULL;

    // Do not set Sacl currently

    // DACL
    status = LwpsLegacyBuildRestrictedDaclForKey(
                    pRootSid,
                    pEveryoneSid,
                    &pDacl);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlSetDaclSecurityDescriptor(
                    pSecDesc,
                    TRUE,
                    pDacl,
                    FALSE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    pDacl = NULL;

    if (!RtlValidSecurityDescriptor(pSecDesc))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        if (pSecDesc)
        {
            LwpsLegacyFreeSecurityDescriptor(&pSecDesc);
        }
    }

    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pRootSid);
    LW_RTL_FREE(&pAdminsSid);
    LW_RTL_FREE(&pEveryoneSid);
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);

    *ppSecurityDescriptor = pSecDesc;

    dwError = LwNtStatusToWin32Error(status);
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LwpsLegacyFreeSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    )
{
    if (*ppSecurityDescriptor)
    {
        PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = *ppSecurityDescriptor;
        PSID pOwner = NULL;
        PSID pGroup = NULL;
        PACL pDacl = NULL;
        PACL pSacl = NULL;
        BOOLEAN bDefaulted = FALSE;
        BOOLEAN bPresent = FALSE;

        RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
        RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);
        RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
        RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

        LW_RTL_FREE(&pSecDesc);
        LW_RTL_FREE(&pOwner);
        LW_RTL_FREE(&pGroup);
        LW_RTL_FREE(&pDacl);
        LW_RTL_FREE(&pSacl);

        *ppSecurityDescriptor = NULL;
    }
}

static
DWORD
LwpsConvertTimeWindowsToUnix(
    IN LONG64 WindowsTime,
    OUT time_t* pUnixTime
    )
{
    DWORD dwError = 0;
    LONG64 unixTime = 0;

    if (WindowsTime < 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    unixTime = WindowsTime/10000000LL - 11644473600LL;

    // Ensure the range is within time_t.
    if (unixTime != (time_t) unixTime)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        unixTime = 0;
    }

    *pUnixTime = (time_t) unixTime;

    return 0;
}

static
DWORD
LwpsConvertTimeUnixToWindows(
    IN time_t UnixTime,
    OUT PLONG64 pWindowsTime
    )
{
    DWORD dwError = 0;
    LONG64 windowsTime = 0;

#if SIZEOF_TIME_T > 4
    if ((LONG64) UnixTime < - 11644473600LL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
#endif

    windowsTime = (LONG64) UnixTime + 11644473600LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
    windowsTime *= 10000000LL;
    if (windowsTime < 0)
    {
        dwError = ERROR_ARITHMETIC_OVERFLOW;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

cleanup:
    if (dwError)
    {
        windowsTime = 0;
    }

    *pWindowsTime = windowsTime;

    return 0;
}

DWORD
LwpsLegacySetJoinedDomainTrustEnumerationWaitTime(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    DWORD dwValue = 0;
    
    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pszDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        dwError = RegUtilSetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAIT_VALUE,
                      REG_DWORD,
                      (PVOID)&dwValue,
                      sizeof(dwValue));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        dwError = RegUtilSetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAITSECONDS_VALUE,
                      REG_DWORD,
                      (PVOID)&dwValue,
                      sizeof(dwValue));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    }

cleanup:
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyGetJoinedDomainTrustEnumerationWaitTime(
    IN PLWPS_LEGACY_STATE pContext,
    IN OPTIONAL PCSTR pszDomainName,
    OUT PDWORD* ppdwTrustEnumerationWaitSeconds,
    OUT PDWORD* ppdwTrustEnumerationWaitEnabled    
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    PVOID dwValue = NULL;
    PVOID dwValue1 = NULL;
    REG_DATA_TYPE readType = 0;
    DWORD dwValueSize = 0;
    DWORD dwValueSize1 = 0;

    if (pszDomainName)
    {
        dwError = LwAllocateStringPrintf(
                  &pszRegistryPath,
                  "%s\\%s",
                  PSTOREDB_REGISTRY_AD_KEY,
                  pszDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        dwError = RegUtilGetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAIT_VALUE,
                      &readType,
                      &dwValue,
                      &dwValueSize);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
        dwError = RegUtilGetValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAITSECONDS_VALUE,
                      &readType,
                      &dwValue1,
                      &dwValueSize1);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        *ppdwTrustEnumerationWaitSeconds = (PDWORD) dwValue1;
        *ppdwTrustEnumerationWaitEnabled = (PDWORD) dwValue;      
    }

cleanup:
    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

DWORD
LwpsLegacyDeleteTrustEnumerationWaitInfo(
    IN PLWPS_LEGACY_STATE pContext,
    IN PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PSTR pszRegistryPath = NULL;
    DWORD dwSubKeysCount = 0;
    DWORD dwValuesCount = 0;
    
    if(pszDomainName)
    { 
        dwError = LwAllocateStringPrintf(
                      &pszRegistryPath,
                      "%s\\%s",
                      PSTOREDB_REGISTRY_AD_KEY,
                      pszDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

        dwError = RegUtilDeleteValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAIT_VALUE);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
  
        dwError = RegUtilDeleteValue(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      PSTOREDB_REGISTRY_TRUSTENUMERATIONWAITSECONDS_VALUE);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
 
        /* Delete domain key only if empty */
        dwError = RegUtilGetKeyObjectCounts(
                      pContext->hReg,
                      HKEY_THIS_MACHINE,
                      pszRegistryPath,
                      NULL,
                      &dwSubKeysCount,
                      &dwValuesCount);
        if (dwError)
        {
            dwError = 0;
        }
        else if (!dwSubKeysCount && !dwValuesCount)
        {
            RegUtilDeleteKey(
                pContext->hReg,
                HKEY_THIS_MACHINE,
                pszRegistryPath,
                NULL);
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszRegistryPath);

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}
