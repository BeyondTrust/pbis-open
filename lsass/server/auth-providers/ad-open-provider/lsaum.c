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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"
#include "lsaum_p.h"

static LSA_UM_STATE_HANDLE gLsaUmState = NULL;

DWORD
LsaUmInitialize(
    IN PLSA_AD_PROVIDER_STATE pProviderState
    )
{
    DWORD dwError = 0;
    LSA_UM_STATE_HANDLE hState = NULL;

    dwError = LsaUmpStateCreate(pProviderState, &hState);
    BAIL_ON_LSA_ERROR(dwError);

    if (gLsaUmState)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gLsaUmState = hState;
    hState = NULL;
    dwError = 0;

cleanup:
    if (hState)
    {
        LsaUmpStateDestroy(hState);
    }

    return dwError;

error:
    goto cleanup;
}

VOID
LsaUmCleanup(
    VOID
    )
{
    if (gLsaUmState)
    {
        LsaUmpStateDestroy(gLsaUmState);
        gLsaUmState = NULL;
    }
}

DWORD
LsaUmAddUser(
    IN uid_t Uid,
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN DWORD dwEndTime
    )
{
    return LsaUmpAddUser(gLsaUmState, Uid, pszUserName, pszPassword, dwEndTime);
}

DWORD
LsaUmModifyUserPassword(
    IN uid_t Uid,
    IN PCSTR pszPassword
    )
{
    return LsaUmpModifyUserPassword(gLsaUmState, Uid, pszPassword);
}

DWORD
LsaUmModifyUserMountedDirectory(
    IN uid_t Uid,
    IN PCSTR pszMountedDirectory
    )
{
    return LsaUmpModifyUserMountedDirectory(gLsaUmState, Uid, pszMountedDirectory);
}

DWORD
LsaUmRemoveUser(
    IN uid_t Uid
    )
{
    return LsaUmpRemoveUser(gLsaUmState, Uid);
}

VOID
LsaUmTriggerCheckUsersThread(
    VOID
    )
{
    LsaUmpTriggerCheckUsersThread(gLsaUmState);
}
