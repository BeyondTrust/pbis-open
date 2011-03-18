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
 * Module Name:
 *
 *        security-types.h
 *
 * Abstract:
 *
 *        Base Security Types
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __LWBASE_SECURITY_TYPES_H__
#define __LWBASE_SECURITY_TYPES_H__

#ifdef _DCE_IDL_

cpp_quote("#include <lw/security-types.h>")
cpp_quote("#if 0")

#endif

#include <lw/types.h>
#include <lw/attrs.h>

//
// An ACCESS_MASK is a 32-bit value divided as follows from high to low bits:
//
// 4 bits - Generic Access Rights (given in request)
// 2 bits - Unused
// 2 bits - Special Access Rights
// 3 bits - Unused
// 5 bits - Standard Access Rights
// 16 bits - Specific Access Rights
//
// When generic rights are specified in an open, they are mapped to
// specific rights for the object type in question.
//
// Since lwio only deals with files, the only specific rights
// that apply are file rights.
//

//
// Generic Access Rights - 0xF0000000
//
// These are converted to specific rights depending on the type
// of object being accessed.
//

typedef ULONG ACCESS_MASK, *PACCESS_MASK;

#define GENERIC_ALL                 0x10000000
#define GENERIC_EXECUTE             0x20000000
#define GENERIC_WRITE               0x40000000
#define GENERIC_READ                0x80000000

//
// Special Access Rights - 0x03000000
//

//
// ACCESS_SYSTEM_SECURITY - This is valid only in a SACL (to audit its use)
//     and in desired access.  When used as desired access, a check is done
//     against the SE_SECURITY_NAME privilege.  Note that this bit is not
//     valid in a DACL.
//

#define ACCESS_SYSTEM_SECURITY      0x01000000 // Read/write SACL in object SD
#define MAXIMUM_ALLOWED             0x02000000 // Maximum allowed for pricipal

//
// Standard Access Rights - 0x001F0000
//

#ifdef DELETE
#undef DELETE
#endif

#define DELETE                      0x00010000 // Delete object
#define READ_CONTROL                0x00020000 // Read object SD (except SACL)
#define WRITE_DAC                   0x00040000 // Write DACL in object SD
#define WRITE_OWNER                 0x00080000 // Write owner in object SD
#define SYNCHRONIZE                 0x00100000 // Synchronize on object

#define STANDARD_RIGHTS_READ        READ_CONTROL
#define STANDARD_RIGHTS_WRITE       READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE     READ_CONTROL

#define STANDARD_RIGHTS_REQUIRED    0x000F0000 // All but SYNCHRONIZE above
#define STANDARD_RIGHTS_ALL         0x001F0000 // All including SYNCHRONIZE

//
// Specific Access Rights - 0x0000FFFF
//

#define SPECIFIC_RIGHTS_ALL         0x0000FFFF

//
// Valid Access Mask Maks
//

#define VALID_DESIRED_ACCESS_MASK ( \
    GENERIC_ALL | \
    GENERIC_EXECUTE | \
    GENERIC_WRITE | \
    GENERIC_READ | \
    ACCESS_SYSTEM_SECURITY | \
    MAXIMUM_ALLOWED | \
    STANDARD_RIGHTS_ALL | \
    SPECIFIC_RIGHTS_ALL | \
    0 )

#define VALID_DACL_ACCESS_MASK ( \
    GENERIC_ALL | \
    GENERIC_EXECUTE | \
    GENERIC_WRITE | \
    GENERIC_READ | \
    STANDARD_RIGHTS_ALL | \
    SPECIFIC_RIGHTS_ALL | \
    0 )

#define VALID_SACL_ACCESS_MASK ( \
    VALID_DACL_ACCESS_MASK | \
    ACCESS_SYSTEM_SECURITY | \
    0 )

#define VALID_GRANTED_ACCESS_MASK ( \
    VALID_SACL_ACCESS_MASK | \
    0 )


//
// Specific Access Rights - File
//

#define FILE_READ_DATA              0x00000001 // File/Pipe
#define FILE_LIST_DIRECTORY         0x00000001 // Directory
#define FILE_WRITE_DATA             0x00000002 // File/Pipe
#define FILE_ADD_FILE               0x00000002 // Directory
#define FILE_APPEND_DATA            0x00000004 // File
#define FILE_ADD_SUBDIRECTORY       0x00000004 // Directory
#define FILE_CREATE_PIPE_INSTANCE   0x00000004 // Pipe
#define FILE_READ_EA                0x00000008 // File/Directory
#define FILE_WRITE_EA               0x00000010 // File/Directory
#define FILE_EXECUTE                0x00000020 // File
#define FILE_TRAVERSE               0x00000020 // Directory
#define FILE_DELETE_CHILD           0x00000040 // Directory
#define FILE_READ_ATTRIBUTES        0x00000080 // File/Pipe/Directory
#define FILE_WRITE_ATTRIBUTES       0x00000100 // File/Pipe/Directory

#define FILE_ALL_ACCESS ( \
    SYNCHRONIZE | \
    STANDARD_RIGHTS_REQUIRED | \
    0x000001FF | \
    0 )

#define FILE_GENERIC_READ ( \
    SYNCHRONIZE | \
    STANDARD_RIGHTS_READ | \
    FILE_READ_ATTRIBUTES | \
    FILE_READ_DATA | \
    FILE_READ_EA | \
    0 )

#define FILE_GENERIC_WRITE ( \
    SYNCHRONIZE | \
    STANDARD_RIGHTS_WRITE | \
    FILE_WRITE_ATTRIBUTES | \
    FILE_WRITE_DATA | \
    FILE_WRITE_EA | \
    FILE_APPEND_DATA | \
    0 )

#define FILE_GENERIC_EXECUTE ( \
    SYNCHRONIZE | \
    STANDARD_RIGHTS_EXECUTE | \
    FILE_READ_ATTRIBUTES | \
    FILE_EXECUTE | \
    0 )

//
// Specific Access Rights - Registry
//

#define KEY_QUERY_VALUE         0x0001 //Required to query the values of a registry key
#define KEY_SET_VALUE           0x0002 //Required to create, delete, or set a registry value
#define KEY_CREATE_SUB_KEY      0x0004 //Required to create, delete, or rename a subkey of a registry key
#define KEY_ENUMERATE_SUB_KEYS  0x0008 //Required to enumerate the subkeys of a registry key
#define KEY_NOTIFY              0x0010 //Required to request change notifications for a registry key or for subkeys of a registry key.
#define KEY_CREATE_LINK         0x0020 //Reserved for system use


#define KEY_ALL_ACCESS ( \
        (~SYNCHRONIZE) & \
        (STANDARD_RIGHTS_REQUIRED | \
        KEY_QUERY_VALUE |\
        KEY_SET_VALUE |\
        KEY_CREATE_SUB_KEY |\
        KEY_ENUMERATE_SUB_KEYS |\
        KEY_NOTIFY |\
        KEY_CREATE_LINK) \
        )

#define KEY_READ ( \
        (~SYNCHRONIZE) & \
        (STANDARD_RIGHTS_READ | \
        KEY_QUERY_VALUE |\
        KEY_ENUMERATE_SUB_KEYS |\
        KEY_NOTIFY) \
        )


#define KEY_WRITE ( \
        (~SYNCHRONIZE) & \
        (STANDARD_RIGHTS_WRITE | \
        KEY_SET_VALUE |\
        KEY_CREATE_SUB_KEY) \
        )

#define KEY_EXECUTE ( \
        (~SYNCHRONIZE) & \
        (KEY_READ)\
        )

//
// GENERIC_MAPPING - Used to map the GENERIC_{ALL,EXECUTE,WRITE,READ}
// bits to specific rights.
//

typedef struct _GENERIC_MAPPING {
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

//
// SID - Security Identifier
//

#define SID_REVISION 1
#define SID_MAX_SUB_AUTHORITIES 15

typedef struct _SID_IDENTIFIER_AUTHORITY {
    UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
   UCHAR Revision;
#ifdef _DCE_IDL_
    [range(0, SID_MAX_SUB_AUTHORITIES)]
#endif
   UCHAR SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef _DCE_IDL_
   [size_is(SubAuthorityCount)]
#endif
   ULONG SubAuthority[];
} SID, *PSID;

#define SID_MIN_SIZE \
    (LW_FIELD_OFFSET(SID, SubAuthority))

#define _SID_GET_SIZE_REQUIRED(SubAuthorityCount) \
    (SID_MIN_SIZE + (LW_FIELD_SIZE(SID, SubAuthority[0]) * (SubAuthorityCount)))

#define SID_MAX_SIZE \
    _SID_GET_SIZE_REQUIRED(SID_MAX_SUB_AUTHORITIES)

// TODO-Can we somehow get rid of IDLREF_PSID?

// IDLREF_SID should only be used in IDL files where a [in,ref] PSID is wanted
// because DCE RPC does not like a typedef-ed pointer type with [ref].
#ifdef _DCE_IDL_
#define IDLREF_PSID SID*
#else
typedef PSID IDLREF_PSID;
#endif

//
// SID Authorities
//
// NULL                         S-1-0-*
// World                        S-1-1-*
// Local                        S-1-2-*
// Creator                      S-1-3-*
// Non-Unique                   S-1-4-*
// NT                           S-1-5-*
// Mandatory Integrity Control  S-1-16-*

#define SECURITY_NULL_SID_AUTHORITY         { 0, 0, 0, 0, 0, 0 }
#define SECURITY_WORLD_SID_AUTHORITY        { 0, 0, 0, 0, 0, 1 }
#define SECURITY_LOCAL_SID_AUTHORITY        { 0, 0, 0, 0, 0, 2 }
#define SECURITY_CREATOR_SID_AUTHORITY      { 0, 0, 0, 0, 0, 3 }
#define SECURITY_NON_UNIQUE_AUTHORITY       { 0, 0, 0, 0, 0, 4 }
#define SECURITY_NT_AUTHORITY               { 0, 0, 0, 0, 0, 5 }
#define SECURITY_MANDATORY_LABEL_AUTHORITY  { 0, 0, 0, 0, 0, 16 }

//
// Well-Known SIDs
//
// NULL                     S-1-0-0 - Group without any members
// World                    S-1-1-0 - Group of all users
// Local                    S-1-2-0 - Group of locally logged on users
// Creator Owner            S-1-3-0 - Represents object owner
// Creator Group            S-1-3-1 - Represents primary group of object owner
// Creator Owner Server     S-1-3-2 - ??? (not defined)
// Creator Group Server     S-1-3-3 - ??? (not defined)

// SECURITY_NULL_SID_AUTHORITY
#define SECURITY_NULL_RID                   0

// SECURITY_WORLD_SID_AUTHORITY
#define SECURITY_WORLD_RID                  0

// SECURITY_LOCAL_SID_AUTHORITY
#define SECURITY_LOCAL_RID                  0

// SECURITY_CREATOR_SID_AUTHORITY
#define SECURITY_CREATOR_OWNER_RID          0
#define SECURITY_CREATOR_GROUP_RID          1

//
// Well-Known Mandatiry Integrity Control (MIC) SIDs
//
// These SIDs are S-1-16-RID where the RID is the mandatory integrity
// level.  They are used to protect objects that require a minimum
// integrity level for access.
//
// Mandatory integrity control (MIC) was introduced in Windows Vista.
// An object is secured with an "integrity label" represented by
// an "integrity SID" via in SYSTEM_MANADATORY_LABEL_ACE_TYPE ACE.
// The default integrity is medium integrity.
//
// The policy in a SYSTEM_MANADORTY_LABEL_ACE determines how access
// to objects of higher integrity by lower integrity tokens is treated.
//

#define SECURITY_MANDATORY_UNTRUSTED_RID            0x00000000
#define SECURITY_MANDATORY_LOW_RID                  0x00001000
#define SECURITY_MANDATORY_MEDIUM_RID               0x00002000
#define SECURITY_MANDATORY_HIGH_RID                 0x00003000
#define SECURITY_MANDATORY_SYSTEM_RID               0x00004000
#define SECURITY_MANDATORY_PROTECTED_PROCESS_RID    0x00005000

//
// Well-Known NT SIDs
//
// Dialup                   S-1-5-1 - Group of users logged on via dialup (NT AUTHORITY\DIALUP)
// Network                  S-1-5-2 - Group of users logged on via network (LOGON32_LOGON_NETWORK) (NT AUTHORITY\NETWORK)
// Batch                    S-1-5-3 - Group of users logged on via batch (LOGON32_LOGON_BATCH) (NT AUTHORITY\BATCH)
// Interactive              S-1-5-4 - Group of users logged on interactively (LOGON32_LOGON_INTERACTIVE) (NT AUTHORITY\INTERACTIVE)
// Logon IDs                S-1-5-5-X-Y - Group representing a logon session (can access window station and such)
// Service                  S-1-5-6 - Group of users logged on as a service (LOGON32_LOGON_SERVICE) (NT AUTHORITY\SERVICE)
// Anonymous                S-1-5-7 - Anonymous logon (aka null logon session) (NT AUTHORITY\ANONYMOUS LOGON)
// Proxy                    S-1-5-8 - Proxy? (NT AUTHORITY\PROXY)
// Enterprise DCs           S-1-5-9 - Group of domain controllers (NT AUTHORITY\ENTERPRISE DOMAIN CONTROLLERS)
// Principal Self           S-1-5-10 - Used in group/user object ACE to represent the group/user object itself (NT AUTHORITY\SELF)
// Authenticated            S-1-5-11 - Group of authenticated users (NT AUTHORITY\Authenticated Users)
// Restricted Code          S-1-5-12 - Restricted code? (NT AUTHORITY\RESTRICTED)
// Terminal Server          S-1-5-13 - Group of users logged onto a terminal server (NT AUTHORITY\TERMINAL SERVICE USER)
// Remote                   S-1-5-14 - (NT AUTHORITY\REMOTE INTERACTIVE LOGON)
// This Organization        S-1-5-15 - (NT AUTHORITY\This Organization)
// IUSR                     S-1-5-17 - (NT AUTHORITY\IUSR) (IIS?)
// Local System             S-1-5-18 - Local System account (NT AUTHORITY\SYSTEM)
// Local Service            S-1-5-19 - Local Service account (NT AUTHORITY\LOCAL SERVICE)
// Network Service          S-1-5-20 - Network Service account (NT AUTHORITY\NETWORK SERVICE)
// NT "non-unique"          S-1-5-21-* - NT domain/computer-specific SIDs (*\* accounts)
// Built-in domain          S-1-5-32-X - Built-in local system domain (BUILTIN\* accounts)
// Security packages        S-1-5-64-X - Security packages
//   S-1-4-64-10 - NT AUTHORITY\NTLM Authentication
//   S-1-4-64-14 - NT AUTHORITY\SChannel Authentication
//   S-1-4-64-21 - NT AUTHORITY\Digest Authentication
// Other Organization       S-1-5-1000 - (NT AUTHORITY\Other Organization)
//

#define SECURITY_DIALUP_RID                 1 // NT AUTHORITY\DIALUP
#define SECURITY_NETWORK_RID                2 // NT AUTHORITY\NETWORK (via LOGON32_LOGON_NETWORK)
#define SECURITY_BATCH_RID                  3 // NT AUTHORITY\BATCH (via LOGON32_LOGON_BATCH)
#define SECURITY_INTERACTIVE_RID            4 // NT AUTHORITY\INTERACTIVE (via LOGON32_LOGON_INTERACTIVE)
#define SECURITY_LOGON_IDS_RID              5
#define SECURITY_SERVICE_RID                6 // NT AUTHORITY\SERVICE
#define SECURITY_ANONYMOUS_LOGON_RID        7
#define SECURITY_PROXY_RID                  8
#define SECURITY_ENTERPRISE_CONTROLLERS_RID 9
#define SECURITY_SERVER_LOGON_RID           SECURITY_ENTERPRISE_CONTROLLERS_RID // alias for above
#define SECURITY_PRINCIPAL_SELF_RID         10
#define SECURITY_AUTHENTICATED_USER_RID     11
#define SECURITY_RESTRICTED_CODE_RID        12
#define SECURITY_TERMINAL_SERVER_RID        13
#define SECURITY_REMOTE_LOGON_RID           14
#define SECURITY_THIS_ORGANIZATION_RID      15
#define SECURITY_IUSER_RID                  17
#define SECURITY_LOCAL_SYSTEM_RID           18
#define SECURITY_LOCAL_SERVICE_RID          19
#define SECURITY_NETWORK_SERVICE_RID        20
#define SECURITY_NT_NON_UNIQUE              21
#define SECURITY_BUILTIN_DOMAIN_RID         32
#define SECURITY_PACKAGE_BASE_RID           64
#define SECURITY_OTHER_ORGANIZATION_RID     1000

//
// Well-Known NT SIDs RID Counts
//

#define SECURITY_LOGON_IDS_RID_COUNT 3
#define SECURITY_BUILTIN_RID_COUNT 2
#define SECURITY_PACKAGE_RID_COUNT 2

// Well-Known NT SIDs Sub-Authority Counts for Domain/Computer
//
// There are 3 sub-authorities used to represent the domain/computer.
// So a user/group SID in the domain/computer would be of the form
// S-1-5-21-A-B-C-RID where A-B-C are the 3 sub-authorities for that
// domain/computer.
//
// The domain itself has a SID of the form S-1-5-21-A-B-C.
// So a domain/computer-based SID can have 3 or 4 sub-authorities
// following the NT non-unique sub-authority (for a total of either
// 4 or 5 sub-authorities).

#define SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT 3

//
// Well-Known Domain/Computer Users
//

#define DOMAIN_USER_RID_ADMIN       500 // domain + local
#define DOMAIN_USER_RID_GUEST       501 // domain + local
#define DOMAIN_USER_RID_KRBTGT      502 // domain only
#define DOMAIN_USER_RID_MAX         999 // anything higher is not "well-known" (e.g., regular users/groups)

//
// Well-Known Domain Groups (S-1-5-21-
//

#define DOMAIN_GROUP_RID_ADMINS                 512 // Domain Admins
#define DOMAIN_GROUP_RID_USERS                  513 // Domain Users
#define DOMAIN_GROUP_RID_GUESTS                 514 // Domain Guests
#define DOMAIN_GROUP_RID_COMPUTERS              515 // Domain Computers
#define DOMAIN_GROUP_RID_CONTROLLERS            516 // Domain Controllers
#define DOMAIN_GROUP_RID_CERT_ADMINS            517 // Cert Publishers
#define DOMAIN_GROUP_RID_SCHEMA_ADMINS          518 // Schema Admins
#define DOMAIN_GROUP_RID_ENTERPRISE_ADMINS      519 // Enterprise Admins
#define DOMAIN_GROUP_RID_POLICY_ADMINS          520 // Group Policy Creator Owners

//
// Well-Known Local Groups (S-1-5-32-*)
//

#define DOMAIN_ALIAS_RID_ADMINS                         544 // BUILTIN\Administrators
#define DOMAIN_ALIAS_RID_USERS                          545 // BUILTIN\Users
#define DOMAIN_ALIAS_RID_GUESTS                         546 // BUILTIN\Guests
#define DOMAIN_ALIAS_RID_POWER_USERS                    547 // BUILTIN\Power Users
#define DOMAIN_ALIAS_RID_ACCOUNT_OPS                    548 // BUILTIN\Account Operators (DC only)
#define DOMAIN_ALIAS_RID_SYSTEM_OPS                     549 // BUILTIN\Server Operators (DC only)
#define DOMAIN_ALIAS_RID_PRINT_OPS                      550 // BUILTIN\Print Operators
#define DOMAIN_ALIAS_RID_BACKUP_OPS                     551 // BUILTIN\Backup Operators
#define DOMAIN_ALIAS_RID_REPLICATOR                     552 // BUILTIN\Replicator
#define DOMAIN_ALIAS_RID_RAS_SERVERS                    553 // ???
#define DOMAIN_ALIAS_RID_PREW2KCOMPACCESS               554 // BUILTIN\Pre-Windows 2000 Compatible Access
#define DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS           555 // BUILTIN\Remote Desktop Users
#define DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS      556 // BUILTIN\Network Configuration Operators
#define DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS 557 // BUILTIN\Incoming Forest Trust Builders (DC only)
#define DOMAIN_ALIAS_RID_MONITORING_USERS               558 // BUILTIN\Performance Monitor Users
#define DOMAIN_ALIAS_RID_LOGGING_USERS                  559 // BUILTIN\Performance Log Users
#define DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS            560 // BUILTIN\Windows Authorization Access Group (DC only)
#define DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS             561 // BUILTIN\Terminal Server License Servers (DC only)
#define DOMAIN_ALIAS_RID_DCOM_USERS                     562 // BUILTIN\Distributed COM Users

#define DOMAIN_ALIAS_RID_IUSERS                         568 // BUILTIN\IIS_IUSRS (used by IIS)
#define DOMAIN_ALIAS_RID_CRYPTO_OPERATORS               569 // BUILTIN\Cryptographic Operators

#define DOMAIN_ALIAS_RID_EVENT_LOG_READERS_GROUP        573 // BUILTIN\Event Log Readers

//
// Well-Known Likewise Local Groups (S-1-5-21-*)

#define DOMAIN_ALIAS_RID_LW_USERS                       800 // MACHINE\Likewise Users

//
// Well-Known Security Package SIDs
//

#define SECURITY_PACKAGE_NTLM_RID       10
#define SECURITY_PACKAGE_SCHANNEL_RID   14
#define SECURITY_PACKAGE_DIGEST_RID     21

//
// SID Types (aka SID_NAME_USE)
//

typedef ULONG SID_NAME_USE, *PSID_NAME_USE;

#define SidTypeUser             1
#define SidTypeGroup            2
#define SidTypeDomain           3
#define SidTypeAlias            4
#define SidTypeWellKnownGroup   5
#define SidTypeDeletedAccount   6
#define SidTypeInvalid          7
#define SidTypeUnknown          8
#define SidTypeComputer         9
#define SidTypeLabel            10

#if 0
// Alternative to SID_NAME_USE's SidType<Type> values
#define SID_TYPE_USER               1
#define SID_TYPE_GROUP              2
#define SID_TYPE_DOMAIN             3
#define SID_TYPE_ALIAS              4
#define SID_TYPE_WELL_KNOWN_GROUP   5
#define SID_TYPE_DELETED_ACCOUNT    6
#define SID_TYPE_INVALID            7
#define SID_TYPE_UNKNOWN            8
#define SID_TYPE_COMPUTER           9
#define SID_TYPE_LABEL              10
#endif


//
// SDDL Version information
//
#define SDDL_REVISION_1     1
#define SDDL_REVISION       SDDL_REVISION_1

//
// SID Attributes
//
// The flags used depend on the type of SID.
// Currently, only the "group" (non-user) SIDs have
// attributes.  These are the SE_GROUP_XXX bits below.
//

typedef ULONG SID_ATTRIBUTES, *PSID_ATTRIBUTES;

// Group Attributes
#define SE_GROUP_MANDATORY                  0x00000001
#define SE_GROUP_ENABLED_BY_DEFAULT         0x00000002
#define SE_GROUP_ENABLED                    0x00000004
#define SE_GROUP_OWNER                      0x00000008
#define SE_GROUP_USE_FOR_DENY_ONLY          0x00000010
#define SE_GROUP_INTEGRITY                  0x00000020
#define SE_GROUP_INTEGRITY_ENABLED          0x00000040
#define SE_GROUP_RESOURCE                   0x20000000
#define SE_GROUP_LOGON_ID                   0xC0000000

#define SE_GROUP_VALID_SID_ATTRIBUTES_MASK ( \
    SE_GROUP_MANDATORY | \
    SE_GROUP_ENABLED_BY_DEFAULT | \
    SE_GROUP_ENABLED | \
    SE_GROUP_OWNER | \
    SE_GROUP_USE_FOR_DENY_ONLY | \
    SE_GROUP_INTEGRITY | \
    SE_GROUP_INTEGRITY_ENABLED | \
    SE_GROUP_RESOURCE | \
    SE_GROUP_LOGON_ID | \
    0 )

typedef struct _SID_AND_ATTRIBUTES {
    PSID Sid;
    SID_ATTRIBUTES Attributes;
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;

//
// ACE - Access Control Entry
//
// An ACE consists of an ACE header (ACE_HEADER) followed by the rest
// of the type-specific ACE structure.  The ACE type is denited by a
// <TYPE>_ACE_TYPE value in the header.  Unless otherwise noted, the ACE
// is stored as a <TYPE>_ACE structure (which starts with an ACE_HEADER).
//

typedef struct _ACE_HEADER {
    UCHAR AceType;
    UCHAR AceFlags;
    USHORT AceSize;
} ACE_HEADER, *PACE_HEADER;

//
// ACE Types
//

// Windows NT
#define ACCESS_ALLOWED_ACE_TYPE                 0 // ACCESS_ALLOWED_ACE
#define ACCESS_DENIED_ACE_TYPE                  1 // ACCESS_DENIED_ACE
#define SYSTEM_AUDIT_ACE_TYPE                   2 // SYSTEM_AUDIT_ACE
#define SYSTEM_ALARM_ACE_TYPE                   3 // SYSTEM_ALARM_ACE (Reserved)

// Reserved
#define ACCESS_ALLOWED_COMPOUND_ACE_TYPE        4 // Reserved

// Object ACE Types (Windows 2000)
#define ACCESS_ALLOWED_OBJECT_ACE_TYPE          5 // ACCESS_ALLOWED_OBJECT_ACE
#define ACCESS_DENIED_OBJECT_ACE_TYPE           6 // ACCESS_DENIED_OBJECT_ACE
#define SYSTEM_AUDIT_OBJECT_ACE_TYPE            7 // SYSTEM_AUDIT_OBJECT_ACE
#define SYSTEM_ALARM_OBJECT_ACE_TYPE            8 // SYSTEM_ALARM_OBJECT_ACE (Reserved)

// Windows Vista
#define ACCESS_ALLOWED_CALLBACK_ACE_TYPE        9  // ACCESS_ALLOWED_CALLBACK_ACE
#define ACCESS_DENIED_CALLBACK_ACE_TYPE         10 // ACCESS_DENIED_CALLBACK_ACE
#define ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE 11 // ACCESS_ALLOWED_CALLBACK_OBJECT_ACE
#define ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE  12 // ACCESS_DENIED_CALLBACK_OBJECT_ACE
#define SYSTEM_AUDIT_CALLBACK_ACE_TYPE          13 // SYSTEM_AUDIT_CALLBACK_ACE
#define SYSTEM_ALARM_CALLBACK_ACE_TYPE          14 // SYSTEM_ALARM_CALLBACK_ACE
#define SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE   15 // SYSTEM_AUDIT_CALLBACK_OBJECT_ACE
#define SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE   16 // SYSTEM_ALARM_CALLBACK_OBJECT_ACE
#define SYSTEM_MANDATORY_LABEL_ACE_TYPE         17 // SYSTEM_MANDATORY_LABEL_ACE


// Min/Max
#define ACCESS_MIN_MS_ACE_TYPE          ACCESS_ALLOWED_ACE_TYPE
#define ACCESS_MAX_MS_V2_ACE_TYPE       SYSTEM_ALARM_ACE_TYPE               // Max Windows NT
#define ACCESS_MAX_MS_V3_ACE_TYPE       ACCESS_ALLOWED_COMPOUND_ACE_TYPE    // Reserved
#define ACCESS_MIN_MS_OBJECT_ACE_TYPE   ACCESS_ALLOWED_OBJECT_ACE_TYPE
#define ACCESS_MAX_MS_OBJECT_ACE_TYPE   SYSTEM_ALARM_OBJECT_ACE_TYPE
#define ACCESS_MAX_MS_V4_ACE_TYPE       SYSTEM_ALARM_OBJECT_ACE_TYPE        // Max Windows 2000
#define ACCESS_MAX_MS_ACE_TYPE          SYSTEM_ALARM_OBJECT_ACE_TYPE
#define ACCESS_MAX_MS_V5_ACE_TYPE       SYSTEM_MANDATORY_LABEL_ACE_TYPE     // Max Windows Vista

//
// ACE Flags
//

#define OBJECT_INHERIT_ACE          0x01
#define CONTAINER_INHERIT_ACE       0x02
#define NO_PROPAGATE_INHERIT_ACE    0x04
#define INHERIT_ONLY_ACE            0x08
#define INHERITED_ACE               0x10

#define VALID_INHERIT_ACE_FLAGS_MASK ( \
    OBJECT_INHERIT_ACE | \
    CONTAINER_INHERIT_ACE | \
    NO_PROPAGATE_INHERIT_ACE | \
    INHERIT_ONLY_ACE | \
    INHERITED_ACE | \
    0 )

#define SUCCESSFUL_ACCESS_ACE_FLAG  0x40
#define FAILED_ACCESS_ACE_FLAG      0x80

#define VALID_AUDIT_ALARM_ACE_FLAGS_MASK ( \
    SUCCESSFUL_ACCESS_ACE_FLAG | \
    FAILED_ACCESS_ACE_FLAG | \
    0 )

#define VALID_ACE_FLAGS_MASK ( \
    VALID_INHERIT_ACE_FLAGS_MASK | \
    VALID_AUDIT_ALARM_ACE_FLAGS_MASK | \
    0 )

//
// ACE Type Structures
//

#if 1
// TODO-Perhaps collapse isomorphic ACE type structures so they are
// typedef-ed to some base type.

typedef struct _ACCESS_ALLOWED_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} ACCESS_ALLOWED_ACE, *PACCESS_ALLOWED_ACE;

#define ACCESS_ALLOWED_ACE_MAX_SIZE \
    (LW_FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + SID_MAX_SIZE);

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _ACCESS_DENIED_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} ACCESS_DENIED_ACE, *PACCESS_DENIED_ACE;

#define ACCESS_DENIED_ACE_MAX_SIZE \
    (LW_FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart) + SID_MAX_SIZE);

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _SYSTEM_AUDIT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} SYSTEM_AUDIT_ACE, *PSYSTEM_AUDIT_ACE;

#define SYSTEM_AUDIT_ACE_MAX_SIZE \
    (LW_FIELD_OFFSET(SYSTEM_AUDIT_ACE, SidStart) + SID_MAX_SIZE);

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _SYSTEM_ALARM_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} SYSTEM_ALARM_ACE, *PSYSTEM_ALARM_ACE;

#define SYSTEM_ALARM_ACE_MAX_SIZE \
    (LW_FIELD_OFFSET(SYSTEM_ALARM_ACE, SidStart) + SID_MAX_SIZE);

#else
struct _COMMON_STANDARD_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
};

typedef struct _COMMON_STANDARD_ACE ACCESS_ALLOWED_ACE, *PACCESS_ALLOWED_ACE;
typedef struct _COMMON_STANDARD_ACE ACCESS_DENIED_ACE, *PACCESS_DENIED_ACE;
typedef struct _COMMON_STANDARD_ACE SYSTEM_AUDIT_ACE, *PSYSTEM_AUDIT_ACE;
typedef struct _COMMON_STANDARD_ACE SYSTEM_ALARM_ACE, *PSYSTEM_ALARM_ACE;
#endif

#if 0
// TODO-Need GUID definition
#if 1
typedef struct _ACCESS_ALLOWED_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
} ACCESS_ALLOWED_OBJECT_ACE, *PACCESS_ALLOWED_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _ACCESS_DENIED_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
} ACCESS_DENIED_OBJECT_ACE,  *PACCESS_DENIED_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _SYSTEM_AUDIT_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
} SYSTEM_AUDIT_OBJECT_ACE, *PSYSTEM_AUDIT_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _SYSTEM_ALARM_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
} SYSTEM_ALARM_OBJECT_ACE, *PSYSTEM_ALARM_OBJECT_ACE;
#else
struct _COMMON_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
};

typedef struct _COMMON_OBJECT_ACE ACCESS_ALLOWED_OBJECT_ACE, *PACCESS_ALLOWED_OBJECT_ACE;
typedef struct _COMMON_OBJECT_ACE ACCESS_DENIED_OBJECT_ACE, *PACCESS_DENIED_OBJECT_ACE;
typedef struct _COMMON_OBJECT_ACE SYSTEM_AUDIT_OBJECT_ACE, *PSYSTEM_AUDIT_OBJECT_ACE;
typedef struct _COMMON_OBJECT_ACE SYSTEM_ALARM_OBJECT_ACE, *PSYSTEM_ALARM_OBJECT_ACE;
#endif
#endif

//
// Object ACE Flags (in <TYPE>_OBJECT_ACE's "Flags" field)
//

#define ACE_OBJECT_TYPE_PRESENT             0x00000001
#define ACE_INHERITED_OBJECT_TYPE_PRESENT   0x00000002

#define VALID_OBJECT_ACE_FLAGS_MASK ( \
    ACE_OBJECT_TYPE_PRESENT | \
    ACE_INHERITED_OBJECT_TYPE_PRESENT | \
    0 )

#if 1
// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _ACCESS_ALLOWED_CALLBACK_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
    // Callback-specific data follows
} ACCESS_ALLOWED_CALLBACK_ACE, *PACCESS_ALLOWED_CALLBACK_ACE;

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _ACCESS_DENIED_CALLBACK_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
    // Callback-specific data follows
} ACCESS_DENIED_CALLBACK_ACE, *PACCESS_DENIED_CALLBACK_ACE;

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _SYSTEM_AUDIT_CALLBACK_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
    // Callback-specific data follows
} SYSTEM_AUDIT_CALLBACK_ACE, *PSYSTEM_AUDIT_CALLBACK_ACE;

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _SYSTEM_ALARM_CALLBACK_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
    // Callback-specific data follows
} SYSTEM_ALARM_CALLBACK_ACE, *PSYSTEM_ALARM_CALLBACK_ACE;
#else
struct _COMMON_CALLBACK_STANDARD_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
};

typedef struct _COMMON_CALLBACK_STANDARD_ACE ACCESS_ALLOWED_CALLBACK_ACE, *PACCESS_ALLOWED_CALLBACK_ACE;
typedef struct _COMMON_CALLBACK_STANDARD_ACE ACCESS_DENIED_CALLBACK_ACE, *PACCESS_DENIED_CALLBACK_ACE;
typedef struct _COMMON_CALLBACK_STANDARD_ACE SYSTEM_AUDIT_CALLBACK_ACE, *PSYSTEM_AUDIT_CALLBACK_ACE;
typedef struct _COMMON_CALLBACK_STANDARD_ACE SYSTEM_ALARM_CALLBACK_ACE, *PSYSTEM_ALARM_CALLBACK_ACE;
#endif

#if 0
#if 1
// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _ACCESS_ALLOWED_CALLBACK_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
    // Callback-specific data follows
} ACCESS_ALLOWED_CALLBACK_OBJECT_ACE, *PACCESS_ALLOWED_CALLBACK_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _ACCESS_DENIED_CALLBACK_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
    // Callback-specific data follows
} ACCESS_DENIED_CALLBACK_OBJECT_ACE, *PACCESS_DENIED_CALLBACK_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _SYSTEM_AUDIT_CALLBACK_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
    // Callback-specific data follows
} SYSTEM_AUDIT_CALLBACK_OBJECT_ACE, *PSYSTEM_AUDIT_CALLBACK_OBJECT_ACE;

// Isomorphic wrt ACCESS_ALLOWED_OBJECT_ACE
typedef struct _SYSTEM_ALARM_CALLBACK_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
    // Callback-specific data follows
} SYSTEM_ALARM_CALLBACK_OBJECT_ACE, *PSYSTEM_ALARM_CALLBACK_OBJECT_ACE;
#else
struct _COMMON_CALLBACK_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    GUID ObjectType;
    GUID InheritedObjectType;
    ULONG SidStart;
};

typedef struct _COMMON_CALLBACK_OBJECT_ACE ACCESS_ALLOWED_CALLBACK_OBJECT_ACE, *PACCESS_ALLOWED_CALLBACK_OBJECT_ACE;
typedef struct _COMMON_CALLBACK_OBJECT_ACE ACCESS_DENIED_CALLBACK_OBJECT_ACE, *PACCESS_DENIED_CALLBACK_OBJECT_ACE;
typedef struct _COMMON_CALLBACK_OBJECT_ACE SYSTEM_AUDIT_CALLBACK_OBJECT_ACE, *PSYSTEM_AUDIT_CALLBACK_OBJECT_ACE;
typedef struct _COMMON_CALLBACK_OBJECT_ACE SYSTEM_ALARM_CALLBACK_OBJECT_ACE, *PSYSTEM_ALARM_CALLBACK_OBJECT_ACE;
#endif
#endif

#if 1
//
// SYSTEM_MANADATOR_LABEL_ACE
//
// This needs additional explanation.  The Mask field is not really an access
// mask.  Rather, it is the access policy for a token with a lower mandatory
// integrity level than the object being protected.
//
// The SID must be an integrity SID (i.e., one of the well-known
// SECURITY_MANDATORY_LABEL_AUTHORITY SIDs) that indicates the integrity
// level of the object.
//

// Isomorphic wrt ACCESS_ALLOWED_ACE
typedef struct _SYSTEM_MANDATORY_LABEL_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} SYSTEM_MANDATORY_LABEL_ACE, *PSYSTEM_MANDATORY_LABEL_ACE;
#else
typedef struct _COMMON_STANDARD_ACE SYSTEM_MANDATORY_LABEL_ACE, *PSYSTEM_MANDATORY_LABEL_ACE;
#endif

// SYSTEM_MANADATOR_LABEL_ACE Mask field values (see above)
#define SYSTEM_MANDATORY_LABEL_NO_WRITE_UP      0x00000001
#define SYSTEM_MANDATORY_LABEL_NO_READ_UP       0x00000002
#define SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP    0x00000004

#define SYSTEM_MANDATORY_LABEL_VALID_MASK ( \
    SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | \
    SYSTEM_MANDATORY_LABEL_NO_READ_UP | \
    SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP | \
    0 )

//
// ACL - Access Control List
//
// In the abstract, an ACL consists of a header followed by 0 or more ACEs.
// The ACL header includes:
//
// - revision
// - ACL size
// - ACE count
//
// The ACL revision should be ACL_REVISION unless ACL contains
// object-specific ACEs (e.g., used by AD), in which case it should be
// ACL_REVISION_DS.
//
// Apparently, the ACL revision values match the corrsponding number
// in ACE type definitions ACCESS_MAX_MS_V<NUMBER>_ACE_TYPE.
//
// The ACL size is the total size of the ACL in bytes including the ACL
// header and all ACE structures.
//

// ISSUE-What happens if a client/server sees a newer revision?  In particular,
// how do the MIC ACEs affect ACL_REVISION level ACLs?

// An ACL is opaque.
#ifndef _DCE_IDL_
typedef struct _ACL *PACL;
#endif

#define ACL_REVISION        2 // For file ACLs
#define ACL_REVISION_DS     4 // For DS ACLs

#define ACL_HEADER_SIZE 8
// TODO-Perhaps this should be rounded to nearest ULONG size
#define ACL_MAX_SIZE ((USHORT)-1)

//
// SD - Security Descriptor
//
// In the abstracts, a security descriptor has these elements:
//
// - revision
// - control bits
// - owner
// - group
// - DACL
// - SACL
//
// There are two types of security descriptors:
//
// 1) Absolute Security Descriptor - This is an in-memory representation
//    that includes pointers to other parts (e.g., owner, DACL).
//
// 2) Self-Relative Security Descriptor - This is a serialized
//    representation of the security descriptor.  For compatibility
//    with other platforms, this is little-endian.
//
// The types are declared as distinct opaque types.
//

// Security descriptors are opaque.
#ifdef _DCE_IDL_
typedef PUCHAR PSECURITY_DESCRIPTOR_ABSOLUTE;
#else
typedef struct _SECURITY_DESCRIPTOR_ABSOLUTE *PSECURITY_DESCRIPTOR_ABSOLUTE;
typedef struct _SECURITY_DESCRIPTOR_RELATIVE *PSECURITY_DESCRIPTOR_RELATIVE;
#endif

#define SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE (5 * sizeof(PVOID))
#define SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE (5 * sizeof(ULONG))
// Maximum for a revision 1 security descriptor
#define SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE (SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE + 2 * SID_MAX_SIZE + 2 * ACL_MAX_SIZE)

#define SECURITY_DESCRIPTOR_REVISION    1

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

#define SE_OWNER_DEFAULTED          0x0001
#define SE_GROUP_DEFAULTED          0x0002
#define SE_DACL_PRESENT             0x0004
#define SE_DACL_DEFAULTED           0x0008
#define SE_SACL_PRESENT             0x0010
#define SE_SACL_DEFAULTED           0x0020
#define SE_DACL_UNTRUSTED           0x0040
#define SE_SERVER_SECURITY          0x0080
#define SE_DACL_AUTO_INHERIT_REQ    0x0100
#define SE_SACL_AUTO_INHERIT_REQ    0x0200
#define SE_DACL_AUTO_INHERITED      0x0400
#define SE_SACL_AUTO_INHERITED      0x0800
#define SE_DACL_PROTECTED           0x1000
#define SE_SACL_PROTECTED           0x2000
#define SE_RM_CONTROL_VALID         0x4000 // Sbz1 contains RM-specific bits
#define SE_SELF_RELATIVE            0x8000

// NOTE: All control bits are defined.
#define SE_VALID_SECURITY_DESCRIPTOR_CONTROL_MASK   0xFFFF
#define SE_SET_SECURITY_DESCRIPTOR_CONTROL_MASK ( \
    SE_DACL_UNTRUSTED | \
    SE_SERVER_SECURITY | \
    SE_DACL_AUTO_INHERIT_REQ | \
    SE_SACL_AUTO_INHERIT_REQ | \
    SE_DACL_AUTO_INHERITED | \
    SE_SACL_AUTO_INHERITED | \
    SE_DACL_PROTECTED | \
    SE_SACL_PROTECTED | \
    0 )

typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;

#define OWNER_SECURITY_INFORMATION              0x00000001
#define GROUP_SECURITY_INFORMATION              0x00000002
#define DACL_SECURITY_INFORMATION               0x00000004
#define SACL_SECURITY_INFORMATION               0x00000008
#if 0
#define LABEL_SECURITY_INFORMATION              0x00000010
#define UNPROTECTED_SACL_SECURITY_INFORMATION   0x10000000
#define UNPROTECTED_DACL_SECURITY_INFORMATION   0x20000000
#define PROTECTED_SACL_SECURITY_INFORMATION     0x40000000
#define PROTECTED_DACL_SECURITY_INFORMATION     0x80000000
#endif

#define VALID_SECURITY_INFORMATION_MASK ( \
    OWNER_SECURITY_INFORMATION | \
    GROUP_SECURITY_INFORMATION | \
    SACL_SECURITY_INFORMATION | \
    DACL_SECURITY_INFORMATION | \
    0 )

//
// RtlCreatePrivateObjectSecurity() AutoInheritFlags
//

#define SEF_DACL_AUTO_INHERIT                   0x00000001
#define SEF_SACL_AUTO_INHERIT                   0x00000002
#define SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT       0x00000004
#define SEF_AVOID_PRIVILEGE_CHECK               0x00000008
#define SEF_AVOID_OWNER_CHECK                   0x00000010
#define SEF_DEFAULT_OWNER_FROM_PARENT           0X00000020
#define SEF_DEFAULT_GROUP_FROM_PARENT           0X00000040
#define SEF_MACL_NO_WRITE_UP                    0X00000100  // unused
#define SEF_MACL_NO_READ_UP                     0X00000200  // unused
#define SEF_AVOID_OWNER_RESTRICTION             0x00001000

//
// Privileges
//

#define SE_AUDIT_PRIVILEGE                 0x00000015
#define SE_BACKUP_PRIVILEGE                0x00000011
#define SE_CHANGE_NOTIFY_PRIVILEGE         0x00000017
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE  0x00000023
#define SE_LOAD_DRIVER_PRIVILEGE           0x0000000a
#define SE_MACHINE_ACCOUNT_PRIVILEGE       0x00000006
#define SE_MANAGE_VOLUME_PRIVILEGE         0x0000001c
#define SE_REMOTE_SHUTDOWN_PRIVILEGE       0x00000018
#define SE_RESTORE_PRIVILEGE               0x00000012
#define SE_SECURITY_PRIVILEGE              0x00000008
#define SE_SHUTDOWN_PRIVILEGE              0x00000013
#define SE_SYSTEM_TIME_PRIVILEGE           0x0000000c
#define SE_TAKE_OWNERSHIP_PRIVILEGE        0x00000009
#define SE_TCB_PRIVILEGE                   0x00000007
#define SE_TIME_ZONE_PRIVILEGE             0x00000022

#define SE_AUDIT_PRIVILEGE_LUID                 { SE_AUDIT_PRIVILEGE, 0 }
#define SE_BACKUP_PRIVILEGE_LUID                { SE_BACKUP_PRIVILEGE, 0 }
#define SE_CHANGE_NOTIFY_PRIVILEGE_LUID         { SE_CHANGE_NOTIFY_PRIVILEGE, 0 }
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE_LUID  { SE_CREATE_SYMBOLIC_LINK_PRIVILEGE, 0 }
#define SE_LOAD_DRIVER_PRIVILEGE_LUID           { SE_LOAD_DRIVER_PRIVILEGE, 0 }
#define SE_MACHINE_ACCOUNT_PRIVILEGE_LUID       { SE_MACHINE_ACCOUNT_PRIVILEGE, 0 }
#define SE_MANAGE_VOLUME_PRIVILEGE_LUID         { SE_MANAGE_VOLUME_PRIVILEGE, 0 }
#define SE_REMOTE_SHUTDOWN_PRIVILEGE_LUID       { SE_REMOTE_SHUTDOWN_PRIVILEGE, 0 }
#define SE_RESTORE_PRIVILEGE_LUID               { SE_RESTORE_PRIVILEGE, 0 }
#define SE_SECURITY_PRIVILEGE_LUID              { SE_SECURITY_PRIVILEGE, 0 }
#define SE_SHUTDOWN_PRIVILEGE_LUID              { SE_SHUTDOWN_PRIVILEGE, 0 }
#define SE_SYSTEM_TIME_PRIVILEGE_LUID           { SE_SYSTEM_TIME_PRIVILEGE }
#define SE_TAKE_OWNERSHIP_PRIVILEGE_LUID        { SE_TAKE_OWNERSHIP_PRIVILEGE, 0 }
#define SE_TCB_PRIVILEGE_LUID                   { SE_TCB_PRIVILEGE, 0 }
#define SE_TIME_ZONE_PRIVILEGE_LUID             { SE_TIME_ZONE_PRIVILEGE, 0 }


typedef struct _LUID_AND_ATTRIBUTES {
    LUID Luid;
    ULONG Attributes;
} LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;

#define SE_PRIVILEGE_ENABLED_BY_DEFAULT   0x00000001
#define SE_PRIVILEGE_ENABLED              0x00000002
#define SE_PRIVILEGE_REMOVED              0x00000004
#define SE_PRIVILEGE_USED_FOR_ACCESS      0x80000000

#define _PRIVILEGE_SET_MAX_PRIVILEGE_COUNT 1000

#define PRIVILEGE_SET_ALL_NECESSARY       0x00000001

typedef struct _PRIVILEGE_SET {
#ifdef _DCE_IDL_
    [range(0,_PRIVILEGE_SET_MAX_PRIVILEGE_COUNT)]
#endif
    ULONG PrivilegeCount;
    ULONG Control;
#ifdef _DCE_IDL_
    [size_is(PrivilegeCount)]
#endif
    LUID_AND_ATTRIBUTES Privilege[];
} PRIVILEGE_SET, *PPRIVILEGE_SET;

#define PRIVILEGE_SET_MIN_SIZE \
    (LW_FIELD_OFFSET(PRIVILEGE_SET, Privilege))

#define _PRIVILEGE_SET_GET_SIZE_REQUIRED(PrivilegeCount) \
    (PRIVILEGE_SET_MIN_SIZE + \
     (LW_FIELD_SIZE(PRIVILEGE_SET, Privilege[0]) * (PrivilegeCount)))

#define _PRIVILEGE_SET_MAX_SIZE \
    _SID_GET_SIZE_REQUIRED(_PRIVILEGE_SET_MAX_PRIVILEGE_COUNT)


//
// Access Token
//
// The underlying (non-handle) type for an access token is PACCESS_TOKEN.
// It is opqaue.
//
// Note that access tokens are not serializable via DCE RPC.
//

#ifndef _DCE_IDL_

typedef struct _ACCESS_TOKEN *PACCESS_TOKEN;

typedef struct _ACCESS_TOKEN_SELF_RELATIVE *PACCESS_TOKEN_SELF_RELATIVE;

typedef ULONG TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

#define TokenNone           0 // sentinel
#define TokenUser           1
#define TokenGroups         2
#define TokenPrivileges     3
#define TokenOwner          4
#define TokenPrimaryGroup   5
#define TokenDefaultDacl    6
#define TokenSource         7 // not implemented
#define TokenInvalid        8 // sentinel

#if 0
#define TOKEN_INFORMATION_CLASS_NONE            0 // sentinel
#define TOKEN_INFORMATION_CLASS_User            1
#define TOKEN_INFORMATION_CLASS_Groups          2
#define TOKEN_INFORMATION_CLASS_Privileges      3
#define TOKEN_INFORMATION_CLASS_Owner           4
#define TOKEN_INFORMATION_CLASS_PrimaryGroup    5
#define TOKEN_INFORMATION_CLASS_DefaultDacl     6
#define TOKEN_INFORMATION_CLASS_Invalid         7 // sentinel
#endif

// User represented by the token.
typedef struct _TOKEN_USER {
    SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

// Groups for the user.
typedef struct _TOKEN_GROUPS {
    ULONG GroupCount;
    SID_AND_ATTRIBUTES Groups[];
} TOKEN_GROUPS, *PTOKEN_GROUPS;

// Privileges for the user
typedef struct _TOKEN_PRIVILEGES {
    ULONG PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

// Default owner for created objects.
typedef struct _TOKEN_OWNER {
    PSID Owner;
} TOKEN_OWNER, *PTOKEN_OWNER;

// Default primary group for created objects.
typedef struct _TOKEN_PRIMARY_GROUP {
    PSID PrimaryGroup;
} TOKEN_PRIMARY_GROUP, *PTOKEN_PRIMARY_GROUP;

// Default DACL for created objects.
typedef struct _TOKEN_DEFAULT_DACL {
    PACL DefaultDacl;
} TOKEN_DEFAULT_DACL, *PTOKEN_DEFAULT_DACL;

// TODO-TOKEN_SOURCE?

typedef struct _TOKEN_UNIX {
    ULONG Uid;
    ULONG Gid;
    ULONG Umask;
} TOKEN_UNIX, *PTOKEN_UNIX;

#else

typedef void *PACCESS_TOKEN;

#endif // _DCE_IDL_

//
// Well-Known SID Types
//

typedef ULONG WELL_KNOWN_SID_TYPE, *PWELL_KNOWN_SID_TYPE;

#define WinNullSid                                   0
#define WinWorldSid                                  1
#define WinLocalSid                                  2
#define WinCreatorOwnerSid                           3
#define WinCreatorGroupSid                           4
#define WinCreatorOwnerServerSid                     5
#define WinCreatorGroupServerSid                     6
#define WinNtAuthoritySid                            7
#define WinDialupSid                                 8
#define WinNetworkSid                                9
#define WinBatchSid                                  10
#define WinInteractiveSid                            11
#define WinServiceSid                                12
#define WinAnonymousSid                              13
#define WinProxySid                                  14
#define WinEnterpriseControllersSid                  15
#define WinSelfSid                                   16
#define WinAuthenticatedUserSid                      17
#define WinRestrictedCodeSid                         18
#define WinTerminalServerSid                         19
#define WinRemoteLogonIdSid                          20
#define WinLogonIdsSid                               21
#define WinLocalSystemSid                            22
#define WinLocalServiceSid                           23
#define WinNetworkServiceSid                         24
#define WinBuiltinDomainSid                          25
#define WinBuiltinAdministratorsSid                  26
#define WinBuiltinUsersSid                           27
#define WinBuiltinGuestsSid                          28
#define WinBuiltinPowerUsersSid                      29
#define WinBuiltinAccountOperatorsSid                30
#define WinBuiltinSystemOperatorsSid                 31
#define WinBuiltinPrintOperatorsSid                  32
#define WinBuiltinBackupOperatorsSid                 33
#define WinBuiltinReplicatorSid                      34
#define WinBuiltinPreWindows2000CompatibleAccessSid  35
#define WinBuiltinRemoteDesktopUsersSid              36
#define WinBuiltinNetworkConfigurationOperatorsSid   37
#define WinAccountAdministratorSid                   38
#define WinAccountGuestSid                           39
#define WinAccountKrbtgtSid                          40
#define WinAccountDomainAdminsSid                    41
#define WinAccountDomainUsersSid                     42
#define WinAccountDomainGuestsSid                    43
#define WinAccountComputersSid                       44
#define WinAccountControllersSid                     45
#define WinAccountCertAdminsSid                      46
#define WinAccountSchemaAdminsSid                    47
#define WinAccountEnterpriseAdminsSid                48
#define WinAccountPolicyAdminsSid                    49
#define WinAccountRasAndIasServersSid                50
#define WinNTLMAuthenticationSid                     51
#define WinDigestAuthenticationSid                   52
#define WinSChannelAuthenticationSid                 53
#define WinThisOrganizationSid                       54
#define WinOtherOrganizationSid                      55
#define WinBuiltinIncomingForestTrustBuildersSid     56
#define WinBuiltinPerfMonitoringUsersSid             57
#define WinBuiltinPerfLoggingUsersSid                58
#define WinBuiltinAuthorizationAccessSid             59
#define WinBuiltinTerminalServerLicenseServersSid    60
#define WinBuiltinDCOMUsersSid                       61
#define WinBuiltinIUsersSid                          62
#define WinIUserSid                                  63
#define WinBuiltinCryptoOperatorsSid                 64
#define WinUntrustedLabelSid                         65
#define WinLowLabelSid                               66
#define WinMediumLabelSid                            67
#define WinHighLabelSid                              68
#define WinSystemLabelSid                            69
#define WinWriteRestrictedCodeSid                    70
#define WinCreatorOwnerRightsSid                     71
#define WinCacheablePrincipalsGroupSid               72
#define WinNonCacheablePrincipalsGroupSid            73
#define WinEnterpriseReadonlyControllersSid          74
#define WinAccountReadonlyControllersSid             75
#define WinBuiltinEventLogReadersGroup               76
#define WinNewEnterpriseReadonlyControllersSid       77
#define WinBuiltinCertSvcDComAccessGroup             78

#if 0
// Alternative to WELL_KNOWN_SID_TYPE's Win<Type>Sid values
#define WELL_KNOWN_SID_TYPE_NULL                                        0
#define WELL_KNOWN_SID_TYPE_WORLD                                       1
#define WELL_KNOWN_SID_TYPE_LOCAL                                       2
#define WELL_KNOWN_SID_TYPE_CREATOR_OWNER                               3
#define WELL_KNOWN_SID_TYPE_CREATOR_GROUP                               4
#define WELL_KNOWN_SID_TYPE_CREATOR_OWNER_SERVER                        5
#define WELL_KNOWN_SID_TYPE_CREATOR_GROUP_SERVER                        6
#define WELL_KNOWN_SID_TYPE_NT_AUTHORITY                                7
#define WELL_KNOWN_SID_TYPE_DIALUP                                      8
#define WELL_KNOWN_SID_TYPE_NETWORK                                     9
#define WELL_KNOWN_SID_TYPE_BATCH                                       10
#define WELL_KNOWN_SID_TYPE_INTERACTIVE                                 11
#define WELL_KNOWN_SID_TYPE_SERVICE                                     12
#define WELL_KNOWN_SID_TYPE_ANONYMOUS                                   13
#define WELL_KNOWN_SID_TYPE_PROXY                                       14
#define WELL_KNOWN_SID_TYPE_ENTERPRISE_CONTROLLERS                      15
#define WELL_KNOWN_SID_TYPE_SELF                                        16
#define WELL_KNOWN_SID_TYPE_AUTHENTICATED_USER                          17
#define WELL_KNOWN_SID_TYPE_RESTRICTED_CODE                             18
#define WELL_KNOWN_SID_TYPE_TERMINAL_SERVER                             19
#define WELL_KNOWN_SID_TYPE_REMOTE_LOGON_ID                             20
#define WELL_KNOWN_SID_TYPE_LOGON_IDS                                   21
#define WELL_KNOWN_SID_TYPE_LOCAL_SYSTEM                                22
#define WELL_KNOWN_SID_TYPE_LOCAL_SERVICE                               23
#define WELL_KNOWN_SID_TYPE_NETWORK_SERVICE                             24
#define WELL_KNOWN_SID_TYPE_BUILTIN_DOMAIN                              25
#define WELL_KNOWN_SID_TYPE_BUILTIN_ADMINISTRATORS                      26
#define WELL_KNOWN_SID_TYPE_BUILTIN_USERS                               27
#define WELL_KNOWN_SID_TYPE_BUILTIN_GUESTS                              28
#define WELL_KNOWN_SID_TYPE_BUILTIN_POWER_USERS                         29
#define WELL_KNOWN_SID_TYPE_BUILTIN_ACCOUNT_OPERATORS                   30
#define WELL_KNOWN_SID_TYPE_BUILTIN_SYSTEM_OPERATORS                    31
#define WELL_KNOWN_SID_TYPE_BUILTIN_PRINT_OPERATORS                     32
#define WELL_KNOWN_SID_TYPE_BUILTIN_BACKUP_OPERATORS                    33
#define WELL_KNOWN_SID_TYPE_BUILTIN_REPLICATOR                          34
#define WELL_KNOWN_SID_TYPE_BUILTIN_PREWINDOWS2000_COMPATIBLE_ACCESS    35
#define WELL_KNOWN_SID_TYPE_BUILTIN_REMOTE_DESKTOP_USERS                36
#define WELL_KNOWN_SID_TYPE_BUILTIN_NETWORK_CONFIGURATION_OPERATORS     37
#define WELL_KNOWN_SID_TYPE_ACCOUNT_ADMINISTRATOR                       38
#define WELL_KNOWN_SID_TYPE_ACCOUNT_GUEST                               39
#define WELL_KNOWN_SID_TYPE_ACCOUNT_KRBTGT                              40
#define WELL_KNOWN_SID_TYPE_ACCOUNT_DOMAIN_ADMINS                       41
#define WELL_KNOWN_SID_TYPE_ACCOUNT_DOMAIN_USERS                        42
#define WELL_KNOWN_SID_TYPE_ACCOUNT_DOMAIN_GUESTS                       43
#define WELL_KNOWN_SID_TYPE_ACCOUNT_COMPUTERS                           44
#define WELL_KNOWN_SID_TYPE_ACCOUNT_CONTROLLERS                         45
#define WELL_KNOWN_SID_TYPE_ACCOUNT_CERT_ADMINS                         46
#define WELL_KNOWN_SID_TYPE_ACCOUNT_SCHEMA_ADMINS                       47
#define WELL_KNOWN_SID_TYPE_ACCOUNT_ENTERPRISE_ADMINS                   48
#define WELL_KNOWN_SID_TYPE_ACCOUNT_POLICY_ADMINS                       49
#define WELL_KNOWN_SID_TYPE_ACCOUNT_RAS_AND_IAS_SERVERS                 50
#define WELL_KNOWN_SID_TYPE_NTLM_AUTHENTICATION                         51
#define WELL_KNOWN_SID_TYPE_DIGEST_AUTHENTICATION                       52
#define WELL_KNOWN_SID_TYPE_SCHANNEL_AUTHENTICATION                     53
#define WELL_KNOWN_SID_TYPE_THIS_ORGANIZATION                           54
#define WELL_KNOWN_SID_TYPE_OTHER_ORGANIZATION                          55
#define WELL_KNOWN_SID_TYPE_BUILTIN_INCOMING_FOREST_TRUST_BUILDERS      56
#define WELL_KNOWN_SID_TYPE_BUILTIN_PERF_MONITORING_USERS               57
#define WELL_KNOWN_SID_TYPE_BUILTIN_PERF_LOGGING_USERS                  58
#define WELL_KNOWN_SID_TYPE_BUILTIN_AUTHORIZATION_ACCESS                59
#define WELL_KNOWN_SID_TYPE_BUILTIN_TERMINAL_SERVER_LICENSE_SERVERS     60
#define WELL_KNOWN_SID_TYPE_BUILTIN_DCOM_USERS                          61
#define WELL_KNOWN_SID_TYPE_BUILTIN_IUSERS                              62
#define WELL_KNOWN_SID_TYPE_IUSER                                       63
#define WELL_KNOWN_SID_TYPE_BUILTIN_CRYPTO_OPERATORS                    64
#define WELL_KNOWN_SID_TYPE_UNTRUSTED_LABEL                             65
#define WELL_KNOWN_SID_TYPE_LOW_LABEL                                   66
#define WELL_KNOWN_SID_TYPE_MEDIUM_LABEL                                67
#define WELL_KNOWN_SID_TYPE_HIGH_LABEL                                  68
#define WELL_KNOWN_SID_TYPE_SYSTEM_LABEL                                69
#define WELL_KNOWN_SID_TYPE_WRITE_RESTRICTED_CODE                       70
#define WELL_KNOWN_SID_TYPE_CREATOR_OWNER_RIGHTS                        71
#define WELL_KNOWN_SID_TYPE_CACHEABLE_PRINCIPALS_GROUP                  72
#define WELL_KNOWN_SID_TYPE_NON_CACHEABLE_PRINCIPALS_GROUP              73
#define WELL_KNOWN_SID_TYPE_ENTERPRISE_READONLY_CONTROLLERS             74
#define WELL_KNOWN_SID_TYPE_ACCOUNT_READONLY_CONTROLLERS                75
#define WELL_KNOWN_SID_TYPE_BUILTIN_EVENT_LOG_READERS_GROUP             76
#define WELL_KNOWN_SID_TYPE_NEW_ENTERPRISE_READONLY_CONTROLLERS         77
#define WELL_KNOWN_SID_TYPE_BUILTIN_CERT_SVC_DCOM_ACCESS_GROUP          78
#endif

//
// Security Impersonation Levels
//
// Anonymous - Server cannot identity or impersonate client.
//
// Identification - Server can identify (effectively getting token info)
//     but cannot impersonate the client.
//
// Impersonation - Server can impersonate the client locally (at the server).
//
// Delegation - Server can impersonate the client locally (at the server) and
//     over the network (at other servers).   (Supported by Win2K and up.)
//

typedef USHORT SECURITY_IMPERSONATION_LEVEL, *PSECURITY_IMPERSONATION_LEVEL;

#define SecurityAnonymous       0
#define SecurityIdentification  1
#define SecurityImpersonation   2
#define SecurityDelegation      3

#if 0
// Alternative to SECURITY_IMPERSONATION_LEVEL's Security<Impersonation> values
#define SECURITY_IMPERSONATION_LEVEL_ANONYMOUS      0
#define SECURITY_IMPERSONATION_LEVEL_IDENTIFICATION 1
#define SECURITY_IMPERSONATION_LEVEL_IMPERSONATION  2
#define SECURITY_IMPERSONATION_LEVEL_DELEGATION     3
#endif

//
// Security Context Tracking Mode
//
// Static - Security context is captured by the server and remains static.
//
// Dynamic - Security context is captured by the server and changes
//     if it is changed on the client.  Support for this depends on
//     the communications channel used.  If not supported, the
//     behavior is the same as static.
//

typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;

#define SECURITY_STATIC_TRACKING    FALSE
#define SECURITY_DYNAMIC_TRACKING   TRUE

//
// Security Quality of Service (QOS)
//
// This is used by a client to specify how/whether the server should
// be able to impersonate the client.
//
// Length - Should be size of structure (sizeof(SECURITY_QUALITY_OF_SERVICE)).
//
// ImpersonationLevel - See SECURITY_IMPERSONATION_LEVEL.
//
// ContextTrackingMode - See SECURITY_CONTEXT_TRACKING_MODE.
//     if it is changed on the client.  Support for this depends on
//     the communications channel used.  If not supported, the
//     behavior is the same as static.
//
// EffectiveOnly - Whether the server can enable/disable client privileges.
//     If TRUE, the server cannot enable/disable privileges and only
//     sees the current privilege set of the client.
//

typedef struct _SECURITY_QUALITY_OF_SERVICE {
    ULONG Length;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode;
    BOOLEAN EffectiveOnly;
} SECURITY_QUALITY_OF_SERVICE, *PSECURITY_QUALITY_OF_SERVICE;

#ifdef _DCE_IDL_

cpp_quote("#endif")

#endif

#endif /* __LWBASE_SECURITY_TYPES_H__ */
