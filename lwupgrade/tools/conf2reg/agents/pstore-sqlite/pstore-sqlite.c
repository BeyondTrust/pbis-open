/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 * Abstract:
 *
 * Authors:
 *
 */

#include "includes.h"
#include "sqlite3.h"
#include <lsa/lsapstore-api.h>

DWORD
SqliteMachineAccountToPstore(
    PCSTR pszMachinePwdDb
    );

#define DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME_V1                  \
    "SELECT DomainSID,                                           \
            upper(DomainName),                                   \
            upper(DomainDnsName),                                \
            upper(HostName),                                     \
            upper(MachineAccountName),                           \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd                                           \
      WHERE upper(HostName) = upper(%Q)"

#define DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME_V2                  \
    "SELECT DomainSID,                                           \
            upper(DomainName),                                   \
            upper(DomainDnsName),                                \
            upper(HostName),                                     \
            HostDnsDomain,                                       \
            upper(MachineAccountName),                           \
            MachineAccountPassword,                              \
            PwdCreationTimestamp,                                \
            PwdClientModifyTimestamp,                            \
            SchannelType                                         \
       FROM machinepwd                                           \
      WHERE upper(HostName) = upper(%Q)"

DWORD
SqliteMachineAccountToPstore(
    PCSTR pszMachinePwdDb
    )
{
    DWORD dwError = 0;
    char szHostname[256] = { 0 };
    PSTR pszDot = NULL;
    PSTR pszError = NULL;
    sqlite3 *pDbHandle = NULL;
    PSTR pszQuery = NULL;
    int numRows = 0;
    int numCols = 0;
    PSTR *ppszResults = NULL;
    PCSTR pszValue = NULL;
    PSTR pszEndPtr = NULL;
    int i;
    LSA_MACHINE_PASSWORD_INFO_A passwordInfo = { { 0 } };
    PSTR pszFqdnHostname = NULL;
    PSTR pszFqdnSuffix = NULL;

    if ( gethostname(szHostname, sizeof(szHostname)-1) != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_UP_ERROR(dwError);
    }

    // Test to see if the name is still dotted. If so we will chop it down to
    // just the hostname field.
    pszDot = strchr(szHostname, '.');
    if (pszDot)
    {
        pszDot[0] = '\0';
    }

    dwError = sqlite3_open(pszMachinePwdDb, &pDbHandle);
    BAIL_ON_UP_ERROR(dwError);

    pszQuery = sqlite3_mprintf(
                DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME_V2,
                szHostname);
    if (pszQuery == NULL)
    {
        dwError = LW_ERROR_OUT_OF_MEMORY;
        BAIL_ON_UP_ERROR(dwError);
    }

    dwError = sqlite3_get_table(
                    pDbHandle,
                    pszQuery,
                    &ppszResults,
                    &numRows,
                    &numCols,
                    &pszError);
    if (dwError)
    {
        if (pszQuery)
        {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }
        pszQuery = sqlite3_mprintf(
                        DB_QUERY_GET_MACHINEPWD_BY_HOST_NAME_V1,
                        szHostname);
        if (pszQuery == NULL)
        {
            dwError = LW_ERROR_OUT_OF_MEMORY;
            BAIL_ON_UP_ERROR(dwError);
        }
        dwError = sqlite3_get_table(
                        pDbHandle,
                        pszQuery,
                        &ppszResults,
                        &numRows,
                        &numCols,
                        &pszError);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (ppszResults == NULL ||
        numRows == 0 ||
        LW_IS_NULL_OR_EMPTY_STR(ppszResults[1]))
    {
        dwError = NERR_SetupNotJoined;
    }
    else if (numRows != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    else if (!(numCols == 9 || numCols == 10))
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_UP_ERROR(dwError);

    i = numCols; // Skip column names

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &passwordInfo.Account.DomainSid);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &passwordInfo.Account.NetbiosDomainName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &passwordInfo.Account.DnsDomainName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &pszFqdnHostname);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (numCols == 10)
    {
        pszValue = ppszResults[i++];
        if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
        {
            dwError = LwAllocateString(pszValue, &pszFqdnSuffix);
            BAIL_ON_UP_ERROR(dwError);
        }
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &passwordInfo.Account.SamAccountName);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        dwError = LwAllocateString(pszValue, &passwordInfo.Password);
        BAIL_ON_UP_ERROR(dwError);
    }

    //tPwdCreationTimestamp -- not used.
    pszValue = ppszResults[i++];

    //tPwdClientModifyTimestamp
    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        time_t unixTime = (time_t) strtoll(pszValue, &pszEndPtr, 10);
        if (!pszEndPtr || (pszEndPtr == pszValue) || *pszEndPtr)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_UP_ERROR(dwError);
        }

        dwError = UpConvertTimeUnixToWindows(unixTime, &passwordInfo.Account.LastChangeTime);
        BAIL_ON_UP_ERROR(dwError);
    }

    pszValue = ppszResults[i++];
    if(!LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        DWORD dwSchannelType = (UINT32)atol(pszValue);
        passwordInfo.Account.AccountFlags = UpConvertSchannelTypeToMachineAccountFlags(dwSchannelType);
    }

    // Done reading database.  Now do data fixups.

    if (!pszFqdnSuffix && passwordInfo.Account.DnsDomainName)
    {
        dwError = LwAllocateString(passwordInfo.Account.DnsDomainName, &pszFqdnSuffix);
        BAIL_ON_UP_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &passwordInfo.Account.Fqdn,
                    "%s.%s",
                    pszFqdnHostname ? pszFqdnHostname : "",
                    pszFqdnSuffix ? pszFqdnSuffix : "");
    BAIL_ON_UP_ERROR(dwError);

    LwStrToUpper(passwordInfo.Account.DnsDomainName);
    LwStrToUpper(passwordInfo.Account.NetbiosDomainName);
    LwStrToUpper(passwordInfo.Account.DomainSid);
    LwStrToUpper(passwordInfo.Account.SamAccountName);
    passwordInfo.Account.KeyVersionNumber = 0;
    LwStrToLower(passwordInfo.Account.Fqdn);

    dwError = LsaPstoreSetPasswordInfoA(&passwordInfo);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (pDbHandle)
    {
        if (pszQuery)
        {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }

        if (ppszResults)
        {
            sqlite3_free_table(ppszResults);
            ppszResults = NULL;
        }

        sqlite3_close(pDbHandle);
        pDbHandle = NULL;
    }

    LW_SAFE_FREE_STRING(pszFqdnHostname);
    LW_SAFE_FREE_STRING(pszFqdnSuffix);

    LW_SAFE_FREE_STRING(passwordInfo.Account.DnsDomainName);
    LW_SAFE_FREE_STRING(passwordInfo.Account.NetbiosDomainName);
    LW_SAFE_FREE_STRING(passwordInfo.Account.DomainSid);
    LW_SAFE_FREE_STRING(passwordInfo.Account.SamAccountName);
    LW_SAFE_FREE_STRING(passwordInfo.Account.Fqdn);
    LW_SECURE_FREE_STRING(passwordInfo.Password);

    return dwError;

error:
    goto cleanup;
}
