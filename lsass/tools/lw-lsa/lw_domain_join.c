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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tool to Manage AD Join/Leave/Query
 *
 */

#include "includes.h"

extern
DWORD
LsaNetJoinDomain(
    PCSTR pszHostname,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    DWORD dwFlags
    );

static
DWORD
LwGetDistroInfo(
    PSTR* ppszOSName,
    PSTR* ppszOSVersion,
    PSTR* ppszOSServicePack
    );

DWORD
LwDomainJoin(
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    PSTR  pszOSName = NULL;
    PSTR  pszOSVersion = NULL;
    PSTR  pszOSServicePack = NULL;
    DWORD dwFlags = 1; // LSA_NET_JOIN_DOMAIN_NOTIMESYNC

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwGetDistroInfo(
                    &pszOSName,
                    &pszOSVersion,
                    &pszOSServicePack);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNetJoinDomain(
                    pszHostname,
                    pszDomain,
                    pszOU,
                    pszUsername,
                    pszPassword,
                    pszOSName,
                    pszOSVersion,
                    pszOSServicePack,
                    dwFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszOSName);
    LW_SAFE_FREE_STRING(pszOSVersion);
    LW_SAFE_FREE_STRING(pszOSServicePack);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwGetDistroInfo(
    PSTR* ppszOSName,
    PSTR* ppszOSVersion,
    PSTR* ppszOSServicePack
    )
{
    DWORD dwError = 0;
    PLW_DOMAIN_DISTRO_INFO pDistroInfo = NULL;
    PSTR  pszOSName = NULL;
    PSTR  pszOSVersion = NULL;
    PSTR  pszOSServicePack = NULL;
    PSTR  pszLikewiseVersion = NULL;
    PSTR  pszLikewiseBuild = NULL;
    PSTR  pszLikewiseRevision = NULL;

    dwError = LwDomainGetDistroInfo(NULL, &pDistroInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwDomainGetOSString(
                    pDistroInfo->osType,
                    &pszOSName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pDistroInfo->pszVersion,
                    &pszOSVersion);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwGetLikewiseVersion(
                    &pszLikewiseVersion,
                    &pszLikewiseBuild,
                    &pszLikewiseRevision);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszOSServicePack,
                    "Likewise Identity %s.%s.%s",
                    LSA_SAFE_LOG_STRING(pszLikewiseVersion),
                    LSA_SAFE_LOG_STRING(pszLikewiseBuild),
                    LSA_SAFE_LOG_STRING(pszLikewiseRevision));
    BAIL_ON_LSA_ERROR(dwError);

    *ppszOSName = pszOSName;
    *ppszOSVersion = pszOSVersion;
    *ppszOSServicePack = pszOSServicePack;

cleanup:

    if (pDistroInfo)
    {
        LwFreeDistroInfo(pDistroInfo);
    }

    LW_SAFE_FREE_STRING(pszLikewiseVersion);
    LW_SAFE_FREE_STRING(pszLikewiseBuild);
    LW_SAFE_FREE_STRING(pszLikewiseRevision);

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszOSName);
    LW_SAFE_FREE_STRING(pszOSVersion);
    LW_SAFE_FREE_STRING(pszOSServicePack);

    goto cleanup;

}

