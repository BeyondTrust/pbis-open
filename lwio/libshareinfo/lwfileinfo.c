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
 *        lwfileinfo.c
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem (LWIO)
 *
 *        File Information IPC
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static LWMsgTypeSpec gFileInfo2Spec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_2),
    LWMSG_MEMBER_UINT32(FILE_INFO_2, fi2_id),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfo3Spec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_3),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_idd),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_permissions),
    LWMSG_MEMBER_UINT32(FILE_INFO_3, fi3_num_locks),
    LWMSG_MEMBER_PWSTR(FILE_INFO_3,  fi3_path_name),
    LWMSG_MEMBER_PWSTR(FILE_INFO_3,  fi3_username),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

#define FILE_INFO_LEVEL_2   2
#define FILE_INFO_LEVEL_3   3

static LWMsgTypeSpec gFileInfoUnionSpec[] =
{
    LWMSG_UNION_BEGIN(FILE_INFO_UNION),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p2, LWMSG_TYPESPEC(gFileInfo2Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_2),
    LWMSG_MEMBER_POINTER(FILE_INFO_UNION, p3, LWMSG_TYPESPEC(gFileInfo3Spec)),
    LWMSG_ATTR_TAG(FILE_INFO_LEVEL_3),
    LWMSG_UNION_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoEnumInParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_ENUM_IN_PARAMS),
    LWMSG_MEMBER_PWSTR(FILE_INFO_ENUM_IN_PARAMS,         pwszBasepath),
    LWMSG_MEMBER_PWSTR(FILE_INFO_ENUM_IN_PARAMS,         pwszUsername),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_IN_PARAMS,        dwInfoLevel),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_IN_PARAMS,        dwPreferredMaxLength),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_IN_PARAMS,        dwEntriesRead),
    LWMSG_MEMBER_UINT32(FILE_INFO_ENUM_IN_PARAMS,        dwTotalEntries),
    LWMSG_MEMBER_POINTER_BEGIN(FILE_INFO_ENUM_IN_PARAMS, pdwResumeHandle),
    LWMSG_UINT32(UINT32),
    LWMSG_POINTER_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoEnumOutParamsPreambleSpec[] =
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

static LWMsgTypeSpec gFileInfoGetInParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_GET_INFO_IN_PARAMS),
    LWMSG_MEMBER_UINT32(FILE_INFO_GET_INFO_IN_PARAMS, dwInfoLevel),
    LWMSG_MEMBER_UINT32(FILE_INFO_GET_INFO_IN_PARAMS, dwFileId),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gFileInfoCloseParamsSpec[] =
{
    LWMSG_STRUCT_BEGIN(FILE_INFO_CLOSE_PARAMS),
    LWMSG_MEMBER_UINT32(FILE_INFO_CLOSE_PARAMS, dwFileId),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
VOID
LwFileInfoFreeEnumOutPreambleInternal(
    LWMsgDataContext*            pDataContext,
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble
    );

static
VOID
LwFileInfoFreeInternal(
    LWMsgDataContext* pDataContext,
    DWORD             dwInfoLevel,
    DWORD             dwCount,
    PFILE_INFO_UNION  pFileInfo
    );

LW_NTSTATUS
LwFileInfoMarshalEnumInputParameters(
    PFILE_INFO_ENUM_IN_PARAMS pParams,
    PBYTE*                    ppBuffer,
    ULONG*                    pulBufferSize
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
            gFileInfoEnumInParamsSpec,
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
LwFileInfoUnmarshalEnumInputParameters(
    PBYTE                      pBuffer,
    ULONG                      ulBufferSize,
    PFILE_INFO_ENUM_IN_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_INFO_ENUM_IN_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoEnumInParamsSpec,
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
        lwmsg_data_free_graph(pDataContext, gFileInfoEnumInParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoFreeEnumInputParameters(
    PFILE_INFO_ENUM_IN_PARAMS pParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    lwmsg_data_free_graph(
                    pDataContext,
                    gFileInfoEnumInParamsSpec,
                    pParams);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoMarshalEnumOutputPreamble(
    PBYTE                        pBuffer,
    ULONG                        ulBufferSize,
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble,
    PULONG                       pulBytesUsed
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
                        gFileInfoEnumOutParamsPreambleSpec,
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
LwFileInfoMarshalEnumOutputInfo_level_2(
    PFILE_INFO_2 pFileInfo,
    PBYTE        pBuffer,
    ULONG        ulBufferSize,
    PULONG       pulBytesUsed
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
                        gFileInfo2Spec,
                        pFileInfo,
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
LwFileInfoMarshalEnumOutputInfo_level_3(
    PFILE_INFO_3 pFileInfo,
    PBYTE        pBuffer,
    ULONG        ulBufferSize,
    PULONG       pulBytesUsed
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
                        gFileInfo3Spec,
                        pFileInfo,
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
LwFileInfoUnmarshalEnumOutputParameters(
    PBYTE                         pBuffer,
    ULONG                         ulBufferSize,
    PFILE_INFO_ENUM_OUT_PREAMBLE* ppPreamble,
    PFILE_INFO_UNION*             ppFileInfo
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
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble   = NULL;
    PFILE_INFO_UNION             pFileInfo = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
                lwmsg_data_unmarshal(
                    pDataContext,
                    gFileInfoEnumOutParamsPreambleSpec,
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
                        sizeof(FILE_INFO_UNION),
                        OUT_PPVOID(&pFileInfo)));
        BAIL_ON_NT_STATUS(Status);

        switch (pPreamble->dwInfoLevel)
        {
            case 2:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(FILE_INFO_2) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pFileInfo->p2)));

                break;

            case 3:

                Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(FILE_INFO_3) * pPreamble->dwEntriesRead,
                            OUT_PPVOID(&pFileInfo->p3)));

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
                case 2:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gFileInfo2Spec,
                                    &mbuf,
                                    &pFileInfo->p2[iInfo],
                                    sizeof(pFileInfo->p2[iInfo])));

                    break;

                case 3:

                    Status = MAP_LWMSG_STATUS(
                                lwmsg_data_unmarshal_into(
                                    pDataContext,
                                    gFileInfo3Spec,
                                    &mbuf,
                                    &pFileInfo->p3[iInfo],
                                    sizeof(pFileInfo->p3[iInfo])));

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
    *ppFileInfo = pFileInfo;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppPreamble    = NULL;
    *ppFileInfo = NULL;

    if (pFileInfo)
    {
        LwFileInfoFreeInternal(
            pDataContext,
            pPreamble->dwInfoLevel,
            pPreamble->dwEntriesRead,
            pFileInfo);
    }

    if (pPreamble)
    {
        LwFileInfoFreeEnumOutPreambleInternal(pDataContext, pPreamble);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoFreeEnumOutPreamble(
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    LwFileInfoFreeEnumOutPreambleInternal(pDataContext, pPreamble);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    goto cleanup;
}

static
VOID
LwFileInfoFreeEnumOutPreambleInternal(
    LWMsgDataContext*            pDataContext,
    PFILE_INFO_ENUM_OUT_PREAMBLE pPreamble
    )
{
    lwmsg_data_free_graph(
                    pDataContext,
                    gFileInfoEnumOutParamsPreambleSpec,
                    pPreamble);
}

LW_NTSTATUS
LwFileInfoFree(
    DWORD            dwInfoLevel,
    DWORD            dwCount,
    PFILE_INFO_UNION pFileInfo
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    LWMsgDataContext* pDataContext = NULL;

    status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(status);

    LwFileInfoFreeInternal(pDataContext, dwInfoLevel, dwCount, pFileInfo);

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return status;

error:

    goto cleanup;
}

static
VOID
LwFileInfoFreeInternal(
    LWMsgDataContext* pDataContext,
    DWORD             dwInfoLevel,
    DWORD             dwCount,
    PFILE_INFO_UNION  pFileInfo
    )
{
    ULONG iInfo = 0;

    for(; iInfo < dwCount; iInfo++)
    {
        switch (dwInfoLevel)
        {
            case 2:

                if (pFileInfo->p2)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gFileInfo2Spec,
                                    &pFileInfo->p2[iInfo]);
                }

                break;

            case 3:

                if (pFileInfo->p3)
                {
                    lwmsg_data_destroy_graph(
                                    pDataContext,
                                    gFileInfo3Spec,
                                    &pFileInfo->p3[iInfo]);
                }

                break;

            default:

                break;
        }
    }

    switch (dwInfoLevel)
    {
        case 2:

            if (pFileInfo->p2)
            {
                lwmsg_data_free_memory(pDataContext, pFileInfo->p2);
            }

            break;

        case 3:

            if (pFileInfo->p3)
            {
                lwmsg_data_free_memory(pDataContext, pFileInfo->p3);
            }

            break;

        default:

            break;
    }

    lwmsg_data_free_memory(pDataContext, pFileInfo);
}


LW_NTSTATUS
LwFileInfoMarshalGetInfoInParameters(
    PFILE_INFO_GET_INFO_IN_PARAMS pParams,
    PBYTE*                        ppBuffer,
    ULONG*                        pulBufferSize
    )
{
    NTSTATUS ntStatus = 0;
    VOID* pBuffer = NULL;
    size_t ulBufferSize = 0;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_marshal_flat_alloc(
            pDataContext,
            gFileInfoGetInParamsSpec,
            pParams,
            &pBuffer,
            &ulBufferSize));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppBuffer = pBuffer;
    *pulBufferSize = (ULONG) ulBufferSize;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

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
LwFileInfoUnmarshalGetInfoInParameters(
    PBYTE                          pBuffer,
    ULONG                          ulBufferSize,
    PFILE_INFO_GET_INFO_IN_PARAMS* ppParams
    )
{
    NTSTATUS ntStatus = 0;
    PFILE_INFO_GET_INFO_IN_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoGetInParamsSpec,
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
        lwmsg_data_free_graph(pDataContext, gFileInfoGetInParamsSpec, pParams);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoFreeGetInfoInParameters(
    PFILE_INFO_GET_INFO_IN_PARAMS pParams
    )
{
    NTSTATUS ntStatus = 0;
    LWMsgDataContext* pDataContext = NULL;

    ntStatus = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pParams)
    {
        lwmsg_data_free_graph(pDataContext, gFileInfoGetInParamsSpec, pParams);
    }

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return ntStatus;

error:

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoUnmarshalGetInfoOutParameters(
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    DWORD             dwInfoLevel,
    PFILE_INFO_UNION* ppFileInfo
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
    PFILE_INFO_UNION  pFileInfo    = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
                lwmsg_data_alloc_memory(
                    pDataContext,
                    sizeof(FILE_INFO_UNION),
                    OUT_PPVOID(&pFileInfo)));
    BAIL_ON_NT_STATUS(Status);

    switch (dwInfoLevel)
    {
        case 2:

            Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(FILE_INFO_2),
                            OUT_PPVOID(&pFileInfo->p2)));
            BAIL_ON_NT_STATUS(Status);

            Status = MAP_LWMSG_STATUS(
                        lwmsg_data_unmarshal_into(
                            pDataContext,
                            gFileInfo2Spec,
                            &mbuf,
                            pFileInfo->p2,
                            sizeof(FILE_INFO_2)));

            break;

        case 3:

            Status = MAP_LWMSG_STATUS(
                        lwmsg_data_alloc_memory(
                            pDataContext,
                            sizeof(FILE_INFO_3),
                            OUT_PPVOID(&pFileInfo->p3)));
            BAIL_ON_NT_STATUS(Status);

            Status = MAP_LWMSG_STATUS(
                        lwmsg_data_unmarshal_into(
                            pDataContext,
                            gFileInfo3Spec,
                            &mbuf,
                            pFileInfo->p3,
                            sizeof(FILE_INFO_3)));

            break;

        default:

            Status = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(Status);

    *ppFileInfo = pFileInfo;

cleanup:

    LwSrvInfoReleaseDataContext(pDataContext);

    return Status;

error:

    *ppFileInfo = NULL;

    if (pFileInfo)
    {
        LwFileInfoFreeInternal(
            pDataContext,
            dwInfoLevel,
            1,
            pFileInfo);
    }

    goto cleanup;
}

LW_NTSTATUS
LwFileInfoMarshalCloseParameters(
    PFILE_INFO_CLOSE_PARAMS pParams,
    PBYTE*                  ppBuffer,
    ULONG*                  pulBufferSize
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
            gFileInfoCloseParamsSpec,
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
LwFileInfoUnmarshalCloseParameters(
    PBYTE                    pBuffer,
    ULONG                    ulBufferSize,
    PFILE_INFO_CLOSE_PARAMS* ppParams
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_INFO_CLOSE_PARAMS pParams = NULL;
    LWMsgDataContext* pDataContext = NULL;

    Status = LwSrvInfoAcquireDataContext(&pDataContext);
    BAIL_ON_NT_STATUS(Status);

    Status = MAP_LWMSG_STATUS(
        lwmsg_data_unmarshal_flat(
            pDataContext,
            gFileInfoCloseParamsSpec,
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
        lwmsg_data_free_graph(pDataContext, gFileInfoCloseParamsSpec, pParams);
    }

    goto cleanup;
}

