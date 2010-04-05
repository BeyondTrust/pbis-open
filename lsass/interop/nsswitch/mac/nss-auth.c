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
 *        nss-auth.c
 *
 * Abstract:
 *
 *        Name Server Switch (Likewise LSASS)
 *
 *        Handle NSS Authentication Information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Glenn Curtis (gcurtis@likewisesoftware.com)
 *
 */

#include "lsanss.h"
#include "externs.h"

DWORD
_nss_lsass_authenticate(
    PCSTR pszUserName,
    PCSTR pszPassword,
    bool  bAuthOnly
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;

    if (hLsaConnection == (HANDLE)NULL)
    {
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAuthenticateUser(hLsaConnection,
                                  pszUserName,
                                  pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckUserInList(hLsaConnection,
                                 pszUserName,
                                 NULL);
    BAIL_ON_LSA_ERROR(dwError);

    if (bAuthOnly == false)
    {
        dwError = LsaOpenSession(hLsaConnection,
                                 pszUserName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (hLsaConnection != (HANDLE)NULL)
    {
       LsaCloseServer(hLsaConnection);
       hLsaConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
_nss_lsass_change_password(
    PCSTR pszUserName,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hLsaConnection = (HANDLE)NULL;

    if (hLsaConnection == (HANDLE)NULL)
    {
        dwError = LsaOpenServer(&hLsaConnection);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaChangePassword(hLsaConnection,
                                pszUserName,
                                pszNewPassword,
                                pszOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (hLsaConnection != (HANDLE)NULL)
    {
       LsaCloseServer(hLsaConnection);
       hLsaConnection = (HANDLE)NULL;
    }

    goto cleanup;
}


