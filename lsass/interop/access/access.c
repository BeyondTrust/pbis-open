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
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "access.h"

typedef struct {
    DWORD   dwUidCount;
    uid_t * pUids;
    DWORD   dwGidCount;
    gid_t * pGids;
} LSA_ACCESS_DATA, *PLSA_ACCESS_DATA;

DWORD
LsaAccessGetData(
    PCSTR * pczConfigData,
    PVOID * ppAccessData
    )
{
    DWORD            dwError = 0;
    PLSA_ACCESS_DATA pAccessData = NULL;
    DWORD            dwAllocUid = 0;
    DWORD            dwAllocGid = 0;
    DWORD            dwCount = 0;
    HANDLE           hLsaConnection = (HANDLE)NULL;
    DWORD            dwInfoLevel = 0;
    PVOID            pUserInfo = NULL;
    PVOID            pGroupInfo = NULL;

    if ( pczConfigData == NULL )
    {
        *ppAccessData = NULL;
        goto cleanup;
    }

    dwError = LwAllocateMemory(sizeof(LSA_ACCESS_DATA),
                  (PVOID*)&pAccessData);
    BAIL_ON_LSA_ERROR(dwError);

    dwAllocUid = 8;
    dwError = LwAllocateMemory(sizeof(uid_t) * dwAllocUid,
                  (PVOID*)&pAccessData->pUids);
    BAIL_ON_LSA_ERROR(dwError);

    dwAllocGid = 16;
    dwError = LwAllocateMemory(sizeof(uid_t) * dwAllocGid,
                  (PVOID*)&pAccessData->pGids);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    for ( dwCount = 0 ; pczConfigData[dwCount] ; dwCount++ )
    {
        dwError = LsaFindGroupByName(
                      hLsaConnection,
                      pczConfigData[dwCount],
                      0,
                      dwInfoLevel,
                      &pGroupInfo);
        if ( !dwError )
        {
            if ( pAccessData->dwGidCount == dwAllocGid )
            {
                dwAllocGid *= 2;
                dwError = LwReallocMemory(
                              (PVOID)pAccessData->pGids,
                              (PVOID *)&pAccessData->pGids,
                              dwAllocGid * sizeof(gid_t) );
                BAIL_ON_LSA_ERROR(dwError);
            }

            pAccessData->pGids[pAccessData->dwGidCount++] =
                ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;

            LsaFreeGroupInfo(
                dwInfoLevel,
                pGroupInfo);
            pGroupInfo = NULL;
        }
        else
        {
            dwError = LsaFindUserByName(
                          hLsaConnection,
                          pczConfigData[dwCount],
                          dwInfoLevel,
                          &pUserInfo);
            if ( dwError )
            {
                continue;
            }
            if ( pAccessData->dwUidCount == dwAllocUid )
            {
                dwAllocUid *= 2;
                dwError = LwReallocMemory(
                              (PVOID)pAccessData->pUids,
                              (PVOID *)&pAccessData->pUids,
                              dwAllocUid * sizeof(uid_t) );
                BAIL_ON_LSA_ERROR(dwError);
            }

            pAccessData->pUids[pAccessData->dwUidCount++] =
                ((PLSA_USER_INFO_0)pUserInfo)->uid;

            LsaFreeUserInfo(
                dwInfoLevel,
                pUserInfo);
            pUserInfo = NULL;
        }
    }

    *ppAccessData = pAccessData;

cleanup:
    if ( pUserInfo )
    {
        LsaFreeUserInfo(
            dwInfoLevel,
            pUserInfo);
    }
    if ( pGroupInfo )
    {
        LsaFreeGroupInfo(
            dwInfoLevel,
            pGroupInfo);
    }
    if ( hLsaConnection != (HANDLE)NULL )
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:
    if ( pAccessData )
        LsaAccessFreeData( (PVOID)pAccessData );

    goto cleanup;
}

DWORD
LsaAccessCheckData(
    PCSTR pczUserName,
    PCVOID pAccessData
    )
{
    DWORD dwError = 0;
    PLSA_ACCESS_DATA pAccessDataLocal = NULL;
    HANDLE           hLsaConnection = (HANDLE)NULL;
    DWORD            dwInfoLevel = 0;
    PVOID            pUserInfo = NULL;
    gid_t *          pGid = NULL;
    DWORD            dwNumGroups = 0;
    DWORD            dwCount = 0;
    DWORD            dwCount2 = 0;
    BOOLEAN          bUserIsOK = FALSE;

    if ( !pAccessData )
    {
        dwError = LW_ERROR_AUTH_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pAccessDataLocal = (PLSA_ACCESS_DATA)pAccessData;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                  hLsaConnection,
                  pczUserName,
                  dwInfoLevel,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    for ( dwCount = 0 ; dwCount < pAccessDataLocal->dwUidCount ; dwCount++ )
    {
        if ( ((PLSA_USER_INFO_0)pUserInfo)->uid == pAccessDataLocal->pUids[dwCount] )
        {
            bUserIsOK = TRUE;
            break;
        }
    }

    if ( !bUserIsOK )
    {
        dwError = LsaGetGidsForUserByName(
                      hLsaConnection,
                      pczUserName,
                      &dwNumGroups,
                      &pGid);
        BAIL_ON_LSA_ERROR(dwError);

        for ( dwCount = 0 ; (dwCount < dwNumGroups) && !bUserIsOK ; dwCount++ )
        {
            for ( dwCount2 = 0 ; dwCount2 < pAccessDataLocal->dwGidCount ; dwCount2++ )
            {
                if ( pAccessDataLocal->pGids[dwCount2] == pGid[dwCount] )
                {
                    bUserIsOK = TRUE;
                    break;
                }
            }
        }
    }

    if ( !bUserIsOK )
    {
        dwError = LW_ERROR_AUTH_ERROR;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pGid);

    if ( pUserInfo )
    {
        LsaFreeUserInfo(
            dwInfoLevel,
            pUserInfo);
    }
    if ( hLsaConnection != (HANDLE)NULL )
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaAccessFreeData(
    PVOID pAccessData
    )
{
    DWORD dwError = 0;

    if ( pAccessData )
    {
        LW_SAFE_FREE_MEMORY(((PLSA_ACCESS_DATA)pAccessData)->pUids);
        LW_SAFE_FREE_MEMORY(((PLSA_ACCESS_DATA)pAccessData)->pGids);
        LW_SAFE_FREE_MEMORY(pAccessData);
    }

    return dwError;
}

