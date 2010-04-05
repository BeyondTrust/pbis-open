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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss-group.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS Group Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"
#include <assert.h>

static LSA_ENUMGROUPS_STATE gEnumGroupsState = {0};

NSS_STATUS
_nss_lsass_setgrent(
    void
    )
{
    return LsaNssCommonGroupSetgrent(&hLsaConnection,
                                     &gEnumGroupsState);
}

NSS_STATUS
_nss_lsass_getgrent_r(
    struct group*  pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    return LsaNssCommonGroupGetgrent(&hLsaConnection,
                                     &gEnumGroupsState,
                                     pResultGroup,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);
}

NSS_STATUS
_nss_lsass_endgrent(
    void
    )
{
    return LsaNssCommonGroupEndgrent(&hLsaConnection, &gEnumGroupsState);
}

NSS_STATUS
_nss_lsass_getgrgid_r(
    gid_t          gid,
    struct group*  pResultGroup,
    char*          pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    return LsaNssCommonGroupGetgrgid(&hLsaConnection,
                                     gid,
                                     pResultGroup,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);
}

NSS_STATUS
_nss_lsass_getgrnam_r(
    const char *   pszGroupName,
    struct group * pResultGroup,
    char *         pszBuf,
    size_t         bufLen,
    int*           pErrorNumber
    )
{
    return LsaNssCommonGroupGetgrnam(&hLsaConnection,
                                     pszGroupName,
                                     pResultGroup,
                                     pszBuf,
                                     bufLen,
                                     pErrorNumber);
}

NSS_STATUS
_nss_lsass_initgroups_dyn(
    PCSTR     pszUserName,
    gid_t     groupGid,
    long int* pResultsSize,
    long int* pResultsCapacity,
    gid_t**   ppGidResults,
    long int  maxGroups,
    int*      pErrorNumber
    )
{
    int   ret = NSS_STATUS_SUCCESS;
    DWORD dwNumGroupsFound = 0;
    gid_t* pGidTotalResult = NULL;
    gid_t* pGidNewResult = NULL;
    gid_t* pExistingResult = NULL;
    DWORD dwNumTotalGroup = 0;
    DWORD iGroup = 0, iExistingGroup = 0, iNewGroup = 0;

    if ((*pResultsCapacity > maxGroups && maxGroups != -1) ||
        *pResultsSize > *pResultsCapacity)
    {
        ret = NSS_STATUS_UNAVAIL;
        *pErrorNumber = EINVAL;
        BAIL_ON_NSS_ERROR(ret);
    }

    if (hLsaConnection == (HANDLE)NULL)
    {
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LsaOpenServer(&hLsaConnection));
        BAIL_ON_NSS_ERROR(ret);
    }

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaGetGidsForUserByName(
                           hLsaConnection,
                           pszUserName,
                           &dwNumGroupsFound,
                           &pGidNewResult));
    BAIL_ON_NSS_ERROR(ret);

    dwNumTotalGroup += dwNumGroupsFound;

    dwNumTotalGroup += 1; //count in the group that is passed in

    //count in the groups that are already in ppGidResults
    dwNumTotalGroup += *pResultsSize;

    if (dwNumTotalGroup > *pResultsCapacity)
    {
        if (dwNumTotalGroup > maxGroups && maxGroups != -1)
            dwNumTotalGroup = maxGroups;

        ret = MAP_LSA_ERROR(pErrorNumber,
                            LwAllocateMemory(
                                sizeof(gid_t) * dwNumTotalGroup * 2,
                                (PVOID*)&pGidTotalResult));
        BAIL_ON_NSS_ERROR(ret);

        pExistingResult = *ppGidResults;
        for (iExistingGroup = 0, iGroup = 0;
             iExistingGroup < *pResultsSize;
             iExistingGroup++, iGroup++)
        {
            pGidTotalResult[iGroup] = pExistingResult[iExistingGroup];
        }

        for (iNewGroup = 0;
             iNewGroup < dwNumGroupsFound && iGroup < dwNumTotalGroup;
             iNewGroup++, iGroup++)
        {
            pGidTotalResult[iGroup] = pGidNewResult[iNewGroup];
        }

        *pResultsCapacity = dwNumTotalGroup * 2;
        *pResultsSize = dwNumTotalGroup;
        *ppGidResults = pGidTotalResult;
        pGidTotalResult = NULL;

        LW_SAFE_FREE_MEMORY(pExistingResult);
    }

    else
    {
        pExistingResult = *ppGidResults;
        iGroup = *pResultsSize;
        pExistingResult[iGroup++] = groupGid;

        for (iNewGroup = 0;
             iNewGroup < dwNumGroupsFound && iGroup < dwNumTotalGroup;
             iNewGroup++, iGroup++)
        {
            pExistingResult[iGroup] = pGidNewResult[iNewGroup];
        }
    }

    *pResultsSize = iGroup;

cleanup:

    LW_SAFE_FREE_MEMORY(pGidNewResult);

    return ret;

error:

    if (ret != NSS_STATUS_TRYAGAIN && hLsaConnection != (HANDLE)NULL)
    {
       LsaCloseServer(hLsaConnection);
       hLsaConnection = (HANDLE)NULL;
    }

    LW_SAFE_FREE_MEMORY(pGidTotalResult);
    LW_SAFE_FREE_MEMORY(pGidNewResult);

    goto cleanup;
}

