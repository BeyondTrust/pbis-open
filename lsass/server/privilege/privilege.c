/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        privilege.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


DWORD
LsaSrvPrivsLookupPrivilegeValue(
    IN HANDLE hProvider,
    IN OPTIONAL PACCESS_TOKEN pAccessToken,
    IN PCWSTR pwszPrivilegeName,
    OUT PLUID pPrivilegeValue
    )
{
    DWORD err = ERROR_SUCCESS;
    PSTR pszPrivilegeName = NULL;
    PLSA_PRIVILEGE pPrivilegeEntry = NULL;

    err = LwWc16sToMbs(pwszPrivilegeName,
                       &pszPrivilegeName);
    BAIL_ON_LSA_ERROR(err);

    err = LsaSrvGetPrivilegeEntryByName(
                       pszPrivilegeName,
                       &pPrivilegeEntry);
    BAIL_ON_LSA_ERROR(err);

    *pPrivilegeValue = pPrivilegeEntry->Luid;

error:
    if (err)
    {
        if (pPrivilegeValue)
        {
            pPrivilegeValue->HighPart = 0;
            pPrivilegeValue->LowPart = 0;
        }
    }

    LW_SAFE_FREE_MEMORY(pszPrivilegeName);

    return err;
}


DWORD
LsaSrvPrivsLookupPrivilegeName(
    IN HANDLE hProvider,
    IN PLUID pPrivilegeValue,
    OUT PWSTR *ppwszPrivilegeName
    )
{
    DWORD err = ERROR_SUCCESS;

    return err;
}


BOOLEAN
LsaSrvIsPrivilegeNameValid(
    PCSTR pszPrivilegeName
    )
{
    BOOLEAN Valid = FALSE;
    PSTR ppszPrefixes[] = LSA_PRIVILEGE_VALID_PREFIXES;
    DWORD i = 0;
    PSTR pszPref = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszPrivilegeName))
    {
        for (i = 0;
             !Valid && i < sizeof(ppszPrefixes)/sizeof(ppszPrefixes[0]);
             i++)
        {
            LwStrStr(pszPrivilegeName,
                     ppszPrefixes[i],
                     &pszPref);
            if (pszPrivilegeName == pszPref)
            {
                Valid = TRUE;
            }
        }
    }
        
    return Valid;
}


BOOLEAN
LsaSrvIsPrivilegeValueValid(
    PLUID pValue
    )
{
    return (pValue->HighPart == 0);
}
