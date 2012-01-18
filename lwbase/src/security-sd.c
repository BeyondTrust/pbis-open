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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        security-sd.c
 *
 * Abstract:
 *
 *        Security Descriptor (SD) Functions in Security Module.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "security-includes.h"

//
// SD Functions
//

NTSTATUS
RtlCreateSecurityDescriptorAbsolute(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN ULONG Revision
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Revision != SECURITY_DESCRIPTOR_REVISION)
    {
        status = STATUS_UNKNOWN_REVISION;
        GOTO_CLEANUP();
    }

    RtlZeroMemory(SecurityDescriptor, sizeof(*SecurityDescriptor));
    SecurityDescriptor->Revision = Revision;

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

static
NTSTATUS
RtlpVerifySecurityDescriptorHeader(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
        status = STATUS_UNKNOWN_REVISION;
        GOTO_CLEANUP();
    }

    if ((SecurityDescriptor->Sbz1 != 0) &&
        !IsSetFlag(SecurityDescriptor->Control, SE_RM_CONTROL_VALID))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // We never handle self-relative through this function.
    if (IsSetFlag(SecurityDescriptor->Control, SE_SELF_RELATIVE))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (!SecurityDescriptor->Owner &&
        IsSetFlag(SecurityDescriptor->Control, SE_OWNER_DEFAULTED))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (!SecurityDescriptor->Group &&
        IsSetFlag(SecurityDescriptor->Control, SE_GROUP_DEFAULTED))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // If present bit is not set, ACL and defaulted bit cannot be set.
    if (!IsSetFlag(SecurityDescriptor->Control, SE_DACL_PRESENT) &&
        (SecurityDescriptor->Dacl ||
         IsSetFlag(SecurityDescriptor->Control, SE_DACL_DEFAULTED)))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // If present bit is not set, ACL and defaulted bit cannot be set.
    if (!IsSetFlag(SecurityDescriptor->Control, SE_SACL_PRESENT) &&
        (SecurityDescriptor->Sacl ||
         IsSetFlag(SecurityDescriptor->Control, SE_SACL_DEFAULTED)))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // TODO-Add more SD control validity checks.

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

BOOLEAN
RtlValidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    BOOLEAN isValid = FALSE;

    if (!NT_SUCCESS(RtlpVerifySecurityDescriptorHeader(SecurityDescriptor)))
    {
        GOTO_CLEANUP();
    }

    // Check non-control fields

    if (SecurityDescriptor->Owner &&
        !RtlValidSid(SecurityDescriptor->Owner))
    {
        GOTO_CLEANUP();
    }

    if (SecurityDescriptor->Group &&
        !RtlValidSid(SecurityDescriptor->Group))
    {
        GOTO_CLEANUP();
    }
        
    if (SecurityDescriptor->Dacl &&
        !RtlValidAcl(SecurityDescriptor->Dacl, NULL))
    {
        GOTO_CLEANUP();
    }

    if (SecurityDescriptor->Sacl &&
        !RtlValidAcl(SecurityDescriptor->Sacl, NULL))
    {
        GOTO_CLEANUP();
    }

    isValid = TRUE;

cleanup:
    return isValid;
}

typedef BOOLEAN (*RTLP_IS_VALID_BUFFER_CALLBACK)(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

static
NTSTATUS
RtlpVerifyRelativeSecurityDescriptorOffset(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN OUT PULONG SizeUsed,
    IN ULONG Offset,
    IN RTLP_IS_VALID_BUFFER_CALLBACK IsValidBufferCallback
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sizeUsed = *SizeUsed;

    if (Offset)
    {
        ULONG size = 0;

        if (Offset > SecurityDescriptorLength)
        {
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP();
        }

        // Validate data
        if (!IsValidBufferCallback(
                    LW_PTR_ADD(SecurityDescriptor, Offset),
                    SecurityDescriptorLength - Offset,
                    &size))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }

        *SizeUsed = sizeUsed + size;
    }

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

static
NTSTATUS
RtlpVerifyRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_RELATIVE relHeader = { 0 };
    SECURITY_DESCRIPTOR_ABSOLUTE absHeader = { 0 };
    ULONG sizeUsed = 0;

    if (!LW_IS_VALID_FLAGS(RequiredInformation, VALID_SECURITY_INFORMATION_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (SecurityDescriptorLength < sizeof(*SecurityDescriptor))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    relHeader.Revision = LW_LTOH8(SecurityDescriptor->Revision);
    relHeader.Sbz1 = LW_LTOH8(SecurityDescriptor->Sbz1);
    relHeader.Control = LW_LTOH16(SecurityDescriptor->Control);
    relHeader.Owner = LW_LTOH32(SecurityDescriptor->Owner);
    relHeader.Group = LW_LTOH32(SecurityDescriptor->Group);
    relHeader.Sacl = LW_LTOH32(SecurityDescriptor->Sacl);
    relHeader.Dacl = LW_LTOH32(SecurityDescriptor->Dacl);

    //
    // The self-relative bit must be set.
    //

    if (!IsSetFlag(relHeader.Control, SE_SELF_RELATIVE))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    //
    // Check that required information is present.
    //

    if (IsSetFlag(RequiredInformation, OWNER_SECURITY_INFORMATION) &&
        !relHeader.Owner)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (IsSetFlag(RequiredInformation, GROUP_SECURITY_INFORMATION) &&
        !relHeader.Group)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // Note that SACL can be present but NULL.
    if (IsSetFlag(RequiredInformation, SACL_SECURITY_INFORMATION) &&
        !IsSetFlag(relHeader.Control, SE_SACL_PRESENT))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // Note that DACL can be present but NULL.
    if (IsSetFlag(RequiredInformation, DACL_SECURITY_INFORMATION) &&
        !IsSetFlag(relHeader.Control, SE_DACL_PRESENT))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    //
    // Verify header information
    //

    absHeader.Revision = relHeader.Revision;
    absHeader.Sbz1 = relHeader.Sbz1;
    absHeader.Control = relHeader.Control;
    // SID and ACL pointers are to denote existence and must not be dereferenced.
    absHeader.Owner = relHeader.Owner ? (PSID) LW_PTR_ADD(SecurityDescriptor, relHeader.Owner) : NULL;
    absHeader.Group = relHeader.Group ? (PSID) LW_PTR_ADD(SecurityDescriptor, relHeader.Group) : NULL;
    absHeader.Sacl = relHeader.Sacl ? (PACL) LW_PTR_ADD(SecurityDescriptor, relHeader.Sacl) : NULL;
    absHeader.Dacl = relHeader.Dacl ? (PACL) LW_PTR_ADD(SecurityDescriptor, relHeader.Dacl) : NULL;

    // Clear the self-relative flag since it cannot be present in the
    // absolute header.
    ClearFlag(absHeader.Control, SE_SELF_RELATIVE);

    status = RtlpVerifySecurityDescriptorHeader(&absHeader);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // Check validity of offsets.
    //

    sizeUsed = sizeof(*SecurityDescriptor);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Owner,
                    RtlpIsValidLittleEndianSidBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Group,
                    RtlpIsValidLittleEndianSidBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Sacl,
                    RtlpIsValidLittleEndianAclBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Dacl,
                    RtlpIsValidLittleEndianAclBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    if (sizeUsed > SecurityDescriptorLength)
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

BOOLEAN
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )
{
    NTSTATUS status = RtlpVerifyRelativeSecurityDescriptor(
                            SecurityDescriptor,
                            SecurityDescriptorLength,
                            RequiredInformation);
    return NT_SUCCESS(status);
}

ULONG
RtlLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    ULONG size = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;

    if (SecurityDescriptor->Owner)
    {
        size += RtlLengthSid(SecurityDescriptor->Owner);
    }

    if (SecurityDescriptor->Group)
    {
        size += RtlLengthSid(SecurityDescriptor->Group);
    }

    if (SecurityDescriptor->Dacl)
    {
        size += SecurityDescriptor->Dacl->AclSize;
    }

    if (SecurityDescriptor->Sacl)
    {
        size += SecurityDescriptor->Sacl->AclSize;
    }

    return size;
}

ULONG
RtlLengthSecurityDescriptorRelative(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor
    )
{
    ULONG size = SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;

    if (SecurityDescriptor->Owner)
    {
        PSID pOwnerlittleEndianSid = (PSID)LwRtlOffsetToPointer(SecurityDescriptor,
                                                                LW_LTOH32(SecurityDescriptor->Owner));

        size += RtlLengthRequiredSid(LW_LTOH8(pOwnerlittleEndianSid->SubAuthorityCount));
    }

    if (SecurityDescriptor->Group)
    {
        PSID pGrouplittleEndianSid = (PSID)LwRtlOffsetToPointer(SecurityDescriptor,
                                                                 LW_LTOH32(SecurityDescriptor->Group));

        size += RtlLengthRequiredSid(LW_LTOH8(pGrouplittleEndianSid->SubAuthorityCount));
    }

    if (SecurityDescriptor->Dacl)
    {
        PACL plittleEndianDacl = (PACL)LwRtlOffsetToPointer(SecurityDescriptor,
                                                             LW_LTOH32(SecurityDescriptor->Dacl));

        size +=  LW_LTOH16(plittleEndianDacl->AclSize);
    }

    if (SecurityDescriptor->Sacl)
    {
        PACL plittleEndianSacl = (PACL)LwRtlOffsetToPointer(SecurityDescriptor,
                                                            LW_LTOH32(SecurityDescriptor->Sacl));

        size += LW_LTOH16(plittleEndianSacl->AclSize);
    }

    return size;
}

NTSTATUS
RtlGetSecurityDescriptorControl(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT OPTIONAL PUCHAR Revision
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL control = 0;
    UCHAR revision = 0;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    control = SecurityDescriptor->Control;
    revision = SecurityDescriptor->Revision;

cleanup:
    if (!NT_SUCCESS(status))
    {
        control = 0;
        revision = 0;
    }

    if (Control)
    {
        *Control = control;
    }

    if (Revision)
    {
        *Revision = revision;
    }

    return status;
}

NTSTATUS
RtlSetSecurityDescriptorControl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToChange,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToSet
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL bitsToClear = 0;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    // Don't allow changes for bits that should be manually set and
    // Ensure that BitsToSet is a proper subset of BitstoChange

    if (!LW_IS_VALID_FLAGS(BitsToChange, SE_SET_SECURITY_DESCRIPTOR_CONTROL_MASK) ||
        ((BitsToChange | BitsToSet) & ~BitsToChange))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    SetFlag(SecurityDescriptor->Control, BitsToSet);
    bitsToClear = (BitsToChange & ~BitsToSet);
    ClearFlag(SecurityDescriptor->Control, bitsToClear);

cleanup:
    return status;
}

static
NTSTATUS
RtlpGetSidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID* SidLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    OUT PSID* Sid,
    OUT PBOOLEAN IsSidDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    BOOLEAN isSidDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    sid = *SidLocation;
    isSidDefaulted = IsSetFlag(SecurityDescriptor->Control, DefaultedBit);

cleanup:
    if (!NT_SUCCESS(status))
    {
        sid = NULL;
        isSidDefaulted = FALSE;
    }

    *Sid  = sid;
    *IsSidDefaulted = isSidDefaulted;

    return status;
}

NTSTATUS
RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Owner,
    OUT PBOOLEAN IsOwnerDefaulted
    )
{
    return RtlpGetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Owner,
                SE_OWNER_DEFAULTED,
                Owner,
                IsOwnerDefaulted);
}

NTSTATUS
RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Group,
    OUT PBOOLEAN IsGroupDefaulted
    )
{
    return RtlpGetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Group,
                SE_GROUP_DEFAULTED,
                Group,
                IsGroupDefaulted);
}

static
NTSTATUS
RtlpGetAclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL PresentBit,
    IN PACL* AclLocation, // use address in case security descriptor was NULL
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    OUT PBOOLEAN IsAclPresent,
    OUT PACL* Acl,
    OUT PBOOLEAN IsAclDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isAclPresent = FALSE;
    PACL acl = NULL;
    BOOLEAN isAclDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    isAclPresent = IsSetFlag(SecurityDescriptor->Control, PresentBit);
    acl = *AclLocation;
    isAclDefaulted = IsSetFlag(SecurityDescriptor->Control, DefaultedBit);

cleanup:
    if (!NT_SUCCESS(status))
    {
        isAclPresent = FALSE;
        acl = NULL;
        isAclDefaulted = FALSE;
    }

    *IsAclPresent= isAclPresent;
    *Acl = acl;
    *IsAclDefaulted = isAclDefaulted;

    return status;
}

NTSTATUS
RtlGetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsSaclPresent,
    OUT PACL* Sacl,
    OUT PBOOLEAN IsSaclDefaulted
    )
{
    return RtlpGetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_SACL_PRESENT,
                &SecurityDescriptor->Sacl,
                SE_SACL_DEFAULTED,
                IsSaclPresent,
                Sacl,
                IsSaclDefaulted);
}

NTSTATUS
RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsDaclPresent,
    OUT PACL* Dacl,
    OUT PBOOLEAN IsDaclDefaulted
    )
{
    return RtlpGetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_DACL_PRESENT,
                &SecurityDescriptor->Dacl,
                SE_DACL_DEFAULTED,
                IsDaclPresent,
                Dacl,
                IsDaclDefaulted);
}

static
NTSTATUS
RtlpSetSidSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* SidLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    IN PSID Sid,
    IN BOOLEAN IsSidDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Sid && IsSidDefaulted)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    *SidLocation = Sid;

    if (IsSidDefaulted)
    {
        SetFlag(SecurityDescriptor->Control, DefaultedBit);
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, DefaultedBit);
    }

cleanup:
    return status;
}

NTSTATUS
RtlSetOwnerSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID Owner,
    IN BOOLEAN IsOwnerDefaulted
    )
{
    return RtlpSetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Owner,
                SE_OWNER_DEFAULTED,
                Owner,
                IsOwnerDefaulted);
}

NTSTATUS
RtlSetGroupSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN OPTIONAL PSID Group,
    IN BOOLEAN IsGroupDefaulted
    )
{
    return RtlpSetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Group,
                SE_GROUP_DEFAULTED,
                Group,
                IsGroupDefaulted);
}

static
NTSTATUS
RtlpSetAclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL PresentBit,
    IN PACL* AclLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    IN BOOLEAN IsAclPresent,
    IN OPTIONAL PACL Acl,
    IN OPTIONAL BOOLEAN IsAclDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL acl = NULL;
    BOOLEAN isAclDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsAclPresent)
    {
        SetFlag(SecurityDescriptor->Control, PresentBit);
        acl = Acl;
        isAclDefaulted = IsAclDefaulted;
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, PresentBit);
        acl = NULL;
        isAclDefaulted = FALSE;
    }

    *AclLocation = acl;
    if (isAclDefaulted)
    {
        SetFlag(SecurityDescriptor->Control, DefaultedBit);
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, DefaultedBit);
    }

cleanup:
    return status;
}

NTSTATUS
RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsSaclPresent,
    IN OPTIONAL PACL Sacl,
    IN OPTIONAL BOOLEAN IsSaclDefaulted
    )
{
    return RtlpSetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_SACL_PRESENT,
                &SecurityDescriptor->Sacl,
                SE_SACL_DEFAULTED,
                IsSaclPresent,
                Sacl,
                IsSaclDefaulted);
}

NTSTATUS
RtlSetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsDaclPresent,
    IN OPTIONAL PACL Dacl,
    IN OPTIONAL BOOLEAN IsDaclDefaulted
    )
{
    return RtlpSetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_DACL_PRESENT,
                &SecurityDescriptor->Dacl,
                SE_DACL_DEFAULTED,
                IsDaclPresent,
                Dacl,
                IsDaclDefaulted);
}

static
NTSTATUS
RtlpGetSizeUsedSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PULONG SizeUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG size = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;
    USHORT aclSizeUsed = 0;

    if (!NT_SUCCESS(RtlpVerifySecurityDescriptorHeader(SecurityDescriptor)))
    {
        GOTO_CLEANUP();
    }

    // Check non-control fields

    if (SecurityDescriptor->Owner)
    {
        if (!RtlValidSid(SecurityDescriptor->Owner))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += RtlLengthSid(SecurityDescriptor->Owner);
    }

    if (SecurityDescriptor->Group)
    {
        if (!RtlValidSid(SecurityDescriptor->Group))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += RtlLengthSid(SecurityDescriptor->Group);
    }

    if (SecurityDescriptor->Dacl)
    {
        if (!RtlValidAcl(SecurityDescriptor->Dacl, &aclSizeUsed))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += aclSizeUsed;
    }

    if (SecurityDescriptor->Sacl)
    {
        if (!RtlValidAcl(SecurityDescriptor->Sacl, &aclSizeUsed))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += aclSizeUsed;
    }

    status = STATUS_SUCCESS;

cleanup:
    *SizeUsed = NT_SUCCESS(status) ? size : 0;
    return status;
}

NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sizeRequired = 0;
    ULONG offset = 0;
    ULONG size = 0;

    if (!AbsoluteSecurityDescriptor ||
        !BufferLength)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    size = *BufferLength;

    if ((size > 0) && !SelfRelativeSecurityDescriptor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSecurityDescriptor(AbsoluteSecurityDescriptor))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    status = RtlpGetSizeUsedSecurityDescriptor(
                    AbsoluteSecurityDescriptor,
                    &sizeRequired);
    GOTO_CLEANUP_ON_STATUS(status);

    // Use self-relative header size
    sizeRequired -= SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;
    sizeRequired += SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;

    if (sizeRequired > size)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    SelfRelativeSecurityDescriptor->Revision = LW_HTOL8(AbsoluteSecurityDescriptor->Revision);
    SelfRelativeSecurityDescriptor->Sbz1 = LW_HTOL8(AbsoluteSecurityDescriptor->Sbz1);
    SelfRelativeSecurityDescriptor->Control = LW_HTOL16(AbsoluteSecurityDescriptor->Control | SE_SELF_RELATIVE);

    offset = SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;

    if (AbsoluteSecurityDescriptor->Owner)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianSid(
                        AbsoluteSecurityDescriptor->Owner,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Owner = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Group)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianSid(
                        AbsoluteSecurityDescriptor->Group,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Group = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Dacl)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianAcl(
                        AbsoluteSecurityDescriptor->Dacl,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Dacl = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Sacl)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianAcl(
                        AbsoluteSecurityDescriptor->Sacl,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Sacl = LW_HTOL32(offset);
        offset += used;
    }

    status = STATUS_SUCCESS;

cleanup:
    if (BufferLength)
    {
        *BufferLength = sizeRequired;
    }

    return status;
}


/* 
 * Byte swap a UINT32 that is not guaranteed to be word-boundary aligned.
 * Advance cursor by the number of bytes swapped afterwards.
 */
static VOID
RtlHTOL32Stream(
    IN OUT PUINT8 *ppCursor,
    OUT PUINT32 pValue
    )
{
    PUINT8 cursor = *ppCursor;
    UINT32 i32 = 0;

    /* Control */
    memcpy(&i32, cursor, sizeof(i32));
    if (pValue)
    {
        *pValue = i32;
    }
    i32 = LW_HTOL32(i32);
    memcpy(cursor, &i32, sizeof(i32));
    cursor += sizeof(i32);
    *ppCursor = cursor;
}


/* Byte swap a Sid that is not guaranteed to be word-boundary aligned */
static VOID
RtlEncodeLittleEndianSidSwab(
    IN PUINT8 cursor,
    OUT PDWORD pLen
    )
{
    DWORD i = 0;
    UINT8 subAuthorityCount = 0;
    DWORD sidSize = 0;

    /* SID List for Owner; Revision and SubAuthorityCount are both 1 bytes */
    cursor++; // Revision

    subAuthorityCount = *cursor;
    cursor++; // SubAuthorityCount

    cursor += 6; //IdentifierAuthority

    sidSize = _SID_GET_SIZE_REQUIRED(subAuthorityCount);
    for (i=0; i<subAuthorityCount; i++)
    {
        RtlHTOL32Stream(&cursor, NULL);
    }
    if (pLen)
    {
        *pLen = sidSize;
    }
}


static VOID
RtlpEncodeLittleEndianAclSwab(
    IN PUINT8 cursor,
    OUT PDWORD pLen
    )
{
    UINT16 aceCount = 0;
    DWORD i = 0;
    PACL littleEndianAcl = (PACL) cursor;
    PUINT8 startCursor = cursor;

    // This value is in host byte order, so get before swapping.
    aceCount = littleEndianAcl->AceCount;

    littleEndianAcl->AclSize = LW_HTOL16(littleEndianAcl->AclSize);
    littleEndianAcl->AceCount = LW_HTOL16(littleEndianAcl->AceCount);
    littleEndianAcl->Sbz2 = LW_HTOL16(littleEndianAcl->Sbz2);

    cursor += sizeof(ACL);
    for (i=0; i<aceCount; i++)
    {
        PACE_HEADER aceHeader = (PACE_HEADER) cursor;
        PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE) cursor;
        UINT16 aceSize = 0;

        aceSize = aceHeader->AceSize;
        aceHeader->AceSize = LW_HTOL16(aceHeader->AceSize);
        ace->Mask = LW_HTOL32(ace->Mask);

        RtlEncodeLittleEndianSidSwab((PUINT8) &ace->SidStart, NULL);
        cursor += aceSize;
    }

    *pLen = cursor - startCursor;
}


/*
 * 6.0 -> 6.1 conversion routine to fix byte swapping problem
 * on big endian systems. The data is supposed to be stored in
 * little endian, but a bug in lwbase affecting BE systems
 * effectively disabled all swapping functions, so the data is 
 * stored in native byte order.
 * Since this bug is fixed in 6.1, the byte order is wrong, and needs
 * to be adjusted, which is the purpose of this function.
 */
NTSTATUS
RtlAbsoluteToSelfRelativeSDSwab(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN ULONG BufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PUINT8 cursor = NULL;
    ULONG itemLen = 0;
    DWORD offset = 0;
    PSECURITY_DESCRIPTOR_RELATIVE selfRelativeSD = NULL;

    if (!SelfRelativeSecurityDescriptor || BufferLength == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    selfRelativeSD = SelfRelativeSecurityDescriptor;
    selfRelativeSD->Control = LW_HTOL16(selfRelativeSD->Control);

    /* Offset SID list for Owner */
    offset = SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;
    cursor = ((PUINT8) SelfRelativeSecurityDescriptor) + offset;

    /* SID List for Owner */
    itemLen = 0;
    if (offset < BufferLength)
    {
        RtlEncodeLittleEndianSidSwab(cursor, &itemLen);
    }
    if (itemLen > 0)
    {
        selfRelativeSD->Owner = LW_HTOL32(offset);
    }
    offset += itemLen;
    cursor += itemLen;

    /* SID List for Group; Group may not be present */
    itemLen = 0;
    if (offset < BufferLength && selfRelativeSD->Group)
    {
        RtlEncodeLittleEndianSidSwab(cursor, &itemLen);
    }
    if (itemLen > 0)
    {
        selfRelativeSD->Group = LW_HTOL32(offset);
    }
    offset += itemLen;
    cursor += itemLen;

    /* Dacl list */
    itemLen = 0;
    if (offset < BufferLength)
    {
        RtlpEncodeLittleEndianAclSwab(cursor, &itemLen);
    }
    if (itemLen > 0)
    {
        selfRelativeSD->Dacl = LW_HTOL32(offset);
    }
    offset += itemLen;
    cursor += itemLen;

    /* Sacl list */
    itemLen = 0;
    if (offset < BufferLength)
    {
        RtlpEncodeLittleEndianAclSwab(cursor, &itemLen);
    }
    if (itemLen > 0)
    {
        selfRelativeSD->Sacl = LW_HTOL32(offset);
    }

    /* ...and Bob's your uncle */
cleanup:
    return status;
}

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
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_RELATIVE relHeader = { 0 };
    SECURITY_DESCRIPTOR_ABSOLUTE absHeader = { 0 };
    ULONG securityDescriptorSize = 0;
    ULONG daclSize = 0;
    ULONG saclSize = 0;
    ULONG ownerSize = 0;
    ULONG groupSize = 0;
    ULONG securityDescriptorSizeRequired = 0;
    ULONG daclSizeRequired = 0;
    ULONG saclSizeRequired = 0;
    ULONG ownerSizeRequired = 0;
    ULONG groupSizeRequired = 0;

    if (!AbsoluteSecurityDescriptorSize ||
        !DaclSize ||
        !SaclSize ||
        !OwnerSize ||
        !PrimaryGroupSize)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    securityDescriptorSize = *AbsoluteSecurityDescriptorSize;
    daclSize = *DaclSize;
    saclSize = *SaclSize;
    ownerSize = *OwnerSize;
    groupSize = *PrimaryGroupSize;

    if (((securityDescriptorSize > 0) && !AbsoluteSecurityDescriptor) ||
        ((daclSize > 0) && !Dacl) ||
        ((saclSize > 0) && !Sacl) ||
        ((ownerSize > 0) && !Owner) ||
        ((groupSize > 0) && !PrimaryGroup))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    //
    // Extract header information
    //

    relHeader.Revision = LW_LTOH8(SelfRelativeSecurityDescriptor->Revision);
    relHeader.Sbz1 = LW_LTOH8(SelfRelativeSecurityDescriptor->Sbz1);
    relHeader.Control = LW_LTOH16(SelfRelativeSecurityDescriptor->Control);
    relHeader.Owner = LW_LTOH32(SelfRelativeSecurityDescriptor->Owner);
    relHeader.Group = LW_LTOH32(SelfRelativeSecurityDescriptor->Group);
    relHeader.Sacl = LW_LTOH32(SelfRelativeSecurityDescriptor->Sacl);
    relHeader.Dacl = LW_LTOH32(SelfRelativeSecurityDescriptor->Dacl);

    //
    // The self-relative bit must be set.
    //
    // The caller is required to have already checked this via
    // RtlValidRelativeSecurityDescriptor().  Otherwise, the caller
    // is violating an invariant and this code will return
    // STATUS_ASSERTION_FAILURE.
    //

    if (!IsSetFlag(relHeader.Control, SE_SELF_RELATIVE))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    //
    // Verify header information
    //

    absHeader.Revision = relHeader.Revision;
    absHeader.Sbz1 = relHeader.Sbz1;
    absHeader.Control = relHeader.Control;
    // SID and ACL pointers are to denote existence and must not be dereferenced.
    absHeader.Owner = relHeader.Owner ? (PSID) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Owner) : NULL;
    absHeader.Group = relHeader.Group ? (PSID) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Group) : NULL;
    absHeader.Sacl = relHeader.Sacl ? (PACL) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Sacl) : NULL;
    absHeader.Dacl = relHeader.Dacl ? (PACL) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Dacl) : NULL;

    // Clear the self-relative flag since it cannot be present in the
    // absolute header.
    ClearFlag(absHeader.Control, SE_SELF_RELATIVE);

    //
    // The security descriptor header must be valid.
    //
    // The caller is required to have already checked this via
    // RtlValidRelativeSecurityDescriptor().  Otherwise, the caller
    // is violating an invariant and this code will return
    // STATUS_ASSERTION_FAILURE.
    //

    status = RtlpVerifySecurityDescriptorHeader(&absHeader);
    if (!NT_SUCCESS(status))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    //
    // Get size requirements
    //

    securityDescriptorSizeRequired = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;

    if (absHeader.Owner)
    {
        ownerSizeRequired = RtlLengthRequiredSid(LW_LTOH8(absHeader.Owner->SubAuthorityCount));
    }

    if (absHeader.Group)
    {
        groupSizeRequired = RtlLengthRequiredSid(LW_LTOH8(absHeader.Group->SubAuthorityCount));
    }

    if (absHeader.Sacl)
    {
        saclSizeRequired = LW_LTOH16(absHeader.Sacl->AclSize);
    }

    if (absHeader.Dacl)
    {
        daclSizeRequired = LW_LTOH16(absHeader.Dacl->AclSize);
    }

    //
    // Check sizes
    //

    if ((securityDescriptorSize < securityDescriptorSizeRequired) ||
        (ownerSize < ownerSizeRequired) ||
        (groupSize < groupSizeRequired) ||
        (saclSize < saclSizeRequired) ||
        (daclSize < daclSizeRequired))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    //
    // Now convert
    //

    if (AbsoluteSecurityDescriptor)
    {
        RtlCopyMemory(AbsoluteSecurityDescriptor, &absHeader, securityDescriptorSizeRequired);
    }

    if (Owner && absHeader.Owner)
    {
        RtlpDecodeLittleEndianSid(absHeader.Owner, Owner);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Owner = Owner;
        }
    }

    if (PrimaryGroup && absHeader.Group)
    {
        RtlpDecodeLittleEndianSid(absHeader.Group, PrimaryGroup);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Group = PrimaryGroup;
        }
    }

    if (Sacl && absHeader.Sacl)
    {
        status = RtlpDecodeLittleEndianAcl(absHeader.Sacl, Sacl);
        GOTO_CLEANUP_ON_STATUS(status);

        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Sacl = Sacl;
        }
    }

    if (Dacl && absHeader.Dacl)
    {
        status = RtlpDecodeLittleEndianAcl(absHeader.Dacl, Dacl);
        GOTO_CLEANUP_ON_STATUS(status);

        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Dacl = Dacl;
        }
    }

    status = STATUS_SUCCESS;

cleanup:
    if (AbsoluteSecurityDescriptorSize)
    {
        *AbsoluteSecurityDescriptorSize = securityDescriptorSizeRequired;
    }
    if (DaclSize)
    {
        *DaclSize = daclSizeRequired;
    }
    if (SaclSize)
    {
        *SaclSize = saclSizeRequired;
    }
    if (OwnerSize)
    {
        *OwnerSize = ownerSizeRequired;
    }
    if (PrimaryGroupSize)
    {
        *PrimaryGroupSize = groupSizeRequired;
    }

    return status;
}

NTSTATUS
RtlQuerySecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;    
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;

    // Sanity checks

    if (SecurityInformation == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }    

    status = RtlpCreateAbsSecDescFromRelative(
                 &pSecDescAbs, 
                 ObjectSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    // Remove any pieces not requested by the caller

    if (!(SecurityInformation & OWNER_SECURITY_INFORMATION))
    {
        PSID pOwnerSid = NULL;
        BOOLEAN bOwnerDefaulted = FALSE;
        
        status = RtlGetOwnerSecurityDescriptor(
                     pSecDescAbs,
                     &pOwnerSid,
                     &bOwnerDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pOwnerSid);

        status = RtlSetOwnerSecurityDescriptor(
                     pSecDescAbs,
                     NULL,
                     FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }
        
    if (!(SecurityInformation & GROUP_SECURITY_INFORMATION))
    {
        PSID pGroupSid = NULL;
        BOOLEAN bGroupDefaulted = FALSE;

        status = RtlGetGroupSecurityDescriptor(
                     pSecDescAbs,
                     &pGroupSid,
                     &bGroupDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
        
        LW_RTL_FREE(&pGroupSid);

        status = RtlSetGroupSecurityDescriptor(pSecDescAbs, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }        

    if (!(SecurityInformation & DACL_SECURITY_INFORMATION))
    {
        PACL pDacl = NULL;
        BOOLEAN bDaclDefaulted = FALSE;
        BOOLEAN bDaclPresent = FALSE;
        SECURITY_DESCRIPTOR_CONTROL DaclControlChange = (SE_DACL_PROTECTED|
                                                         SE_DACL_AUTO_INHERITED|
                                                         SE_DACL_AUTO_INHERIT_REQ|
                                                         SE_DACL_UNTRUSTED);

        status = RtlGetDaclSecurityDescriptor(
                     pSecDescAbs,
                     &bDaclPresent,
                     &pDacl,
                     &bDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pDacl);

        status = RtlSetDaclSecurityDescriptor(pSecDescAbs, FALSE, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);        
    
        status = RtlSetSecurityDescriptorControl(
                     pSecDescAbs,
                     DaclControlChange,
                     0);
        GOTO_CLEANUP_ON_STATUS(status);        
    }
    
    if (!(SecurityInformation & SACL_SECURITY_INFORMATION))
    {
        PACL pSacl = NULL;
        BOOLEAN bSaclDefaulted = FALSE;
        BOOLEAN bSaclPresent = FALSE;
        SECURITY_DESCRIPTOR_CONTROL SaclControlChange = (SE_SACL_PROTECTED|
                                                         SE_SACL_AUTO_INHERITED|
                                                         SE_SACL_AUTO_INHERIT_REQ);

        status = RtlGetSaclSecurityDescriptor(
                     pSecDescAbs,
                     &bSaclPresent,
                     &pSacl,
                     &bSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);        
    
        LW_RTL_FREE(&pSacl);

        status = RtlSetSaclSecurityDescriptor(pSecDescAbs, FALSE, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);        

        status = RtlSetSecurityDescriptorControl(
                     pSecDescAbs,
                     SaclControlChange,
                     0);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Convert back to relative form

    status = RtlAbsoluteToSelfRelativeSD(
                 pSecDescAbs,
                 SecurityDescriptor,
                 Length);
    GOTO_CLEANUP_ON_STATUS(status);    
    

cleanup:
    if (pSecDescAbs)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pSecDescAbs);
    }

    return status;
}

NTSTATUS
RtlSetSecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE InputSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE NewObjectSecurityDescriptor,
    IN OUT PULONG NewObjectSecurityDescriptorLength,
    IN PGENERIC_MAPPING GenericMapping
    )
{
    NTSTATUS status = STATUS_SUCCESS;    
    PSECURITY_DESCRIPTOR_ABSOLUTE pObjSecDescAbs = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pInputSecDescAbs = NULL;
    SECURITY_DESCRIPTOR_CONTROL InputSecDescControl = 0;
    UCHAR Revision = 0;    
    
    // Sanity checks

    if (SecurityInformation == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }    

    // Convert to an Absolute SecDesc

    status = RtlpCreateAbsSecDescFromRelative(
                 &pObjSecDescAbs, 
                 ObjectSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);
        
    status = RtlpCreateAbsSecDescFromRelative(
                 &pInputSecDescAbs, 
                 InputSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    // Merge
        
    status = RtlGetSecurityDescriptorControl(
                 pInputSecDescAbs,
                 &InputSecDescControl,
                 &Revision);
    GOTO_CLEANUP_ON_STATUS(status);
        
    // Owner

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        PSID pObjOwnerSid = NULL;
        BOOLEAN bObjOwnerDefaulted = FALSE;
        PSID pInputOwnerSid = NULL;
        BOOLEAN bInputOwnerDefaulted = FALSE;

        status = RtlGetOwnerSecurityDescriptor(
                     pObjSecDescAbs,
                     &pObjOwnerSid,
                     &bObjOwnerDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pObjOwnerSid);

        status = RtlGetOwnerSecurityDescriptor(
                     pInputSecDescAbs,
                     &pInputOwnerSid,
                     &bInputOwnerDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetOwnerSecurityDescriptor(
                     pInputSecDescAbs,
                     NULL,
                     FALSE);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetOwnerSecurityDescriptor(
                     pObjSecDescAbs,
                     pInputOwnerSid,
                     bInputOwnerDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
        
    }

    // Group

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        PSID pObjGroupSid = NULL;
        BOOLEAN bObjGroupDefaulted = FALSE;
        PSID pInputGroupSid = NULL;
        BOOLEAN bInputGroupDefaulted = FALSE;

        status = RtlGetGroupSecurityDescriptor(
                     pObjSecDescAbs,
                     &pObjGroupSid,
                     &bObjGroupDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pObjGroupSid);

        status = RtlGetGroupSecurityDescriptor(
                     pInputSecDescAbs,
                     &pInputGroupSid,
                     &bInputGroupDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
        
        status = RtlSetGroupSecurityDescriptor(
                     pInputSecDescAbs,
                     NULL,
                     FALSE);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetGroupSecurityDescriptor(
                     pObjSecDescAbs,
                     pInputGroupSid,
                     bInputGroupDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }        

    // Dacl

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        PACL pObjDacl = NULL;
        BOOLEAN bObjDaclDefaulted = FALSE;
        BOOLEAN bObjDaclPresent = FALSE;
        PACL pInputDacl = NULL;
        BOOLEAN bInputDaclDefaulted = FALSE;
        BOOLEAN bInputDaclPresent = FALSE;
        SECURITY_DESCRIPTOR_CONTROL DaclControlSet = 0;
        SECURITY_DESCRIPTOR_CONTROL DaclControlChange = (SE_DACL_PROTECTED|
                                                         SE_DACL_AUTO_INHERITED|
                                                         SE_DACL_AUTO_INHERIT_REQ|
                                                         SE_DACL_UNTRUSTED);

        status = RtlGetDaclSecurityDescriptor(
                     pObjSecDescAbs,
                     &bObjDaclPresent,
                     &pObjDacl,
                     &bObjDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pObjDacl);

        status = RtlGetDaclSecurityDescriptor(
                     pInputSecDescAbs,
                     &bInputDaclPresent,
                     &pInputDacl,
                     &bInputDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetDaclSecurityDescriptor(
                     pInputSecDescAbs,
                     FALSE,
                     NULL,
                     FALSE);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetSecurityDescriptorControl(
                     pObjSecDescAbs,
                     DaclControlChange,
                     0);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetDaclSecurityDescriptor(
                     pObjSecDescAbs,
                     TRUE,
                     pInputDacl,
                     bInputDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        DaclControlSet = InputSecDescControl & DaclControlChange;
        
        status = RtlSetSecurityDescriptorControl(
                     pObjSecDescAbs,
                     DaclControlChange,
                     DaclControlSet);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    // Sacl

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        PACL pObjSacl = NULL;
        BOOLEAN bObjSaclDefaulted = FALSE;
        BOOLEAN bObjSaclPresent = FALSE;
        PACL pInputSacl = NULL;
        BOOLEAN bInputSaclDefaulted = FALSE;
        BOOLEAN bInputSaclPresent = FALSE;
        SECURITY_DESCRIPTOR_CONTROL SaclControlSet = 0;
        SECURITY_DESCRIPTOR_CONTROL SaclControlChange = (SE_SACL_PROTECTED|
                                                         SE_SACL_AUTO_INHERITED|
                                                         SE_SACL_AUTO_INHERIT_REQ);

        status = RtlGetSaclSecurityDescriptor(
                     pObjSecDescAbs,
                     &bObjSaclPresent,
                     &pObjSacl,
                     &bObjSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        LW_RTL_FREE(&pObjSacl);

        status = RtlGetSaclSecurityDescriptor(
                     pInputSecDescAbs,
                     &bInputSaclPresent,
                     &pInputSacl,
                     &bInputSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    
        status = RtlSetSaclSecurityDescriptor(
                     pInputSecDescAbs,
                     FALSE,
                     NULL,
                     FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    
        status = RtlSetSecurityDescriptorControl(
                     pObjSecDescAbs,
                     SaclControlChange,
                     0);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlSetSaclSecurityDescriptor(
                     pObjSecDescAbs,
                     TRUE,
                     pInputSacl,
                     bInputSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        SaclControlSet = InputSecDescControl & SaclControlChange;

        status = RtlSetSecurityDescriptorControl(
                     pObjSecDescAbs,
                     SaclControlChange,
                     SaclControlSet);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Convert back to relative form

    status = RtlAbsoluteToSelfRelativeSD(
                 pObjSecDescAbs,
                 NewObjectSecurityDescriptor,
                 NewObjectSecurityDescriptorLength);
    GOTO_CLEANUP_ON_STATUS(status);    
    

cleanup:
    if (pObjSecDescAbs)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pObjSecDescAbs);
    }

    if (pInputSecDescAbs)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pInputSecDescAbs);
    }

    return status;
}

NTSTATUS
RtlpCreateAbsSecDescFromRelative(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc = NULL;
    ULONG ulAbsSecDescLen = 0;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;

    // Get sizes

    status = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    // Allocate

    if (ulOwnerLen)
    {
        status = RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulGroupLen)
    {
        status = RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulDaclLen)
    {
        status = RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulSaclLen)
    {
        status = RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pAbsSecDesc, VOID, ulAbsSecDescLen);
    GOTO_CLEANUP_ON_STATUS(status);

    // Translate

    status = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    GOTO_CLEANUP_ON_STATUS(status);

    *ppAbsSecDesc = pAbsSecDesc;

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pAbsSecDesc);
        LW_RTL_FREE(&pOwner);
        LW_RTL_FREE(&pGroup);
        LW_RTL_FREE(&pSacl);
        LW_RTL_FREE(&pDacl);
    }
            
    return status;
}

VOID
RtlpFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL))
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    status = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    status = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    status = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    status = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

