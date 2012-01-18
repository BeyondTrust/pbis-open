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
 *        security-api2.h
 *
 * Abstract:
 *
 *        Base Security API - Internal Functions (for now).
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LW_SECURITY_API2_H__
#define __LW_SECURITY_API2_H__

#include <lw/security-types.h>
#include <lw/ntstatus.h>

//
// ACE Functions
//

// TODO-Use #define for this
USHORT
RtlLengthAccessAuditAce(
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

NTSTATUS
RtlInitializeAccessAllowedAce(
    OUT PACCESS_ALLOWED_ACE Ace,
    IN ULONG AceLength,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );
///<
/// Initialize an ACCESS_ALLOWED_ACE buffer.
///
/// This function initializes an ACCESS_ALLOWED_ACE buffer.
///
/// @param[out] Ace - Buffer to initialize.
/// @param[in] AceLength - Length of buffer.
/// @param[in] AceFlags - Flags for ACE.
/// @param[in] AccessMask - Access mask for ACE.
/// @param[in] Sid - SID for ACE.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER
///   @arg STATUS_BUFFER_TOO_SMALL
///

NTSTATUS
RtlInitializeAccessDeniedAce(
    OUT PACCESS_DENIED_ACE Ace,
    IN ULONG AceLength,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );
///<
/// Initialize an ACCESS_DENIED_ACE buffer.
///
/// This function initializes an ACCESS_DENIED_ACE buffer.
///
/// @param[out] Ace - Buffer to initialize.
/// @param[in] AceLength - Length of buffer.
/// @param[in] AceFlags - Flags for ACE.
/// @param[in] AccessMask - Access mask for ACE.
/// @param[in] Sid - SID for ACE.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER
///   @arg STATUS_BUFFER_TOO_SMALL
///

NTSTATUS
RtlInitializeSystemAuditAce(
    OUT PSYSTEM_AUDIT_ACE Ace,
    IN ULONG AceLength,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    );
///<
/// Initialize an SYSTEM_AUDIT_ACE buffer.
///
/// This function initializes an SYSTEM_AUDIT_ACE buffer.
///
/// @param[out] Ace - Buffer to initialize.
/// @param[in] AceLength - Length of buffer.
/// @param[in] AceFlags - Flags for ACE.
/// @param[in] AccessMask - Access mask for ACE.
/// @param[in] Sid - SID for ACE.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER
///   @arg STATUS_BUFFER_TOO_SMALL
///

//
// ACL Functions
//

NTSTATUS
RtlInitializeAcl(
    OUT PACL Acl,
    OUT OPTIONAL PUSHORT AclSizeUsed,
    IN ULONG AclLength,
    IN ULONG AclRevision
    );
///<
/// Initialize an ACL buffer.
///
/// This function initializes an ACL buffer.
///
/// @param[out] Acl - Buffer to initialize.
/// @param[out_opt] AclSizeUsed - Optionally returns how many bytes
///     are used so far in the ACL (for the header).  This is useful
///     for the ACE insertion/removal functions.
/// @param[in] AclLength - Length of ACL buffer.
/// @param[in] AclRevision - Revision for ACL.  Should be ACL_REVISION for
///    most cases.  The only exception is for securing AD objects, in which
///    case it should be ACL_REVISION_DS.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER
///   @arg STATUS_BUFFER_TOO_SMALL
///

#define ACL_END_ACE_OFFSET ((USHORT)-1)

NTSTATUS
RtlInsertAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN PACE_HEADER Ace
    );
///<
/// Insert an ACE into an ACL.
///
/// This function insert an ACE into an ACL at the specified offset.
///
/// Note that is it up to the caller to ensure that the offset
/// is correct.
///
/// @param[inout] Acl - Pointer to an ACL.
/// @param[inout] AclSizeUsed - On input, specifies how much of the space
///     in the ACL is used.  On output, receives the new value with the
///     new ACE size added.
/// @param[in] AceOffset - Offset of ACE past ACL header.
///    Use 0 for first position, ACL_END_ACE_OFFSET to append to the end.
/// @param[in] Ace - Pointer to ACE to insert.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if size used or offset is bad
///   @arg STATUS_BUFFER_TOO_SMALL if not enough space in ACL
///

NTSTATUS
RtlInsertAccessAllowedAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    OUT OPTIONAL PACCESS_ALLOWED_ACE* Ace
    );
///<
/// Construct and insert an ACCESS_ALLOWED_ACE into an ACL.
///
/// This function construct an ACCESS_ALLOWED_ACE and inserts it into
/// an ACL at the specified offset.
///
/// Note that is it up to the caller to ensure that the offset
/// is correct.
///
/// @param[inout] Acl - Pointer to an ACL.
/// @param[inout] AclSizeUsed - On input, specifies how much of the space
///     in the ACL is used.  On output, receives the new value with the
///     new ACE size added.
/// @param[in] AceOffset - Offset of ACE past ACL header.
///    Use 0 for first position, ACL_END_ACE_OFFSET to append to the end.
/// @param[in] AceFlags - Flags for ACE.
/// @param[in] AccessMask - Access mask for ACE.
/// @param[in] Sid - SID for ACE.
/// @param[out_opt] Ace - Returns pointer to ACE inside the ACL.  NULL on error.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if size used or offset is bad
///   @arg STATUS_BUFFER_TOO_SMALL if not enough space in ACL
///

NTSTATUS
RtlInsertAccessDeniedAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    OUT OPTIONAL PACCESS_DENIED_ACE* Ace
    );
///<
/// Construct and insert an ACCESS_DENIED_ACE into an ACL.
///
/// This function construct an ACCESS_DENIED_ACE and inserts it into
/// an ACL at the specified offset.
///
/// Note that is it up to the caller to ensure that the offset
/// is correct.
///
/// @param[inout] Acl - Pointer to an ACL.
/// @param[inout] AclSizeUsed - On input, specifies how much of the space
///     in the ACL is used.  On output, receives the new value with the
///     new ACE size added.
/// @param[in] AceOffset - Offset of ACE past ACL header.
///    Use 0 for first position, ACL_END_ACE_OFFSET to append to the end.
/// @param[in] AceFlags - Flags for ACE.
/// @param[in] AccessMask - Access mask for ACE.
/// @param[in] Sid - SID for ACE.
/// @param[out_opt] Ace - Returns pointer to ACE inside the ACL.  NULL on error.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if size used or offset is bad
///   @arg STATUS_BUFFER_TOO_SMALL if not enough space in ACL
///

NTSTATUS
RtlRemoveAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset
    );
///<
/// Remove an ACE from an ACL.
///
/// This removes an ACE from an ACL at the specified offset.
///
/// Note that is it up to the caller to ensure that the offset
/// is correct.
///
/// @param[inout] Acl - Pointer to an ACL.
/// @param[inout] AclSizeUsed - On input, specifies how much of the space
///     in the ACL is used.  On output, receives the new value with the
///     new ACE size added.
/// @param[in] AceOffset - Offset of ACE past ACL header.
///    Use 0 for first position.  For remove, ACL_END_ACE_OFFSET is invalid.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if size used or offset is bad
///

NTSTATUS
RtlIterateAce(
    IN PACL Acl,
    IN USHORT AclSizeUsed,
    IN OUT PUSHORT AceOffset,
    OUT PACE_HEADER* AceHeader
    );
///<
/// ACL accessor function to get an ACEs in an ACL.
///
/// This function gets an ACEs in an ACL.  It is normally used to iterate
/// over an ACL.  The AceOffset should be 0 to get the first ACE.  To get
/// the next ACE, the AceOffset should be the previous offset (e.g., 0),
/// plus the AceSize in the returned AceHeader.
///
/// Note that is it up to the caller to ensure that the offset
/// is correct.
///
/// @param[in] Acl - Pointer to an ACL.
/// @param[in] AclSizeUsed - Specifies how much of the space in the ACL
///     is used.
/// @param[inout] AceOffset - Offset of ACE past ACL header.  Returns
///     offset of next ACE.  If no ACE is next, returns ACL_END_ACE_OFFSET.
/// @param[out] AceHeader - Returns pointer to ACE header at given offset.
///
/// @return NTSTATUS
///   @arg STATUS_SUCCESS on success
///   @arg STATUS_INVALID_PARAMETER if offset is bad.
///   @arg STATUS_NO_MORE_ENTRIES if AceOffset is ACL_END_ACE_OFFSET.
///

UCHAR
RtlGetAclRevision(
    IN PACL Acl
    );
///<
/// ACL accessor function to get ACL revision.
///
/// This function gets the ACL revision for an ACL.
///
/// @param[in] Acl - Pointer to an ACL.
///
/// @return ACL revision number.
///

USHORT
RtlGetAclSize(
    IN PACL Acl
    );
///<
/// ACL accessor function to get ACL size (as stored in ACL header).
///
/// This function gets the ACL size as stored in the ACL header.
///
/// @param[in] Acl - Pointer to an ACL.
///
/// @return ACL size.
///

USHORT
RtlGetAclAceCount(
    IN PACL Acl
    );
///<
/// ACL accessor function to get the number of ACEs in the ACL.
///
/// This function gets the number of ACEs in an ACL.
///
/// @param[in] Acl - Pointer to an ACL.
///
/// @return Number of ACEs in the ACL.
///

USHORT
RtlGetAclSizeUsed(
    IN PACL Acl
    );
///<
/// ACL function to compute ACL size used based on the ACEs in the ACL.
///
/// This function computes how much of the ACL is used based on the ACEs
/// in the ACL.  This number may be smaller than the size of the ACL.
///
/// @param[in] Acl - Pointer to an ACL.
///
/// @return ACL size used.  Zero is returned if there is some inconsistency.
///

BOOLEAN
RtlValidAcl(
    IN PACL Acl,
    OUT OPTIONAL PUSHORT AclSizeUsed
    );
///<
/// ACL function to validate ACL and optionally compute ACL size used
/// based on the ACEs in the ACL.
///
/// This function computes how much of the ACL is used based on the ACEs
/// in the ACL.  This number may be smaller than the size of the ACL.
///
/// @param[in] Acl - Pointer to an ACL.
/// @param[out] AclSize - Optionally returns computed ACL size used based
///     on the ACEs in the ACL.  Returns 0 if the ACL is invalid.
///
/// @return Whether ACL is valid.
///

BOOLEAN
RtlValidAceOffset(
    IN PACL Acl,
    IN USHORT AceOffset
    );
///<
/// ACL function to validate ACL offset.
///
/// This function determines whether a given ACE offset is valid
/// by traversing all the ACEs up to the specified ACE offset.
/// In this context, ACL_END_ACE_OFFSET is invalid.
///
/// @param[in] Acl - Pointer to an ACL.
/// @param[inout] AceOffset - Offset of ACE past ACL header.
///     This must point to the start of an ACE.
///
/// @return TRUE if ACE offset if valid.  FALSE otherwise (including
///     if the ACL is determined to be invalid in the process of
///     validating the offset).
///

#endif /* __LW_SECURITY_API2_H__ */
