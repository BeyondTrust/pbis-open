/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

#include "djapi.h"

#include "domainjoin.h"

#include <stdio.h>


DWORD
DJInit(
    VOID
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL; 
    BOOLEAN bEnableDcerpcd = FALSE;

    setlocale(LC_ALL, "");

    LW_CLEANUP_CTERR(&exc, dj_disable_logging());

    LW_TRY(&exc, DJNetInitialize(bEnableDcerpcd, &LW_EXC));

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

    LW_TRY(&exc, DJSetComputerName(pszComputerName, NULL, &LW_EXC));

cleanup:

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
    PSTR* ppszDomainName
    )
{
    DWORD dwError = 0;
    LWException *exc = NULL;

    LW_TRY(&exc, DJQuery(ppszComputerName, ppszDomainName, NULL, &LW_EXC));

cleanup:

    if (!LW_IS_OK(exc))
    {
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
     DJNetShutdown(NULL);

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
