/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (NTLM Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"


static LWMsgPeer* gpClient = NULL;
static LWMsgProtocol* gpProtocol = NULL;
static LWMsgSession* gpSession = NULL;
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

static
DWORD
NtlmInitialize(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    pthread_mutex_lock(&gLock);

    if (!gpProtocol)
    {
        dwError = LwMapLwmsgStatusToLwError(lwmsg_protocol_new(NULL, &gpProtocol));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMapLwmsgStatusToLwError(
            lwmsg_protocol_add_protocol_spec(gpProtocol, NtlmIpcGetProtocolSpec()));
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!gpClient)
    {
        dwError = LwMapLwmsgStatusToLwError(
            lwmsg_peer_new(NULL, gpProtocol, &gpClient));
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LwMapLwmsgStatusToLwError(
            lwmsg_peer_add_connect_endpoint(
                gpClient,
                LWMSG_ENDPOINT_DIRECT,
                "ntlm"));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMapLwmsgStatusToLwError(
            lwmsg_peer_add_connect_endpoint(
                gpClient,
                LWMSG_ENDPOINT_LOCAL,
                CACHEDIR "/" NTLM_SERVER_FILENAME));
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!gpSession)
    {
        dwError = LwMapLwmsgStatusToLwError(
            lwmsg_peer_connect(gpClient, &gpSession));
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    pthread_mutex_unlock(&gLock);

    return dwError;
}

static
DWORD
NtlmIpcAcquireCall(
    LWMsgCall** ppCall
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    dwError = NtlmInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapLwmsgStatusToLwError(
        lwmsg_peer_acquire_call(
            gpClient,
            ppCall));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

VOID
NtlmIpcReleaseHandle(
    LWMsgHandle* pHandle
    )
{
    lwmsg_session_release_handle(gpSession, pHandle);
}

DWORD
NtlmTransactAcceptSecurityContext(
    IN NTLM_CRED_HANDLE hCredential,
    IN NTLM_CONTEXT_HANDLE hContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    OUT PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_ACCEPT_SEC_CTXT_REQ AcceptSecCtxtReq = {0};
    // Do not free pResult and pError
    PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    *pfContextAttr = 0;
    *ptsTimeStamp = 0;

    memset(&AcceptSecCtxtReq, 0, sizeof(AcceptSecCtxtReq));

    AcceptSecCtxtReq.hCredential = (LWMsgHandle*) hCredential;
    AcceptSecCtxtReq.hContext = (LWMsgHandle*) hContext;
    if (pInput->cBuffers != 1)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    AcceptSecCtxtReq.pInput = &pInput->pBuffers[0];
    AcceptSecCtxtReq.fContextReq = fContextReq;
    AcceptSecCtxtReq.TargetDataRep = TargetDataRep;

    In.tag = NTLM_Q_ACCEPT_SEC_CTXT;
    In.data = &AcceptSecCtxtReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_ACCEPT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE)Out.data;

            dwError = NtlmTransferSecBufferToDesc(pOutput,
                    &pResultList->Output,
                    FALSE);
            BAIL_ON_LSA_ERROR(dwError);

            if (phNewContext)
            {
                *phNewContext = (NTLM_CONTEXT_HANDLE) pResultList->hNewContext;
                pResultList->hNewContext = NULL;
            }

            if (hContext)
            {
                NtlmIpcReleaseHandle((LWMsgHandle*) hContext);
            }

            *pfContextAttr = pResultList->fContextAttr;
            *ptsTimeStamp = pResultList->tsTimeStamp;
            dwError = pResultList->dwStatus;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (phNewContext)
    {
        *phNewContext = hContext;
    }
    
    *pfContextAttr = 0;
    *ptsTimeStamp = 0;

    goto cleanup;
}

DWORD
NtlmTransactAcquireCredentialsHandle(
    IN const SEC_CHAR *pszPrincipal,
    IN const SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_ACQUIRE_CREDS_REQ AcquireCredsReq;
    // Do not free pResult and pError
    PNTLM_IPC_ACQUIRE_CREDS_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&AcquireCredsReq, 0, sizeof(AcquireCredsReq));

    AcquireCredsReq.pszPrincipal = pszPrincipal;
    AcquireCredsReq.pszPackage = pszPackage;
    AcquireCredsReq.fCredentialUse = fCredentialUse;
    AcquireCredsReq.pvLogonID = pvLogonID;
    AcquireCredsReq.pAuthData = pAuthData;

    In.tag = NTLM_Q_ACQUIRE_CREDS;
    In.data = &AcquireCredsReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_ACQUIRE_CREDS_SUCCESS:
            pResultList = (PNTLM_IPC_ACQUIRE_CREDS_RESPONSE)Out.data;

            *phCredential = (NTLM_CRED_HANDLE) pResultList->hCredential;
            pResultList->hCredential = NULL;

            *ptsExpiry = pResultList->tsExpiry;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactDecryptMessage(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbEncrypted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_DECRYPT_MSG_REQ DecryptMsgReq;
    // Do not free pResult and pError
    PNTLM_IPC_DECRYPT_MSG_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&DecryptMsgReq, 0, sizeof(DecryptMsgReq));

    DecryptMsgReq.hContext = (LWMsgHandle*) hContext;
    DecryptMsgReq.pMessage = pMessage;
    DecryptMsgReq.MessageSeqNo = MessageSeqNo;

    In.tag = NTLM_Q_DECRYPT_MSG;
    In.data = &DecryptMsgReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_DECRYPT_MSG_SUCCESS:
            pResultList = (PNTLM_IPC_DECRYPT_MSG_RESPONSE)Out.data;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message,
                TRUE
                );
            BAIL_ON_LSA_ERROR(dwError);

            *pbEncrypted = pResultList->bEncrypted;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

typedef struct DELETE_CONTEXT
{
    NTLM_IPC_DELETE_SEC_CTXT_REQ Req;
    LWMsgParams In;
    LWMsgParams Out;
} DELETE_CONTEXT, *PDELETE_CONTEXT;

static
VOID
NtlmDeleteSecurityContextComplete(
    LWMsgCall* pCall,
    LWMsgStatus status,
    PVOID pData
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    PDELETE_CONTEXT pContext = (PDELETE_CONTEXT) pData;

    lwmsg_session_release_handle(pSession, pContext->Req.hContext);
    lwmsg_call_destroy_params(pCall, &pContext->Out);
    lwmsg_call_release(pCall);
    LwFreeMemory(pContext);
}


DWORD
NtlmTransactDeleteSecurityContext(
    IN OUT NTLM_CONTEXT_HANDLE hContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgCall* pCall = NULL;
    PDELETE_CONTEXT pContext = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    dwError = LwAllocateMemory(sizeof(*pContext), OUT_PPVOID(&pContext));
    BAIL_ON_LSA_ERROR(dwError);

    pContext->Req.hContext = (LWMsgHandle*) hContext;
    pContext->In.tag = NTLM_Q_DELETE_SEC_CTXT;
    pContext->In.data = &pContext->Req;
    pContext->Out.tag = LWMSG_TAG_INVALID;
    pContext->Out.data = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    status = lwmsg_call_dispatch(
            pCall,
            &pContext->In,
            &pContext->Out,
            NtlmDeleteSecurityContextComplete, pContext);
    switch(status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
        break;
    default:
        dwError = MAP_LWMSG_ERROR(status);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (status != LWMSG_STATUS_PENDING)
    {
        if (pCall)
        {
            if (pContext)
            {
                lwmsg_call_destroy_params(pCall, &pContext->Out);
            }
            lwmsg_call_release(pCall);
        }

        if (pContext)
        {
            LwFreeMemory(pContext);
        }

        NtlmIpcReleaseHandle((LWMsgHandle*) hContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NtlmTransactEncryptMessage(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_ENCRYPT_MSG_REQ EncryptMsgReq;
    // Do not free pResult and pError
    PNTLM_IPC_ENCRYPT_MSG_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&EncryptMsgReq, 0, sizeof(EncryptMsgReq));

    EncryptMsgReq.hContext = (LWMsgHandle*) hContext;
    EncryptMsgReq.bEncrypt = bEncrypt;
    EncryptMsgReq.pMessage = pMessage;
    EncryptMsgReq.MessageSeqNo = MessageSeqNo;

    In.tag = NTLM_Q_ENCRYPT_MSG;
    In.data = &EncryptMsgReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_ENCRYPT_MSG_SUCCESS:
            pResultList = (PNTLM_IPC_ENCRYPT_MSG_RESPONSE)Out.data;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message,
                TRUE
                );
            BAIL_ON_LSA_ERROR(dwError);

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactExportSecurityContext(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_EXPORT_SEC_CTXT_REQ ExportSecCtxtReq;
    // Do not free pResult and pError
    PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&ExportSecCtxtReq, 0, sizeof(ExportSecCtxtReq));

    ExportSecCtxtReq.hContext = (LWMsgHandle*) hContext;
    ExportSecCtxtReq.fFlags = fFlags;

    In.tag = NTLM_Q_EXPORT_SEC_CTXT;
    In.data = &ExportSecCtxtReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_EXPORT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE)Out.data;

            pPackedContext->cbBuffer = pResultList->PackedContext.cbBuffer;
            pPackedContext->BufferType = pResultList->PackedContext.BufferType;
            pPackedContext->pvBuffer = pResultList->PackedContext.pvBuffer;
            pResultList->PackedContext.pvBuffer = NULL;

            if (fFlags & SECPKG_CONTEXT_EXPORT_DELETE_OLD)
            {
                NtlmIpcReleaseHandle((LWMsgHandle*) hContext);
                hContext = NULL;
            }

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactFreeCredentialsHandle(
    IN OUT NTLM_CRED_HANDLE hCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_FREE_CREDS_REQ FreeCredsReq;
    // Do not free pError
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&FreeCredsReq, 0, sizeof(FreeCredsReq));

    FreeCredsReq.hCredential = (LWMsgHandle*) hCredential;

    In.tag = NTLM_Q_FREE_CREDS;
    In.data = &FreeCredsReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_FREE_CREDS_SUCCESS:
            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    NtlmIpcReleaseHandle((LWMsgHandle*) hCredential);

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactImportSecurityContext(
    IN PSecBuffer pPackedContext,
    OUT PNTLM_CONTEXT_HANDLE phContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_IMPORT_SEC_CTXT_REQ ImportSecCtxtReq;
    // Do not free pResult and pError
    PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&ImportSecCtxtReq, 0, sizeof(ImportSecCtxtReq));

    ImportSecCtxtReq.pPackedContext = pPackedContext;

    In.tag = NTLM_Q_IMPORT_SEC_CTXT;
    In.data = &ImportSecCtxtReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_IMPORT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE)Out.data;

            *phContext = (NTLM_CONTEXT_HANDLE) pResultList->hContext;
            pResultList->hContext = NULL;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NtlmTransactInitializeSecurityContext(
    IN OPTIONAL NTLM_CRED_HANDLE hCredential,
    IN OPTIONAL NTLM_CONTEXT_HANDLE hContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_INIT_SEC_CTXT_REQ InitSecCtxtReq;
    // Do not free pResult and pError
    PNTLM_IPC_INIT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&InitSecCtxtReq, 0, sizeof(InitSecCtxtReq));


    InitSecCtxtReq.hCredential = (LWMsgHandle*) hCredential;
    InitSecCtxtReq.hContext = (LWMsgHandle*) hContext;
    InitSecCtxtReq.pszTargetName = pszTargetName;
    InitSecCtxtReq.fContextReq = fContextReq;
    InitSecCtxtReq.Reserved1 = Reserved1;
    InitSecCtxtReq.TargetDataRep = TargetDataRep;
    if (pInput->cBuffers != 1)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    InitSecCtxtReq.pInput = &pInput->pBuffers[0];
    InitSecCtxtReq.Reserved2 = Reserved2;

    In.tag = NTLM_Q_INIT_SEC_CTXT;
    In.data = &InitSecCtxtReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_INIT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_INIT_SEC_CTXT_RESPONSE)Out.data;

            if (pOutput)
            {

                dwError = NtlmTransferSecBufferToDesc(
                    pOutput,
                    &pResultList->Output,
                    FALSE
                    );
                BAIL_ON_LSA_ERROR(dwError);

            }

            if (phNewContext)
            {
                *phNewContext = (NTLM_CONTEXT_HANDLE) pResultList->hNewContext;
                pResultList->hNewContext = NULL;
            }

            if (hContext)
            {
                NtlmIpcReleaseHandle((LWMsgHandle*) hContext);
                hContext = NULL;
            }

            *pfContextAttr = pResultList->fContextAttr;

            if (ptsExpiry)
            {
               *ptsExpiry = pResultList->tsExpiry;
            }

            dwError = pResultList->dwStatus;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (phNewContext)
    {
        *phNewContext = hContext;
    }
    
    *pfContextAttr = 0;
    *ptsExpiry = 0;
    memset(pOutput, 0, sizeof(SecBufferDesc));

    goto cleanup;
}

DWORD
NtlmTransactMakeSignature(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN DWORD dwQop,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_MAKE_SIGN_REQ MakeSignReq;
    // Do not free pResult and pError
    PNTLM_IPC_MAKE_SIGN_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&MakeSignReq, 0, sizeof(MakeSignReq));

    MakeSignReq.hContext = (LWMsgHandle*) hContext;
    MakeSignReq.dwQop = dwQop;
    MakeSignReq.pMessage = pMessage;
    MakeSignReq.MessageSeqNo = MessageSeqNo;

    In.tag = NTLM_Q_MAKE_SIGN;
    In.data = &MakeSignReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_MAKE_SIGN_SUCCESS:
            pResultList = (PNTLM_IPC_MAKE_SIGN_RESPONSE)Out.data;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message,
                TRUE
                );
            BAIL_ON_LSA_ERROR(dwError);

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactQueryContextAttributes(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_QUERY_CTXT_REQ QueryCtxtReq;
    // Do not free pResult and pError
    PNTLM_IPC_QUERY_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&QueryCtxtReq, 0, sizeof(QueryCtxtReq));

    QueryCtxtReq.hContext = (LWMsgHandle*) hContext;
    QueryCtxtReq.ulAttribute = ulAttribute;

    In.tag = NTLM_Q_QUERY_CTXT;
    In.data = &QueryCtxtReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_QUERY_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_QUERY_CTXT_RESPONSE)Out.data;

            switch(pResultList->ulAttribute)
            {
            case SECPKG_ATTR_NAMES:
                ((PSecPkgContext_Names)pBuffer)->pUserName =
                    pResultList->Buffer.pNames->pUserName;
                pResultList->Buffer.pNames->pUserName = NULL;
                break;
            case SECPKG_ATTR_SESSION_KEY:
                ((PSecPkgContext_SessionKey)pBuffer)->pSessionKey =
                    pResultList->Buffer.pSessionKey->pSessionKey;
                ((PSecPkgContext_SessionKey)pBuffer)->SessionKeyLength =
                    pResultList->Buffer.pSessionKey->SessionKeyLength;
                pResultList->Buffer.pSessionKey->pSessionKey = NULL;
                break;
            case SECPKG_ATTR_SIZES:
                ((PSecPkgContext_Sizes)pBuffer)->cbMaxToken =
                    pResultList->Buffer.pSizes->cbMaxToken;
                ((PSecPkgContext_Sizes)pBuffer)->cbMaxSignature =
                    pResultList->Buffer.pSizes->cbMaxSignature;
                ((PSecPkgContext_Sizes)pBuffer)->cbBlockSize =
                    pResultList->Buffer.pSizes->cbBlockSize;
                ((PSecPkgContext_Sizes)pBuffer)->cbSecurityTrailer =
                    pResultList->Buffer.pSizes->cbSecurityTrailer;
                break;
            case SECPKG_ATTR_PAC_LOGON_INFO:
                ((PSecPkgContext_PacLogonInfo)pBuffer)->pLogonInfo =
                    pResultList->Buffer.pLogonInfo->pLogonInfo;
                ((PSecPkgContext_PacLogonInfo)pBuffer)->LogonInfoLength =
                    pResultList->Buffer.pLogonInfo->LogonInfoLength;
                pResultList->Buffer.pLogonInfo->pLogonInfo = NULL;
                break;
            case SECPKG_ATTR_FLAGS:
                ((PSecPkgContext_Flags)pBuffer)->Flags =
                    pResultList->Buffer.pFlags->Flags;
                break;
            case SECPKG_ATTR_CUSTOM_MAPPED_TO_GUEST:
                ((PSecPkgContext_MappedToGuest)pBuffer)->MappedToGuest =
                    pResultList->Buffer.pMappedToGuest->MappedToGuest;
                break;
            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
                break;
            }

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NtlmTransactQueryCredentialsAttributes(
    IN NTLM_CRED_HANDLE hCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_QUERY_CREDS_REQ QueryCredsReq;
    // Do not free pResult and pError
    PNTLM_IPC_QUERY_CREDS_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&QueryCredsReq, 0, sizeof(QueryCredsReq));

    QueryCredsReq.hCredential = (LWMsgHandle*) hCredential;
    QueryCredsReq.ulAttribute = ulAttribute;

    In.tag = NTLM_Q_QUERY_CREDS;
    In.data = &QueryCredsReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_QUERY_CREDS_SUCCESS:
            pResultList = (PNTLM_IPC_QUERY_CREDS_RESPONSE)Out.data;

            switch(pResultList->ulAttribute)
            {
            case SECPKG_CRED_ATTR_NAMES:
                ((PSecPkgCred_Names)pBuffer)->pUserName =
                    pResultList->Buffer.pNames->pUserName;
                pResultList->Buffer.pNames->pUserName = NULL;
                break;
            default:
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactSetCredentialsAttributes(
    IN NTLM_CRED_HANDLE hCredential,
    IN DWORD ulAttribute,
    IN PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_SET_CREDS_REQ SetCredsReq;
    // Do not free pResult and pError
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&SetCredsReq, 0, sizeof(SetCredsReq));

    SetCredsReq.hCredential = (LWMsgHandle*) hCredential;
    SetCredsReq.ulAttribute = ulAttribute;
    switch (ulAttribute)
    {
        case SECPKG_CRED_ATTR_DOMAIN_NAME:
            SetCredsReq.Buffer.pDomainName = (PSecPkgCred_DomainName)pBuffer;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    In.tag = NTLM_Q_SET_CREDS;
    In.data = &SetCredsReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_SET_CREDS_SUCCESS:
            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransactVerifySignature(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PDWORD pQop
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_IPC_VERIFY_SIGN_REQ VerifySignReq;
    // Do not free pResult and pError
    PNTLM_IPC_VERIFY_SIGN_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    LWMsgParams In= LWMSG_PARAMS_INITIALIZER;
    LWMsgParams Out= LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    NtlmInitialize();

    dwError = NtlmIpcAcquireCall(&pCall);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&VerifySignReq, 0, sizeof(VerifySignReq));

    VerifySignReq.hContext = (LWMsgHandle*) hContext;
    VerifySignReq.pMessage = pMessage;
    VerifySignReq.MessageSeqNo = MessageSeqNo;

    In.tag = NTLM_Q_VERIFY_SIGN;
    In.data = &VerifySignReq;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_call_dispatch(pCall, &In, &Out, NULL, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    switch (Out.tag)
    {
        case NTLM_R_VERIFY_SIGN_SUCCESS:
            pResultList = (PNTLM_IPC_VERIFY_SIGN_RESPONSE)Out.data;

            *pQop = pResultList->dwQop;

            break;
        case NTLM_R_GENERIC_FAILURE:
            pError = (PNTLM_IPC_ERROR) Out.data;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &Out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmTransferSecBufferDesc(
    OUT PSecBufferDesc pOut,
    IN PSecBufferDesc pIn,
    BOOLEAN bDeepCopy
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD nIndex = 0;

    if (pOut->cBuffers != pIn->cBuffers)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (nIndex= 0; nIndex < pIn->cBuffers; nIndex++)
    {
        if (bDeepCopy)
        {
            if (pOut->pBuffers[nIndex].cbBuffer !=
                    pIn->pBuffers[nIndex].cbBuffer)
            {
                dwError = ERROR_INCORRECT_SIZE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            memcpy(pOut->pBuffers[nIndex].pvBuffer,
                    pIn->pBuffers[nIndex].pvBuffer,
                    pIn->pBuffers[nIndex].cbBuffer);
        }
        else
        {
            pOut->pBuffers[nIndex].pvBuffer = pIn->pBuffers[nIndex].pvBuffer;
            pIn->pBuffers[nIndex].pvBuffer = NULL;

            pOut->pBuffers[nIndex].cbBuffer = pIn->pBuffers[nIndex].cbBuffer;
            pIn->pBuffers[nIndex].cbBuffer = 0;
        }
        pOut->pBuffers[nIndex].BufferType = pIn->pBuffers[nIndex].BufferType;
    }

cleanup:
    return dwError;
error:
    goto cleanup;

}

DWORD
NtlmTransferSecBufferToDesc(
    OUT PSecBufferDesc pOut,
    IN PSecBuffer pIn,
    BOOLEAN bDeepCopy
    )
{
    SecBufferDesc desc;

    if (pOut->cBuffers != 1)
    {
        return LW_ERROR_INVALID_PARAMETER;
    }
    desc.cBuffers = 1;
    desc.pBuffers = pIn;

    return NtlmTransferSecBufferDesc(
                pOut,
                &desc,
                bDeepCopy);
}

VOID
NtlmClientIpcShutdown()
{
    if (gpProtocol)
    {
        lwmsg_protocol_delete(gpProtocol);
        gpProtocol = 0;
    }
    if (gpClient)
    {
        lwmsg_peer_delete(gpClient);
        gpClient = NULL;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
