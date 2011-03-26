/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        security-types-internal.h
 *
 * Abstract:
 *
 *        Base Security Internal Types
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LW_SECURITY_TYPES_INTERNAL_H__
#define __LW_SECURITY_TYPES_INTERNAL_H__

#include <lw/security-types.h>
#include <pthread.h>

//
// ACL - Access Control List
//
// See the documentation with the PACL type for a description.
//

typedef struct _ACL {
    UCHAR AclRevision;
    UCHAR Sbz1; // Padding (should be 0)
    USHORT AclSize;
    USHORT AceCount;
    USHORT Sbz2; // Padding (should be 0)
} ACL;

typedef char _LW_C_ASSERT_CHECK_ACL_SIZE[(sizeof(ACL) == ACL_HEADER_SIZE)?1:-1];

//
// SD - Security Descriptor
//
// See the documentation with the PSECURITY_DESCRIPTOR_ABSOLUTE and
// PSECURITY_DESCRIPTOR_RELATIVE types for a description.
//

typedef struct _SECURITY_DESCRIPTOR_ABSOLUTE {
   UCHAR Revision;
   UCHAR Sbz1; // Padding (should be 0 unless SE_RM_CONTROL_VALID)
   SECURITY_DESCRIPTOR_CONTROL Control;
   PSID Owner;
   PSID Group; /// Can be NULL.
   PACL Sacl;
   PACL Dacl;
} SECURITY_DESCRIPTOR_ABSOLUTE;

typedef char _LW_C_ASSERT_CHECK_SECURITY_DESCRIPTOR_ABSOLUTE_SIZE[(sizeof(SECURITY_DESCRIPTOR_ABSOLUTE) == SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE)?1:-1];

typedef struct _SECURITY_DESCRIPTOR_RELATIVE {
    UCHAR Revision;
    UCHAR Sbz1; // Padding (should be 0 unless SE_RM_CONTROL_VALID)
    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG Owner; // offset to Owner SID
    ULONG Group; // offset to Group SID
    ULONG Sacl; // offset to system ACL
    ULONG Dacl; // offset to discretional ACL
    // Owner, Group, Sacl, and Dacl data follows
} SECURITY_DESCRIPTOR_RELATIVE;

typedef char _LW_C_ASSERT_CHECK_SECURITY_DESCRIPTOR_RELATIVE_SIZE[(sizeof(SECURITY_DESCRIPTOR_RELATIVE) == SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE)?1:-1];

//
// Access Token
//
// This is an opaque type.
//

typedef ULONG ACCESS_TOKEN_FLAGS, *PACCESS_TOKEN_FLAGS;

#define ACCESS_TOKEN_FLAG_UNIX_PRESENT 0x00000001

typedef struct _ACCESS_TOKEN {
    LONG ReferenceCount;
    pthread_rwlock_t RwLock;
    pthread_rwlock_t* pRwLock;
    ACCESS_TOKEN_FLAGS Flags;
    // TOKEN_USER:
    SID_AND_ATTRIBUTES User;
    // TOKEN_GROUPS:
    ULONG GroupCount;
    PSID_AND_ATTRIBUTES Groups;
    // TOKEN_PRIVILEGES:
    ULONG PrivilegeCount;
    PLUID_AND_ATTRIBUTES Privileges;
    // TOKEN_OWNER:
    PSID Owner;
    // TOKEN_PRIMARY_GROUP:
    PSID PrimaryGroup;
    // TOKEN_DEFAULT_DACL:
    PACL DefaultDacl;
#if 0
    TOKEN_SOURCE Source;
#endif
    // TOKEN_UNIX:
    ULONG Uid;
    ULONG Gid;
    ULONG Umask;
} ACCESS_TOKEN;

typedef struct _SID_AND_ATTRIBUTES_SELF_RELATIVE
{
    ULONG SidOffset;
    SID_ATTRIBUTES Attributes;
} SID_AND_ATTRIBUTES_SELF_RELATIVE, *PSID_AND_ATTRIBUTES_SELF_RELATIVE;

typedef struct _ACCESS_TOKEN_SELF_RELATIVE {
    ACCESS_TOKEN_FLAGS Flags;
    // TOKEN_USER:
    SID_AND_ATTRIBUTES_SELF_RELATIVE User;
    // TOKEN_GROUPS:
    ULONG GroupCount;
    ULONG GroupsOffset;
    // TOKEN_PRIVILEGES:
    ULONG PrivilegeCount;
    ULONG PrivilegesOffset;
    // TOKEN_OWNER:
    ULONG OwnerOffset;
    // TOKEN_PRIMARY_GROUP:
    ULONG PrimaryGroupOffset;
    // TOKEN_DEFAULT_DACL:
    ULONG DefaultDaclOffset;
#if 0
    TOKEN_SOURCE Source;
#endif
    // TOKEN_UNIX:
    ULONG Uid;
    ULONG Gid;
    ULONG Umask;
} ACCESS_TOKEN_SELF_RELATIVE;

#endif /* __LW_SECURITY_TYPES_INTERNAL_H__ */
