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
 *        lwsecurityidentifier.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LWSS)
 *
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWSECURITY_IDENTIFIER_H__
#define __LWSECURITY_IDENTIFIER_H__

#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LSA_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;  //byte representation of multi-byte Security Identification Descriptor
    DWORD dwByteLength;
} LSA_SECURITY_IDENTIFIER, *PLSA_SECURITY_IDENTIFIER;

DWORD
LsaAllocSecurityIdentifierFromBinary(
    UCHAR* sidBytes,
    DWORD dwSidBytesLength,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LsaAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLSA_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LsaFreeSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LsaGetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LsaSetSecurityIdentifierRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

DWORD
LsaReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    );

DWORD
LsaGetSecurityIdentifierHashedRid(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

// The UID is a DWORD constructued using a non-cryptographic hash
// of the User's domain SID and user RID.
DWORD
LsaHashSecurityIdentifierToId(
    IN PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    );

DWORD
LsaHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    );

DWORD
LsaGetSecurityIdentifierBinary(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LsaGetSecurityIdentifierString(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LsaSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PSTR* pszSidString
    );

DWORD
LsaGetDomainSecurityIdentifier(
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLSA_SECURITY_IDENTIFIER* ppDomainSID
    );

DWORD
LsaHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD* pdwByteArrayLength
    );

DWORD
LsaByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LsaSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    );

DWORD
LsaHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

#endif /* __LWSECURITY_IDENTIFIER_H__ */
