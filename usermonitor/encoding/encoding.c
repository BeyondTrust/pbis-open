/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        packing.c
 *
 * Abstract:
 *
 *        User monitor encoding and decoding routines
 *
 * Authors: Kyle Stemen <kstemen@beyondtrust.com>
 */

#ifdef _WIN32
#define LW_USERMONITORLIB_API __declspec(dllexport) __stdcall
#endif

#include "includes.h"
#include "encoding_h.h"


VOID
FreeUserMonitorPasswdContents(
    PUSER_MONITOR_PASSWD pValue
    )
{
    rpc_ss_client_free(pValue->pw_name);
    rpc_ss_client_free(pValue->pw_passwd);
    rpc_ss_client_free(pValue->pw_gecos);
    rpc_ss_client_free(pValue->pw_dir);
    rpc_ss_client_free(pValue->pw_shell);
    rpc_ss_client_free(pValue->pDisplayName);
}

VOID
LW_USERMONITORLIB_API
FreeUserChange(
    PUSER_CHANGE pValue
    )
{
    if (pValue == NULL)
    {
        return;
    }

    FreeUserMonitorPasswdContents(&pValue->OldValue);
    FreeUserMonitorPasswdContents(&pValue->NewValue);
    rpc_ss_client_free(pValue);
}

VOID
FreeUserMonitorGroupContents(
    PUSER_MONITOR_GROUP pValue
    )
{
    rpc_ss_client_free(pValue->gr_name);
    rpc_ss_client_free(pValue->gr_passwd);
}

VOID
LW_USERMONITORLIB_API
FreeGroupChange(
    PGROUP_CHANGE pValue
    )
{
    if (pValue == NULL)
    {
        return;
    }

    FreeUserMonitorGroupContents(&pValue->OldValue);
    FreeUserMonitorGroupContents(&pValue->NewValue);
    rpc_ss_client_free(pValue);
}

VOID
LW_USERMONITORLIB_API
FreeGroupMembershipChange(
    PGROUP_MEMBERSHIP_CHANGE pValue
    )
{
    if (pValue == NULL)
    {
        return;
    }

    rpc_ss_client_free(pValue->pUserName);
    rpc_ss_client_free(pValue->pGroupName);
    rpc_ss_client_free(pValue);
}

#ifdef _WIN32
#define BAIL_ON_ERR_STATUS(err)                 \
    dwError = err; \
    if (dwError != 0) \
    { \
        goto error; \
    }
#else
#define BAIL_ON_ERR_STATUS(err)                 \
    dwError = LwNtStatusToWin32Error(LwRpcStatusToNtStatus(err)); \
    BAIL_ON_UMN_ERROR(dwError);
#endif

DWORD
LW_USERMONITORLIB_API
DecodeUserChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PUSER_CHANGE* ppValue
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;
    PUSER_CHANGE pValue = NULL;

    idl_es_decode_buffer(
            (idl_byte *)pBuffer,
            sBufferLen,
            &encodingHandle,
            &status);
    BAIL_ON_ERR_STATUS(status);

    DCETHREAD_TRY
    USER_CHANGE_Decode(encodingHandle, &pValue);
    DCETHREAD_CATCH_ALL(status);
    DCETHREAD_ENDTRY
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

    *ppValue = pValue;

cleanup:
    return dwError;

error:
    if (pValue != NULL)
    {
        FreeUserChange(pValue);
    }
    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }
    goto cleanup;
}

DWORD
LW_USERMONITORLIB_API
EncodeUserChange(
    IN PUSER_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;

    idl_es_encode_dyn_buffer(
        (idl_byte**) (void*) ppEncodedBuffer,
        (idl_ulong_int*) pdwEncodedSize,
        &encodingHandle,
        &status);
    BAIL_ON_ERR_STATUS(status);

    DCETHREAD_TRY
    USER_CHANGE_Encode(encodingHandle, pValue);
    DCETHREAD_CATCH_ALL(status);
    DCETHREAD_ENDTRY
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

cleanup:
    return dwError;

error:

    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }

    goto cleanup;
}

DWORD
LW_USERMONITORLIB_API
DecodeGroupChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PGROUP_CHANGE* ppValue
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;
    PGROUP_CHANGE pValue = NULL;

    idl_es_decode_buffer(
            (idl_byte *)pBuffer,
            sBufferLen,
            &encodingHandle,
            &status);
    BAIL_ON_ERR_STATUS(status);

    GROUP_CHANGE_Decode(encodingHandle, &pValue);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

    *ppValue = pValue;

cleanup:
    return dwError;

error:
    if (pValue != NULL)
    {
        FreeGroupChange(pValue);
    }
    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }
    goto cleanup;
}

DWORD
LW_USERMONITORLIB_API
EncodeGroupChange(
    IN PGROUP_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;

    idl_es_encode_dyn_buffer(
        (idl_byte**) (void*) ppEncodedBuffer,
        (idl_ulong_int*) pdwEncodedSize,
        &encodingHandle,
        &status);
    BAIL_ON_ERR_STATUS(status);

    GROUP_CHANGE_Encode(encodingHandle, pValue);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

cleanup:
    return dwError;

error:

    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }

    goto cleanup;
}

DWORD
LW_USERMONITORLIB_API
DecodeGroupMembershipChange(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PGROUP_MEMBERSHIP_CHANGE* ppValue
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;
    PGROUP_MEMBERSHIP_CHANGE pValue = NULL;

    idl_es_decode_buffer(
            (idl_byte *)pBuffer,
            sBufferLen,
            &encodingHandle,
            &status);
    BAIL_ON_ERR_STATUS(status);

    GROUP_MEMBERSHIP_CHANGE_Decode(encodingHandle, &pValue);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

    *ppValue = pValue;

cleanup:
    return dwError;

error:
    if (pValue != NULL)
    {
        FreeGroupMembershipChange(pValue);
    }
    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }
    goto cleanup;
}

DWORD
LW_USERMONITORLIB_API
EncodeGroupMembershipChange(
    IN PGROUP_MEMBERSHIP_CHANGE pValue,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    )
{
    DWORD dwError = 0;
    idl_es_handle_t encodingHandle = NULL;
    error_status_t status = 0;
    error_status_t status2;

    idl_es_encode_dyn_buffer(
        (idl_byte**) (void*) ppEncodedBuffer,
        (idl_ulong_int*) pdwEncodedSize,
        &encodingHandle,
        &status);
    BAIL_ON_ERR_STATUS(status);

    GROUP_MEMBERSHIP_CHANGE_Encode(encodingHandle, pValue);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&encodingHandle, &status);
    encodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

cleanup:
    return dwError;

error:

    if (encodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&encodingHandle, &status2);
    }

    goto cleanup;
}

#ifdef _WIN32
void
__RPC_USER
midl_user_free(
    void* p
    )
{
    free(p);
}

void*
__RPC_USER
midl_user_allocate(
    size_t size
    )
{
    return malloc(size);
}
#endif
