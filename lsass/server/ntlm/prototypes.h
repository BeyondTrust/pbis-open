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
 *        prototypes.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __NTLM_PROTOTYPES_H__
#define __NTLM_PROTOTYPES_H__

DWORD
NtlmServerAcceptSecurityContext(
    IN HANDLE Handle,
    IN NTLM_CRED_HANDLE hCred,
    IN OUT PNTLM_CONTEXT_HANDLE phContext,
    IN const SecBuffer* pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PNTLM_CONTEXT_HANDLE phNewContext,
    OUT PSecBuffer pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    );

DWORD
NtlmServerAcquireCredentialsHandle(
    IN LWMsgCall* pCall,
    IN const SEC_CHAR* pszPrincipal,
    IN const SEC_CHAR* pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    );

DWORD
NtlmGetNameInformation(
    IN OPTIONAL PCSTR pszJoinedDomain,
    OUT PSTR* ppszServerName,
    OUT PSTR* ppszDomainName,
    OUT PSTR* ppszDnsServerName,
    OUT PSTR* ppszDnsDomainName
    );

DWORD
NtlmServerDecryptMessage(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbEncrypted
    );

DWORD
NtlmServerEncryptMessage(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmInitializeSignature(
    PNTLM_CONTEXT pContext,
    const PSecBufferDesc pMessage,
    PNTLM_SIGNATURE pSignature
    );

VOID
NtlmFinalizeSignature(
    PNTLM_CONTEXT pContext,
    PNTLM_SIGNATURE pSignature
    );

DWORD
NtlmCrc32(
    const SecBufferDesc* pMessage,
    PDWORD pdwCrc32
    );

DWORD
NtlmServerExportSecurityContext(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext
    );

DWORD
NtlmServerFreeCredentialsHandle(
    IN PNTLM_CRED_HANDLE phCredential
    );

DWORD
NtlmServerImportSecurityContext(
    IN PSecBuffer pPackedContext,
    OUT PNTLM_CONTEXT_HANDLE phContext
    );

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL NTLM_CRED_HANDLE hCredential,
    IN OPTIONAL const NTLM_CONTEXT_HANDLE hContext,
    IN OPTIONAL SEC_CHAR* pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL const SecBuffer* pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    OUT PSecBuffer pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    );

DWORD
NtlmServerMakeSignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD dwQop,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    );

DWORD
NtlmServerQueryCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerQueryContextAttributes(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    );

DWORD
NtlmServerSetCredentialsAttributes(
    IN PNTLM_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    IN PSecPkgCred pCred
    );

DWORD
NtlmServerVerifySignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN const SecBufferDesc* pMessage,
    IN DWORD MessageSeqNo,
    OUT PDWORD pQop
    );

DWORD
NtlmVerifySignature(
    IN PNTLM_CONTEXT pContext,
    IN const SecBufferDesc* pMessage,
    IN const SecBuffer* pToken
    );

DWORD
NtlmServerQueryCtxtNameAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Names* ppBuffer
    );

DWORD
NtlmServerQueryCtxtSessionKeyAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_SessionKey* ppBuffer
    );

DWORD
NtlmServerQueryCtxtSizeAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Sizes* ppBuffer
    );

DWORD
NtlmServerQueryCredNameAttribute(
    IN PNTLM_CRED_HANDLE phCred,
    OUT PSecPkgCred_Names *ppNames
    );

DWORD
NtlmServerSetCredDomainNameAttribute(
    IN NTLM_CRED_HANDLE hCred,
    IN PSecPkgCred_DomainName pDomainName
    );

VOID
NtlmReleaseContext(
    IN PNTLM_CONTEXT_HANDLE phContext
    );

VOID
NtlmGetContextInfo(
    IN NTLM_CONTEXT_HANDLE ContextHandle,
    OUT OPTIONAL PNTLM_STATE pNtlmState,
    OUT OPTIONAL PDWORD pNegotiatedFlags,
    OUT OPTIONAL PBYTE* ppSessionKey,
    OUT OPTIONAL PNTLM_CRED_HANDLE pCredHandle,
    OUT OPTIONAL PBOOLEAN pMappedToGuest
    );

DWORD
NtlmCreateContext(
    IN NTLM_CRED_HANDLE hCred,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmInsertContext(
    PNTLM_CONTEXT pNtlmContext
    );

DWORD
NtlmRemoveContext(
    IN PNTLM_CONTEXT_HANDLE pCtxtHandle
    );

VOID
NtlmRemoveAllContext(
    VOID
    );

DWORD
NtlmFindContext(
    IN PNTLM_CONTEXT_HANDLE pCtxtHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

VOID
NtlmFreeContext(
    PNTLM_CONTEXT* ppNtlmContext
    );

VOID
NtlmReleaseCredential(
    IN PNTLM_CRED_HANDLE phCred
    );

DWORD
NtlmCreateCredential(
    IN PLSA_CRED_HANDLE pLsaCredHandle,
    IN DWORD dwDirection,
    OUT PNTLM_CREDENTIALS* ppNtlmCreds
    );

VOID
NtlmGetCredentialInfo(
    IN NTLM_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL uid_t* pUid
    );

VOID
NtlmReferenceCredential(
    IN NTLM_CRED_HANDLE hCredential
    );

VOID
NtlmFreeCredential(
    IN PNTLM_CREDENTIALS pCreds
    );

DWORD
NtlmGetMessageFromSecBufferDesc(
    IN const SecBufferDesc* pSecBufferDesc,
    OUT PDWORD pdwMessageSize,
    OUT const VOID** ppMessage
    );

DWORD
NtlmGetRandomBuffer(
    PBYTE pBuffer,
    DWORD dwLen
    );

DWORD
NtlmCreateNegotiateMessage(
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_NEGOTIATE_MESSAGE_V1* ppNegMsg
    );

DWORD
NtlmAddTargetInfoBuffer(
    IN SHORT InfoType,
    IN PWSTR pInput,
    IN OUT PBYTE* ppBufferPos
    );

DWORD
NtlmCopyStringToSecBuffer(
    IN PVOID pInput,
    IN DWORD Length,
    IN PBYTE pBufferStart,
    IN OUT PBYTE* ppBufferPos,
    OUT PNTLM_SEC_BUFFER pSec
    );

DWORD
NtlmGetCStringFromUnicodeBuffer(
    IN PBYTE pBuffer,
    IN DWORD Length,
    OUT PSTR *ppString
    );

DWORD
NtlmCreateChallengeMessage(
    IN const NTLM_NEGOTIATE_MESSAGE_V1* pNegMsg,
    IN PCSTR pServerName,
    IN PCSTR pDomainName,
    IN PCSTR pDnsHostName,
    IN PCSTR pDnsDomainName,
    IN PBYTE pOsVersion,
    IN BYTE Challenge[NTLM_CHALLENGE_SIZE],
    OUT PDWORD pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE *ppChlngMsg
    );

DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pDomainName,
    IN PCSTR pPassword,
    IN PBYTE pOsVersion,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PDWORD pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE_V1 *ppRespMsg,
    OUT PBYTE pLmUserSessionKey,
    OUT PBYTE pNtlmUserSessionKey
    );

VOID
NtlmStoreSecondaryKey(
    IN PBYTE pMasterKey,
    IN PBYTE pSecondaryKey,
    IN OUT PNTLM_RESPONSE_MESSAGE_V1 pMessage
    );

VOID
NtlmWeakenSessionKey(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN OUT PBYTE pMasterKey,
    OUT PDWORD pcbKeyLength
    );

DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN PCSTR pTarget,
    IN DWORD dwResponseType,
    OUT PDWORD pdwBufferSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppBuffer
    );

DWORD
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    );

DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    );

DWORD
NtlmBuildNtlmV2Response(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN PCSTR pTarget,
    OUT PDWORD pdwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE* ppResponse
    );

DWORD
NtlmCreateNtlmV2Blob(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN BYTE NtlmHashV2[MD4_DIGEST_LENGTH],
    OUT PDWORD pdwSize,
    OUT PBYTE* ppBlob
    );

DWORD
NtlmBuildLmV2Response(
    VOID
    );

DWORD
NtlmBuildNtlm2Response(
    UCHAR ServerChallenge[8],
    PCSTR pPassword,
    PDWORD pdwLmRespSize,
    PBYTE* ppLmResp,
    PDWORD pdwNtRespSize,
    PBYTE* ppNtResp,
    BYTE pUserSessionKey[NTLM_SESSION_KEY_SIZE]
    );

DWORD
NtlmBuildAnonymousResponse(
    VOID
    );

DWORD
NtlmCreateNegotiateContext(
    IN NTLM_CRED_HANDLE hCred,
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_CONTEXT* ppNtlmContext,
    OUT PSecBuffer pOutput
    );

DWORD
NtlmCreateChallengeContext(
    IN const NTLM_NEGOTIATE_MESSAGE_V1* pNtlmNegMsg,
    IN NTLM_CRED_HANDLE hCred,
    OUT PNTLM_CONTEXT *ppNtlmContext,
    OUT PSecBuffer pOutput
    );

DWORD
NtlmCreateResponseContext(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN NTLM_CRED_HANDLE hCred,
    IN BOOLEAN bDoAnonymous,
    OUT PNTLM_CONTEXT* ppNtlmContext,
    OUT PSecBuffer pOutput
    );

DWORD
NtlmFixUserName(
    IN PCSTR pOriginalUserName,
    OUT PSTR* ppUserName
    );

DWORD
NtlmCreateGuestContext(
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateValidatedContext(
    IN PNTLM_RESPONSE_MESSAGE_V1 pNtlmRespMsg,
    IN DWORD dwMsgSize,
    IN DWORD NegotiatedFlags,
    IN PBYTE pSessionKey,
    IN DWORD dwSessionKeyLen,
    IN NTLM_CRED_HANDLE hCred,
    OUT PNTLM_CONTEXT *ppNtlmContext
    );

DWORD
NtlmCreateSubkey(
    DWORD dwMasterKeyLen,
    PBYTE pMasterKey,
    PCSTR pszSubkeyMagic,
    RC4_KEY** ppResult
    );

DWORD
NtlmInitializeKeys(
    PNTLM_CONTEXT pNtlmContext
    );

DWORD
NtlmValidateResponse(
    IN HANDLE Handle,
    IN NTLM_CRED_HANDLE hCred,
    IN PNTLM_RESPONSE_MESSAGE_V1 pRespMsg,
    IN DWORD dwRespMsgSize,
    IN PNTLM_CONTEXT pChlngCtxt,
    OUT BYTE pSessionKey[NTLM_SESSION_KEY_SIZE]
    );

DWORD
NtlmGetUserNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE_V1 pRespMsg,
    IN DWORD dwRespMsgSize,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppUserName
    );

DWORD
NtlmGetDomainNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE_V1 pRespMsg,
    IN DWORD dwRespMsgSize,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppDomainName
    );

DWORD
NtlmGetWorkstationFromResponse(
    IN PNTLM_RESPONSE_MESSAGE_V1 pRespMsg,
    IN DWORD dwRespMsgSize,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppWorkstation
    );

DWORD
NtlmCreateNtlmV1Hash(
    PCSTR pPassword,
    BYTE Hash[MD4_DIGEST_LENGTH]
    );

DWORD
NtlmCreateNtlmV2Hash(
    PCSTR pUserName,
    PCSTR pDomain,
    BYTE NtlmV1Hash[MD4_DIGEST_LENGTH],
    BYTE NtlmV2Hash[MD4_DIGEST_LENGTH]
    );

VOID
NtlmGenerateLanManagerSessionKey(
    IN PNTLM_RESPONSE_MESSAGE_V1 pMessage,
    IN PBYTE pLmUserSessionKey,
    OUT PBYTE pLanManagerSessionKey
    );

DWORD
NtlmCreateMD4Digest(
    IN PBYTE pBuffer,
    IN DWORD dwBufferLen,
    OUT BYTE MD4Digest[MD4_DIGEST_LENGTH]
    );

ULONG64
NtlmCreateKeyFromHash(
    IN PBYTE pBuffer,
    IN DWORD dwLength
    );

VOID
NtlmSetParityBit(
    IN OUT PULONG64 pKey
    );

DWORD
NtlmGetProcessSecurity(
    IN LWMsgCall* pCall,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    );

VOID
NtlmGetSecBuffers(
    PSecBufferDesc pMessage,
    PSecBuffer* ppToken,
    PSecBuffer* ppPadding
    );

DWORD
NtlmReadRegistry(
    OUT PNTLM_CONFIG pConfig
    );

#endif /* __NTLM_PROTOTYPES_H__ */
