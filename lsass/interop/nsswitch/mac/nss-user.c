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
 *        Handle NSS User Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"

static LSA_ENUMUSERS_STATE gEnumUsersState = {0};

NSS_STATUS
_nss_lsass_setpwent(
    void
    )
{
    return LsaNssCommonPasswdSetpwent(&hLsaConnection,
                                      &gEnumUsersState);
}

NSS_STATUS
_nss_lsass_getpwent_r(
    struct passwd * pResultUser,
    char *          pszBuf,
    size_t          bufLen,
    int *           pErrorNumber
    )
{
    return LsaNssCommonPasswdGetpwent(&hLsaConnection,
                                      &gEnumUsersState,
                                      pResultUser,
                                      pszBuf,
                                      bufLen,
                                      pErrorNumber);
}

NSS_STATUS
_nss_lsass_endpwent(
    void
    )
{
    return LsaNssCommonPasswdEndpwent(&hLsaConnection, &gEnumUsersState);
}

NSS_STATUS
_nss_lsass_getpwnam_r(
    const char *     pszLoginId,
    struct passwd *  pResultUser,
    char *           pszBuf,
    size_t           bufLen,
    int *            pErrorNumber
    )
{
    return LsaNssCommonPasswdGetpwnam(&hLsaConnection,
                                      pszLoginId,
                                      pResultUser,
                                      pszBuf,
                                      bufLen,
                                      pErrorNumber);
}

NSS_STATUS
_nss_lsass_getpwuid_r(
    uid_t           uid,
    struct passwd * pResultUser,
    char *          pszBuf,
    size_t          bufLen,
    int *           pErrorNumber
    )
{
    return LsaNssCommonPasswdGetpwuid(&hLsaConnection,
                                      uid,
                                      pResultUser,
                                      pszBuf,
                                      bufLen,
                                      pErrorNumber);
}

DWORD
_nss_lsass_get_principal(
    PCSTR pszUserName,
    PSTR* ppszPrincipalName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;
    char * pszPrincipalName = NULL;

    if (!ppszPrincipalName)
    {
       dwError = LW_ERROR_INVALID_PARAMETER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (hLsaConnection == (HANDLE)NULL)
    {
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaFindUserByName(hLsaConnection,
                                pszUserName,
                                dwUserInfoLevel,
                                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(((PLSA_USER_INFO_1)pUserInfo)->pszUPN,
                                &pszPrincipalName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPrincipalName = pszPrincipalName;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    if (ppszPrincipalName)
    {
        *ppszPrincipalName = NULL;
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
       LsaCloseServer(hLsaConnection);
       hLsaConnection = (HANDLE)NULL;
    }

    if (pszPrincipalName)
    {
       LwFreeString(pszPrincipalName);
    }

    goto cleanup;
}

VOID
_nss_lsass_free_principal(
    PSTR pszPrincipalName
    )
{
    if (pszPrincipalName) {
        LwFreeString(pszPrincipalName);
    }
}

DWORD
_nss_lsass_get_user_groups(
    PCSTR    pszUserName,
    gid_t ** ppGroups,
    int *    pNumberOfGroups
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwCountOfGroups = 0;
    gid_t* pGidResults = NULL;

    if (!pNumberOfGroups || !ppGroups) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (hLsaConnection == (HANDLE)NULL)
    {
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaGetGidsForUserByName(hLsaConnection,
                                      pszUserName,
                                      &dwCountOfGroups,
                                      &pGidResults);
    BAIL_ON_LSA_ERROR(dwError);

    *pNumberOfGroups = dwCountOfGroups;
    *ppGroups = pGidResults;

cleanup:

    return dwError;

error:

   if (hLsaConnection != (HANDLE)NULL)
   {
      LsaCloseServer(hLsaConnection);
      hLsaConnection = (HANDLE)NULL;
   }

    if (pNumberOfGroups)
    {
        *pNumberOfGroups = 0;
    }

    if (ppGroups)
    {
        *ppGroups = NULL;
    }

    if (pGidResults) {
        LwFreeMemory(pGidResults);
    }

    goto cleanup;
}

VOID
_nss_lsass_free_user_groups(
    gid_t * pGroups
    )
{
    if (pGroups)
        LwFreeMemory(pGroups);
}
