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
 *        Handle NSS Group Information (Common)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"

static const DWORD MAX_NUM_GROUPS = 500;

VOID
LsaNssClearEnumGroupsState(
    HANDLE                hLsaConnection,
    PLSA_ENUMGROUPS_STATE pState
    )
{
    if (pState->ppGroupInfoList) {
        LsaFreeGroupInfoList(
            pState->dwGroupInfoLevel,
            pState->ppGroupInfoList,
            pState->dwNumGroups
            );
        pState->ppGroupInfoList = (HANDLE)NULL;
    }
    
    if (hLsaConnection && pState->hResume != (HANDLE)NULL) {
        LsaEndEnumGroups(hLsaConnection, pState->hResume);
        pState->hResume = (HANDLE)NULL;
    }

    memset(pState, 0, sizeof(LSA_ENUMGROUPS_STATE));

    pState->dwGroupInfoLevel = 1;
}

DWORD
LsaNssGetNumberGroupMembers(
    PSTR* ppszMembers
    )
{
    DWORD dwNumMembers = 0;

    if (ppszMembers){
       for (; ppszMembers && !LW_IS_NULL_OR_EMPTY_STR(*ppszMembers); ppszMembers++)
       {
           dwNumMembers++;
       }
    }

    return dwNumMembers;
}

DWORD
LsaNssComputeGroupStringLength(
    DWORD dwAlignBytes,
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    DWORD dwLength = 0;
    PSTR* ppszMember = NULL;
    DWORD dwNumMembers = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszName)) {
       dwLength += strlen(pGroupInfo->pszName) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->pszPasswd)) {
       dwLength += strlen(pGroupInfo->pszPasswd) + 1;
    }

    /* Adding space for group members */
    dwLength += dwAlignBytes;

    for(ppszMember = pGroupInfo->ppszMembers;
        ppszMember && !LW_IS_NULL_OR_EMPTY_STR(*ppszMember);
        ppszMember++)
    {
        dwLength += sizeof(PSTR);
        dwLength += strlen(*ppszMember) + 1;
        dwNumMembers++;
    }
    // Account for terminating NULL always
    dwLength += sizeof(PSTR);

    return dwLength;
}

DWORD
LsaNssWriteGroupInfo(
    DWORD       dwGroupInfoLevel,
    PVOID       pGroupInfo,
    group_ptr_t pResultGroup,
    char**      ppszBuf,
    int         bufLen)
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo_1 = NULL;
    PSTR  pszMarker = *ppszBuf;
    DWORD dwLen = 0;
    DWORD dwAlignBytes = 0;
    DWORD dwNumMembers = 0;

    memset(pResultGroup, 0, sizeof(struct group));

    if ((dwGroupInfoLevel != 0) && (dwGroupInfoLevel != 1)) {
        dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGroupInfo_1 = (PLSA_GROUP_INFO_1)pGroupInfo;

    dwNumMembers = LsaNssGetNumberGroupMembers(pGroupInfo_1->ppszMembers);

    dwAlignBytes = (dwNumMembers ? ((((size_t)pszMarker) % sizeof(size_t)) * sizeof(size_t)) : 0);

    if (LsaNssComputeGroupStringLength(dwAlignBytes, pGroupInfo_1) > bufLen) {
       dwError = LW_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    pResultGroup->gr_gid = pGroupInfo_1->gid;

    memset(pszMarker, 0, bufLen);

    pszMarker += dwAlignBytes;
    pResultGroup->gr_mem = (PSTR*)pszMarker;

    //
    // Handle Group Members first, because we computed the
    // alignment adjustment based on the first pointer position
    //
    if (!dwNumMembers) {
       *(pResultGroup->gr_mem) = NULL;
       pszMarker += sizeof(PSTR);

    } else {
        PSTR pszMemberMarker = NULL;
        DWORD iMember = 0;

        // This is where we start writing the members
        pszMemberMarker = pszMarker + (sizeof(PSTR) * (dwNumMembers + 1));

        for (iMember = 0; iMember < dwNumMembers; iMember++)
        {
            *(pResultGroup->gr_mem+iMember) = pszMemberMarker;
            pszMarker += sizeof(PSTR);

            dwLen = strlen(*(pGroupInfo_1->ppszMembers + iMember));
            memcpy(pszMemberMarker, *(pGroupInfo_1->ppszMembers + iMember), dwLen);
            pszMemberMarker += dwLen + 1;
        }
        // Handle the terminating NULL
        *(pResultGroup->gr_mem+iMember) = NULL;
        pszMarker = ++pszMemberMarker; // skip NULL
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo_1->pszName)) {
       dwLen = strlen(pGroupInfo_1->pszName);
       memcpy(pszMarker, pGroupInfo_1->pszName, dwLen);
       pResultGroup->gr_name = pszMarker;
       pszMarker += dwLen + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo_1->pszPasswd)) {
       dwLen = strlen(pGroupInfo_1->pszPasswd);
       memcpy(pszMarker, pGroupInfo_1->pszPasswd, dwLen);
       pResultGroup->gr_passwd = pszMarker;
       pszMarker += dwLen + 1;
    }
    else{
        dwLen = sizeof("x") - 1;
        *pszMarker = 'x';
        pResultGroup->gr_passwd = pszMarker;
        pszMarker += dwLen + 1;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

NSS_STATUS
LsaNssCommonGroupSetgrent(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PLSA_ENUMGROUPS_STATE pEnumGroupsState
    )
{
    int ret = NSS_STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;

    LsaNssClearEnumGroupsState(hLsaConnection, pEnumGroupsState);

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(NULL,
                        LsaBeginEnumGroups(
                            hLsaConnection,
                            pEnumGroupsState->dwGroupInfoLevel,
                            MAX_NUM_GROUPS,
                            LSA_FIND_FLAGS_NSS,
                            &pEnumGroupsState->hResume));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    return ret;

error:

    LsaNssClearEnumGroupsState(hLsaConnection, pEnumGroupsState);

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonGroupGetgrent(
    PLSA_NSS_CACHED_HANDLE    pConnection,
    PLSA_ENUMGROUPS_STATE     pEnumGroupsState,
    struct group*             pResultGroup,
    char *                    pszBuf,
    size_t                    bufLen,
    int*                      pErrorNumber
    )
{
    int                       ret = NSS_STATUS_NOTFOUND;
    HANDLE hLsaConnection = pConnection->hLsaConnection;

    if (hLsaConnection == (HANDLE)NULL)
    {
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LW_ERROR_INVALID_LSA_CONNECTION);
        BAIL_ON_NSS_ERROR(ret);
    }

    if (!pEnumGroupsState->bTryAgain)
    {
        if (!pEnumGroupsState->idxGroup ||
            (pEnumGroupsState->idxGroup >= pEnumGroupsState->dwNumGroups))
        {
            if (pEnumGroupsState->ppGroupInfoList) {
                LsaFreeGroupInfoList(
                   pEnumGroupsState->dwGroupInfoLevel,
                   pEnumGroupsState->ppGroupInfoList,
                   pEnumGroupsState->dwNumGroups);
                pEnumGroupsState->ppGroupInfoList = NULL;
                pEnumGroupsState->dwNumGroups = 0;
                pEnumGroupsState->idxGroup = 0;
            }

            ret = MAP_LSA_ERROR(pErrorNumber,
                           LsaEnumGroups(
                               hLsaConnection,
                               pEnumGroupsState->hResume,
                               &pEnumGroupsState->dwNumGroups,
                               &pEnumGroupsState->ppGroupInfoList));
            BAIL_ON_NSS_ERROR(ret);
        }

    }

    if (pEnumGroupsState->dwNumGroups) {
        PLSA_GROUP_INFO_1 pGroupInfo =
            (PLSA_GROUP_INFO_1)*(pEnumGroupsState->ppGroupInfoList+pEnumGroupsState->idxGroup);
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LsaNssWriteGroupInfo(
                                pEnumGroupsState->dwGroupInfoLevel,
                                pGroupInfo,
                                pResultGroup,
                                &pszBuf,
                                bufLen));
        BAIL_ON_NSS_ERROR(ret);
        pEnumGroupsState->idxGroup++;

        ret = NSS_STATUS_SUCCESS;
    } else {
        ret = NSS_STATUS_UNAVAIL;

        if (pErrorNumber) {
            *pErrorNumber = ENOENT;
        }
    }

    pEnumGroupsState->bTryAgain = FALSE;

cleanup:

    return ret;

error:

    if ((ret == NSS_STATUS_TRYAGAIN) && pErrorNumber && (*pErrorNumber == ERANGE))
    {
        pEnumGroupsState->bTryAgain = TRUE;
    }
    else
    {
        LsaNssClearEnumGroupsState(hLsaConnection, pEnumGroupsState);
        if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
        {
            LsaNssCommonCloseConnection(pConnection);
        }
    }

    if (bufLen && pszBuf) {
        memset(pszBuf, 0, bufLen);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonGroupEndgrent(
    PLSA_NSS_CACHED_HANDLE    pConnection,
    PLSA_ENUMGROUPS_STATE     pEnumGroupsState
    )
{
    HANDLE hLsaConnection = pConnection->hLsaConnection;
    LsaNssClearEnumGroupsState(hLsaConnection, pEnumGroupsState);

    return NSS_STATUS_SUCCESS;
}

NSS_STATUS
LsaNssCommonGroupGetgrgid(
    PLSA_NSS_CACHED_HANDLE pConnection,
    gid_t gid,
    struct group* pResultGroup,
    char* pszBuf,
    size_t bufLen,
    int* pErrorNumber
    )
{
    int ret = NSS_STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaFindGroupById(
                            hLsaConnection,
                            gid,
                            LSA_FIND_FLAGS_NSS,
                            dwGroupInfoLevel,
                            &pGroupInfo));
    BAIL_ON_NSS_ERROR(ret);

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaNssWriteGroupInfo(
                            dwGroupInfoLevel,
                            pGroupInfo,
                            pResultGroup,
                            &pszBuf,
                            bufLen
                            ));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return ret;

error:

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonGroupGetgrnam(
    PLSA_NSS_CACHED_HANDLE pConnection,
    const char * pszGroupName,
    struct group * pResultGroup,
    char * pszBuf,
    size_t bufLen,
    int* pErrorNumber
    )
{
    int ret = NSS_STATUS_SUCCESS;
    HANDLE  hLsaConnection = NULL;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;

    if (LsaShouldIgnoreGroup(pszGroupName))
    {
        ret = MAP_LSA_ERROR(NULL, LW_ERROR_NOT_HANDLED);
        BAIL_ON_NSS_ERROR(ret);
    }

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaFindGroupByName(
                            hLsaConnection,
                            pszGroupName,
                            LSA_FIND_FLAGS_NSS,
                            dwGroupInfoLevel,
                            &pGroupInfo));
    BAIL_ON_NSS_ERROR(ret);

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaNssWriteGroupInfo(
                            dwGroupInfoLevel,
                            pGroupInfo,
                            pResultGroup,
                            &pszBuf,
                            bufLen));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return ret;

error:

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonGroupGetGroupsByUserName(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PCSTR pszUserName,
    size_t resultsExistingSize,
    size_t resultsCapacity,
    size_t* pResultSize,
    gid_t* pGidResults,
    int* pErrorNumber
    )
{
    int                     ret = NSS_STATUS_SUCCESS;
    HANDLE                  hLsaConnection = NULL;
    DWORD                   dwNumGroupsFound = 0;
    gid_t*                  pGidNewResult = NULL;
    DWORD                   dwNumTotalGroup = 0;
    DWORD                   iGroup, iNewGroup, iOldGroup;

    if (resultsExistingSize > resultsCapacity)
    {
        ret = NSS_STATUS_UNAVAIL;
        *pErrorNumber = EINVAL;
        BAIL_ON_NSS_ERROR(ret);
    }
    
    if (LsaShouldIgnoreUser(pszUserName))
    {
        ret = MAP_LSA_ERROR(NULL, LW_ERROR_NOT_HANDLED);
        BAIL_ON_NSS_ERROR(ret);
    }

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaGetGidsForUserByName(
                           hLsaConnection,
                           pszUserName,
                           &dwNumGroupsFound,
                           &pGidNewResult));
    BAIL_ON_NSS_ERROR(ret);

    dwNumTotalGroup = dwNumGroupsFound + resultsExistingSize;
    *pResultSize = dwNumTotalGroup;

    if (dwNumTotalGroup > resultsCapacity)
        dwNumTotalGroup = resultsCapacity;

    for (iGroup = resultsExistingSize, iNewGroup = 0;
	 iGroup < dwNumTotalGroup && iNewGroup < dwNumGroupsFound;
         iNewGroup++)
    {
        gid_t gid = pGidNewResult[iNewGroup];

        for(iOldGroup = 0; iOldGroup < resultsExistingSize; iOldGroup++)
        {
            if (pGidResults[iOldGroup] == gid)
            {
	        /* Decrement the total number of groups we
		   would return by 1 since there was a duplicate */
	        *pResultSize -= 1;
                goto skip;
            }
        }
        pGidResults[iGroup++] = gid;
skip:   continue;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pGidNewResult);

    return ret;

error:

    LW_SAFE_FREE_MEMORY(pGidNewResult);

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}
