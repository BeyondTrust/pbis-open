/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        client.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        API (Client)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__

DWORD
NtlmTransactAcceptSecurityContext(
    IN PNTLM_CRED_HANDLE phCredential,
    IN OUT PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmTransactAcquireCredentialsHandle(
    IN const SEC_CHAR *pszPrincipal,
    IN const SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmTransactDecryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbEncrypt
    );

DWORD
NtlmTransactDeleteSecurityContext(
    IN OUT PNTLM_CONTEXT_HANDLE phContext
    );

DWORD
NtlmTransactEncryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmTransactExportSecurityContext(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    );

DWORD
NtlmTransactFreeCredentialsHandle(
    IN PNTLM_CRED_HANDLE phCredential
    );

DWORD
NtlmTransactImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PNTLM_CONTEXT_HANDLE phContext
    );

DWORD
NtlmTransactInitializeSecurityContext(
    IN OPTIONAL PNTLM_CRED_HANDLE phCredential,
    IN OPTIONAL PNTLM_CONTEXT_HANDLE phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserverd1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmTransactMakeSignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD dwQop,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmTransactQueryCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactQueryContextAttributes(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmTransactSetCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    IN PVOID pBuffer
    );

DWORD
NtlmTransactVerifySignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PDWORD pQop
    );

DWORD
NtlmTransferSecBufferDesc(
    OUT PSecBufferDesc pOut,
    IN PSecBufferDesc pIn,
    BOOLEAN bDeepCopy
    );

DWORD
NtlmTransferSecBufferToDesc(
    OUT PSecBufferDesc pOut,
    IN PSecBuffer pIn,
    BOOLEAN bDeepCopy
    );

#endif // __PROTOTYPES_H__
