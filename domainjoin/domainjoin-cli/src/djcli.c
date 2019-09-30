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
#include "djcli.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <stdlib.h>

void
DoFixFqdn(LWException **exc)
{
    PDOMAINJOININFO pDomainJoinInfo = NULL;

    // If this function fails, assume we are not joined
    LW_TRY(NULL, QueryInformation(&pDomainJoinInfo, &LW_EXC));

    if (pDomainJoinInfo &&
        !IsNullOrEmptyString(pDomainJoinInfo->pszName) &&
        !IsNullOrEmptyString(pDomainJoinInfo->pszDomainName))
    {
       LW_TRY(exc, DJSetComputerName(pDomainJoinInfo->pszName,
                               pDomainJoinInfo->pszDomainName,
                               &LW_EXC));
    }

cleanup:

    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);
}

void
DoQuery(
    LWException **exc
    )
{
    PDOMAINJOININFO pDomainJoinInfo = NULL;
    PLICENSEINFO pLicenseInfo = NULL;
    PSTR dn = NULL;

    LW_TRY(exc, QueryInformation(&pDomainJoinInfo, &LW_EXC));

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
        LW_TRY(exc, DJGetComputerDN(&dn, &LW_EXC));
    }

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszName)) {
        fprintf(stdout, "Name = %s\n", pDomainJoinInfo->pszName);
    } else {
        fprintf(stdout, "Name =\n");
    }

    if (!IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {
        fprintf(stdout, "Domain = %s\n", pDomainJoinInfo->pszDomainName);

        fprintf(stdout, "Distinguished Name = %s\n", dn);
    } else {
        fprintf(stdout, "Domain =\n");
    }

cleanup:

    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);

    if (pLicenseInfo)
        CTFreeMemory(pLicenseInfo);

    CT_SAFE_FREE_STRING(dn);
}

DWORD
GetPassword(
    PSTR* ppszPassword
    )
{
    DWORD ceError = ERROR_SUCCESS;
    CHAR szBuf[129];
    DWORD idx = 0;
    struct termios old, new;
    CHAR ch;

    memset(szBuf, 0, sizeof(szBuf));

    tcgetattr(0, &old);
    memcpy(&new, &old, sizeof(struct termios));
    new.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &new);

    while ( (idx < 128) ) {

        if (read(0, &ch, 1)) {

            if (ch != '\n') {

                szBuf[idx++] = ch;

            } else {

                break;

            }

        } else {

            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_CENTERIS_ERROR(ceError);

        }
    }

    if (idx == 128) {
        ceError = LwMapErrnoToLwError(ENOBUFS);
        BAIL_ON_CENTERIS_ERROR(ceError);
    }

    if (idx > 0) {

        ceError = CTAllocateString(szBuf, ppszPassword);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        *ppszPassword = NULL;

    }

error:

    tcsetattr(0, TCSANOW, &old);

    return ceError;
}
