/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        exportsecctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        ExportSecurityContext client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

static LWMsgTypeSpec gNtlmSecurityContextStateSpec[] =
{
    LWMSG_ENUM_BEGIN(NTLM_STATE, 1, LWMSG_UNSIGNED),
    LWMSG_ENUM_VALUE(NtlmStateBlank),
    LWMSG_ENUM_VALUE(NtlmStateNegotiate),
    LWMSG_ENUM_VALUE(NtlmStateChallenge),
    LWMSG_ENUM_VALUE(NtlmStateResponse),
    LWMSG_ENUM_END,
    LWMSG_TYPE_END,
};

static LWMsgTypeSpec gNtlmRC4KeySpec[] =
{
    LWMSG_STRUCT_BEGIN(RC4_KEY),
    LWMSG_MEMBER_UINT32(RC4_KEY, x),
    LWMSG_MEMBER_UINT32(RC4_KEY, y),
    LWMSG_MEMBER_ARRAY(RC4_KEY, data, LWMSG_UINT32(RC4_INT)),
    LWMSG_ATTR_LENGTH_STATIC(256),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/* Not marshalling CredHandle or pUserInfo as part of this
 * structure.  pUserInfo will be handled separately.
 * CredHandle is not used once the context is established
 * and would need special handling to relocate the
 * credentials within the repository.
 */
static LWMsgTypeSpec gNtlmSecurityContextSpec[] =
{
    LWMSG_STRUCT_BEGIN(NTLM_CONTEXT),
    LWMSG_MEMBER_TYPESPEC(NTLM_CONTEXT, NtlmState, gNtlmSecurityContextStateSpec),
    LWMSG_MEMBER_ARRAY(NTLM_CONTEXT, Challenge, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_LENGTH_STATIC(NTLM_CHALLENGE_SIZE),
    LWMSG_MEMBER_PSTR(NTLM_CONTEXT, pszClientUsername),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT, NegotiatedFlags),
    LWMSG_MEMBER_INT32(NTLM_CONTEXT, nRefCount),
    LWMSG_MEMBER_ARRAY(NTLM_CONTEXT, SessionKey, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_ATTR_LENGTH_STATIC(NTLM_SESSION_KEY_SIZE),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT, cbSessionKeyLen),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT, bDoAnonymous),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT, bInitiatedSide),
    LWMSG_MEMBER_ARRAY(NTLM_CONTEXT, SignKey, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_ATTR_LENGTH_STATIC(MD5_DIGEST_LENGTH),
    LWMSG_MEMBER_ARRAY(NTLM_CONTEXT, VerifyKey, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_ATTR_LENGTH_STATIC(MD5_DIGEST_LENGTH),
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT, pSealKey, LWMSG_TYPESPEC(gNtlmRC4KeySpec)),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT, pUnsealKey, LWMSG_TYPESPEC(gNtlmRC4KeySpec)),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT, pdwSendMsgSeq, LWMSG_UINT32(DWORD)),
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT, pdwRecvMsgSeq, LWMSG_UINT32(DWORD)),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT, MappedToGuest),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

typedef struct _NTLM_CONTEXT_EXPORT
{
    PNTLM_CONTEXT pContext;
    DWORD UserInfoLength;
    PBYTE pUserInfo;
} NTLM_CONTEXT_EXPORT, *PNTLM_CONTEXT_EXPORT;

static LWMsgTypeSpec gNtlmSecurityContextExportSpec[] =
{
    LWMSG_STRUCT_BEGIN(NTLM_CONTEXT_EXPORT),
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT_EXPORT, pContext, LWMSG_TYPESPEC(gNtlmSecurityContextSpec)),
    LWMSG_MEMBER_UINT32(NTLM_CONTEXT_EXPORT, UserInfoLength),
    LWMSG_MEMBER_POINTER(NTLM_CONTEXT_EXPORT, pUserInfo, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_NOT_NULL,
    LWMSG_ATTR_LENGTH_MEMBER(NTLM_CONTEXT_EXPORT, UserInfoLength),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

DWORD
NtlmServerExportSecurityContext(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    PVOID pBlobUserInfo = NULL;
    PVOID pBlobContext = NULL;
    size_t BlobSize = 0;
    LWMsgContext *context = NULL;
    LWMsgDataContext *pDataContext = NULL;
    NTLM_CONTEXT_EXPORT ContextExport = {0};

    if (pContext == NULL || pContext->NtlmState != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaIPCGetAuthUserInfoSpec(),
                              pContext->pUserInfo,
                              &pBlobUserInfo,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    ContextExport.pContext = pContext;
    ContextExport.UserInfoLength = BlobSize;
    ContextExport.pUserInfo = pBlobUserInfo;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              gNtlmSecurityContextExportSpec,
                              &ContextExport,
                              &pBlobContext,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (dwError)
    {
        pPackedContext->cbBuffer = 0;
        pPackedContext->BufferType = 0;
        pPackedContext->pvBuffer = NULL;
    }
    else
    {
        pPackedContext->cbBuffer = BlobSize;
        pPackedContext->BufferType = SECBUFFER_TOKEN;
        pPackedContext->pvBuffer = pBlobContext;
        pBlobContext = NULL;
    }

    LW_SAFE_FREE_MEMORY(pBlobUserInfo);
    LW_SAFE_FREE_MEMORY(pBlobContext);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (context)
    {
        lwmsg_context_delete(context);
    }

    return dwError;
}

DWORD
NtlmServerImportSecurityContext(
    IN PSecBuffer pPackedContext,
    OUT PNTLM_CONTEXT_HANDLE phContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgContext *context = NULL;
    LWMsgDataContext *pDataContext = NULL;
    PNTLM_CONTEXT_EXPORT pContextExport = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              gNtlmSecurityContextExportSpec,
                              pPackedContext->pvBuffer,
                              pPackedContext->cbBuffer,
                              (PVOID*)&pContextExport));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaIPCGetAuthUserInfoSpec(),
                              pContextExport->pUserInfo,
                              pContextExport->UserInfoLength,
                              (PVOID*)&pContextExport->pContext->pUserInfo));
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (dwError)
    {
        *phContext = NULL;
    }
    else
    {
        *phContext = pContextExport->pContext;
        pContextExport->pContext = NULL;
    }

    if (pContextExport)
    {
        lwmsg_data_free_graph(
            pDataContext,
            gNtlmSecurityContextExportSpec,
            pContextExport);
    }
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (context)
    {
        lwmsg_context_delete(context);
    }

    return dwError;
}

