/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        provider-main.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
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
