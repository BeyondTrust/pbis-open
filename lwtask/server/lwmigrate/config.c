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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise Task Service (LWTASK)
 *
 *        Share Migration Management
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

DWORD
LwTaskGetDefaultSharePathW(
    PWSTR* ppwszFileSystemRoot
    )
{
    DWORD  dwError = 0;
    PCSTR  pszConfigKey   = "Services\\lwio\\Parameters\\Drivers\\srv";
    PCSTR  pszKeyName     = "DefaultSharePath";
    PWSTR  pwszKeyValue    = NULL;
    HANDLE hConnection    = NULL;
    HKEY   hKey           = NULL;
    char   szValue[MAX_VALUE_LENGTH] = {0};
    ULONG  ulType         = 0;
    ULONG  ulSize         = 0;

    dwError = LwRegOpenServer(&hConnection);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwRegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_LW_TASK_ERROR(dwError);

    ulSize = sizeof(szValue);

    dwError = LwRegGetValueA(
                    hConnection,
                    hKey,
                    pszConfigKey,
                    pszKeyName,
                    RRF_RT_REG_SZ,
                    &ulType,
                    szValue,
                    &ulSize);
    BAIL_ON_LW_TASK_ERROR(dwError);

    dwError = LwMbsToWc16s(szValue, &pwszKeyValue);
    BAIL_ON_LW_TASK_ERROR(dwError);

    *ppwszFileSystemRoot = pwszKeyValue;

cleanup:

    if (hConnection)
    {
        if ( hKey )
        {
            LwRegCloseKey(hConnection, hKey);
        }
        LwRegCloseServer(hConnection);
    }

    return dwError;

error:

    *ppwszFileSystemRoot = NULL;

    LW_SAFE_FREE_MEMORY(pwszKeyValue);

    goto cleanup;
}


