/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
