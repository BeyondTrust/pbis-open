/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwconnectioninfo.c
 *
 * Abstract:
 *
 *        BeyondTrust Input/Output Subsystem (LWIO)
 *
 *        Server connection info marshalling/unmarshalling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static LWMsgTypeSpec gConnectionInfo0Spec[] =
{
    LWMSG_STRUCT_BEGIN(CONNECTION_INFO_0),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_0, coni0_id),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gConnectionInfo1Spec[] =
{
    LWMSG_STRUCT_BEGIN(CONNECTION_INFO_1),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_1, coni1_id),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_1, coni1_type),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_1, coni1_num_opens),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_1, coni1_num_users),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_1, coni1_time),
    LWMSG_MEMBER_PWSTR(CONNECTION_INFO_1, coni1_username),
    LWMSG_MEMBER_PWSTR(CONNECTION_INFO_1, coni1_netname),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


#define CONNECTION_INFO_LEVEL_0     0
#define CONNECTION_INFO_LEVEL_1     1


/*static LWMsgTypeSpec gShareInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(CONNECTION_INFO_UNION),
    LWMSG_MEMBER_POINTER(CONNECTION_INFO_UNION, p0, LWMSG_TYPESPEC(gConnectionInfo0Spec)),
    LWMSG_ATTR_TAG(CONNECTION_INFO_LEVEL_0),
    LWMSG_MEMBER_POINTER(CONNECTION_INFO_UNION, p1, LWMSG_TYPESPEC(gConnectionInfo1Spec)),
    LWMSG_ATTR_TAG(CONNECTION_INFO_LEVEL_1),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};
*/

static LWMsgTypeSpec gConnectionInfoEnumInParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONNECTION_INFO_ENUM_IN_PARAMS),
    LWMSG_MEMBER_PWSTR(CONNECTION_INFO_ENUM_IN_PARAMS, pwszQualifier),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_IN_PARAMS, dwLevel),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_IN_PARAMS, dwPreferredMaxLen),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_IN_PARAMS, dwResume),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gConnectionInfoEnumOutParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(CONNECTION_INFO_ENUM_OUT_PARAMS),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_OUT_PARAMS, dwLevel),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_OUT_PARAMS, dwNumEntries),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_OUT_PARAMS, dwTotalNumEntries),
    LWMSG_MEMBER_UINT32(CONNECTION_INFO_ENUM_OUT_PARAMS, dwResume),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static
VOID
LwConnectionInfoFreeEnumOutputParametersInternal(
    LWMsgDataContext* pDataContext,
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams
    );

static
VOID
LwConnectionInfoFreeInternal(
    LWMsgDataContext*      pDataContext,
    DWORD                  dwInfoLevel,
    DWORD                  dwCount,
    PCONNECTION_INFO_UNION pConnectionInfo
    );


LW_NTSTATUS
LwConnectionInfoMarshalEnumInputParameters(
    PCONNECTION_INFO_ENUM_IN_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    size_t sBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gConnectionInfoEnumInParamsSpec,
            pParams,
            &pBuffer,
            &sBufferSize));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG)sBufferSize;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    *ppBuffer      = NULL;
    *pulBufferSize = 0;

    RTL_FREE(&pBuffer);

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoUnmarshalEnumInputParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PCONNECTION_INFO_ENUM_IN_PARAMS* ppParams
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCONNECTION_INFO_ENUM_IN_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gConnectionInfoEnumInParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppParams = pParams;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gConnectionInfoEnumInParamsSpec, pParams);
    }

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoFreeEnumInputParameters(
    PCONNECTION_INFO_ENUM_IN_PARAMS pParams
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    lwmsg_data_free_graph(
                    pDataContext,
                    gConnectionInfoEnumInParamsSpec,
                    pParams);

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputParameters(
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams,
    PBYTE                            pBuffer,
    ULONG                            ulBufferSize,
    PULONG                           pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf =
    {
        .base     = pBuffer,
        .end      = pBuffer + ulBufferSize,
        .cursor   = pBuffer,
        .wrap     = NULL
    };

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gConnectionInfoEnumOutParamsSpec,
                        pParams,
                        &mbuf));
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    *pulBytesUsed = 0;

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputInfo_level_0(
    PCONNECTION_INFO_0  pConnectionInfo,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf =
    {
        .base     = pBuffer,
        .end      = pBuffer + ulBufferSize,
        .cursor   = pBuffer,
        .wrap     = NULL
    };

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gConnectionInfo0Spec,
                        pConnectionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    *pulBytesUsed = 0;

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoMarshalEnumOutputInfo_level_1(
    PCONNECTION_INFO_1  pConnectionInfo,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf =
    {
        .base     = pBuffer,
        .end      = pBuffer + ulBufferSize,
        .cursor   = pBuffer,
        .wrap     = NULL
    };

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gConnectionInfo1Spec,
                        pConnectionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    *pulBytesUsed = 0;

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoUnmarshalEnumOutputParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PCONNECTION_INFO_ENUM_OUT_PARAMS* ppParamsOut,
    PCONNECTION_INFO_UNION*           ppConnectionInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgBuffer mbuf =
        {
            .base   = pBuffer,
            .end    = pBuffer + ulBufferSize,
            .cursor = pBuffer,
            .wrap   = NULL
        };
    ULONG ulBytesUsed = 0;
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParamsOut = NULL;
    PCONNECTION_INFO_UNION pConnectionInfo = NULL;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
                lwmsg_data_unmarshal(
                    pDataContext,
                    gConnectionInfoEnumOutParamsSpec,
                    &mbuf,
                    OUT_PPVOID(&pParamsOut)));
    BAIL_ON_NT_STATUS(ntStatus);

    ulBytesUsed = mbuf.cursor - mbuf.base;

    if (pParamsOut->dwNumEntries)
    {
        ULONG iInfo = 0;

        ntStatus = MAP_LWMSG_STATUS(
                    lwmsg_data_alloc_memory(
                        pDataContext,
                        sizeof(CONNECTION_INFO_UNION),
                        OUT_PPVOID(&pConnectionInfo)));
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pParamsOut->dwLevel)
        {
            case 0:
                ntStatus = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(CONNECTION_INFO_0) * pParamsOut->dwNumEntries,
                            OUT_PPVOID(&pConnectionInfo->p0)));
                break;

            case 1:
                ntStatus = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(CONNECTION_INFO_1) * pParamsOut->dwNumEntries,
                            OUT_PPVOID(&pConnectionInfo->p1)));
                break;

            default:
                ntStatus = STATUS_INVALID_INFO_CLASS;
                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        for (; iInfo < pParamsOut->dwNumEntries; iInfo++)
        {
            mbuf.cursor = pBuffer + ulBytesUsed;

            switch (pParamsOut->dwLevel)
            {
                case 0:
                    ntStatus = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gConnectionInfo0Spec,
                                    &mbuf,
                                    &pConnectionInfo->p0[iInfo],
                                    sizeof(pConnectionInfo->p0[iInfo])));
                    break;

                case 1:
                    ntStatus = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gConnectionInfo1Spec,
                                    &mbuf,
                                    &pConnectionInfo->p1[iInfo],
                                    sizeof(pConnectionInfo->p1[iInfo])));
                    break;

                default:
                    ntStatus = STATUS_INVALID_INFO_CLASS;
                    break;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            ulBytesUsed = mbuf.cursor - mbuf.base;
        }
    }

    *ppParamsOut      = pParamsOut;
    *ppConnectionInfo = pConnectionInfo;

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:

    *ppParamsOut      = NULL;
    *ppConnectionInfo = NULL;

    if (pConnectionInfo)
    {
        LwConnectionInfoFreeInternal(
            pDataContext,
            pParamsOut->dwLevel,
            pParamsOut->dwNumEntries,
            pConnectionInfo);
    }

    if (pParamsOut)
    {
        LwConnectionInfoFreeEnumOutputParametersInternal(pDataContext, pParamsOut);
    }

    goto cleanup;
}


LW_NTSTATUS
LwConnectionInfoFreeEnumOutputParameters(
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LWMsgDataContext *pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    LwConnectionInfoFreeEnumOutputParametersInternal(pDataContext, pParams);

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    goto cleanup;
}

static
VOID
LwConnectionInfoFreeEnumOutputParametersInternal(
    LWMsgDataContext* pDataContext,
    PCONNECTION_INFO_ENUM_OUT_PARAMS pParams
    )
{
    lwmsg_data_free_graph(
                    pDataContext,
                    gConnectionInfoEnumOutParamsSpec,
                    pParams);
}


LW_NTSTATUS
LwConnectionInfoFree(
    LWMsgDataContext* pDataContext,
    DWORD dwInfoLevel,
    DWORD dwCount,
    PCONNECTION_INFO_UNION pConnectionInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    LwConnectionInfoFreeInternal(pDataContext,
                                 dwInfoLevel,
                                 dwCount,
                                 pConnectionInfo);

cleanup:
    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:
    goto cleanup;
}


static
VOID
LwConnectionInfoFreeInternal(
    LWMsgDataContext*      pDataContext,
    DWORD                  dwInfoLevel,
    DWORD                  dwCount,
    PCONNECTION_INFO_UNION pConnectionInfo
    )
{
    ULONG iInfo = 0;

    for(; iInfo < dwCount; iInfo++)
    {
        switch (dwInfoLevel)
        {
            case 0:
                if (pConnectionInfo->p0)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gConnectionInfo0Spec,
                                    &pConnectionInfo->p0[iInfo]);
                }
                break;

            case 1:
                if (pConnectionInfo->p1)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gConnectionInfo1Spec,
                                    &pConnectionInfo->p1[iInfo]);
                }
                break;

            default:
                break;
        }
    }

    switch (dwInfoLevel)
    {
        case 0:
            if (pConnectionInfo->p0)
            {
                lwmsg_data_free_memory(pDataContext, pConnectionInfo->p0);
            }
            break;

        case 1:
            if (pConnectionInfo->p1)
            {
                lwmsg_data_free_memory(pDataContext, pConnectionInfo->p1);
            }
            break;

        default:
            break;
    }

    lwmsg_data_free_memory(pDataContext, pConnectionInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
