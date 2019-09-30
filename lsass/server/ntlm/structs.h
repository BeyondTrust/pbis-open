/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        structs.h
 *
 * Abstract:
 *
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _NTLM_OS_VERSION
{
    BYTE MajorVersion;
    BYTE MinorVersion;
    WORD BuildNumber;
    DWORD Unknown; // Must be 0x0000000F
} __attribute__((__packed__))
NTLM_OS_VERSION, *PNTLM_OS_VERSION;

typedef struct _NTLM_NEGOTIATE_MESSAGE_V1
{
    UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
    DWORD MessageType;
    DWORD NtlmFlags;
} __attribute__((__packed__))
NTLM_NEGOTIATE_MESSAGE_V1, *PNTLM_NEGOTIATE_MESSAGE_V1;

typedef struct _NTLM_NEGOTIATE_MESSAGE_V2
{
    union
    {
        NTLM_NEGOTIATE_MESSAGE_V1 V1;
        struct
        {
            UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
            DWORD MessageType;
            DWORD NtlmFlags;
        };
    };
    // These refer to the machine the client is using, not the account they
    // want to authenticate with.
    NTLM_SEC_BUFFER ClientDomain;
    NTLM_SEC_BUFFER ClientWorkstation;
} __attribute__((__packed__))
NTLM_NEGOTIATE_MESSAGE_V2, *PNTLM_NEGOTIATE_MESSAGE_V2;

typedef struct _NTLM_NEGOTIATE_MESSAGE_V3
{
    union
    {
        NTLM_NEGOTIATE_MESSAGE_V2 V2;
        struct
        {
            union
            {
                NTLM_NEGOTIATE_MESSAGE_V1 V1;
                struct
                {
                    UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
                    DWORD MessageType;
                    DWORD NtlmFlags;
                };
            };
            // These refer to the machine the client is using, not the account
            // they want to authenticate with.
            NTLM_SEC_BUFFER ClientDomain;
            NTLM_SEC_BUFFER ClientWorkstation;
        };
    };
    NTLM_OS_VERSION Version;
    // Optional Data
} __attribute__((__packed__))
NTLM_NEGOTIATE_MESSAGE_V3, *PNTLM_NEGOTIATE_MESSAGE_V3;

typedef struct _NTLM_CHALLENGE_MESSAGE
{
    UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
    DWORD MessageType;
    NTLM_SEC_BUFFER Target;
    DWORD NtlmFlags;
    UCHAR Challenge[NTLM_CHALLENGE_SIZE];
    // Optional Context 8 bytes
    // Optional Target Information NTLM_SEC_BUFFER
    // Optional OS Version 8 bytes
    // Optional Data
} __attribute__((__packed__))
NTLM_CHALLENGE_MESSAGE, *PNTLM_CHALLENGE_MESSAGE;

typedef struct _NTLM_RESPONSE_MESSAGE_V1
{
    UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
    DWORD MessageType;
    NTLM_SEC_BUFFER LmResponse;
    NTLM_SEC_BUFFER NtResponse;
    NTLM_SEC_BUFFER AuthTargetName;
    NTLM_SEC_BUFFER UserName;
    NTLM_SEC_BUFFER Workstation;
    // Optional Data
} __attribute__((__packed__))
NTLM_RESPONSE_MESSAGE_V1, *PNTLM_RESPONSE_MESSAGE_V1;

typedef struct _NTLM_RESPONSE_MESSAGE_V2
{
    union
    {
        NTLM_RESPONSE_MESSAGE_V1 V1;
        struct
        {
            UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
            DWORD MessageType;
            NTLM_SEC_BUFFER LmResponse;
            NTLM_SEC_BUFFER NtResponse;
            NTLM_SEC_BUFFER AuthTargetName;
            NTLM_SEC_BUFFER UserName;
            NTLM_SEC_BUFFER Workstation;
        };
    };
    NTLM_SEC_BUFFER SessionKey;
    DWORD Flags;
    // Optional Data
} __attribute__((__packed__))
NTLM_RESPONSE_MESSAGE_V2, *PNTLM_RESPONSE_MESSAGE_V2;

typedef struct _NTLM_RESPONSE_MESSAGE_V3
{
    union
    {
        NTLM_RESPONSE_MESSAGE_V2 V2;
        struct
        {
            union
            {
                NTLM_RESPONSE_MESSAGE_V1 V1;
                struct
                {
                    UCHAR NtlmSignature[NTLM_NETWORK_SIGNATURE_SIZE];
                    DWORD MessageType;
                    NTLM_SEC_BUFFER LmResponse;
                    NTLM_SEC_BUFFER NtResponse;
                    NTLM_SEC_BUFFER AuthTargetName;
                    NTLM_SEC_BUFFER UserName;
                    NTLM_SEC_BUFFER Workstation;
                };
            };
            NTLM_SEC_BUFFER SessionKey;
            DWORD Flags;
        };
    };
    NTLM_OS_VERSION Version;
    // Optional Data
} __attribute__((__packed__))
NTLM_RESPONSE_MESSAGE_V3, *PNTLM_RESPONSE_MESSAGE_V3;

typedef struct _NTLM_BLOB
{
    BYTE NtlmBlobSignature[4];
    DWORD Reserved1;
    ULONG64 TimeStamp;
    ULONG64 ClientNonce;
    DWORD Reserved2;
    // Target information block
    // DWORD Reserved3
} __attribute__((__packed__))
NTLM_BLOB, *PNTLM_BLOB;

typedef struct _NTLM_TARGET_INFO_BLOCK
{
    SHORT sType;
    SHORT sLength;
    //BYTE  Contents[0];
} __attribute__((__packed__))
NTLM_TARGET_INFO_BLOCK, *PNTLM_TARGET_INFO_BLOCK;

typedef enum
{
    NtlmStateBlank,
    NtlmStateNegotiate,
    NtlmStateChallenge,
    NtlmStateResponse
} NTLM_STATE, *PNTLM_STATE;

typedef struct _NTLM_CONTEXT
{
    NTLM_STATE NtlmState;
    BYTE Challenge[NTLM_CHALLENGE_SIZE];
    PSTR pszClientUsername;
    NTLM_CRED_HANDLE CredHandle;
    DWORD NegotiatedFlags;
    LONG nRefCount;
    BYTE SessionKey[NTLM_SESSION_KEY_SIZE];
    DWORD cbSessionKeyLen;
    BOOLEAN bDoAnonymous;
    PLSA_AUTH_USER_INFO pUserInfo;
    // Set to true if this context was generated through
    // InitializeSecurityContext.
    BOOLEAN bInitiatedSide;

    // These keys are only used with NTLM2 session security. They are ignored
    // with NTLM1 session security.
    BYTE SignKey[MD5_DIGEST_LENGTH];
    BYTE VerifyKey[MD5_DIGEST_LENGTH];

    // With NTLM1 session security, these keys will all point to the same
    // address. With NTLM2 session security, both keys will be unique.
    RC4_KEY* pSealKey;
    RC4_KEY* pUnsealKey;

    // With NTLM1 session security, these sequences numbers will all point to
    // the same address. With NTLM2 session security, the sequence numbers are
    // unique.
    DWORD *pdwSendMsgSeq;
    DWORD *pdwRecvMsgSeq;

    BOOLEAN MappedToGuest;
} NTLM_CONTEXT, *PNTLM_CONTEXT;

typedef struct _NTLM_CREDENTIALS
{
    LSA_CRED_HANDLE CredHandle;
    DWORD dwCredDirection;
    PSTR pszDomainName;
    LONG nRefCount;
    pthread_mutex_t Mutex;
    pthread_mutex_t *pMutex;
} NTLM_CREDENTIALS, *PNTLM_CREDENTIALS;

typedef struct _NTLM_CONFIG
{
    BOOLEAN bSendNTLMv2;
    BOOLEAN bSupport56bit;
    BOOLEAN bSupport128bit;
    BOOLEAN bSupportUnicode;
    BOOLEAN bSupportNTLM2SessionSecurity;
    BOOLEAN bSupportKeyExchange;
} NTLM_CONFIG, *PNTLM_CONFIG;

#endif /* __STRUCTS_H__ */
