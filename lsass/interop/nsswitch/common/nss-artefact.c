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
 *        Handle NSS Foo Information (Common)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"

static const DWORD MAX_NUM_GROUPS = 500;

VOID
LsaNssClearEnumFoosState(
    PLSA_ENUMFOOS_STATE pState
    )
{
    if (pState->hLsaConnection != (HANDLE)NULL) {

        if (pState->ppNSSArtefactInfoList) {
            LsaFreeNSSArtefactInfoList(
                pState->dwNSSArtefactInfoLevel,
                pState->ppNSSArtefactInfoList,
                pState->dwNumNSSArtefacts
                );
            pState->ppNSSArtefactInfoList = (HANDLE)NULL;
        }
        
        if (pState->hResume != (HANDLE)NULL) {
            LsaEndEnumNSSArtefacts(pState->hLsaConnection, pState->hResume);
            pState->hResume = (HANDLE)NULL;
        }
        
        LsaCloseServer(pState->hLsaConnection);
        
        pState->hLsaConnection = (HANDLE)NULL;
    }

    memset(pState, 0, sizeof(LSA_ENUMGROUPS_STATE));
    
    pState->dwNSSArtefactInfoLevel = 1;
}

DWORD
LsaNssGetNumberFooMembers(
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
LsaNssComputeFooStringLength(
    DWORD dwAlignBytes,
    PLSA_NSS_ARTEFACTINFO_1 pNSSArtefactInfo
    )
{
    DWORD dwLength = 0;
    PSTR* ppszMember = NULL;
    DWORD dwNumMembers = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pNSSArtefactInfo->pszName)) {
       dwLength += strlen(pNSSArtefactInfo->pszName) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pNSSArtefactInfo->pszPasswd)) {
       dwLength += strlen(pNSSArtefactInfo->pszPasswd) + 1;
    }

    /* Adding space for group members */
    dwLength += dwAlignBytes;
    
    for(ppszMember = pNSSArtefactInfo->ppszMembers;
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
LsaNssWriteFooInfo(
    DWORD       dwNSSArtefactInfoLevel,
    PVOID       pNSSArtefactInfo,
    group_ptr_t pResultFoo,
    char**      ppszBuf,
    int         bufLen)
{
    DWORD dwError = 0;
    PLSA_NSS_ARTEFACT_INFO_1 pNSSArtefactInfo_1 = NULL;
    PSTR  pszMarker = *ppszBuf;
    DWORD dwLen = 0;
    DWORD dwAlignBytes = 0;
    DWORD dwNumMembers = 0;
    
    memset(pResultFoo, 0, sizeof(struct group));
    
    if ((dwNSSArtefactInfoLevel != 0) && (dwNSSArtefactInfoLevel != 1)) {
        dwError = LW_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pNSSArtefactInfo_1 = (PLSA_NSS_ARTEFACT_INFO_1)pNSSArtefactInfo;
    
    dwNumMembers = LsaNssGetNumberFooMembers(pNSSArtefactInfo_1->ppszMembers);
    
    dwAlignBytes = (dwNumMembers ? ((pszMarker % sizeof(PSTR)) * sizeof(PSTR)) : 0);

    if (LsaNssComputeFooStringLength(dwAlignBytes, pNSSArtefactInfo_1) > bufLen) {
       dwError = LW_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    pResultFoo->gr_gid = pGroupInfo_1->gid;

    memset(pszMarker, 0, bufLen);
    
    pszMarker += dwAlignBytes;
    pResultFoo->gr_mem = (PSTR*)pszMarker;
    
    //
    // Handle Group Members first, because we computed the
    // alignment adjustment based on the first pointer position
    //
    if (!dwNumMembers) {        
       *(pResultFoo->gr_mem) = NULL;
       pszMarker += sizeof(PSTR);
       
    } else {
        PSTR pszMemberMarker = NULL;
        DWORD iMember = 0;
        
        // This is where we start writing the members
        pszMemberMarker = pszMarker + (sizeof(PSTR) * (dwNumMembers + 1));

        for (iMember = 0; iMember < dwNumMembers; iMember++)
        {
            *(pResultFoo->gr_mem+iMember) = pszMemberMarker;
            pszMarker += sizeof(PSTR);
            
            dwLen = strlen(*(pNSSArtefactInfo_1->ppszMembers + iMember));
            memcpy(pszMemberMarker, *(pGroupInfo_1->ppszMembers + iMember), dwLen);
            pszMemberMarker += dwLen + 1;
        }
        // Handle the terminating NULL
        *(pResultFoo->gr_mem+iMember) = NULL;
        pszMarker = ++pszMemberMarker; // skip NULL
    }
    
    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo_1->pszName)) {
       dwLen = strlen(pGroupInfo_1->pszName);
       memcpy(pszMarker, pGroupInfo_1->pszName, dwLen);
       pResultFoo->gr_name = pszMarker;
       pszMarker += dwLen + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo_1->pszPasswd)) {
       dwLen = strlen(pGroupInfo_1->pszPasswd);
       memcpy(pszMarker, pGroupInfo_1->pszPasswd, dwLen);
       pResultFoo->gr_passwd = pszMarker;
       pszMarker += dwLen + 1;
    }
    else{
        dwLen = sizeof("x") - 1;
        *pszMarker = 'x';
        pResultFoo->gr_passwd = pszMarker;
        pszMarker += dwLen + 1;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

NSS_STATUS
LsaNssCommonFooSetgrent(
    PLSA_ENUMGROUPS_STATE     pEnumFoosState
    )
{
    int                       ret = NSS_STATUS_SUCCESS;
    
    LsaNssClearEnumFoosState(pEnumFoosState);
    
    ret = MAP_LSA_ERROR(NULL,
                        LsaOpenServer(&pEnumFoosState->hLsaConnection));
    BAIL_ON_NSS_ERROR(ret);
    
    ret = MAP_LSA_ERROR(NULL,
                        LsaBeginEnumNSSArtefacts(
                            pEnumFoosState->hLsaConnection,
                            pEnumFoosState->dwNSSArtefactInfoLevel,
                            MAX_NUM_GROUPS,
                            &pEnumFoosState->hResume));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    return ret;
    
error:

    LsaNssClearEnumFoosState(pEnumFoosState);

    goto cleanup;
}

NSS_STATUS
LsaNssCommonFooGetgrent(
    PLSA_ENUMGROUPS_STATE     pEnumFoosState,
    struct group*             pResultFoo,
    char *                    pszBuf,
    size_t                    bufLen,
    int*                      pErrorNumber
    )
{
    int                       ret = NSS_STATUS_NOTFOUND;
    
    if (pEnumFoosState->hLsaConnection == (HANDLE)NULL)
    {
        ret = MAP_LSA_ERROR(pErrorNumber, LW_ERROR_INVALID_LSA_CONNECTION);
        BAIL_ON_NSS_ERROR(ret);
    }
    
    if (!pEnumFoosState->bTryAgain)
    {
        if (!pEnumFoosState->idxGroup ||
            (pEnumFoosState->idxGroup >= pEnumGroupsState->dwNumGroups))
        {    
            if (pEnumFoosState->ppNSSArtefactInfoList) {
                LsaFreeNSSArtefactInfoList(
                   pEnumFoosState->dwNSSArtefactInfoLevel,
                   pEnumFoosState->ppNSSArtefactInfoList,
                   pEnumFoosState->dwNumNSSArtefacts);
                pEnumFoosState->ppNSSArtefactInfoList = NULL;
                pEnumFoosState->dwNumNSSArtefacts = 0;
                pEnumFoosState->idxGroup = 0;
            }
            
            ret = MAP_LSA_ERROR(pErrorNumber,
                           LsaEnumNSSArtefacts(
                               pEnumFoosState->hLsaConnection,
                               pEnumFoosState->hResume,
                               &pEnumFoosState->dwNumNSSArtefacts,
                               &pEnumFoosState->ppNSSArtefactInfoList));
            BAIL_ON_NSS_ERROR(ret);
        }
        
    }

    if (pEnumFoosState->dwNumNSSArtefacts) {
        PLSA_NSS_ARTEFACT_INFO_1 pNSSArtefactInfo = 
            (PLSA_NSS_ARTEFACT_INFO_1)*(pEnumFoosState->ppNSSArtefactInfoList+pEnumFoosState->idxGroup);
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LsaNssWriteFooInfo(
                                pEnumFoosState->dwNSSArtefactInfoLevel,
                                pFooInfo,
                                pResultFoo,
                                &pszBuf,
                                bufLen));
        BAIL_ON_NSS_ERROR(ret);
        pEnumFoosState->idxGroup++;
        
        ret = NSS_STATUS_SUCCESS;
    } else {
        ret = NSS_STATUS_UNAVAIL;
        
        if (pErrorNumber) {
            *pErrorNumber = ENOENT;
        }
    }   
    
    pEnumFoosState->bTryAgain = FALSE;
    
cleanup:

    return ret;
     
error:

    if ((ret == NSS_STATUS_TRYAGAIN) && pErrorNumber && (*pErrorNumber == ERANGE))
    {
        pEnumFoosState->bTryAgain = TRUE;
    }
    else
    {
       LsaNssClearEnumFoosState(pEnumFoosState);
    }
    
    if (bufLen && pszBuf) {
        memset(pszBuf, 0, bufLen);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonFooEndgrent(
    PLSA_ENUMGROUPS_STATE     pEnumFoosState
    )
{
    LsaNssClearEnumFoosState(pEnumFoosState);

    return NSS_STATUS_SUCCESS;
}


