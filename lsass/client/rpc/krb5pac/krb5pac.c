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
 *        krb5pac.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Kerberos 5 PAC decoding functions
 *
 * Authors: Kyle Stemen (kstemen@likewise.com)
 */

#include "includes.h"

VOID
FreeUnicodeStringContents(
    PUNICODE_STRING  pStr
    )
{
    if (pStr->Buffer == NULL)
    {
        return;
    }

    rpc_ss_client_free(pStr->Buffer);
    pStr->Buffer        = NULL;
    pStr->Length        = 0;
    pStr->MaximumLength = 0;
 
    return;    
}


VOID
FreeRidWithAttributeArrayContents(
    PRID_WITH_ATTRIBUTE_ARRAY  pArr
    )
{
    if (pArr->pRids == NULL)
    {
        return;
    }

    rpc_ss_client_free(pArr->pRids);
    pArr->pRids   = NULL;
    pArr->dwCount = 0;

    return;    
}


VOID
FreeNetrSamBaseInfoContents(
    NetrSamBaseInfo *pBase
    )
{
    PUNICODE_STRING pStringArray[] = {
        &pBase->account_name,
        &pBase->full_name,
        &pBase->logon_script,
        &pBase->profile_path,
        &pBase->home_directory,
        &pBase->home_drive,
        &pBase->logon_server,
        &pBase->domain,
    };
    int i;

    for (i = 0; i < sizeof(pStringArray)/sizeof(pStringArray[0]); i++)
    {
        FreeUnicodeStringContents(pStringArray[i]);
    }

    FreeRidWithAttributeArrayContents(&pBase->groups);

    if (pBase->domain_sid == NULL)
    {
        return;
    }

    rpc_ss_client_free(pBase->domain_sid);
    pBase->domain_sid = NULL;

    return;    
}


VOID
FreeSidAttrArray(
    NetrSidAttr *pSids,
    size_t sCount
    )
{
    size_t sIndex;

    if (pSids == NULL)
    {
        return;
    }

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (pSids[sIndex].sid != NULL)
        {
            rpc_ss_client_free(pSids[sIndex].sid);
        }
    }
    rpc_ss_client_free(pSids);
}

VOID
FreeNetrSamInfo3Contents(
    NetrSamInfo3 *pInfo
    )
{
    FreeNetrSamBaseInfoContents(&pInfo->base);

    if (pInfo->sids == NULL)
    {
        return;
    }

    FreeSidAttrArray(pInfo->sids, pInfo->sidcount);
    pInfo->sids = NULL;
    pInfo->sidcount = 0;

    return;    
}

VOID
FreePacLogonInfo(
    PAC_LOGON_INFO *pInfo)
{
    if (pInfo == NULL)
    {
        return;
    }

    FreeNetrSamInfo3Contents(&pInfo->info3);

    if (pInfo->res_group_dom_sid != NULL)
    {
        rpc_ss_client_free(pInfo->res_group_dom_sid);
    }

    FreeRidWithAttributeArrayContents(&pInfo->res_groups);
    rpc_ss_client_free(pInfo);

    return;
}

#ifdef BAIL_ON_ERR_STATUS
#undef BAIL_ON_ERR_STATUS
#endif

#define BAIL_ON_ERR_STATUS(err)                 \
    if ((err) != error_status_ok) {             \
        goto error;                             \
    }


NTSTATUS
DecodePacLogonInfo(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT PAC_LOGON_INFO** ppLogonInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    idl_es_handle_t decodingHandle = NULL;
    error_status_t status;
    error_status_t status2;
    PPAC_LOGON_INFO pLogonInfo = NULL;

    idl_es_decode_buffer(
            (unsigned char *)pBuffer,
            sBufferLen,
            &decodingHandle,
            &status);
    BAIL_ON_ERR_STATUS(status);


    idl_es_set_attrs(decodingHandle, IDL_ES_MIDL_COMPAT, &status);
    BAIL_ON_ERR_STATUS(status);

    PAC_LOGON_INFO_Decode(decodingHandle, &pLogonInfo);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&decodingHandle, &status);
    decodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

    *ppLogonInfo = pLogonInfo;

cleanup:
    if (status != error_status_ok)
    {
        ntStatus = LwRpcStatusToNtStatus(status);
    }

    return ntStatus;

error:
    if (pLogonInfo != NULL)
    {
        FreePacLogonInfo(pLogonInfo);
    }
    if (decodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&decodingHandle, &status2);
    }
    goto cleanup;
}


NTSTATUS
EncodePacLogonInfo(
    IN PAC_LOGON_INFO* pLogonInfo,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    idl_es_handle_t decodingHandle = NULL;
    error_status_t status;
    error_status_t status2;

    idl_es_encode_dyn_buffer(
        (idl_byte**) (void*) ppEncodedBuffer,
        (idl_ulong_int*) pdwEncodedSize,
        &decodingHandle,
        &status);
    BAIL_ON_ERR_STATUS(status);

    idl_es_set_attrs(decodingHandle, IDL_ES_MIDL_COMPAT, &status);
    BAIL_ON_ERR_STATUS(status);

    PAC_LOGON_INFO_Encode(decodingHandle, pLogonInfo);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&decodingHandle, &status);
    decodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

cleanup:
    if (status != error_status_ok)
    {
        ntStatus = LwRpcStatusToNtStatus(status);
    }

    return ntStatus;

error:

    if (decodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&decodingHandle, &status2);
    }

    goto cleanup;
}
