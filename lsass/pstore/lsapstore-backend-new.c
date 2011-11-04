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
 *     lsapstore-backend-new.h
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     New Backend (NOT FINISHED)
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"

#define LSA_PSTORE_DOMAIN_KEY_PATH \
    HKEY_THIS_MACHINE "\\Services\\lsass\\Parameters\\Providers\\ActiveDirectory\\DomainJoin"

#define LSA_PSTORE_ACCOUNT_SUBKEY_NAME  "Pstore"
#define LSA_PSTORE_PASSWORD_SUBKEY_NAME "MachinePassword"

// Top-Level
#define LSA_PSTORE_VALUE_NAME_DEFAULT_DOMAIN        "DefaultDomain"

// Domain-Specific

// Account
#define LSA_PSTORE_VALUE_NAME_DNS_DOMAIN_NAME       "DnsDomainName"
#define LSA_PSTORE_VALUE_NAME_NETBIOS_DOMAIN_NAME   "NetbiosDomainName"
#define LSA_PSTORE_VALUE_NAME_DOMAIN_SID            "DomainSid"
#define LSA_PSTORE_VALUE_NAME_SAM_ACCOUNT_NAME      "SamAccountName"
#define LSA_PSTORE_VALUE_NAME_FQDN                  "Fqdn"
#define LSA_PSTORE_VALUE_NAME_TYPE                  "Type"
#define LSA_PSTORE_VALUE_NAME_KEY_VERSION_NUMBER    "KeyVersionNumber"
#define LSA_PSTORE_VALUE_NAME_LAST_CHANGE_TIME      "LastChangeTime"
// Password
#define LSA_PSTORE_VALUE_NAME_PASSWORD              "MachinePassword"

typedef struct _LSA_PSTORE_BACKEND_STATE {
    HANDLE RegistryConnection;
    PSID RootSid;
} LSA_PSTORE_BACKEND_STATE, *PLSA_PSTORE_BACKEND_STATE;

static LSA_PSTORE_BACKEND_STATE LsaPstoreBackendState = { 0 };

//
// Prototypes
//

static
DWORD
LsaPstorepReadData(
    IN HANDLE RegistryConnection,
    IN HKEY AccountKeyHandle,
    IN HKEY PasswordKeyHandle,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    );

//
// Functions
//

DWORD
LsaPstorpeBackendInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_BACKEND_STATE pState = &LsaPstoreBackendState;

    dwError = LwRegOpenServer(&pState->RegistryConnection);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    // TODO: RootSid

cleanup:
    if (dwError)
    {
        LsaPstorepBackendCleanup();
    }

    return dwError;
}

VOID
LsaPstorepBackendCleanup(
    VOID
    )
{
    PLSA_PSTORE_BACKEND_STATE pState = &LsaPstoreBackendState;

    if (pState->RegistryConnection)
    {
        LwRegCloseServer(pState->RegistryConnection);
        pState->RegistryConnection = NULL;
    }
    LSA_PSTORE_FREE(&pState->RootSid);
}

// See RegDB_ReadPassword().
DWORD
LsaPstorepBackendGetPasswordInfoW(
    IN OPTIONAL PCWSTR DomainName,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    HANDLE registryConnection = LsaPstoreBackendState.RegistryConnection;
    HKEY keyHandle = NULL;
    HKEY accountKeyHandle = NULL;
    HKEY passwordKeyHandle = NULL;
    PSTR pszUseDomainName = NULL;
    PSTR pszAccountRelativePath = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    NULL,
                    LSA_PSTORE_DOMAIN_KEY_PATH,
                    0,
                    GENERIC_READ,
                    &keyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    if (!DomainName)
    {
        dwError = LsaPstorepRegGetStringA(
                        registryConnection,
                        keyHandle,
                        LSA_PSTORE_VALUE_NAME_DEFAULT_DOMAIN,
                        &pszUseDomainName);
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }
    else
    {
        dwError = LwNtStatusToWin32Error(LwRtlCStringAllocateFromWC16String(
                        &pszUseDomainName,
                        DomainName));
        GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    }

    dwError = LwNtStatusToWin32Error(LwRtlCStringAllocatePrintf(
                    &pszAccountRelativePath,
                    "%s\\%s",
                    pszUseDomainName,
                    LSA_PSTORE_ACCOUNT_SUBKEY_NAME));
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    keyHandle,
                    pszAccountRelativePath,
                    0,
                    GENERIC_READ,
                    &accountKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LwRegOpenKeyExA(
                    registryConnection,
                    accountKeyHandle,
                    LSA_PSTORE_PASSWORD_SUBKEY_NAME,
                    0,
                    GENERIC_READ,
                    &passwordKeyHandle);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepReadData(
                    registryConnection,
                    accountKeyHandle,
                    passwordKeyHandle,
                    &pPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = ERROR_NOT_FOUND;
    }

    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&pPasswordInfo);
    }

    if (keyHandle)
    {
        LwRegCloseKey(registryConnection, keyHandle);
    }

    if (registryConnection)
    {
        LwRegCloseServer(registryConnection);
    }

    *PasswordInfo = pPasswordInfo;

    return dwError;
}

// See RegDB_WritePassword().
DWORD
LsaPstorepBackendSetPasswordInfoW(
    IN PLSA_MACHINE_PASSWORD_INFO_W PasswordInfo
    )
{
#if 0
    DWORD dwError = 0;
    int EE = 0;
    HANDLE registryConnection = LsaPstoreBackendState.RegistryConnection;
    HKEY keyHandle = NULL;
    HKEY accountKeyHandle = NULL;
    HKEY passwordKeyHandle = NULL;
    PSTR pszUseDomainName = NULL;
    PSTR pszAccountRelativePath = NULL;

    dwError = LsaPstoreGetCanonicalPasswordInfo(PasswordInfo, &pCanonicalPasswordInfo);

    dwError = LsaPstoreCreateKeyPath();
#else
    return 0;
#endif
}

static
DWORD
LsaPstorepReadData(
    IN HANDLE RegistryConnection,
    IN HKEY AccountKeyHandle,
    IN HKEY PasswordKeyHandle,
    OUT PLSA_MACHINE_PASSWORD_INFO_W* PasswordInfo
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfoA = NULL;
    PLSA_MACHINE_PASSWORD_INFO_W pPasswordInfo = NULL;
    ULONG64 lastChangeTime = 0;

    dwError = LSA_PSTORE_ALLOCATE_AUTO(&pPasswordInfoA);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_DNS_DOMAIN_NAME,
                    &pPasswordInfoA->Account.DnsDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_NETBIOS_DOMAIN_NAME,
                    &pPasswordInfoA->Account.NetbiosDomainName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_DOMAIN_SID,
                    &pPasswordInfoA->Account.DomainSid);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_SAM_ACCOUNT_NAME,
                    &pPasswordInfoA->Account.SamAccountName);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_FQDN,
                    &pPasswordInfoA->Account.Fqdn);

    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);
    dwError = LsaPstorepRegGetDword(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_TYPE,
                    &pPasswordInfoA->Account.AccountFlags);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetDword(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_KEY_VERSION_NUMBER,
                    &pPasswordInfoA->Account.KeyVersionNumber);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    dwError = LsaPstorepRegGetQword(
                    RegistryConnection,
                    AccountKeyHandle,
                    LSA_PSTORE_VALUE_NAME_LAST_CHANGE_TIME,
                    &lastChangeTime);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pPasswordInfoA->Account.LastChangeTime = (LONG64) lastChangeTime;

    dwError = LsaPstorepRegGetStringA(
                    RegistryConnection,
                    PasswordKeyHandle,
                    LSA_PSTORE_VALUE_NAME_PASSWORD,
                    &pPasswordInfoA->Password);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    LsaPstorepCStringUpcase(pPasswordInfoA->Account.DnsDomainName);
    LsaPstorepCStringUpcase(pPasswordInfoA->Account.NetbiosDomainName);
    LsaPstorepCStringUpcase(pPasswordInfoA->Account.SamAccountName);
    LsaPstorepCStringDowncase(pPasswordInfoA->Account.Fqdn);

    dwError = LsaPstorepConvertAnsiToWidePasswordInfo(
                    pPasswordInfoA,
                    &pPasswordInfo);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LSA_PSTORE_FREE_PASSWORD_INFO_A(&pPasswordInfoA);
        LSA_PSTORE_FREE_PASSWORD_INFO_W(&pPasswordInfo);
    }

    *PasswordInfo = pPasswordInfo;

    return dwError;
}
