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
