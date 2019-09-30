/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        lsa_stubmemory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Lsa rpc DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSA_STUB_MEMORY_H_
#define _LSA_STUB_MEMORY_H_


VOID
LsaCleanStubTranslatedSidArray(
    TranslatedSidArray *pArray
    );

VOID
LsaCleanStubTranslatedSidArray2(
    TranslatedSidArray2 *pArray
    );

VOID
LsaCleanStubTranslatedSidArray3(
    TranslatedSidArray3 *pArray
    );

VOID
LsaCleanStubTranslatedNameArray(
    TranslatedNameArray *pArray
    );

VOID
LsaCleanStubRefDomainList(
    RefDomainList *pRefDomList
    );

VOID
LsaFreeStubRefDomainList(
    RefDomainList *pRefDomList
    );

VOID
LsaCleanStubPolicyInformation(
    LsaPolicyInformation *pPolicyInfo,
    UINT32 Level
    );

VOID
LsaFreeStubPolicyInformation(
    LsaPolicyInformation *pPolicyInfo,
    UINT32 Level
    );

VOID
LsaFreeStubPrivilegeSet(
    PPRIVILEGE_SET pPrivileges
    );

VOID
LsaCleanStubUnicodeString(
    PUNICODE_STRING pString
    );

VOID
LsaFreeStubUnicodeString(
    PUNICODE_STRING pString
    );

void
LsaCleanStubSecurityDescriptorBuffer(
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    );

void
LsaFreeStubSecurityDescriptorBuffer(
    PLSA_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    );

void
LsaCleanStubAccountBuffer(
    PLSA_ACCOUNT_ENUM_BUFFER pBuffer
    );

void
LsaCleanStubAccountRights(
    PLSA_ACCOUNT_RIGHTS pBuffer
    );

void
LsaCleanStubPrivilegeBuffer(
    PLSA_PRIVILEGE_ENUM_BUFFER pBuffer
    );


#endif /* _LSA_STUB_MEMORY_H_ */
