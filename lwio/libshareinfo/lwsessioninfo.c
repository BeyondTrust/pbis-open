/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwsessioninfo.c
 *
 * Abstract:
 *
 *        Likewise I/O subsystem (LWIO)
 *
 *        Session Information IPC
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gSessionInfo0Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_0),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_0, sesi0_cname),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo1Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_1),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_1,  sesi1_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_1,  sesi1_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_1, sesi1_user_flags),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo2Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_2),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_2, sesi2_user_flags),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_2,  sesi2_cltype_name),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo10Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_10),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_10,  sesi10_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_10,  sesi10_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_10, sesi10_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_10, sesi10_idle_time),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfo502Spec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_502),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_cname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_username),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_num_opens),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_idle_time),
    LWMSG_MEMBER_UINT32(SESSION_INFO_502, sesi502_user_flags),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_cltype_name),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_502,  sesi502_transport),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

#define SESSION_INFO_LEVEL_0     0
#define SESSION_INFO_LEVEL_1     1
#define SESSION_INFO_LEVEL_2     2
#define SESSION_INFO_LEVEL_10   10
#define SESSION_INFO_LEVEL_502 502

static LWMsgTypeSpec gSessionInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(SESSION_INFO_UNION),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p0, LWMSG_TYPESPEC(gSessionInfo0Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_0),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p1, LWMSG_TYPESPEC(gSessionInfo1Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_1),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p2, LWMSG_TYPESPEC(gSessionInfo2Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p10, LWMSG_TYPESPEC(gSessionInfo10Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_10),
    LWMSG_MEMBER_POINTER(SESSION_INFO_UNION, p502, LWMSG_TYPESPEC(gSessionInfo502Spec)),
    LWMSG_ATTR_TAG(SESSION_INFO_LEVEL_502),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfoEnumInParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_ENUM_IN_PARAMS),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_ENUM_IN_PARAMS,  pwszServername),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_ENUM_IN_PARAMS,  pwszUncClientname),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_IN_PARAMS, dwInfoLevel),
    LWMSG_MEMBER_UINT32(SESSION_INFO_ENUM_IN_PARAMS, dwPreferredMaxLength),
    LWMSG_MEMBER_POINTER_BEGIN(SESSION_INFO_ENUM_IN_PARAMS, pdwResumeHandle),
    LWMSG_UINT32(UINT32),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfoEnumOutParamsPreambleSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_ENUM_OUT_PREAMBLE),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_OUT_PREAMBLE, dwInfoLevel),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_OUT_PREAMBLE, dwEntriesRead),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_OUT_PREAMBLE, dwTotalEntries),
    LWMSG_MEMBER_POINTER_BEGIN(FILE_INFO_ENUM_OUT_PREAMBLE, pdwResumeHandle),
    LWMSG_UINT32(UINT32),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gSessionInfoDeleteParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(SESSION_INFO_DELETE_PARAMS),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszServername),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszUncClientname),
    LWMSG_MEMBER_PWSTR(SESSION_INFO_DELETE_PARAMS, pwszUncUsername),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
VOID
LwSessionInfoFreeEnumOutPreambleInternal(
    LWMsgDataContext* pDataContext,
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble
    );

static
VOID
LwSessionInfoFreeInternal(
    LWMsgDataContext*   pDataContext,
    DWORD               dwInfoLevel,
    DWORD               dwCount,
    PSESSION_INFO_UNION pSessionInfo
    );

LW_NTSTATUS
LwSessionInfoMarshalEnumInputParameters(
    PSESSION_INFO_ENUM_IN_PARAMS pParams,
    PBYTE*                       ppBuffer,
    ULONG*                       pulBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gSessionInfoEnumInParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:
    *ppBuffer = NULL;
    *pulBufferSize = 0;

    if (pBuffer)
    {
        RtlMemoryFree(pBuffer);
    }

    goto cleanup;
}


LW_NTSTATUS
LwSessionInfoUnmarshalEnumInputParameters(
    PBYTE                         pBuffer,
    ULONG                         ulBufferSize,
    PSESSION_INFO_ENUM_IN_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSESSION_INFO_ENUM_IN_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gSessionInfoEnumInParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(
                pDataContext,
                gSessionInfoEnumInParamsSpec,
                pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoFreeEnumInputParameters(
    PSESSION_INFO_ENUM_IN_PARAMS pParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    lwmsg_data_free_graph(
                    pDataContext,
                    gSessionInfoEnumInParamsSpec,
                    pParams);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputPreamble(
    PBYTE                           pBuffer,
    ULONG                           ulBufferSize,
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble,
    PULONG                          pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfoEnumOutParamsPreambleSpec,
                        pPreamble,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_0(
    PSESSION_INFO_0 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfo0Spec,
                        pSessionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_1(
    PSESSION_INFO_1 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfo1Spec,
                        pSessionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_2(
    PSESSION_INFO_2 pSessionInfo,
    PBYTE           pBuffer,
    ULONG           ulBufferSize,
    PULONG          pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfo2Spec,
                        pSessionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_10(
    PSESSION_INFO_10 pSessionInfo,
    PBYTE            pBuffer,
    ULONG            ulBufferSize,
    PULONG           pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfo10Spec,
                        pSessionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoMarshalEnumOutputInfo_level_502(
    PSESSION_INFO_502 pSessionInfo,
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    PULONG            pulBytesUsed
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;
    LWMsgBuffer mbuf   =
    {
        .base   = pBuffer,
        .end    = pBuffer + ulBufferSize,
        .cursor = pBuffer,
        .wrap   = NULL
    };

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    status = MAP_LWMSG_STATUS(
                lwmsg_data_marshal(
                        pDataContext,
                        gSessionInfo502Spec,
                        pSessionInfo,
                        &mbuf));
    BAIL_ON_NT_STATUS(status);

    *pulBytesUsed = mbuf.cursor - mbuf.base;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoUnmarshalEnumOutputParameters(
    PBYTE                            pBuffer,
    ULONG                            ulBufferSize,
    PSESSION_INFO_ENUM_OUT_PREAMBLE* ppPreamble,
    PSESSION_INFO_UNION*             ppSessionInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LWMsgBuffer mbuf   =
        {
            .base   = pBuffer,
            .end    = pBuffer + ulBufferSize,
            .cursor = pBuffer,
            .wrap   = NULL
        };
    ULONG                           ulBytesUsed = 0;
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble   = NULL;
    PSESSION_INFO_UNION             pSessionInfo = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
                lwmsg_data_unmarshal(
                    pDataContext,
                    gSessionInfoEnumOutParamsPreambleSpec,
                    &mbuf,
                    OUT_PPVOID(&pPreamble)));
    BAIL_ON_NT_STATUS(Status);

    ulBytesUsed = mbuf.cursor - mbuf.base;

    if (pPreamble->dwEntriesRead)
    {
        ULONG iInfo = 0;

        Status = MAP_LWMSG_STATUS(
                    lwmsg_data_alloc_memory(
                        pDataContext,
                        sizeof(SESSION_INFO_UNION),
                        OUT_PPVOID(&pSessionInfo)));
        BAIL_ON_NT_STATUS(Status);

        switch (pPreamble->dwInfoLevel)
        {
            case 0:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(SESSION_INFO_0) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pSessionInfo->p0)));

                break;

            case 1:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(SESSION_INFO_1) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pSessionInfo->p1)));

                break;

            case 2:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(SESSION_INFO_2) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pSessionInfo->p2)));

                break;

            case 10:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(SESSION_INFO_10) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pSessionInfo->p10)));

                break;

            case 502:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(SESSION_INFO_502) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pSessionInfo->p502)));

                break;

            default:

                Status = STATUS_INVALID_INFO_CLASS;

                break;
        }
        BAIL_ON_NT_STATUS(Status);

        for (; iInfo < pPreamble->dwEntriesRead; iInfo++)
        {
            mbuf.cursor = pBuffer + ulBytesUsed;

            switch (pPreamble->dwInfoLevel)
            {
                case 0:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gSessionInfo0Spec,
                                    &mbuf,
                                    &pSessionInfo->p0[iInfo],
                                    sizeof(pSessionInfo->p0[iInfo])));

                    break;

                case 1:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gSessionInfo1Spec,
                                    &mbuf,
                                    &pSessionInfo->p1[iInfo],
                                    sizeof(pSessionInfo->p1[iInfo])));

                    break;

                case 2:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gSessionInfo2Spec,
                                    &mbuf,
                                    &pSessionInfo->p2[iInfo],
                                    sizeof(pSessionInfo->p2[iInfo])));

                    break;

                case 10:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gSessionInfo10Spec,
                                    &mbuf,
                                    &pSessionInfo->p10[iInfo],
                                    sizeof(pSessionInfo->p10[iInfo])));

                    break;

                case 502:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gSessionInfo502Spec,
                                    &mbuf,
                                    &pSessionInfo->p502[iInfo],
                                    sizeof(pSessionInfo->p502[iInfo])));

                    break;

                default:

                    Status = STATUS_INVALID_INFO_CLASS;

                    break;
            }
            BAIL_ON_NT_STATUS(Status);

            ulBytesUsed = mbuf.cursor - mbuf.base;
        }
    }

    *ppPreamble = pPreamble;
    *ppSessionInfo = pSessionInfo;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppPreamble    = NULL;
    *ppSessionInfo = NULL;

    if (pSessionInfo)
    {
        LwSessionInfoFreeInternal(
            pDataContext,
            pPreamble->dwInfoLevel,
            pPreamble->dwEntriesRead,
            pSessionInfo);
    }

    if (pPreamble)
    {
        LwSessionInfoFreeEnumOutPreambleInternal(pDataContext, pPreamble);
    }

    goto cleanup;
}

LW_NTSTATUS
LwSessionInfoFreeEnumOutPreamble(
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    LwSessionInfoFreeEnumOutPreambleInternal(pDataContext, pPreamble);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    goto cleanup;
}

static
VOID
LwSessionInfoFreeEnumOutPreambleInternal(
    LWMsgDataContext* pDataContext,
    PSESSION_INFO_ENUM_OUT_PREAMBLE pPreamble
    )
{
    lwmsg_data_free_graph(
                    pDataContext,
                    gSessionInfoEnumOutParamsPreambleSpec,
                    pPreamble);
}

LW_NTSTATUS
LwSessionInfoFree(
    DWORD               dwInfoLevel,
    DWORD               dwCount,
    PSESSION_INFO_UNION pSessionInfo
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    LwSessionInfoFreeInternal(pDataContext, dwInfoLevel, dwCount, pSessionInfo);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    goto cleanup;
}

static
VOID
LwSessionInfoFreeInternal(
    LWMsgDataContext*   pDataContext,
    DWORD               dwInfoLevel,
    DWORD               dwCount,
    PSESSION_INFO_UNION pSessionInfo
    )
{
    ULONG iInfo = 0;

    for(; iInfo < dwCount; iInfo++)
    {
        switch (dwInfoLevel)
        {
            case 0:

                if (pSessionInfo->p0)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gSessionInfo0Spec,
                                    &pSessionInfo->p0[iInfo]);
                }

                break;

            case 1:

                if (pSessionInfo->p1)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gSessionInfo1Spec,
                                    &pSessionInfo->p1[iInfo]);
                }

                break;

            case 2:

                if (pSessionInfo->p2)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gSessionInfo2Spec,
                                    &pSessionInfo->p2[iInfo]);
                }

                break;

            case 10:

                if (pSessionInfo->p10)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gSessionInfo10Spec,
                                    &pSessionInfo->p10[iInfo]);
                }

                break;

            case 502:

                if (pSessionInfo->p502)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gSessionInfo502Spec,
                                    &pSessionInfo->p502[iInfo]);
                }

                break;

            default:

                break;
        }
    }

    switch (dwInfoLevel)
    {
        case 0:

            if (pSessionInfo->p0)
            {
                lwmsg_data_free_memory(pDataContext, pSessionInfo->p0);
            }

            break;

        case 1:

            if (pSessionInfo->p1)
            {
                lwmsg_data_free_memory(pDataContext, pSessionInfo->p1);
            }

            break;

        case 2:

            if (pSessionInfo->p2)
            {
                lwmsg_data_free_memory(pDataContext, pSessionInfo->p2);
            }

            break;

        case 10:

            if (pSessionInfo->p10)
            {
                lwmsg_data_free_memory(pDataContext, pSessionInfo->p10);
            }

            break;

        case 502:

            if (pSessionInfo->p502)
            {
                lwmsg_data_free_memory(pDataContext, pSessionInfo->p502);
            }

            break;

        default:

            break;
    }

    lwmsg_data_free_memory(pDataContext, pSessionInfo);
}

LW_NTSTATUS
LwSessionInfoMarshalDeleteParameters(
    PSESSION_INFO_DELETE_PARAMS pParams,
    PBYTE* ppBuffer,
    ULONG* pulBufferSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gSessionInfoDeleteParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(Status);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:
    *ppBuffer = NULL;
    *pulBufferSize = 0;

    if (pBuffer)
    {
        RtlMemoryFree(pBuffer);
    }

    goto cleanup;
}


LW_NTSTATUS
LwSessionInfoUnmarshalDeleteParameters(
    PBYTE pBuffer,
    ULONG ulBufferSize,
    PSESSION_INFO_DELETE_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSESSION_INFO_DELETE_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gSessionInfoDeleteParamsSpec,
            pBuffer,
            ulBufferSize,
            OUT_PPVOID(&pParams)));
    BAIL_ON_NT_STATUS(Status);

    *ppParams = pParams;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppParams = NULL;

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gSessionInfoDeleteParamsSpec, pParams);
    }

    goto cleanup;
}


