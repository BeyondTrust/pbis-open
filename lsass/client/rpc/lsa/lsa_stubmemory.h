/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
