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
 *        nss-user.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS User Information (Common)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "lsanss.h"

static const int MAX_NUM_USERS = 500;

VOID
LsaNssClearEnumUsersState(
    HANDLE hLsaConnection,
    PLSA_ENUMUSERS_STATE pState
    )
{
    if (pState->ppUserInfoList)
    {
        LsaFreeUserInfoList(
            pState->dwUserInfoLevel,
            pState->ppUserInfoList,
            pState->dwNumUsers
            );
        pState->ppUserInfoList = (HANDLE)NULL;
    }
    
    if (hLsaConnection && pState->hResume != (HANDLE)NULL)
    {
        LsaEndEnumUsers(hLsaConnection, pState->hResume);
        pState->hResume = (HANDLE)NULL;
    }

    memset(pState, 0, sizeof(LSA_ENUMUSERS_STATE));
}

DWORD
LsaNssComputeUserStringLength(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwLength = 0;

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszName)) {
       dwLength += strlen(pUserInfo->pszName) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszPasswd)) {
       dwLength += strlen(pUserInfo->pszPasswd) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszShell)) {
       dwLength += strlen(pUserInfo->pszShell) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszGecos)) {
       dwLength += strlen(pUserInfo->pszGecos) + 1;
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszHomedir)) {
       dwLength += strlen(pUserInfo->pszHomedir) + 1;
    }

#ifdef HAVE_STRUCT_PASSWD_PW_CLASS
   dwLength++;
#endif

    return dwLength;
}

#define PASSWD_SAFE_STRING(x, y) \
    ( (x) ? (x) : (y) )

DWORD
LsaNssWriteUserInfo(
    DWORD        dwUserInfoLevel,
    PVOID        pUserInfo,
    passwd_ptr_t pResultUser,
    char**       ppszBuf,
    int          bufLen)
{
    DWORD dwError = 0;
    PLSA_USER_INFO_0 pUserInfo_0 = NULL;
    PSTR  pszMarker = NULL;
    DWORD dwLen = 0;

    if (dwUserInfoLevel != 0) {
        dwError = LW_ERROR_UNSUPPORTED_USER_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if(!ppszBuf)
    {
        dwError = LW_ERROR_NULL_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszMarker = *ppszBuf;

    pUserInfo_0 = (PLSA_USER_INFO_0)pUserInfo;

    if (LsaNssComputeUserStringLength(pUserInfo_0) > bufLen) {
       dwError = LW_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    memset(pszMarker, 0, bufLen);

    /* Solaris NSS2 passes a NULL result to indicate it requires an etc files formatted result for NSCD */
    if (pResultUser)
    {
        memset(pResultUser , 0, sizeof(struct passwd));
        pResultUser->pw_uid = pUserInfo_0->uid;
        pResultUser->pw_gid = pUserInfo_0->gid;

        if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo_0->pszName)) {
           dwLen = strlen(pUserInfo_0->pszName);
           memcpy(pszMarker, pUserInfo_0->pszName, dwLen);
           pResultUser->pw_name = pszMarker;
           pszMarker += dwLen + 1;
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo_0->pszPasswd)) {
           dwLen = strlen(pUserInfo_0->pszPasswd);
           memcpy(pszMarker, pUserInfo_0->pszPasswd, dwLen);
           pResultUser->pw_passwd = pszMarker;
           pszMarker += dwLen + 1;
        }
        else {
            dwLen = sizeof("x") - 1;
            *pszMarker = 'x';
            pResultUser->pw_passwd = pszMarker;
            pszMarker += dwLen + 1;
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo_0->pszGecos)) {
           dwLen = strlen(pUserInfo_0->pszGecos);
           memcpy(pszMarker, pUserInfo_0->pszGecos, dwLen);
           pResultUser->pw_gecos = pszMarker;
           pszMarker += dwLen + 1;
        }
        else {
            *pszMarker = '\0';
            pResultUser->pw_gecos = pszMarker;
            pszMarker++;
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo_0->pszShell)) {
           dwLen = strlen(pUserInfo_0->pszShell);
           memcpy(pszMarker, pUserInfo_0->pszShell, dwLen);
           pResultUser->pw_shell = pszMarker;
           pszMarker += dwLen + 1;
        }

        if (!LW_IS_NULL_OR_EMPTY_STR(pUserInfo_0->pszHomedir)) {
           dwLen = strlen(pUserInfo_0->pszHomedir);
           memcpy(pszMarker, pUserInfo_0->pszHomedir, dwLen);
           pResultUser->pw_dir = pszMarker;
           pszMarker += dwLen + 1;
        }

    #ifdef HAVE_STRUCT_PASSWD_PW_CLASS
        *pszMarker = 0;
        pResultUser->pw_class = pszMarker;
        pszMarker ++;
    #endif
    }
    else
    {
        int len;

        // etc passwd format expects: "pw_name:pw_passwd:pw_uid:pw_gid:pw_gecos:pw_dir:pw_shell"
        len = snprintf(pszMarker, bufLen, "%s:%s:%lu:%lu:%s:%s:%s",
                PASSWD_SAFE_STRING(pUserInfo_0->pszName, ""),
                PASSWD_SAFE_STRING(pUserInfo_0->pszPasswd, "x"),
                (unsigned long)pUserInfo_0->uid,
                (unsigned long)pUserInfo_0->gid,
                PASSWD_SAFE_STRING(pUserInfo_0->pszGecos, ""),
                PASSWD_SAFE_STRING(pUserInfo_0->pszHomedir, ""),
                PASSWD_SAFE_STRING(pUserInfo_0->pszShell, "")
        );

        if (len < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (len >= bufLen)
        {
            dwError = LW_ERROR_NULL_BUFFER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

NSS_STATUS
LsaNssCommonPasswdSetpwent(
    PLSA_NSS_CACHED_HANDLE pConnection,
    PLSA_ENUMUSERS_STATE pEnumUsersState
    )
{
    int ret = NSS_STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;
    
    LsaNssClearEnumUsersState(hLsaConnection, pEnumUsersState);

    ret = MAP_LSA_ERROR(NULL,
                        LsaBeginEnumUsers(
                            hLsaConnection,
                            pEnumUsersState->dwUserInfoLevel,
                            MAX_NUM_USERS,
                            LSA_FIND_FLAGS_NSS,
                            &pEnumUsersState->hResume));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    return ret;

error:

    LsaNssClearEnumUsersState(hLsaConnection, pEnumUsersState);

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonPasswdGetpwent(
    PLSA_NSS_CACHED_HANDLE  pConnection,
    PLSA_ENUMUSERS_STATE    pEnumUsersState,
    struct passwd *         pResultUser,
    char*                   pszBuf,
    size_t                  bufLen,
    int*                    pErrorNumber
    )
{
    int  ret = NSS_STATUS_NOTFOUND;
    HANDLE hLsaConnection = pConnection->hLsaConnection;
    PSTR pDisabled = getenv(DISABLE_NSS_ENUMERATION_ENV);

    if (hLsaConnection == (HANDLE)NULL)
    {
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LW_ERROR_INVALID_LSA_CONNECTION);
        BAIL_ON_NSS_ERROR(ret);
    }
    
    if (!pEnumUsersState->bTryAgain)
    {
        if (!pEnumUsersState->idxUser ||
            (pEnumUsersState->idxUser >= pEnumUsersState->dwNumUsers))
        {
            if (pEnumUsersState->ppUserInfoList) {
                LsaFreeUserInfoList(
                   pEnumUsersState->dwUserInfoLevel,
                   pEnumUsersState->ppUserInfoList,
                   pEnumUsersState->dwNumUsers);
                pEnumUsersState->ppUserInfoList = NULL;
                pEnumUsersState->dwNumUsers = 0;
                pEnumUsersState->idxUser = 0;
            }
            if (LW_IS_NULL_OR_EMPTY_STR(pDisabled))
            {
                ret = MAP_LSA_ERROR(pErrorNumber,
                               LsaEnumUsers(
                                   hLsaConnection,
                                   pEnumUsersState->hResume,
                                   &pEnumUsersState->dwNumUsers,
                                   &pEnumUsersState->ppUserInfoList));
                BAIL_ON_NSS_ERROR(ret);
            }
        }
    }

    if (pEnumUsersState->dwNumUsers) {
        PLSA_USER_INFO_0 pUserInfo =
            (PLSA_USER_INFO_0)*(pEnumUsersState->ppUserInfoList+pEnumUsersState->idxUser);
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LsaNssWriteUserInfo(
                                pEnumUsersState->dwUserInfoLevel,
                                pUserInfo,
                                pResultUser,
                                &pszBuf,
                                bufLen));
        BAIL_ON_NSS_ERROR(ret);

        pEnumUsersState->idxUser++;

        ret = NSS_STATUS_SUCCESS;
    } else {
        ret = NSS_STATUS_UNAVAIL;
        if (pErrorNumber) {
            *pErrorNumber = ENOENT;
        }
    }

    pEnumUsersState->bTryAgain = FALSE;

cleanup:

    return ret;

error:

    if ((ret == NSS_STATUS_TRYAGAIN) && pErrorNumber && (*pErrorNumber == ERANGE))
    {
        pEnumUsersState->bTryAgain = TRUE;
    }
    else
    {
        LsaNssClearEnumUsersState(hLsaConnection, pEnumUsersState);
        
        if ( hLsaConnection != (HANDLE)NULL)
        {
            if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
            {
                LsaNssCommonCloseConnection(pConnection);
            }
        }
    }

    if (bufLen && pszBuf)
    {
        memset(pszBuf, 0, bufLen);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonPasswdEndpwent(
    PLSA_NSS_CACHED_HANDLE  pConnection,
    PLSA_ENUMUSERS_STATE    pEnumUsersState
    )
{
    HANDLE hLsaConnection = pConnection->hLsaConnection;

    LsaNssClearEnumUsersState(hLsaConnection, pEnumUsersState);

    return NSS_STATUS_SUCCESS;
}

NSS_STATUS
LsaNssCommonPasswdGetpwnam(
    PLSA_NSS_CACHED_HANDLE pConnection,
    const char * pszLoginId,
    struct passwd * pResultUser,
    char * pszBuf,
    size_t bufLen,
    int * pErrorNumber
    )
{
    int ret;
    HANDLE hLsaConnection = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        ret = MAP_LSA_ERROR(NULL, LW_ERROR_NOT_HANDLED);
        BAIL_ON_NSS_ERROR(ret);
    }

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaFindUserByName(
                            hLsaConnection,
                            pszLoginId,
                            dwUserInfoLevel,
                            &pUserInfo));
    BAIL_ON_NSS_ERROR(ret);

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaNssWriteUserInfo(
                            dwUserInfoLevel,
                            pUserInfo,
                            pResultUser,
                            &pszBuf,
                            bufLen));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
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
LsaNssCommonPasswdGetpwuid(
    PLSA_NSS_CACHED_HANDLE pConnection,
    uid_t uid,
    struct passwd * pResultUser,
    char * pszBuf,
    size_t bufLen,
    int * pErrorNumber
    )
{
    int ret = NSS_STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    ret = MAP_LSA_ERROR(NULL,
            LsaNssCommonEnsureConnected(pConnection));
    BAIL_ON_NSS_ERROR(ret);
    hLsaConnection = pConnection->hLsaConnection;

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaFindUserById(
                            hLsaConnection,
                            uid,
                            dwUserInfoLevel,
                            &pUserInfo));
    BAIL_ON_NSS_ERROR(ret);

    ret = MAP_LSA_ERROR(pErrorNumber,
                        LsaNssWriteUserInfo(
                            dwUserInfoLevel,
                            pUserInfo,
                            pResultUser,
                            &pszBuf,
                            bufLen));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return ret;

error:

    if (ret != NSS_STATUS_TRYAGAIN && ret != NSS_STATUS_NOTFOUND)
    {
        LsaNssCommonCloseConnection(pConnection);
    }

    goto cleanup;
}
