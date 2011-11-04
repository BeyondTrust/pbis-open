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
 *        tests.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Test helper functions
 *
 * Authors: Kyle Stemen <kstemen@likewisesoftware.com>
 *
 */

#include "tests.h"

BOOL
RunConnectDisconnect(
    IN PVOID unused
    )
{
    DWORD dwError = 0;
    HANDLE connection = (HANDLE)NULL;
    
    dwError = LsaOpenServer(&connection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCloseServer(connection);
    connection = (HANDLE)NULL;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError == 0;

error:
    if (connection != (HANDLE)NULL)
    {
        LsaCloseServer(connection);
    }
    goto cleanup;
}

BOOL
SetupFindUserById(
    IN PVOID username,
    OUT PVOID *ppvFindState
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_0 userInfo = NULL;
    HANDLE connection = (HANDLE)NULL;
    FIND_STATE *state = NULL;
    
    dwError = LsaOpenServer(&connection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                connection,
                (PSTR)username,
                0,
                (PVOID*)&userInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*state), (PVOID*)&state);
    BAIL_ON_LSA_ERROR(dwError);

    state->Uid = userInfo->uid;
    state->Connection = connection;

    *(FIND_STATE **)ppvFindState = state;

cleanup:
    return dwError == 0;

error:
    if (connection != (HANDLE)NULL)
    {
        LsaCloseServer(connection);
    }
    if (userInfo != NULL)
    {
        LsaFreeUserInfo(0, userInfo);
    }
    LW_SAFE_FREE_MEMORY(state);
    *(FIND_STATE **)ppvFindState = NULL;
    goto cleanup;
}

BOOL
RunFindUserById(
    IN PVOID pvState
    )
{
    DWORD dwError = 0;
    FIND_STATE *state = (FIND_STATE *)pvState;
    PLSA_USER_INFO_0 userInfo = NULL;

    dwError = LsaFindUserById(state->Connection,
                state->Uid,
                0,
                (PVOID*)&userInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    return dwError == 0;

error:
    if (userInfo != NULL)
    {
        LsaFreeUserInfo(0, userInfo);
    }
    goto cleanup;
}

void
CleanupFindUserById(
    IN PVOID pvState
    )
{
    FIND_STATE *state = (FIND_STATE *)pvState;

    if (state == NULL)
    {
        return;
    }

    if (state->Connection != (HANDLE)NULL)
    {
        LsaCloseServer(state->Connection);
    }
    LW_SAFE_FREE_MEMORY(state);
}

BOOL
SetupConnectLsass(
    IN PVOID username,
    OUT PVOID *pHandle
    )
{
    DWORD dwError = 0;
    HANDLE connection = (HANDLE)NULL;
    
    dwError = LsaOpenServer(&connection);
    BAIL_ON_LSA_ERROR(dwError);

    *(HANDLE*)pHandle = connection;

cleanup:
    return dwError == 0;

error:
    if (connection != (HANDLE)NULL)
    {
        LsaCloseServer(connection);
    }
    *(HANDLE*)pHandle = (HANDLE)NULL;
    goto cleanup;
}

BOOL
RunGetLogLevel(
    IN PVOID handle
    )
{
    DWORD dwError = 0;
    HANDLE connection = (HANDLE)handle;
    PLSA_LOG_INFO logInfo = NULL;

    dwError = LsaGetLogInfo(connection, &logInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError == 0;

error:
    if (logInfo != NULL)
    {
        LsaFreeLogInfo(logInfo);
    }
    goto cleanup;
}

void
CleanupConnectLsass(
    IN PVOID handle
    )
{
    HANDLE connection = (HANDLE)handle;
    if (connection != NULL)
    {
        LsaCloseServer(connection);
    }
}
