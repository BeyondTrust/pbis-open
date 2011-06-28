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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        NTLM IPC
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ipc.h"

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecBufferSpec[] =
{
    // DWORD cbBuffer;
    // DWORD BufferType;
    // PVOID pvBuffer;

    LWMSG_STRUCT_BEGIN(SecBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, cbBuffer),
    LWMSG_MEMBER_UINT32(SecBuffer, BufferType),
    LWMSG_MEMBER_POINTER(SecBuffer, pvBuffer, LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SecBuffer, cbBuffer),
    LWMSG_ATTR_ENCODING("hex+ascii"),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecBufferDescSpec[] =
{
    // For now, I don't believe we need this version information
    // DWORD      ulVersion;
    // DWORD      cBuffers;
    // PSecBuffer pBuffers;

    LWMSG_STRUCT_BEGIN(SecBufferDesc),
    //LWMSG_MEMBER_UINT32(SecBufferDesc, ulVersion),
    LWMSG_MEMBER_UINT32(SecBufferDesc, cBuffers),
    LWMSG_MEMBER_POINTER_BEGIN(SecBufferDesc, pBuffers),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(SecBufferDesc, cBuffers),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmLuidSpec[] =
{
    // DWORD LowPart;
    // INT  HighPart;

    LWMSG_STRUCT_BEGIN(LUID),
    LWMSG_MEMBER_UINT32(LUID, LowPart),
    LWMSG_MEMBER_INT32(LUID, HighPart),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecWinntAuthIdSpec[] =
{
    // PCHAR User;
    // DWORD UserLength;
    // PCHAR Domain;
    // DWORD DomainLength;
    // PCHAR Password;
    // DWORD PasswordLength;
    // DWORD Flags;

    LWMSG_STRUCT_BEGIN(SEC_WINNT_AUTH_IDENTITY),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, User, LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, UserLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_POINTER(SEC_WINNT_AUTH_IDENTITY, Domain, LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, DomainLength),

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, PasswordLength),

    LWMSG_MEMBER_POINTER(
        SEC_WINNT_AUTH_IDENTITY,
        Password,
        LWMSG_UINT8(CHAR)
        ),
    LWMSG_ATTR_LENGTH_MEMBER(SEC_WINNT_AUTH_IDENTITY, PasswordLength),
    LWMSG_ATTR_SENSITIVE,

    LWMSG_MEMBER_UINT32(SEC_WINNT_AUTH_IDENTITY, Flags),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecPkgContextNamesSpec[] =
{
    // SEC_CHAR *pUserName;

    LWMSG_STRUCT_BEGIN(SecPkgContext_Names),

    LWMSG_MEMBER_PSTR(SecPkgContext_Names, pUserName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecPkgContextSessionKeySpec[] =
{
    // ULONG SessionKeyLength;
    // PBYTE SessionKey;

    LWMSG_STRUCT_BEGIN(SecPkgContext_SessionKey),

    LWMSG_MEMBER_UINT32(SecPkgContext_SessionKey, SessionKeyLength),

    LWMSG_MEMBER_POINTER(
        SecPkgContext_SessionKey,
        pSessionKey,
        LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SecPkgContext_SessionKey, SessionKeyLength),
    LWMSG_ATTR_SENSITIVE,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gNtlmSecPkgContextPacLogonInfoSpec[] =
{
    // ULONG PacLogonInfoLength;
    // PBYTE PacLogonInfo;

    LWMSG_STRUCT_BEGIN(SecPkgContext_PacLogonInfo),

    LWMSG_MEMBER_UINT32(SecPkgContext_PacLogonInfo, LogonInfoLength),

    LWMSG_MEMBER_POINTER(
        SecPkgContext_PacLogonInfo,
        pLogonInfo,
        LWMSG_UINT8(CHAR)),
    LWMSG_ATTR_LENGTH_MEMBER(SecPkgContext_PacLogonInfo, LogonInfoLength),
    LWMSG_ATTR_ENCODING("hex+ascii"),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


static LWMsgTypeSpec gNtlmSecPkgContextFlagsSpec[] =
{
    // DWORD dwFlags;

    LWMSG_STRUCT_BEGIN(SecPkgContext_Flags),

    LWMSG_MEMBER_UINT32(SecPkgContext_Flags, Flags),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmSecPkgContextMappedToGuestSpec[] =
{
    LWMSG_STRUCT_BEGIN(SecPkgContext_MappedToGuest),

    LWMSG_MEMBER_UINT8(SecPkgContext_MappedToGuest, MappedToGuest),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecPkgContextSizesSpec[] =
{
    // DWORD cbMaxToken;
    // DWORD cbMaxSignature;
    // DWORD cbBlockSize;
    // DWORD cbSecurityTrailer;

    LWMSG_STRUCT_BEGIN(SecPkgContext_Sizes),

    LWMSG_MEMBER_UINT32(SecPkgContext_Sizes, cbMaxToken),

    LWMSG_MEMBER_UINT32(SecPkgContext_Sizes, cbMaxSignature),

    LWMSG_MEMBER_UINT32(SecPkgContext_Sizes, cbBlockSize),

    LWMSG_MEMBER_UINT32(SecPkgContext_Sizes, cbSecurityTrailer),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecPkgCredNamesSpec[] =
{
    LWMSG_STRUCT_BEGIN(SecPkgCred_Names),

    LWMSG_MEMBER_PSTR(SecPkgCred_Names, pUserName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSecPkgCredDomainNameSpec[] =
{
    LWMSG_STRUCT_BEGIN(SecPkgCred_DomainName),

    LWMSG_MEMBER_PSTR(SecPkgCred_DomainName, pName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmIpcErrorSpec[] =
{
    // DWORD dwError;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ERROR),
    LWMSG_MEMBER_UINT32(NTLM_IPC_ERROR, dwError),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmAcceptSecCtxtSpec[] =
{
    // NTLM_CRED_HANDLE hCredential;
    // NTLM_CONTEXT_HANDLE hContext;
    // PSecBufferDesc pInput;
    // DWORD fContextReq;
    // DWORD TargetDataRep;
    // PSecBufferDesc pOutput;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, hCredential, NTLM_CRED_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_HANDLE(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcceptSecCtxtRespSpec[] =
{
    //NTLM_CONTEXT_HANDLE hNewContext;
    //SecBufferDesc Output;
    //DWORD  fContextAttr;
    //TimeStamp tsTimeStamp;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_HANDLE(
        NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE,
        hNewContext,
        NTLM_CONTEXT_HANDLE
        ),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE,
        Output,
        gNtlmSecBufferSpec
        ),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, fContextAttr),

    LWMSG_MEMBER_UINT64(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, tsTimeStamp),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE, dwStatus),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmAcquireCredsSpec[] =
{
    // SEC_CHAR *pszPrincipal;
    // SEC_CHAR *pszPackage;
    // DWORD fCredentialUse;
    // PLUID pvLogonID;
    // PVOID pAuthData;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPrincipal),

    LWMSG_MEMBER_PSTR(NTLM_IPC_ACQUIRE_CREDS_REQ, pszPackage),

    LWMSG_MEMBER_UINT32(NTLM_IPC_ACQUIRE_CREDS_REQ, fCredentialUse),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pvLogonID),
    LWMSG_TYPESPEC(gNtlmLuidSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ACQUIRE_CREDS_REQ, pAuthData),
    LWMSG_TYPESPEC(gNtlmSecWinntAuthIdSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmAcquireCredsRespSpec[] =
{
    //NTLM_CRED_HANDLE hCredential;
    //TimeStamp tsExpiry;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ACQUIRE_CREDS_RESPONSE),

    LWMSG_MEMBER_HANDLE(
        NTLM_IPC_ACQUIRE_CREDS_RESPONSE,
        hCredential,
        NTLM_CRED_HANDLE
        ),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_MEMBER_UINT64(NTLM_IPC_ACQUIRE_CREDS_RESPONSE, tsExpiry),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmDecryptMsgSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_DECRYPT_MSG_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_DECRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_DECRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmDecryptMsgRespSpec[] =
{
    //SecBufferDesc pMessage;
    //BOOLEAN pbEncrypted;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DECRYPT_MSG_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_DECRYPT_MSG_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),
    LWMSG_ATTR_SENSITIVE,

    LWMSG_MEMBER_UINT32(NTLM_IPC_DECRYPT_MSG_RESPONSE, bEncrypted),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmDeleteSecCtxtSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext

    LWMSG_STRUCT_BEGIN(NTLM_IPC_DELETE_SEC_CTXT_REQ),

	LWMSG_MEMBER_HANDLE(NTLM_IPC_DELETE_SEC_CTXT_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};
    // No Response

/******************************************************************************/

static LWMsgTypeSpec gNtlmEncryptMsgSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // BOOLEAN bEncrypt;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_ENCRYPT_MSG_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, bEncrypt),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_ENCRYPT_MSG_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_SENSITIVE,

    LWMSG_MEMBER_UINT32(NTLM_IPC_ENCRYPT_MSG_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmEncryptMsgRespSpec[] =
{
    //SecBufferDesc Message;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_ENCRYPT_MSG_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_ENCRYPT_MSG_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmExportSecCtxtSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // ULONG fFlags;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_EXPORT_SEC_CTXT_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_EXPORT_SEC_CTXT_REQ, fFlags),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmExportSecCtxtRespSpec[] =
{
    //SecBuffer PackedContext;
    //HANDLE hToken;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE,
        PackedContext,
        gNtlmSecBufferSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmFreeCredsSpec[] =
{
    // NTLM_CRED_HANDLE hCredential;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_FREE_CREDS_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_FREE_CREDS_REQ, hCredential, NTLM_CRED_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

    // No Response

/******************************************************************************/

static LWMsgTypeSpec gNtlmImportSecCtxtSpec[] =
{
    // PSecBuffer pPackedContext;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_REQ, pPackedContext),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmImportSecCtxtRespSpec[] =
{
    //NTLM_CONTEXT_HANDLE hContext;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_HANDLE(
        NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE,
        hContext,
        NTLM_CONTEXT_HANDLE
        ),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmInitSecCtxtSpec[] =
{
    //PNTLM_CRED_HANDLE phCredential;
    //PNTLM_CONTEXT_HANDLE phContext;
    //SEC_CHAR * pszTargetName;
    //ULONG fContextReq;
    //ULONG Reserverd1;
    //ULONG TargetDataRep;
    //PSecBufferDesc pInput;
    //ULONG Reserved2;
    //PSecBufferDesc pOutput;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_INIT_SEC_CTXT_REQ, hCredential, NTLM_CRED_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_HANDLE(NTLM_IPC_INIT_SEC_CTXT_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PSTR(NTLM_IPC_INIT_SEC_CTXT_REQ, pszTargetName),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, fContextReq),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved1),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, TargetDataRep),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_INIT_SEC_CTXT_REQ, pInput),
    LWMSG_TYPESPEC(gNtlmSecBufferSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_REQ, Reserved2),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmInitSecCtxtRespSpec[] =
{
    //NTLM_CONTEXT_HANDLE hNewContext;
    //SecBufferDesc Output;
    //DWORD fContextAttr;
    //TimeStamp tsExpiry;
    //DWORD dwStatus;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_INIT_SEC_CTXT_RESPONSE),

    LWMSG_MEMBER_HANDLE(
        NTLM_IPC_INIT_SEC_CTXT_RESPONSE,
        hNewContext,
        NTLM_CONTEXT_HANDLE
        ),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_INIT_SEC_CTXT_RESPONSE,
        Output,
        gNtlmSecBufferSpec
        ),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_RESPONSE, fContextAttr),

    LWMSG_MEMBER_UINT64(NTLM_IPC_INIT_SEC_CTXT_RESPONSE, tsExpiry),

    LWMSG_MEMBER_UINT32(NTLM_IPC_INIT_SEC_CTXT_RESPONSE, dwStatus),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmMakeSignSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // DWORD dwQop;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_MAKE_SIGN_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_MAKE_SIGN_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, dwQop),

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_MAKE_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_MAKE_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmMakeSignRespSpec[] =
{
    //SecBufferDesc Message;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_MAKE_SIGN_RESPONSE),

    LWMSG_MEMBER_TYPESPEC(
        NTLM_IPC_MAKE_SIGN_RESPONSE,
        Message,
        gNtlmSecBufferDescSpec
        ),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmQueryCredsSpec[] =
{
    // PNTLM_CRED_HANDLE phCredential;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CREDS_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_QUERY_CREDS_REQ, hCredential, NTLM_CRED_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CREDS_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCredsRespSpec[] =
{
    //DWORD ulAttribute;
    //SecPkgCred Buffer;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CREDS_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CREDS_RESPONSE, ulAttribute),

    LWMSG_MEMBER_UNION_BEGIN(NTLM_IPC_QUERY_CREDS_RESPONSE, Buffer),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgCred, pNames),
    LWMSG_TYPESPEC(gNtlmSecPkgCredNamesSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_CRED_ATTR_NAMES),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgCred, pDomainName),
    LWMSG_TYPESPEC(gNtlmSecPkgCredDomainNameSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_CRED_ATTR_DOMAIN_NAME),

    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(NTLM_IPC_QUERY_CREDS_RESPONSE, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmQueryCtxtSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // ULONG ulAttribute;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CTXT_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_QUERY_CTXT_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CTXT_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmQueryCtxtRespSpec[] =
{
    // ULONG ulAttribute;
    // SecPkgContext Buffer;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_QUERY_CTXT_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_QUERY_CTXT_RESPONSE, ulAttribute),

    LWMSG_MEMBER_UNION_BEGIN(NTLM_IPC_QUERY_CTXT_RESPONSE, Buffer),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pNames),
    LWMSG_TYPESPEC(gNtlmSecPkgContextNamesSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_NAMES),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pSessionKey),
    LWMSG_TYPESPEC(gNtlmSecPkgContextSessionKeySpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_SESSION_KEY),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pSizes),
    LWMSG_TYPESPEC(gNtlmSecPkgContextSizesSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_SIZES),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pLogonInfo),
    LWMSG_TYPESPEC(gNtlmSecPkgContextPacLogonInfoSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_PAC_LOGON_INFO),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pFlags),
    LWMSG_TYPESPEC(gNtlmSecPkgContextFlagsSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_FLAGS),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgContext, pMappedToGuest),
    LWMSG_TYPESPEC(gNtlmSecPkgContextMappedToGuestSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_ATTR_CUSTOM_MAPPED_TO_GUEST),

    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(NTLM_IPC_QUERY_CTXT_RESPONSE, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gNtlmSetCredsSpec[] =
{
    // PNTLM_CRED_HANDLE phCredential;
    // DWORD ulAttribute;
    // SecPkgCred Buffer;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_SET_CREDS_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_SET_CREDS_REQ, hCredential, NTLM_CRED_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(NTLM_IPC_SET_CREDS_REQ, ulAttribute),

    LWMSG_MEMBER_UNION_BEGIN(NTLM_IPC_SET_CREDS_REQ, Buffer),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgCred, pNames),
    LWMSG_TYPESPEC(gNtlmSecPkgCredNamesSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_CRED_ATTR_NAMES),

    LWMSG_MEMBER_POINTER_BEGIN(SecPkgCred, pDomainName),
    LWMSG_TYPESPEC(gNtlmSecPkgCredDomainNameSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_TAG(SECPKG_CRED_ATTR_DOMAIN_NAME),

    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(NTLM_IPC_SET_CREDS_REQ, ulAttribute),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

    // No Response

/******************************************************************************/

static LWMsgTypeSpec gNtlmVerifySignSpec[] =
{
    // PNTLM_CONTEXT_HANDLE phContext;
    // PSecBufferDesc pMessage;
    // ULONG MessageSeqNo;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ),

    LWMSG_MEMBER_HANDLE(NTLM_IPC_VERIFY_SIGN_REQ, hContext, NTLM_CONTEXT_HANDLE),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_POINTER_BEGIN(NTLM_IPC_VERIFY_SIGN_REQ, pMessage),
    LWMSG_TYPESPEC(gNtlmSecBufferDescSpec),
    LWMSG_POINTER_END,

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_REQ, MessageSeqNo),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gNtlmVerifySignRespSpec[] =
{
    //DWORD dwQop;

    LWMSG_STRUCT_BEGIN(NTLM_IPC_VERIFY_SIGN_RESPONSE),

    LWMSG_MEMBER_UINT32(NTLM_IPC_VERIFY_SIGN_RESPONSE, dwQop),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgProtocolSpec gNtlmIpcSpec[] =
{
    LWMSG_MESSAGE(NTLM_R_GENERIC_FAILURE, gNtlmIpcErrorSpec),

    LWMSG_MESSAGE(NTLM_Q_ACCEPT_SEC_CTXT, gNtlmAcceptSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_ACCEPT_SEC_CTXT_SUCCESS, gNtlmAcceptSecCtxtRespSpec),

    LWMSG_MESSAGE(NTLM_Q_ACQUIRE_CREDS, gNtlmAcquireCredsSpec),
    LWMSG_MESSAGE(NTLM_R_ACQUIRE_CREDS_SUCCESS, gNtlmAcquireCredsRespSpec),

    LWMSG_MESSAGE(NTLM_Q_DECRYPT_MSG, gNtlmDecryptMsgSpec),
    LWMSG_MESSAGE(NTLM_R_DECRYPT_MSG_SUCCESS, gNtlmDecryptMsgRespSpec),

    LWMSG_MESSAGE(NTLM_Q_DELETE_SEC_CTXT, gNtlmDeleteSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_DELETE_SEC_CTXT_SUCCESS, NULL),

    LWMSG_MESSAGE(NTLM_Q_ENCRYPT_MSG, gNtlmEncryptMsgSpec),
    LWMSG_MESSAGE(NTLM_R_ENCRYPT_MSG_SUCCESS, gNtlmEncryptMsgRespSpec),

    LWMSG_MESSAGE(NTLM_Q_EXPORT_SEC_CTXT, gNtlmExportSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_EXPORT_SEC_CTXT_SUCCESS, gNtlmExportSecCtxtRespSpec),

    LWMSG_MESSAGE(NTLM_Q_FREE_CREDS, gNtlmFreeCredsSpec),
    LWMSG_MESSAGE(NTLM_R_FREE_CREDS_SUCCESS, NULL),

    LWMSG_MESSAGE(NTLM_Q_IMPORT_SEC_CTXT, gNtlmImportSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_IMPORT_SEC_CTXT_SUCCESS, gNtlmImportSecCtxtRespSpec),

    LWMSG_MESSAGE(NTLM_Q_INIT_SEC_CTXT, gNtlmInitSecCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_INIT_SEC_CTXT_SUCCESS, gNtlmInitSecCtxtRespSpec),

    LWMSG_MESSAGE(NTLM_Q_MAKE_SIGN, gNtlmMakeSignSpec),
    LWMSG_MESSAGE(NTLM_R_MAKE_SIGN_SUCCESS, gNtlmMakeSignRespSpec),

    LWMSG_MESSAGE(NTLM_Q_QUERY_CREDS, gNtlmQueryCredsSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CREDS_SUCCESS, gNtlmQueryCredsRespSpec),

    LWMSG_MESSAGE(NTLM_Q_QUERY_CTXT, gNtlmQueryCtxtSpec),
    LWMSG_MESSAGE(NTLM_R_QUERY_CTXT_SUCCESS, gNtlmQueryCtxtRespSpec),

    LWMSG_MESSAGE(NTLM_Q_SET_CREDS, gNtlmSetCredsSpec),
    LWMSG_MESSAGE(NTLM_R_SET_CREDS_SUCCESS, NULL),

    LWMSG_MESSAGE(NTLM_Q_VERIFY_SIGN, gNtlmVerifySignSpec),
    LWMSG_MESSAGE(NTLM_R_VERIFY_SIGN_SUCCESS, gNtlmVerifySignRespSpec),

    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
NtlmIpcGetProtocolSpec(
    VOID
    )
{
    return gNtlmIpcSpec;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
