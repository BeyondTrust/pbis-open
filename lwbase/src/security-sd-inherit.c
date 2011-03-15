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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        security-sd-inherit.c
 *
 * Abstract:
 *
 *        Security Descriptor Inheritance (SD) Functions in
 *        Security Module.
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "security-includes.h"


static
NTSTATUS
RtlpObjectSetOwner(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    );

static
NTSTATUS
RtlpObjectSetGroup(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    );

static
NTSTATUS
RtlpObjectSetDacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    );

static
NTSTATUS
RtlpObjectSetSacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
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
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDesc = NULL;
    ULONG ulNewSecDescLength = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsParentSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsCreatorSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsNewSecDesc = NULL;

    if (!ppNewSecDesc || !pNewSecDescLength || !pGenericMap)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    *ppNewSecDesc = NULL;
    *pNewSecDescLength = 0;

    // pUserToken can only be NULL if bother the following flags
    // Are set in the AutoInheritFlags
    //      (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK)

    if (!((AutoInheritFlags & (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK))
          != (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK)) &&
        (pUserToken == NULL))
    {
        status = STATUS_NO_TOKEN;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pParentSecDesc)
    {
        status = RtlpCreateAbsSecDescFromRelative(
                     &pAbsParentSecDesc,
                     pParentSecDesc);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pCreatorSecDesc)
    {
        status = RtlpCreateAbsSecDescFromRelative(
                     &pAbsCreatorSecDesc,
                     pCreatorSecDesc);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Create new Absolute SecDesc

    status= LW_RTL_ALLOCATE(
                 &pAbsNewSecDesc,
                 SECURITY_DESCRIPTOR_ABSOLUTE,
                 SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(
                  pAbsNewSecDesc,
                  SECURITY_DESCRIPTOR_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set Owner

    status = RtlpObjectSetOwner(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 AutoInheritFlags,
                 pUserToken);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set Group

    status = RtlpObjectSetGroup(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 AutoInheritFlags,
                 pUserToken);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set DACL

    status = RtlpObjectSetDacl(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 bIsContainerObject,
                 AutoInheritFlags,
                 pUserToken,
                 pGenericMap);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set SACL

    status = RtlpObjectSetSacl(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 bIsContainerObject,
                 AutoInheritFlags,
                 pUserToken,
                 pGenericMap);
    GOTO_CLEANUP_ON_STATUS(status);

    // All done - convert to Self-Relative form and return

    status = RtlAbsoluteToSelfRelativeSD(
                 pAbsNewSecDesc,
                 NULL,
                 &ulNewSecDescLength);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = LW_RTL_ALLOCATE(
                     &pNewSecDesc,
                     SECURITY_DESCRIPTOR_RELATIVE,
                     ulNewSecDescLength);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlAbsoluteToSelfRelativeSD(
                     pAbsNewSecDesc,
                     pNewSecDesc,
                     &ulNewSecDescLength);
    }
    GOTO_CLEANUP_ON_STATUS(status);

    *ppNewSecDesc = pNewSecDesc;
    *pNewSecDescLength = ulNewSecDescLength;

cleanup:
    // Normal cleanup

    if (pAbsParentSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsParentSecDesc);
    }

    if (pAbsCreatorSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsCreatorSecDesc);
    }

    if (pAbsNewSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsNewSecDesc);
    }

    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pNewSecDesc);
    }


    return status;
}

static
NTSTATUS
RtlpObjectSetOwner(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pCreatorSecDescOwner = NULL;
    PSID pParentSecDescOwner = NULL;
    BOOLEAN bDefaulted = FALSE;
    PSID pOwner = NULL;
    union {
        TOKEN_OWNER TokenOwnerInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;

    // Use the CreatorSecDesc Owner if present and is
    // a member of the user's token

    if (pCreatorSecDesc)
    {
        status = RtlGetOwnerSecurityDescriptor(
                     pCreatorSecDesc,
                     &pCreatorSecDescOwner,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pCreatorSecDescOwner 
            && RtlIsSidMemberOfToken(pUserToken, pCreatorSecDescOwner))
        {
            status = RtlDuplicateSid(&pOwner, pCreatorSecDescOwner);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetOwnerSecurityDescriptor(
                         pSecurityDescriptor,
                         pOwner,
                         bDefaulted);
            GOTO_CLEANUP();
        }
    }

    // Check for Defaulting the owner from the parent Sec Desc

    if (pParentSecDesc && (AutoInheritFlags & SEF_DEFAULT_OWNER_FROM_PARENT))
    {
        status = RtlGetOwnerSecurityDescriptor(
                     pParentSecDesc,
                     &pParentSecDescOwner,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pParentSecDescOwner)
        {
            status = RtlDuplicateSid(&pOwner, pCreatorSecDescOwner);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetOwnerSecurityDescriptor(
                         pSecurityDescriptor,
                         pOwner,
                         TRUE);
            GOTO_CLEANUP();
        }
    }

    // Copy the owner of the ACCESS_TOKEN

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenOwner,
                 (PVOID)pTokenOwnerInformation,
                 sizeof(TokenOwnerBuffer),
                 &ulTokenOwnerLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&pOwner, pTokenOwnerInformation->Owner);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetOwnerSecurityDescriptor(
                 pSecurityDescriptor,
                 pOwner,
                 TRUE);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pOwner);
    }
    
        
    return status;
}


static
NTSTATUS
RtlpObjectSetGroup(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pCreatorSecDescGroup = NULL;
    PSID pParentSecDescGroup = NULL;
    BOOLEAN bDefaulted = FALSE;
    PSID pGroup = NULL;
    struct {
        TOKEN_PRIMARY_GROUP TokenPrimaryGroupInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenPrimaryGroupBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroupInfo = (PTOKEN_PRIMARY_GROUP)&TokenPrimaryGroupBuffer;
    ULONG ulTokenPrimaryGroupLength = 0;

    // Use the CreatorSecDesc Group if present and if a
    // member of the user's token

    if (pCreatorSecDesc)
    {
        status = RtlGetGroupSecurityDescriptor(
                     pCreatorSecDesc,
                     &pCreatorSecDescGroup,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pCreatorSecDescGroup
            && RtlIsSidMemberOfToken(pUserToken, pCreatorSecDescGroup))
        {
            status = RtlDuplicateSid(&pGroup, pCreatorSecDescGroup);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecurityDescriptor,
                         pGroup,
                         bDefaulted);
            GOTO_CLEANUP();
        }
    }

    // Check for Defaulting the owner from the parent Sec Desc
    if (pParentSecDesc && (AutoInheritFlags & SEF_DEFAULT_GROUP_FROM_PARENT))
    {
        status = RtlGetGroupSecurityDescriptor(
                     pParentSecDesc,
                     &pParentSecDescGroup,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pParentSecDescGroup)
        {
            status = RtlDuplicateSid(&pGroup, pCreatorSecDescGroup);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecurityDescriptor,
                         pGroup,
                         TRUE);
            GOTO_CLEANUP();
        }
    }

    // Copy the Group SID from the Token

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenPrimaryGroup,
                 (PVOID)pTokenPrimaryGroupInfo,
                 sizeof(TokenPrimaryGroupBuffer),
                 &ulTokenPrimaryGroupLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&pGroup, pTokenPrimaryGroupInfo->PrimaryGroup);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetGroupSecurityDescriptor(
                 pSecurityDescriptor,
                 pGroup,
                 TRUE);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pGroup);
    }

    return status;
}

static
NTSTATUS
RtlpDuplicateAcl(
    OUT PACL* NewAcl,
    IN PACL Acl
    );

static
NTSTATUS
RtlpObjectInheritSecurity(
    OUT PACL *ppNewDacl,
    OUT PBOOLEAN pbIsNewDaclDefaulted,
    IN OPTIONAL PACL pParentDacl,
    IN BOOLEAN bParentIsDaclDefaulted,
    IN OPTIONAL PACL pCreatorDacl,
    IN BOOLEAN bCreatorIsDaclDefaulted,
    IN BOOLEAN bIsContainerObject,
    IN PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    );

static
NTSTATUS
RtlpObjectSetDacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pParentDacl = NULL;
    PACL pCreatorDacl = NULL;
    PACL pFinalDacl = NULL;
    BOOLEAN bCreatorIsDaclPresent = FALSE;
    BOOLEAN bCreatorIsDaclDefaulted = FALSE;
    BOOLEAN bParentIsDaclPresent = FALSE;
    BOOLEAN bParentIsDaclDefaulted = FALSE;
    BOOLEAN bFinalIsDaclDefaulted = FALSE;
    SECURITY_DESCRIPTOR_CONTROL CreatorSecDescControl = 0;
    PBYTE pBuffer = NULL;
    PTOKEN_DEFAULT_DACL pTokenDefaultDaclInformation = NULL;
    ULONG ulTokenDefaultDaclLength = 0;
    ULONG ulTokenDefaultDaclSize = 0;
    SECURITY_DESCRIPTOR_CONTROL DaclControlSet = 0;
    SECURITY_DESCRIPTOR_CONTROL DaclControlChange = (SE_DACL_PROTECTED|
                                                     SE_DACL_AUTO_INHERITED);

    // Pull the DACLs so we have something to work with

    if (pParentSecDesc && (AutoInheritFlags & SEF_DACL_AUTO_INHERIT))
    {
        status = RtlGetDaclSecurityDescriptor(
                     pParentSecDesc,
                     &bParentIsDaclPresent,
                     &pParentDacl,
                     &bParentIsDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pCreatorSecDesc)
    {
        status = RtlGetDaclSecurityDescriptor(
                     pCreatorSecDesc,
                     &bCreatorIsDaclPresent,
                     &pCreatorDacl,
                     &bCreatorIsDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlGetSecurityDescriptorControl(
                     pCreatorSecDesc,
                     &CreatorSecDescControl,
                     NULL);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // If the creator wants to block inheritance, we are done

    if (bCreatorIsDaclPresent &&
        (CreatorSecDescControl & SE_DACL_PROTECTED))
    {
        status = RtlpDuplicateAcl(&pFinalDacl, pCreatorDacl);
        GOTO_CLEANUP_ON_STATUS(status);

        bFinalIsDaclDefaulted = FALSE;
    }
    // Do the inheritance if we have at least one SecDesc to work with
    else if (bParentIsDaclPresent || bCreatorIsDaclPresent)
    {
        status = RtlpObjectInheritSecurity(
                     &pFinalDacl,
                     &bFinalIsDaclDefaulted,
                     bParentIsDaclPresent ? pParentDacl : NULL,
                     bParentIsDaclDefaulted,
                     bCreatorIsDaclPresent ? pCreatorDacl : NULL,
                     bCreatorIsDaclDefaulted,
                     bIsContainerObject,
                     pUserToken,
                     pGenericMap);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Use the Token Default Dacl IFF we don't have a valid DACL with
    // at least 1 ACE

    if (!pFinalDacl || (RtlGetAclAceCount(pFinalDacl) == 0))
    {
        if (pFinalDacl)
        {
            LW_RTL_FREE(&pFinalDacl);
        }
        
        status = RtlQueryAccessTokenInformation(
                     pUserToken,
                     TokenDefaultDacl,
                     NULL,
                     0,
                     &ulTokenDefaultDaclLength);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            ulTokenDefaultDaclSize = ulTokenDefaultDaclLength;

            status = LW_RTL_ALLOCATE(&pBuffer, VOID, ulTokenDefaultDaclSize);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlQueryAccessTokenInformation(
                         pUserToken,
                         TokenDefaultDacl,
                         pBuffer,
                         ulTokenDefaultDaclLength,
                         &ulTokenDefaultDaclLength);
        }
        GOTO_CLEANUP_ON_STATUS(status);

        pTokenDefaultDaclInformation = (PTOKEN_DEFAULT_DACL)pBuffer;

        // Call RtlpObjectInheritSecurity() again but with the Token
        // Default DACL as the creator Dacl so that it can handle
        // adding the ACEs and mapping to the object specific bits

        status = RtlpObjectInheritSecurity(
                     &pFinalDacl,
                     &bFinalIsDaclDefaulted,
                     NULL,
                     FALSE,
                     pTokenDefaultDaclInformation->DefaultDacl,
                     TRUE,
                     bIsContainerObject,
                     pUserToken,
                     pGenericMap);
        GOTO_CLEANUP_ON_STATUS(status);

        // Final DACL is defaulted only if there is no parent DACL

        bFinalIsDaclDefaulted = bParentIsDaclPresent ? FALSE : TRUE;
    }

    // Finally set the new DACL in the outgoing Seccurity Descriptor

    if (CreatorSecDescControl & SE_DACL_PROTECTED)
    {
        DaclControlSet |= SE_DACL_PROTECTED;
    } else if (AutoInheritFlags & SEF_DACL_AUTO_INHERIT)
    {
        DaclControlSet |= SE_DACL_AUTO_INHERITED;
    }

    status = RtlSetSecurityDescriptorControl(
                 pSecurityDescriptor,
                 DaclControlChange,
                 DaclControlSet);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetDaclSecurityDescriptor(
                 pSecurityDescriptor,
                 pFinalDacl ? TRUE : FALSE,
                 pFinalDacl,
                 bFinalIsDaclDefaulted);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pFinalDacl);
    }    
            
    LW_RTL_FREE(&pBuffer);

    return status;
}

static
NTSTATUS
RtlpDuplicateAcl(
    OUT PACL* NewAcl,
    IN PACL Acl
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL newAcl = NULL;
    USHORT daclSize = 0;

    daclSize = RtlGetAclSize(Acl);

    status = LW_RTL_ALLOCATE(&newAcl, VOID, daclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    RtlCopyMemory(newAcl, Acl, daclSize);

    *NewAcl = newAcl;

cleanup:

    return status;
}

static
NTSTATUS
RtlpObjectAclAssignSecurity(
    OUT PACL *ppObjectAcl,
    IN PACCESS_TOKEN pUserToken,
    IN PACL pSrcAcl,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pAcl = NULL;
    ACCESS_MASK mask = 0;
    USHORT aclSize = 0;
    USHORT srcAclSizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceHeader = NULL;
    PACCESS_ALLOWED_ACE aceAllow = NULL;
    PACCESS_DENIED_ACE aceDeny = NULL;
    PSYSTEM_AUDIT_ACE aceAudit = NULL;
    PSID aceSid = NULL;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } CreatorOwner;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } CreatorGroup;
    ULONG CreatorOwnerSidSize = sizeof(CreatorOwner.buffer);
    ULONG CreatorGroupSidSize = sizeof(CreatorGroup.buffer);
    union {
        TOKEN_OWNER TokenOwnerInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;
    struct {
        TOKEN_PRIMARY_GROUP TokenPrimaryGroupInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenPrimaryGroupBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroupInfo = (PTOKEN_PRIMARY_GROUP)&TokenPrimaryGroupBuffer;
    ULONG ulTokenPrimaryGroupLength = 0;

    aclSize = ACL_HEADER_SIZE +
              (RtlGetAclAceCount(pSrcAcl) *
               (sizeof(ACCESS_ALLOWED_ACE) + SID_MAX_SIZE));

    if (aclSize == 0)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Build the "CREATOR OWNER" and "CREATOR GROUP" sids for comparison

    status = RtlCreateWellKnownSid(
                 WinCreatorOwnerSid,
                 NULL,
                 &CreatorOwner.sid,
                 &CreatorOwnerSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateWellKnownSid(
                 WinCreatorGroupSid,
                 NULL,
                 &CreatorGroup.sid,
                 &CreatorGroupSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenOwner,
                 (PVOID)pTokenOwnerInformation,
                 sizeof(TokenOwnerBuffer),
                 &ulTokenOwnerLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenPrimaryGroup,
                 (PVOID)pTokenPrimaryGroupInfo,
                 sizeof(TokenPrimaryGroupBuffer),
                 &ulTokenPrimaryGroupLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LW_RTL_ALLOCATE(&pAcl, VOID, aclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAcl(pAcl, aclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    srcAclSizeUsed = RtlGetAclSizeUsed(pSrcAcl);

    while (TRUE)
    {
        status = RtlIterateAce(
                     pSrcAcl,
                     srcAclSizeUsed,
                     &aceOffset,
                     &aceHeader);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        GOTO_CLEANUP_ON_STATUS(status);

        switch (aceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            {
                aceAllow = (PACCESS_ALLOWED_ACE)aceHeader;
                mask = aceAllow->Mask;
                aceSid = (PSID)&aceAllow->SidStart;

                RtlMapGenericMask(&mask, pGenericMap);

                if (RtlEqualSid(aceSid, &CreatorOwner.sid))
                {
                    aceSid = pTokenOwnerInformation->Owner;
                }
                else if (RtlEqualSid(aceSid, &CreatorGroup.sid))
                {
                    aceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
                }

                status = RtlAddAccessAllowedAceEx(
                             pAcl,
                             ACL_REVISION,
                             aceAllow->Header.AceFlags,
                             mask,
                             aceSid);
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;
        
            case ACCESS_DENIED_ACE_TYPE:
            {
                aceDeny = (PACCESS_DENIED_ACE)aceHeader;
                mask = aceDeny->Mask;
                aceSid = (PSID)&aceDeny->SidStart;

                RtlMapGenericMask(&mask, pGenericMap);

                if (RtlEqualSid(aceSid, &CreatorOwner.sid))
                {
                    aceSid = pTokenOwnerInformation->Owner;
                }
                else if (RtlEqualSid(aceSid, &CreatorGroup.sid))
                {
                    aceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
                }

                status = RtlAddAccessDeniedAceEx(
                             pAcl,
                             ACL_REVISION,
                             aceDeny->Header.AceFlags,
                             mask,
                             aceSid);
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;

            case SYSTEM_AUDIT_ACE_TYPE:
            {
                aceAudit = (PSYSTEM_AUDIT_ACE)aceHeader;
                mask = aceAudit->Mask;
                aceSid = (PSID)&aceAudit->SidStart;

                RtlMapGenericMask(&mask, pGenericMap);

                if (RtlEqualSid(aceSid, &CreatorOwner.sid))
                {
                    aceSid = pTokenOwnerInformation->Owner;
                }
                else if (RtlEqualSid(aceSid, &CreatorGroup.sid))
                {
                    aceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
                }

                status = RtlAddSystemAuditAceEx(
                             pAcl,
                             ACL_REVISION,
                             aceAudit->Header.AceFlags,
                             mask,
                             aceSid);
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;

            default:
                // ignore
                break;
        }
    }

    *ppObjectAcl = pAcl;

    status = STATUS_SUCCESS;    

cleanup:

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pAcl);
    }

    return status;
}


static
NTSTATUS
RtlpObjectInheritSecurity(
    OUT PACL *NewAcl,
    OUT PBOOLEAN IsNewAclDefaulted,
    IN OPTIONAL PACL ParentAcl,
    IN BOOLEAN ParentIsAclDefaulted,
    IN OPTIONAL PACL CreateAcl,
    IN BOOLEAN CreatorIsAclDefaulted,
    IN BOOLEAN IsContainerObject,
    IN PACCESS_TOKEN AccessToken,
    IN PGENERIC_MAPPING GenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pAcl = NULL;
    USHORT usAclSize = 0;
    USHORT usCreatorNumAces = 0;
    USHORT usParentNumAces = 0;
    USHORT i = 0;
    PACE_HEADER pAceHeader = NULL;
    PACCESS_ALLOWED_ACE pAllowAce = NULL;
    PACCESS_DENIED_ACE pDenyAce = NULL;
    PSYSTEM_AUDIT_ACE pAuditAce = NULL;
    ACCESS_MASK mask = 0;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } CreatorOwner;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } CreatorGroup;
    ULONG CreatorOwnerSidSize = sizeof(CreatorOwner.buffer);
    ULONG CreatorGroupSidSize = sizeof(CreatorGroup.buffer);
    union {
        TOKEN_OWNER TokenOwnerInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;
    struct {
        TOKEN_PRIMARY_GROUP TokenPrimaryGroupInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenPrimaryGroupBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroupInfo = (PTOKEN_PRIMARY_GROUP)&TokenPrimaryGroupBuffer;
    ULONG ulTokenPrimaryGroupLength = 0;

    // Inheritance Algorithm:
    //
    // (a) If defaulted creator DACL and no parent, just use defaulted
    //     creator DACL, 
    //
    // else
    //
    // (b) Add in all creator DACL entries
    // (c) Add in inheritable ACEs from parent
    //      i.   Skip if is a container and is NOT a container inherit
    //           ace, or if is NOT a container and is NOT a object 
    //           (non-container) inherit ace
    //      ii.  For containers, add propagatable inheritance entries
    //      iii. Add inherited ace, applying generic map
    //

    // Case (a)

    if (!ParentAcl && CreatorIsAclDefaulted)
    {
        status = RtlpObjectAclAssignSecurity(
                     &pAcl,
                     AccessToken,
                     CreateAcl,
                     GenericMap);
        GOTO_CLEANUP_ON_STATUS(status);

        *IsNewAclDefaulted = TRUE;
        *NewAcl = pAcl;

        GOTO_CLEANUP();
    }

    if (CreateAcl)
    {
        usAclSize = ACL_HEADER_SIZE +
                     (RtlGetAclAceCount(CreateAcl) *
                      (sizeof(SYSTEM_AUDIT_ACE) + SID_MAX_SIZE));
    }

    if (ParentAcl)
    {
        usAclSize += ACL_HEADER_SIZE +
                      ((RtlGetAclAceCount(ParentAcl) * 2) *
                       (sizeof(SYSTEM_AUDIT_ACE) + SID_MAX_SIZE));
    }

    if (usAclSize <= 0)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Build the "CREATOR OWNER" and "CREATOR GROUP" sids for comparison

    status = RtlCreateWellKnownSid(
                 WinCreatorOwnerSid,
                 NULL,
                 &CreatorOwner.sid,
                 &CreatorOwnerSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateWellKnownSid(
                 WinCreatorGroupSid,
                 NULL,
                 &CreatorGroup.sid,
                 &CreatorGroupSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlQueryAccessTokenInformation(
                 AccessToken,
                 TokenOwner,
                 (PVOID)pTokenOwnerInformation,
                 sizeof(TokenOwnerBuffer),
                 &ulTokenOwnerLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlQueryAccessTokenInformation(
                 AccessToken,
                 TokenPrimaryGroup,
                 (PVOID)pTokenPrimaryGroupInfo,
                 sizeof(TokenPrimaryGroupBuffer),
                 &ulTokenPrimaryGroupLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = LW_RTL_ALLOCATE(&pAcl, VOID, usAclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAcl(pAcl, usAclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    // Case (b) - Direct ACEs first

    if (CreateAcl)
    {
        usCreatorNumAces = RtlGetAclAceCount(CreateAcl);
    }

    for (i=0; i<usCreatorNumAces; i++)
    {
        status = RtlGetAce(CreateAcl, i, OUT_PPVOID(&pAceHeader));
        GOTO_CLEANUP_ON_STATUS(status);

        switch(pAceHeader->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;
            mask = pAllowAce->Mask;

            RtlMapGenericMask(&mask, GenericMap);

            status = RtlAddAccessAllowedAceEx(
                         pAcl,
                         ACL_REVISION,
                         pAllowAce->Header.AceFlags,
                         mask,
                         (PSID)&pAllowAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;
            mask = pDenyAce->Mask;

            RtlMapGenericMask(&mask, GenericMap);

            status = RtlAddAccessDeniedAceEx(
                         pAcl,
                         ACL_REVISION,
                         pDenyAce->Header.AceFlags,
                         mask,
                         (PSID)&pDenyAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            pAuditAce = (PSYSTEM_AUDIT_ACE)pAceHeader;
            mask = pAuditAce->Mask;

            RtlMapGenericMask(&mask, GenericMap);

            status = RtlAddSystemAuditAceEx(
                         pAcl,
                         ACL_REVISION,
                         pAuditAce->Header.AceFlags,
                         mask,
                         (PSID)&pAuditAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        default:
            // Skip all other types
            break;
        }
    }

    // Case (c) - Inheritable ACEs

    if (ParentAcl)
    {
        usParentNumAces = RtlGetAclAceCount(ParentAcl);
    }

    for (i=0; i<usParentNumAces; i++)
    {
        UCHAR AceFlags;
        PSID pAceSid = NULL;

        status = RtlGetAce(ParentAcl, i, OUT_PPVOID(&pAceHeader));
        GOTO_CLEANUP_ON_STATUS(status);

        // Skip if no inheritable access rights

        if ((IsContainerObject &&
             !(pAceHeader->AceFlags & CONTAINER_INHERIT_ACE)) ||
            (!IsContainerObject &&
             !(pAceHeader->AceFlags & OBJECT_INHERIT_ACE)))
        {
            continue;
        }

        // See if the inherit ACE should continue to be propagated.
        // If so, then copy it

        switch(pAceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            {
                pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;
                mask = pAllowAce->Mask;
                if (IsContainerObject &&
                    !IsSetFlag(pAceHeader->AceFlags, NO_PROPAGATE_INHERIT_ACE))
                {
                    status = RtlAddAccessAllowedAceEx(
                                 pAcl,
                                 ACL_REVISION,
                                 pAllowAce->Header.AceFlags,
                                 pAllowAce->Mask,
                                 (PSID)&pAllowAce->SidStart);
                    GOTO_CLEANUP_ON_STATUS(status);
                }
            }
            break;

            case ACCESS_DENIED_ACE_TYPE:
            {
                pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;
                mask = pDenyAce->Mask;
                if (IsContainerObject &&
                    !IsSetFlag(pAceHeader->AceFlags, NO_PROPAGATE_INHERIT_ACE))
                {
                    status = RtlAddAccessDeniedAceEx(
                                 pAcl,
                                 ACL_REVISION,
                                 pDenyAce->Header.AceFlags,
                                 pDenyAce->Mask,
                                 (PSID)&pDenyAce->SidStart);
                    GOTO_CLEANUP_ON_STATUS(status);
                }
            }
            break;

            case SYSTEM_AUDIT_ACE_TYPE:
            {
                pAuditAce = (PSYSTEM_AUDIT_ACE)pAceHeader;
                mask = pAuditAce->Mask;
                if (IsContainerObject &&
                    !IsSetFlag(pAceHeader->AceFlags, NO_PROPAGATE_INHERIT_ACE))
                {
                    status = RtlAddSystemAuditAceEx(
                                 pAcl,
                                 ACL_REVISION,
                                 pAuditAce->Header.AceFlags,
                                 pAuditAce->Mask,
                                 (PSID)&pAuditAce->SidStart);
                    GOTO_CLEANUP_ON_STATUS(status);
                }
            }
            break;

            default:
                // Skip all other types
                continue;
        }

        // Map the generic bits to specific bits and remove inherit flags

        RtlMapGenericMask(&mask, GenericMap);
        AceFlags = pAceHeader->AceFlags;
        AceFlags &= ~(CONTAINER_INHERIT_ACE|
                      OBJECT_INHERIT_ACE|
                      INHERIT_ONLY_ACE);
        AceFlags |= INHERITED_ACE;

        // Set the inherited direct ACE
        // Deal with CREATOR OWNER and CREATOR GROUP

        switch(pAceHeader->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;

            pAceSid = (PSID)&pAllowAce->SidStart;
            if (RtlEqualSid(pAceSid, &CreatorOwner.sid))
            {
                pAceSid = pTokenOwnerInformation->Owner;
            }
            else if (RtlEqualSid(pAceSid, &CreatorGroup.sid))
            {
                pAceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
            }                

            status = RtlAddAccessAllowedAceEx(
                         pAcl,
                         ACL_REVISION,
                         AceFlags,
                         mask,
                         pAceSid);
            GOTO_CLEANUP_ON_STATUS(status);

            break;

        case ACCESS_DENIED_ACE_TYPE:
            pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;

            pAceSid = (PSID)&pDenyAce->SidStart;
            if (RtlEqualSid(pAceSid, &CreatorOwner.sid))
            {
                pAceSid = pTokenOwnerInformation->Owner;
            }
            else if (RtlEqualSid(pAceSid, &CreatorGroup.sid))
            {
                pAceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
            }                

            status = RtlAddAccessDeniedAceEx(
                         pAcl,
                         ACL_REVISION,
                         AceFlags,
                         mask,
                         pAceSid);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            pAuditAce = (PSYSTEM_AUDIT_ACE)pAceHeader;

            pAceSid = (PSID)&pAuditAce->SidStart;
            if (RtlEqualSid(pAceSid, &CreatorOwner.sid))
            {
                pAceSid = pTokenOwnerInformation->Owner;
            }
            else if (RtlEqualSid(pAceSid, &CreatorGroup.sid))
            {
                pAceSid = pTokenPrimaryGroupInfo->PrimaryGroup;
            }

            status = RtlAddSystemAuditAceEx(
                         pAcl,
                         ACL_REVISION,
                         AceFlags,
                         mask,
                         pAceSid);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        default:
            // Skip all other types
            continue;
        }
    }

    *IsNewAclDefaulted = CreatorIsAclDefaulted;
    *NewAcl = pAcl;

    status = STATUS_SUCCESS;

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pAcl);
    }
    
    return status;
}

static
NTSTATUS
RtlpObjectSetSacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE ParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE CreatorSecDesc,
    IN BOOLEAN IsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PGENERIC_MAPPING GenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL parentSacl = NULL;
    PACL creatorSacl = NULL;
    PACL finalSacl = NULL;
    BOOLEAN creatorIsSaclPresent = FALSE;
    BOOLEAN creatorIsSaclDefaulted = FALSE;
    BOOLEAN parentIsSaclPresent = FALSE;
    BOOLEAN parentIsSaclDefaulted = FALSE;
    BOOLEAN finalIsSaclDefaulted = FALSE;
    SECURITY_DESCRIPTOR_CONTROL creatorSecDescControl = 0;
    SECURITY_DESCRIPTOR_CONTROL saclControlSet = 0;
    SECURITY_DESCRIPTOR_CONTROL saclControlChange = (SE_SACL_PROTECTED|
                                                     SE_SACL_AUTO_INHERITED);

    // Pull the DACLs so we have something to work with

    if (ParentSecDesc && IsSetFlag(AutoInheritFlags, SEF_SACL_AUTO_INHERIT))
    {
        status = RtlGetSaclSecurityDescriptor(
                     ParentSecDesc,
                     &parentIsSaclPresent,
                     &parentSacl,
                     &parentIsSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (CreatorSecDesc)
    {
        status = RtlGetSaclSecurityDescriptor(
                     CreatorSecDesc,
                     &creatorIsSaclPresent,
                     &creatorSacl,
                     &creatorIsSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlGetSecurityDescriptorControl(
                     CreatorSecDesc,
                     &creatorSecDescControl,
                     NULL);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IsSetFlag(creatorSecDescControl, SE_SACL_PROTECTED))
    {
        // If the creator wants to block inheritance, we are done
        if (creatorIsSaclPresent)
        {
            status = RtlpDuplicateAcl(&finalSacl, creatorSacl);
            GOTO_CLEANUP_ON_STATUS(status);
        }

        finalIsSaclDefaulted = FALSE;
    }
    else if (parentIsSaclPresent || creatorIsSaclPresent)
    {
        // Do the inheritance if we have at least one SecDesc to work with
        status = RtlpObjectInheritSecurity(
                     &finalSacl,
                     &finalIsSaclDefaulted,
                     parentIsSaclPresent ? parentSacl : NULL,
                     parentIsSaclDefaulted,
                     creatorIsSaclPresent ? creatorSacl : NULL,
                     creatorIsSaclDefaulted,
                     IsContainerObject,
                     AccessToken,
                     GenericMap);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Finally set the new DACL in the outgoing Seccurity Descriptor

    if (creatorSecDescControl & SE_SACL_PROTECTED)
    {
        saclControlSet |= SE_SACL_PROTECTED;
    }
    else if (AutoInheritFlags & SEF_SACL_AUTO_INHERIT)
    {
        saclControlSet |= SE_SACL_AUTO_INHERITED;
    }

    status = RtlSetSecurityDescriptorControl(
                 SecurityDescriptor,
                 saclControlChange,
                 saclControlSet);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetSaclSecurityDescriptor(
                 SecurityDescriptor,
                 finalSacl ? TRUE : FALSE,
                 finalSacl,
                 finalIsSaclDefaulted);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&finalSacl);
    }

    return status;
}

