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

#include "domainjoin.h"
#include "djroutines.h"
#include "djauthinfo.h"

void
QueryInformation(
    PDOMAINJOININFO* ppDomainJoinInfo,
    LWException **exc
    )
{
    PDOMAINJOININFO pDomainJoinInfo = NULL;
    LWException *inner = NULL;

    LW_CLEANUP_CTERR(exc, CTAllocateMemory(sizeof(DOMAINJOININFO), (PVOID*)(PVOID)&pDomainJoinInfo));

    LW_CLEANUP_CTERR(exc, DJGetComputerName(&pDomainJoinInfo->pszName));

    DJGetConfiguredDnsDomain(&pDomainJoinInfo->pszDomainName, &inner);
    if (!LW_IS_OK(inner) &&
            inner->code == NERR_SetupNotJoined)
    {
        LW_HANDLE(&inner);
    }
    LW_CLEANUP(exc, inner);

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
        LW_TRY(exc, DJGetConfiguredShortDomain(
                    &pDomainJoinInfo->pszDomainShortName, &LW_EXC));
    }

    *ppDomainJoinInfo = pDomainJoinInfo;
    pDomainJoinInfo = NULL;

cleanup:

    LW_HANDLE(&inner);
    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);
}


void
FreeDomainJoinInfo(
    PDOMAINJOININFO pDomainJoinInfo
    )
{
    if (pDomainJoinInfo) {

        if (pDomainJoinInfo->pszName)
            CTFreeString(pDomainJoinInfo->pszName);

        if (pDomainJoinInfo->pszDnsDomain)
            CTFreeString(pDomainJoinInfo->pszDnsDomain);

        if (pDomainJoinInfo->pszDomainName)
            CTFreeString(pDomainJoinInfo->pszDomainName);

        if (pDomainJoinInfo->pszDomainShortName)
            CTFreeString(pDomainJoinInfo->pszDomainShortName);

        if (pDomainJoinInfo->pszLogFilePath)
            CTFreeString(pDomainJoinInfo->pszLogFilePath);

        CTFreeMemory(pDomainJoinInfo);
    }
}
