/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "DomainJoinConfig.h"
#include "djapi.h"

#include "domainjoin.h"
#include <lsa/lsa.h>

#include <stdio.h>


DWORD
DJInit(
    VOID
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL; 

    setlocale(LC_ALL, "");

    LW_CLEANUP_CTERR(&exc, dj_disable_logging());

cleanup:

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }

    return dwError;
}

DWORD
DJSetComputerNameEx(
    PCSTR pszComputerName
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL;
    HANDLE hLsaConnection = NULL;

    LW_TRY(&exc, DJSetComputerName(pszComputerName, NULL, &LW_EXC));

    LW_CLEANUP_CTERR(&exc, LsaOpenServer(&hLsaConnection));

    LW_CLEANUP_CTERR(&exc, LsaSetMachineName(hLsaConnection, pszComputerName));

cleanup:

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }

    return dwError;
}

DWORD
DJJoinDomain(
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL; 
    JoinProcessOptions options;

    DJZeroJoinProcessOptions(&options);
    options.joiningDomain = TRUE;

    if (IsNullOrEmptyString(pszDomain))
    {
        LW_RAISE(&exc, ERROR_INVALID_PARAMETER);
    }

    LW_CLEANUP_CTERR(&exc, CTStrdup(pszDomain, &options.domainName));

    if (!IsNullOrEmptyString(pszOU))
    {
        LW_CLEANUP_CTERR(&exc, CTStrdup(pszOU, &options.ouName));
    }

    if (!IsNullOrEmptyString(pszUsername))
    {
        LW_CLEANUP_CTERR(&exc, CTStrdup(pszUsername, &options.username));
    }

    if (!IsNullOrEmptyString(pszPassword))
    {
        LW_CLEANUP_CTERR(&exc, CTStrdup(pszPassword, &options.password));
    }

    LW_CLEANUP_CTERR(&exc, DJGetComputerName(&options.computerName));

    LW_TRY(&exc, DJInitModuleStates(&options, &LW_EXC));

    LW_TRY(&exc, DJRunJoinProcess(&options, &LW_EXC));

cleanup:

    DJFreeJoinProcessOptions(&options);

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }

    return dwError;
}

DWORD
DJQueryJoinInformation(
    PSTR* ppszComputerName,
    PSTR* ppszDomainName,
    PSTR* ppszComputerDN
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL;
    PSTR  pszComputerName = NULL;
    PSTR  pszDomainName = NULL;
    PSTR  pszComputerDN = NULL;

    LW_TRY(&exc, DJQuery(&pszComputerName, &pszDomainName, NULL, &LW_EXC));

    if (!IsNullOrEmptyString(pszDomainName))
    {
       LW_TRY(&exc, DJGetComputerDN(&pszComputerDN, &LW_EXC));
    }

    *ppszComputerName = pszComputerName;
    *ppszDomainName = pszDomainName;
    *ppszComputerDN = pszComputerDN;

cleanup:

    if (!LW_IS_OK(exc))
    {
        CT_SAFE_FREE_STRING(pszComputerName);
        CT_SAFE_FREE_STRING(pszDomainName);
        CT_SAFE_FREE_STRING(pszComputerDN);

        dwError = exc->code;
        LWHandle(&exc);
    }

    return dwError;
}

DWORD
DJUnjoinDomain(
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL; 
    JoinProcessOptions options;

    DJZeroJoinProcessOptions(&options);
    options.joiningDomain = FALSE;

    if (!IsNullOrEmptyString(pszUsername))
    {
        LW_CLEANUP_CTERR(&exc, CTStrdup(pszUsername, &options.username));
    }

    if (!IsNullOrEmptyString(pszPassword))
    {
        LW_CLEANUP_CTERR(&exc, CTStrdup(pszPassword, &options.password));
    }

    LW_CLEANUP_CTERR(&exc, DJGetComputerName(&options.computerName));

    LW_TRY(&exc, DJInitModuleStates(&options, &LW_EXC));

    LW_TRY(&exc, DJRunJoinProcess(&options, &LW_EXC));

cleanup:

    DJFreeJoinProcessOptions(&options);

    if (!LW_IS_OK(exc))
    {
        dwError = exc->code;
        LWHandle(&exc);
    }

    return dwError;
}

VOID
DJFreeMemory(
    PVOID pMemory
    )
{
    CTFreeMemory(pMemory);
}

DWORD
DJShutdown(
    VOID
    )
{
     return 0;
}

void
DJQuery(
    char **computer, 
    char **domain,
    DJOptions* options,
    LWException** exc
    )
{
    PDOMAINJOININFO info = NULL;

    LW_TRY(exc, QueryInformation(&info, &LW_EXC));
    
    if (info->pszName)
    {
	LW_CLEANUP_CTERR(exc,
			 CTAllocateString(info->pszName, computer));
    }
    else
    {
	*computer = NULL;
    }

    if (info->pszDomainName)
    {
	LW_CLEANUP_CTERR(exc,
			 CTAllocateString(info->pszDomainName, domain));
    }
    else
    {
	*domain = NULL;
    }
    
cleanup:
    
    if (info)
    {
	FreeDomainJoinInfo(info);
    }
}

void
DJRenameComputer(
    const char* computer,
    const char* domain,
    DJOptions* options,
    LWException** exc
    )
{
    LW_TRY(exc, DJSetComputerName((char*) computer, (char*) domain, &LW_EXC));

cleanup:

    return;
}
