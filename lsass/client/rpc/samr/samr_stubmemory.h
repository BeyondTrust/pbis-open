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
 *        samr_stubmemory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr DCE/RPC stub memory cleanup functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMR_STUB_MEMORY_H_
#define _SAMR_STUB_MEMORY_H_

void SamrCleanStubRidNameArray(RID_NAME_ARRAY *r);

void SamrFreeStubRidNameArray(RID_NAME_ARRAY *ptr);

void SamrCleanStubIds(IDS *r);

void SamrCleanStubUnicodeStringArray(UNICODE_STRING_ARRAY *r);

void SamrCleanStubEntryArray(ENTRY_ARRAY *r);

void SamrFreeStubEntryArray(ENTRY_ARRAY *ptr);

void SamrFreeStubDomSid(PSID ptr);

void SamrCleanStubSidArray(SID_ARRAY *r);

void SamrCleanStubRidWithAttributeArray(RID_WITH_ATTRIBUTE_ARRAY *r);

void SamrFreeStubRidWithAttributeArray(RID_WITH_ATTRIBUTE_ARRAY *ptr);

void SamrCleanStubAliasInfo(AliasInfo *r, UINT16 level);

void SamrFreeStubAliasInfo(AliasInfo *ptr, UINT16 level);

void SamrCleanStubDomainInfo(DomainInfo *r, UINT16 level);

void SamrFreeStubDomainInfo(DomainInfo *ptr, UINT16 level);

void SamrCleanStubUserInfo(UserInfo *r, UINT16 level);

void SamrFreeStubUserInfo(UserInfo *ptr, UINT16 level);

void SamrCleanStubDisplayInfo(SamrDisplayInfo *ptr, UINT16 level);

void SamrCleanStubSecurityDescriptorBuffer(
    PSAMR_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    );

void
SamrFreeStubSecurityDescriptorBuffer(
    PSAMR_SECURITY_DESCRIPTOR_BUFFER pSecDescBuffer
    );

#endif /* _SAMR_STUB_MEMORY_H */
