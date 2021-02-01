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
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Security Identifier API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWSECURITY_IDENTIFIER_H__
#define __LWSECURITY_IDENTIFIER_H__

#define SID_REVISION 1

#define WELLKNOWN_SID_DOMAIN_USER_GROUP_RID 513

typedef struct __LW_SECURITY_IDENTIFIER {
    UCHAR* pucSidBytes;  //byte representation of multi-byte Security Identification Descriptor
    DWORD dwByteLength;
} LW_SECURITY_IDENTIFIER, *PLW_SECURITY_IDENTIFIER;


LW_BEGIN_EXTERN_C

DWORD
LwAllocSecurityIdentifierFromBinary(
    UCHAR* pucSidBytes,
    DWORD dwSidBytesLength,
    PLW_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

DWORD
LwAllocSecurityIdentifierFromString(
    PCSTR pszSidString,
    PLW_SECURITY_IDENTIFIER* ppSecurityIdentifier
    );

VOID
LwFreeSecurityIdentifier(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier
    );

DWORD
LwGetSecurityIdentifierRid(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD pdwRid
    );

DWORD
LwSetSecurityIdentifierRid(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    DWORD dwRid
    );

DWORD
LwReplaceSidRid(
    IN PCSTR pszSid,
    IN DWORD dwNewRid,
    OUT PSTR* ppszNewSid
    );

DWORD
LwGetSecurityIdentifierHashedRid(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    PDWORD dwHashedRid
    );

DWORD
LwHashSecurityIdentifierToId(
    IN PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    OUT PDWORD pdwId
    );

DWORD
LwHashSidStringToId(
    IN PCSTR pszSidString,
    OUT PDWORD pdwId
    );

DWORD
LwGetSecurityIdentifierBinary(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    UCHAR** ppucSidBytes,
    DWORD* pdwSidBytesLength
    );

DWORD
LwGetSecurityIdentifierString(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    PSTR* ppszSidStr
    );

DWORD
LwGetDomainSecurityIdentifier(
    PLW_SECURITY_IDENTIFIER pSecurityIdentifier,
    PLW_SECURITY_IDENTIFIER* ppDomainSID
    );

DWORD
LwByteArrayToLdapFormatHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    );

DWORD
LwSidStrToLdapFormatHexStr(
    IN PCSTR pszSid,
    OUT PSTR* ppszHexSid
    );

void
LwUidHashCalc(
    PDWORD pdwAuthorities,
    DWORD dwAuthorityCount,
    PDWORD pdwHash
    );

DWORD
LwSidStringToBytes(
    IN PCSTR pszSidString,
    OUT UCHAR** ppucSidBytes,
    OUT DWORD* pdwSidBytesLength
    );

DWORD
LwSidBytesToString(
    UCHAR* pucSidBytes,
    DWORD  dwSidBytesLength,
    PSTR*  ppszSidString
    );

DWORD
LwBuildSIDString(
    PCSTR pszRevision,
    PCSTR pszAuth,
    PBYTE pucSidBytes,
    DWORD dwWordCount,
    PSTR* ppszSidString
    );

LW_END_EXTERN_C


#endif /* __LWSECURITY_IDENTIFIER_H__ */
