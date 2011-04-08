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
 *        security-api.h
 *
 * Abstract:
 *
 *        Base Security API
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LWBASE_SECURITY_API_H__
#define __LWBASE_SECURITY_API_H__

#include <lw/security-types.h>
#include <lw/ntstatus.h>
#include <lw/rtlstring.h>

LW_BEGIN_EXTERN_C

//
// SID Functions
//

NTSTATUS
RtlInitializeSid(
    OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
    );
///<
/// Initialize a SID buffer.
///
/// Initialize SID buffer of sufficiently large size with
/// the relevant identifier authority and sub-authority count.
/// Note that the sub-authority values are not initialized.
///
/// @param[out] Sid - SID buffer
/// @param[in] IdentifierAuthority - Identifier authority
/// @param[in] SubAuthorityCount - Count of sub-authories.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if sub-authority count is invalid.
///

ULONG
RtlLengthRequiredSid(
    IN ULONG SubAuthorityCount
    );
///<
/// Get length required for a SID.
///
/// Get number of bytes required to store a SID with the specified
/// number of sub-authorities.
///
/// @param[in] SubAuthorityCount - Count of sub-authorities.
///
/// @return Number of bytes required to store the SID.
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if sub-authority count is invalid.
///

ULONG
RtlLengthSid(
    IN PSID Sid
    );
///<
/// Get length of a SID.
///
/// Get number of bytes required to store the specified SID.
///
/// @param[in] SID - A SID
///
/// @return Number of bytes required to store the SID.
///

BOOLEAN
RtlValidSid(
    IN PSID Sid
    );

BOOLEAN
RtlEqualSid(
    IN PSID Sid1,
    IN PSID Sid2
    );

BOOLEAN
RtlEqualPrefixSid(
    IN PSID Sid1,
    IN PSID Sid2
    );

BOOLEAN
RtlIsPrefixSid(
    IN PSID Prefix,
    IN PSID Sid
    );

#if 0
PSID_IDENTIFIER_AUTHORITY
RtlIdentifierAuthoritySid(
    IN PSID Sid
    );

PULONG
RtlGetSubAuthorityPointerSid(
    IN PSID Sid,
    IN ULONG SubAuthorityIndex
    );

PUCHAR
RtlGetSubAuthorityCountSid(
    IN PSID Sid
    );
#endif

NTSTATUS
RtlCopySid(
    IN ULONG DestinationSidLength,
    OUT PSID DestinationSid,
    IN PSID SourceSid
    );

NTSTATUS
RtlAppendRidSid(
    IN ULONG SidLength,
    IN OUT PSID Sid,
    IN ULONG Rid
    );

NTSTATUS
RtlGetRidSid(
    OUT PULONG Rid,
    IN PSID Sid
    );

NTSTATUS
RtlDuplicateSid(
    OUT PSID* NewSid,
    IN PSID OriginalSid
    );

NTSTATUS
RtlConvertLittleEndianToSid(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PSID Sid,           /* The pBuffer MAY be used here */
    IN OUT PULONG SidSize
    );

NTSTATUS
RtlConvertSidToLittleEndian(
    IN PSID Sid,
    OUT PVOID Buffer,        /* The pSid MAY be used here */
    IN ULONG Length,
    OUT OPTIONAL PULONG UsedLength
    );

//
// LUID Functions
//

LUID
RtlConvertLongToLuid(
    IN LONG Long
    );
///<
/// Convert LONG type to LUID.
///
/// Assign long integer value to LowPart of the LUID.
///
/// @param[in] Long - long integer value
///
/// @return LUID
///

LUID
RtlConvertUlongToLuid(
    IN ULONG Ulong
    );
///<
/// Convert ULONG type to LUID.
///
/// Assign unsigned long integer value to LowPart of the LUID.
///
/// @param[in] Ulong - unsigned long integer value
///
/// @return LUID
///

BOOLEAN
RtlEqualLuid(
    IN PLUID Luid1,
    IN PLUID Luid2
    );
///<
/// Compare two LUIDs.
///
/// Returns TRUE if both LUIDs represent the same value.
///
/// @param[in] Luid1 - pointer to LUID variable
/// @param[in] Luid2 - pointer to LUID variable
///
/// @return BOOLEAN
///


//
// PRIVILEGE_SET Functions
//

ULONG
RtlLengthRequiredPrivilegeSet(
    IN ULONG PrivilegeCount
    );
///<
/// Get length required for a PRIVILEGE_SET.
///
/// Get number of bytes required to store a PRIVILEGE_SET with
/// the specified number of privileges.
///
/// @param[in] PrivilegeCount - Number of privileges
///
/// @return Number of bytes required to store the PRIVILEGE_SET.
///

ULONG
RtlLengthPrivilegeSet(
    IN PPRIVILEGE_SET PrivilegeSet
    );
///<
/// Get length of a PRIVILEGE_SET.
///
/// Get number of bytes required to store the specified PRIVILEGE_SET.
///
/// @param[in] PrivilegeSet - A privilege set
///
/// @return Number of bytes required to store the PRIVILEGE_SET.
///

NTSTATUS
RtlCopyPrivilegeSet(
    IN ULONG DestinationPrivilegeSetLength,
    OUT PPRIVILEGE_SET DestinationPrivilegeSet,
    IN PPRIVILEGE_SET SourcePrivilegeSet
    );
///<
/// Copy a privilege set.
///
/// Copy a privilege set to allocated buffer of specified length.
///
/// @param[in] DestinationPrivilegeSetLength - Length of the destination
/// PRIVILEGE_SET
/// @param[out] DestinationPrivilegeSet - The PRIVILEGE_SET to copy to
/// @param[in] SourcePrivilegeSet - The PRIVILEGE_SET to copy from
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_BUFFER_TOO_SMALL if the destination buffer length is
///   too small
///


//
// ACL Functions
//

NTSTATUS
RtlCreateAcl(
    OUT PACL Acl,
    IN ULONG AclLength,
    IN ULONG AclRevision
    );

NTSTATUS
RtlAddAce(
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG StartingAceIndex,
    IN PVOID AceList,
    IN ULONG AceListLength
    );

NTSTATUS
RtlDeleteAce(
    IN OUT PACL Acl,
    IN ULONG AceIndex
    );

NTSTATUS
RtlGetAce(
    IN PACL Acl,
    IN ULONG AceIndex,
    OUT PVOID* Ace
    );

USHORT
RtlGetAclAceCount(
    IN PACL Acl
    );

///<
/// Get size required for an ACCESS_ALLOWED_ACE.
///
/// This function gets the number of bytes required for an
/// access allowed ACE (ACCESS_ALLOWED_ACE) given a particular SID.
///
/// @param[in] Sid - SID for ACE.
///
/// @return Number of bytes required or 0 if Sid is invalid.
///
USHORT
RtlLengthAccessAllowedAce(
    IN PSID Sid
    );

///<
/// Get size required for an ACCESS_DENIED_ACE.
///
/// This function gets the number of bytes required for an
/// access denied ACE (ACCESS_DENIED_ACE) given a particular SID.
///
/// @param[in] Sid - SID for ACE.
///
/// @return Number of bytes required or 0 if Sid is invalid.
///
USHORT
RtlLengthAccessDeniedAce(
    IN PSID Sid
    );

///<
/// Get size required for an SYSTEM_AUDIT_ACE.
///
/// This function gets the number of bytes required for an
/// system audit ACE (SYSTEM_AUDIT_ACE) given a particular SID.
///
/// @param[in] Sid - SID for ACE.
///
/// @return Number of bytes required or 0 if Sid is invalid.
///
USHORT
RtlLengthSystemAuditAce(
    IN PSID Sid
    );

NTSTATUS
RtlAddAccessAllowedAceEx(
    IN PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );

NTSTATUS
RtlAddAccessDeniedAceEx(
    IN PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );

NTSTATUS
RtlAddSystemAuditAceEx(
    IN PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );

//
// SD Functions
//

NTSTATUS
RtlCreateSecurityDescriptorAbsolute(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN ULONG Revision
    );

NTSTATUS
RtlCreateSecurityDescriptorRelative(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN ULONG Revision
    );

BOOLEAN
RtlValidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    );

BOOLEAN
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    );

ULONG
RtlLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    );

ULONG
RtlLengthSecurityDescriptorRelative(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor
    );

NTSTATUS
RtlGetSecurityDescriptorControl(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT OPTIONAL PUCHAR Revision
    );

NTSTATUS
RtlSetSecurityDescriptorControl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToChange,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToSet
    );

#if 0
NTSTATUS
RtlGetSecurityDescriptorRmControl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN UCHAR BitsToChange,
    IN UCHAR BitsToSet
    );

NTSTATUS
RtlSetSecurityDescriptorRmControl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN UCHAR BitsToChange,
    IN UCHAR BitsToSet
    );
#endif

// ISSUE-For complete-ness, would need a way to set RM bits.

NTSTATUS
RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Owner,
    OUT PBOOLEAN IsOwnerDefaulted
    );

NTSTATUS
RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Group,
    OUT PBOOLEAN IsGroupDefaulted
    );

NTSTATUS
RtlGetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsSaclPresent,
    OUT PACL* Sacl,
    OUT PBOOLEAN IsSaclDefaulted
    );

NTSTATUS
RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsDaclPresent,
    OUT PACL* Dacl,
    OUT PBOOLEAN IsDaclDefaulted
    );

NTSTATUS
RtlSetOwnerSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID Owner,
    IN BOOLEAN IsOwnerDefaulted
    );

NTSTATUS
RtlSetGroupSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN OPTIONAL PSID Group,
    IN BOOLEAN IsGroupDefaulted
    );

NTSTATUS
RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsSaclPresent,
    IN OPTIONAL PACL Sacl,
    IN OPTIONAL BOOLEAN IsSaclDefaulted
    );

NTSTATUS
RtlSetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsDaclPresent,
    IN OPTIONAL PACL Dacl,
    IN OPTIONAL BOOLEAN IsDaclDefaulted
    );

NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    );

NTSTATUS
RtlAbsoluteToSelfRelativeSDSwab(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN ULONG BufferLength
    );

NTSTATUS
RtlSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    IN OUT PULONG AbsoluteSecurityDescriptorSize,
    OUT OPTIONAL PACL Dacl,
    IN OUT PULONG DaclSize,
    OUT OPTIONAL PACL Sacl,
    IN OUT PULONG SaclSize,
    OUT OPTIONAL PSID Owner,
    IN OUT PULONG OwnerSize,
    OUT OPTIONAL PSID PrimaryGroup,
    IN OUT PULONG PrimaryGroupSize
    );

NTSTATUS
RtlQuerySecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor
    );

NTSTATUS
RtlSetSecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE InputSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE NewObjectSecurityDescriptor,
    IN OUT PULONG NewObjectSecurityDescriptorLength,
    IN PGENERIC_MAPPING GenericMapping
    ); 

NTSTATUS
RtlCreatePrivateObjectSecurityEx(
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pCreatorSecDesc,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppNewSecDesc,
    OUT PULONG pNewSecDescLength,
    IN OPTIONAL PVOID pObjectType,  // Unused
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    );

//
// ACCESS_MASK Functions
//

VOID
RtlMapGenericMask(
    IN OUT PACCESS_MASK AccessMask,
    IN PGENERIC_MAPPING GenericMapping
    );

//
// Access Token Functions
//

// TODO-Do we need an AuthenticationId LUID?
// (Note: Multiple tokens can belong to a single AuthenticationId.  The TokenId
// is what is unique.)
// TODO-Support SID_ATTRIBUTES (SID_AND_ATTRIBUTES) for user/groups.
// (However, note that user cannot have any attributes).
// TODO-Support TOKEN_PRIVILEGES?
// TODO-Support TOKEN_SOURCE?

NTSTATUS
RtlCreateAccessToken(
    OUT PACCESS_TOKEN* AccessToken,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_PRIVILEGES Privileges,
    IN PTOKEN_OWNER Owner,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl,
#if 0
    IN PTOKEN_SOURCE Source,
#endif
    IN OPTIONAL PTOKEN_UNIX Unix
    );

VOID
RtlReferenceAccessToken(
    IN PACCESS_TOKEN AccessToken
    );

VOID
RtlReleaseAccessToken(
    IN OUT PACCESS_TOKEN* AccessToken
    );

NTSTATUS
RtlQueryAccessTokenInformation(
    IN PACCESS_TOKEN AccessToken,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT OPTIONAL PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnedLength
    );

NTSTATUS
RtlQueryAccessTokenUnixInformation(
    IN PACCESS_TOKEN AccessToken,
    OUT PTOKEN_UNIX TokenInformation
    );
///<
/// Query access token for Unix-specific information.
///
/// @param[in] AccessToken - Token to query.
/// @param[out] TokenInformation - Returns Unix token information,
///     if the token contains it.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER
///   @arg STATUS_NOT_FOUND - If token does not have Unix information.
///

BOOLEAN
RtlIsSidMemberOfToken(
    IN PACCESS_TOKEN AccessToken,
    IN PSID Sid
    );

BOOLEAN
RtlAccessCheck(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PACCESS_TOKEN AccessToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
#if 0
    OUT OPTIONAL PPRIVILEGE_SET* Privileges,
#endif
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    );

BOOLEAN
RtlAccessCheckEx(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PACCESS_TOKEN AccessToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
#if 0
    OUT OPTIONAL PPRIVILEGE_SET* Privileges,
#endif
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK RemainingDesiredAccess,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    );


NTSTATUS
RtlAccessTokenToSelfRelativeAccessToken(
    IN PACCESS_TOKEN pToken,
    OUT OPTIONAL PACCESS_TOKEN_SELF_RELATIVE pRelative,
    IN OUT PULONG ulSize
    );

NTSTATUS
RtlSelfRelativeAccessTokenToAccessToken(
    PACCESS_TOKEN_SELF_RELATIVE pRelative,
    ULONG ulRelativeSize,
    PACCESS_TOKEN* ppToken
    );

BOOLEAN
RtlPrivilegeCheck(
   IN OUT PPRIVILEGE_SET RequiredPrivileges,
   IN PACCESS_TOKEN AccessToken
   );
///<
/// Check required privileges.
///
/// Check if RequiredPrivileges are available in AccessToken. If a privilege
/// is in access token and is enabled (SE_PRIVILEGE_ENABLED) it gets
/// SE_PRIVILEGE_USED_FOR_ACCESS flag set in Attributes field.
///
/// @param[in,out] RequiredPrivileges - The required set of privileges to be
/// checked and marked (if found enabled in AccessToken) with
/// SE_PRIVILEGE_USER_FOR_ACCESS
/// @param[in] AccessToken - An access token checked for the required privileges
///
/// @return BOOLEAN result of the privilege check.
///    @arg TRUE if required privileges are enabled in AccessToken
///    @arg FALSE otherwise
///


NTSTATUS
RtlAdjustTokenPrivileges(
    IN PACCESS_TOKEN AccessToken,
    IN BOOLEAN DisableAll,
    IN OPTIONAL PTOKEN_PRIVILEGES NewState,
    IN ULONG BufferLength,
    OUT OPTIONAL PTOKEN_PRIVILEGES PreviousState,
    OUT OPTIONAL PULONG pReturnedLength
    );

//
// SID <-> String Conversion Functions
//

NTSTATUS
RtlAllocateUnicodeStringFromSid(
    OUT PUNICODE_STRING StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateAnsiStringFromSid(
    OUT PANSI_STRING StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateWC16StringFromSid(
    OUT PWSTR* StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateCStringFromSid(
    OUT PSTR* StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateSidFromUnicodeString(
    OUT PSID* Sid,
    IN PUNICODE_STRING StringSid
    );

NTSTATUS
RtlAllocateSidFromAnsiString(
    OUT PSID* Sid,
    IN PANSI_STRING StringSid
    );

NTSTATUS
RtlAllocateSidFromWC16String(
    OUT PSID* Sid,
    IN PCWSTR StringSid
    );

NTSTATUS
RtlAllocateSidFromCString(
    OUT PSID* Sid,
    IN PCSTR StringSid
    );

//
// Well-Known SID Functions
//

NTSTATUS
RtlCreateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN OPTIONAL PSID DomainOrComputerSid,
    OUT OPTIONAL PSID Sid,
    IN OUT PULONG SidSize
    );

NTSTATUS
RtlAllocateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN OPTIONAL PSID DomainOrComputerSid,
    OUT PSID* Sid
    );

//
// SDDL Functions
//

NTSTATUS
RtlAllocateSecurityDescriptorFromSddlCString(
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecurityDescriptor,
    OUT OPTIONAL PULONG pSecurityDescriptorLength,
    IN PCSTR pszStringSecurityDescriptor,
    IN ULONG SddlRevision
    );


NTSTATUS
RtlAllocateSddlCStringFromSecurityDescriptor(
    OUT PSTR* ppszStringSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG SddlRevision,
    IN SECURITY_INFORMATION SecurityInformation
    );

NTSTATUS
RtlGetSecurityInformationFromSddlCString(
    IN PCSTR pszStringSecurityDescriptor,
    OUT SECURITY_INFORMATION* pSecInfo
    );


LW_END_EXTERN_C

#endif /* __LWBASE_SECURITY_API_H__ */
