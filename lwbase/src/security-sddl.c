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
 *        security-sddl.c
 *
 * Abstract:
 *
 *        Security Descriptor (SD) SDDL conversion Functions in Security Module.
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "security-includes.h"


typedef struct _SDDL_ACE {
    UCHAR AceType;
    UCHAR AceFlag;
    ACCESS_MASK Access;
    PSTR pszObjectGuid; // Currenlty not supported (NULL)
    PSTR pszObjectInheritedGuid; // Currently not supported (NULL)
    PSID pSid;
} SDDL_ACE, *PSDDL_ACE;

typedef NTSTATUS (*RTL_GET_SID_CALLBACK)(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Sid,
    OUT PBOOLEAN IsDefaulted
    );


typedef NTSTATUS (*RTL_GET_ACL_CALLBACK)(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsAclPresent,
    OUT PACL* Acl,
    OUT PBOOLEAN IsAclDefaulted
    );

static
void
RtlpFreeStringArray(
    PSTR * ppStringArray,
    size_t sSize
    );

static
NTSTATUS
RtlpGetSddlSidStringFromSid(
    OUT PSTR* ppszSddlSidString,
    IN PSID pSid
    );

static
NTSTATUS
RtlpGetSddlSidStringFromSecurityDescriptor(
    OUT PSTR* ppszSddlSidString,
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptorAbs,
    IN RTL_GET_SID_CALLBACK pGetSidCallBack
    );

static
NTSTATUS
RtlpGetSddlAceStringFromSecurityDescriptor(
    OUT PSTR* ppszSddlAceString,
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptorAbs,
    IN RTL_GET_ACL_CALLBACK pGetAclCallBack
    );

static
PCSTR
RtlpMapAceTypeToSddlType(
    UCHAR AceType
    );

static
UCHAR
RtlpMapSddlTypeToAceType(
    PCSTR pszAceType
    );

static
NTSTATUS
RtlpMapAceFlagToSddlFlag(
    IN UCHAR AceFlag,
    OUT PSTR* ppszAceFlag
    );

static
UCHAR
RtlpMapSddlFlagToAceFlag(
    PCSTR pszAceFlag
    );

static
NTSTATUS
RtlpMapAccessMaskToSddlRights(
    IN ACCESS_MASK Access,
    OUT PSTR* ppszMask
    );

static
ACCESS_MASK
RtlpMapSddlRightsToAccessMask(
    IN PCSTR pszMask
    );

static
VOID
RtlSddlSafeFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

static
NTSTATUS
RtlpParseSddlString(
    OUT PSTR* ppszOwner,
    OUT PSTR* ppszGroup,
    OUT PSTR* ppszDacl,
    OUT PSTR* ppszSacl,
    IN PCSTR pszSddlString
    );

static
NTSTATUS
RtlpParseSddlAclString(
    OUT PSTR** pppszAceStrings,
    OUT size_t* psCount,
    IN PCSTR pszSddlAclString
    );

static
NTSTATUS
RtlpParseSddlAceString(
    OUT PSDDL_ACE* ppSddlAce,
    IN PCSTR pszSddlAceString
    );

static
NTSTATUS
RtlpGetAclFromSddlAclString(
    OUT PACL* ppAcl,
    OUT PSECURITY_DESCRIPTOR_CONTROL pControl,
    IN PSTR pszAclString,
    IN BOOLEAN bIsDacl
    );

static
NTSTATUS
RtlpMapAclControlToSddlControl(
    IN SECURITY_DESCRIPTOR_CONTROL Control,
    IN BOOLEAN bIsDacl,
    PSTR* ppszControl
    );

static
NTSTATUS
RtlpMapSddlControlToAclControl(
    IN PCSTR pszControl,
    IN BOOLEAN bIsDacl,
    SECURITY_DESCRIPTOR_CONTROL* pControl
    );

NTSTATUS
RtlGetSecurityInformationFromSddlCString(
    IN PCSTR pszStringSecurityDescriptor,
    OUT SECURITY_INFORMATION* pSecInfo
    )
{
    NTSTATUS status = 0;
    PSTR pszOwnerPrefix = NULL;
    PSTR pszGroupPrefix = NULL;
    PSTR pszDaclPrefix = NULL;
    PSTR pszSaclPrefix = NULL;
    SECURITY_INFORMATION SecInfo = 0;

    if (LwRtlCStringIsNullOrEmpty(pszStringSecurityDescriptor))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlCStringAllocatePrintf(&pszOwnerPrefix,
                                      "%s%c",
                                      SDDL_OWNER,
                                      SDDL_DELIMINATOR_C);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCStringAllocatePrintf(&pszGroupPrefix,
                                      "%s%c",
                                      SDDL_GROUP,
                                      SDDL_DELIMINATOR_C);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCStringAllocatePrintf(&pszDaclPrefix,
                                      "%s%c",
                                      SDDL_DACL,
                                      SDDL_DELIMINATOR_C);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCStringAllocatePrintf(&pszSaclPrefix,
                                      "%s%c",
                                      SDDL_SACL,
                                      SDDL_DELIMINATOR_C);
    GOTO_CLEANUP_ON_STATUS(status);

    if (strstr(pszStringSecurityDescriptor, pszOwnerPrefix))
    {
        SecInfo |= OWNER_SECURITY_INFORMATION;
    }
    // a SDDL string has to have owner
    else
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (strstr(pszStringSecurityDescriptor, pszGroupPrefix))
    {
        SecInfo |= GROUP_SECURITY_INFORMATION;
    }

    if (strstr(pszStringSecurityDescriptor, pszDaclPrefix))
    {
        SecInfo |= DACL_SECURITY_INFORMATION;
    }

    if (strstr(pszStringSecurityDescriptor, pszSaclPrefix))
    {
        SecInfo |= SACL_SECURITY_INFORMATION;
    }

cleanup:

    *pSecInfo = SecInfo;

    RTL_FREE(&pszOwnerPrefix);
    RTL_FREE(&pszGroupPrefix);
    RTL_FREE(&pszDaclPrefix);
    RTL_FREE(&pszSaclPrefix);

    return status;
}


NTSTATUS
RtlAllocateSecurityDescriptorFromSddlCString(
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecurityDescriptor,
    OUT OPTIONAL PULONG pSecurityDescriptorLength,
    IN PCSTR pszStringSecurityDescriptor,
    IN ULONG SddlRevision
    )
{
    NTSTATUS status = 0;
    PSTR pszOwner = NULL;
    PSTR pszGroup = NULL;
    PSTR pszDacl = NULL;
    PSTR pszSacl = NULL;

    PCSTR pszOwnerSid = NULL;
    PCSTR pszGroupSid = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;

    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;

    status = LW_RTL_ALLOCATE(&pSecDescAbs,
                            VOID,
                            SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(pSecDescAbs,
                                                 SECURITY_DESCRIPTOR_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    if (SDDL_REVISION_1 != SddlRevision ||
        LwRtlCStringIsNullOrEmpty(pszStringSecurityDescriptor))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Parse SDDL string into maximum four parts

    status = RtlpParseSddlString(&pszOwner,
                                 &pszGroup,
                                 &pszDacl,
                                 &pszSacl,
                                 pszStringSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    // Check to see if it is alias sddl sid-string
    // OWNER
    pszOwnerSid = RtlpSddlToSidString(pszOwner);

    if (LwRtlCStringIsNullOrEmpty(pszOwnerSid))
    {
        pszOwnerSid = pszOwner;
    }

    // Check OWNER
    if (LwRtlCStringIsNullOrEmpty(pszOwnerSid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlAllocateSidFromCString(&pOwnerSid, pszOwnerSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetOwnerSecurityDescriptor(
                 pSecDescAbs,
                 pOwnerSid,
                 FALSE);
    GOTO_CLEANUP_ON_STATUS(status);
    pOwnerSid = NULL;

    // GROUP
    pszGroupSid = RtlpSddlToSidString(pszGroup);

    if (LwRtlCStringIsNullOrEmpty(pszGroupSid))
    {
        pszGroupSid = pszGroup;
    }

    // Check GROUP
    if (LwRtlCStringIsNullOrEmpty(pszGroupSid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlAllocateSidFromCString(&pGroupSid, pszGroupSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetGroupSecurityDescriptor(
                 pSecDescAbs,
                 pGroupSid,
                 FALSE);
    GOTO_CLEANUP_ON_STATUS(status);
    pGroupSid = NULL;

    // DACL (no dacl flag, deal with that later
    status = RtlpGetAclFromSddlAclString(&pDacl,
                                         &pSecDescAbs->Control,
                                         pszDacl,
                                         TRUE);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pDacl)
    {
        status = RtlSetDaclSecurityDescriptor(pSecDescAbs,
                                              TRUE,
                                              pDacl,
                                              FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
        pDacl = NULL;
    }

    // DACL (no dacl flag, deal with that later
    status = RtlpGetAclFromSddlAclString(&pSacl,
                                         &pSecDescAbs->Control,
                                         pszSacl,
                                         FALSE);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pSacl)
    {
        status = RtlSetSaclSecurityDescriptor(pSecDescAbs,
                                              TRUE,
                                              pSacl,
                                              FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
        pSacl = NULL;
    }

    if (!RtlValidSecurityDescriptor(pSecDescAbs))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    do
    {
        if ((pSecDescRel = RtlMemoryRealloc(pSecDescRel, ulSecDescLen)) == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        memset(pSecDescRel, 0, ulSecDescLen);

        status = RtlAbsoluteToSelfRelativeSD(pSecDescAbs,
                                             pSecDescRel,
                                             &ulSecDescLen);
        if (STATUS_BUFFER_TOO_SMALL  == status)
        {
            ulSecDescLen *= 2;
        }
        else
        {
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }
    while((status != STATUS_SUCCESS) &&
          (ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));

    status = STATUS_SUCCESS;

cleanup:

    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSacl);

    LW_RTL_FREE(&pszOwner);
    LW_RTL_FREE(&pszGroup);
    LW_RTL_FREE(&pszDacl);
    LW_RTL_FREE(&pszSacl);

    RtlSddlSafeFreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pSecDescRel);
    }

    *ppSecurityDescriptor = pSecDescRel;
    if (pSecurityDescriptorLength)
    {
        *pSecurityDescriptorLength = ulSecDescLen;
    }

    return status;
}


NTSTATUS
RtlAllocateSddlCStringFromSecurityDescriptor(
    OUT PSTR* ppszStringSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG SddlRevision,
    IN SECURITY_INFORMATION SecurityInformation
    )
{
    NTSTATUS status = 0;
    ULONG ulSecDescRelLength = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    ULONG ulSecDescAbsLen = 0;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;

    // Owner
    PSTR pszOwnerSid = NULL;
    // Group
    PSTR pszGroupSid = NULL;
    // DACL
    PSTR pszDacl = NULL;
    PSTR pszDaclControl = NULL;
    // SACL
    PSTR pszSacl = NULL;
    PSTR pszSaclControl = NULL;
    // SDDL
    PSTR pszStringSecurityDescriptor = NULL;
    size_t sSddlLength = 0;
    size_t sWorkingLength = 0;

    if (SDDL_REVISION_1 != SddlRevision || !pSecurityDescriptor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    ulSecDescRelLength = RtlLengthSecurityDescriptorRelative(pSecurityDescriptor);

    if (!RtlValidRelativeSecurityDescriptor(
                    pSecurityDescriptor,
                    ulSecDescRelLength,
                    0))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(
                   pSecurityDescriptor,
                    NULL,
                    &ulSecDescAbsLen,
                    NULL,
                    &ulDaclLen,
                    NULL,
                    &ulSaclLen,
                    NULL,
                    &ulOwnerLen,
                    NULL,
                    &ulGroupLen);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&pSecDescAbs, VOID, ulSecDescAbsLen);
    GOTO_CLEANUP_ON_STATUS(status);

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

    if (ulOwnerLen)
    {
        status = RTL_ALLOCATE(&pOwner, VOID, ulOwnerLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulGroupLen)
    {
        status = RTL_ALLOCATE(&pGroup, VOID, ulGroupLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(
                    pSecurityDescriptor,
                    pSecDescAbs,
                    &ulSecDescAbsLen,
                    pDacl,
                    &ulDaclLen,
                    pSacl,
                    &ulSaclLen,
                    pOwner,
                    &ulOwnerLen,
                    pGroup,
                    &ulGroupLen);
    GOTO_CLEANUP_ON_STATUS(status);

    // (1) SDDL-Owner  O:
    if (IsSetFlag(SecurityInformation, OWNER_SECURITY_INFORMATION))
    {
        status = RtlpGetSddlSidStringFromSecurityDescriptor(
                               &pszOwnerSid,
                               pSecDescAbs,
                               RtlGetOwnerSecurityDescriptor
                               );
        GOTO_CLEANUP_ON_STATUS(status);

        if (!LwRtlCStringIsNullOrEmpty(pszOwnerSid))
        {
            sSddlLength += LwRtlCStringNumChars(SDDL_OWNER) + LwRtlCStringNumChars(SDDL_DELIMINATOR_S); // O:
            sSddlLength += LwRtlCStringNumChars(pszOwnerSid);
        }
    }

    // (2) SDDL - Group G:
    if (IsSetFlag(SecurityInformation, GROUP_SECURITY_INFORMATION))
    {
        status = RtlpGetSddlSidStringFromSecurityDescriptor(
                               &pszGroupSid,
                               pSecDescAbs,
                               RtlGetGroupSecurityDescriptor
                               );
        GOTO_CLEANUP_ON_STATUS(status);

        if (!LwRtlCStringIsNullOrEmpty(pszGroupSid))
        {
            sSddlLength += LwRtlCStringNumChars(SDDL_GROUP) + LwRtlCStringNumChars(SDDL_DELIMINATOR_S); // G:
            sSddlLength += LwRtlCStringNumChars(pszGroupSid);
        }
    }

    // (3) SDDL - DACL D:
    if (IsSetFlag(SecurityInformation, DACL_SECURITY_INFORMATION))
    {
        status = RtlpGetSddlAceStringFromSecurityDescriptor(
                                &pszDacl,
                                pSecDescAbs,
                                RtlGetDaclSecurityDescriptor
                                );
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlpMapAclControlToSddlControl(pSecDescAbs->Control,
                                                TRUE,
                                                &pszDaclControl);
        GOTO_CLEANUP_ON_STATUS(status);

        if (!LwRtlCStringIsNullOrEmpty(pszDacl))
        {
            sSddlLength += LwRtlCStringNumChars(SDDL_DACL) + LwRtlCStringNumChars(SDDL_DELIMINATOR_S); // D:
            if (!LwRtlCStringIsNullOrEmpty(pszDaclControl))
            {
                sSddlLength += LwRtlCStringNumChars(pszDaclControl);
            }
            sSddlLength += LwRtlCStringNumChars(pszDacl);
        }
    }

    // (4) SDDL - SACL S:
    if (IsSetFlag(SecurityInformation, SACL_SECURITY_INFORMATION))
    {
        status = RtlpGetSddlAceStringFromSecurityDescriptor(
                                &pszSacl,
                                pSecDescAbs,
                                RtlGetSaclSecurityDescriptor
                                );
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlpMapAclControlToSddlControl(pSecDescAbs->Control,
                                                FALSE,
                                                &pszSaclControl);
        GOTO_CLEANUP_ON_STATUS(status);

        if (!LwRtlCStringIsNullOrEmpty(pszSacl))
        {
            sSddlLength += LwRtlCStringNumChars(SDDL_SACL) + LwRtlCStringNumChars(SDDL_DELIMINATOR_S); // S:
            if (!LwRtlCStringIsNullOrEmpty(pszSaclControl))
            {
                sSddlLength += LwRtlCStringNumChars(pszSaclControl);
            }
            sSddlLength += LwRtlCStringNumChars(pszSacl);
        }
    }

    sSddlLength++; // include NULL

    status = RTL_ALLOCATE(&pszStringSecurityDescriptor, CHAR, sSddlLength);
    GOTO_CLEANUP_ON_STATUS(status);

    sWorkingLength = RtlCStringNumChars(pszStringSecurityDescriptor);

    if (IsSetFlag(SecurityInformation, OWNER_SECURITY_INFORMATION) &&
         !LwRtlCStringIsNullOrEmpty(pszOwnerSid))
    {
        strncat(pszStringSecurityDescriptor, SDDL_OWNER, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_OWNER);

        strncat(pszStringSecurityDescriptor, SDDL_DELIMINATOR_S, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_DELIMINATOR_S);

        strncat(pszStringSecurityDescriptor, pszOwnerSid, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(pszOwnerSid);
    }

    if (IsSetFlag(SecurityInformation, GROUP_SECURITY_INFORMATION) &&
         !LwRtlCStringIsNullOrEmpty(pszGroupSid))
    {
        strncat(pszStringSecurityDescriptor, SDDL_GROUP, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_GROUP);

        strncat(pszStringSecurityDescriptor, SDDL_DELIMINATOR_S, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_DELIMINATOR_S);

        strncat(pszStringSecurityDescriptor, pszGroupSid, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(pszGroupSid);
    }

    if (IsSetFlag(SecurityInformation, DACL_SECURITY_INFORMATION) &&
         !LwRtlCStringIsNullOrEmpty(pszDacl))
    {
        strncat(pszStringSecurityDescriptor, SDDL_DACL, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_DACL);

        strncat(pszStringSecurityDescriptor, SDDL_DELIMINATOR_S, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_DELIMINATOR_S);

        if (!LwRtlCStringIsNullOrEmpty(pszDaclControl))
        {
            strncat(pszStringSecurityDescriptor, pszDaclControl, sSddlLength-sWorkingLength-1);
            sWorkingLength += RtlCStringNumChars(pszDaclControl);
        }

        strncat(pszStringSecurityDescriptor, pszDacl, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(pszDacl);
    }

    if (IsSetFlag(SecurityInformation, SACL_SECURITY_INFORMATION) &&
         !LwRtlCStringIsNullOrEmpty(pszSacl))
    {
        strncat(pszStringSecurityDescriptor, SDDL_SACL, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_SACL);

        strncat(pszStringSecurityDescriptor, SDDL_DELIMINATOR_S, sSddlLength-sWorkingLength-1);
        sWorkingLength += RtlCStringNumChars(SDDL_DELIMINATOR_S);

        if (!LwRtlCStringIsNullOrEmpty(pszSaclControl))
        {
            strncat(pszStringSecurityDescriptor, pszSaclControl, sSddlLength-sWorkingLength-1);
            sWorkingLength += RtlCStringNumChars(pszSaclControl);
        }

        strncat(pszStringSecurityDescriptor, pszSacl, sSddlLength-sWorkingLength-1);
    }

    status = STATUS_SUCCESS;

cleanup:

    RtlSddlSafeFreeAbsoluteSecurityDescriptor(&pSecDescAbs);
    RTL_FREE(&pszOwnerSid);
    RTL_FREE(&pszGroupSid);
    RTL_FREE(&pszDacl);
    RTL_FREE(&pszSacl);
    RTL_FREE(&pszDaclControl);
    RTL_FREE(&pszSaclControl);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszStringSecurityDescriptor);
    }

    *ppszStringSecurityDescriptor = pszStringSecurityDescriptor;

    return status;
}

static
VOID
RtlSddlSafeFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL)) {
        return;
    }

    pSecDesc = *ppSecDesc;

    RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);
    RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    LW_RTL_FREE(&pOwner);
    LW_RTL_FREE(&pGroup);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSacl);
    LW_RTL_FREE(&pSecDesc);

    *ppSecDesc = NULL;

    return;
}

static
NTSTATUS
RtlpGetSddlSidStringFromSecurityDescriptor(
    OUT PSTR* ppszSddlSidString,
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptorAbs,
    IN RTL_GET_SID_CALLBACK pGetSidCallBack
    )
{
    NTSTATUS status = 0;
    PSTR pszSddlSidString = NULL;
    // Owner or Group
    // Do not free
    PSID pSid = NULL;
    BOOLEAN IsDefaulted = FALSE;

    status = pGetSidCallBack(pSecurityDescriptorAbs,
                             &pSid,
                             &IsDefaulted);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpGetSddlSidStringFromSid(&pszSddlSidString,
                                         pSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszSddlSidString);
    }

    *ppszSddlSidString = pszSddlSidString;

    return status;
}


static
NTSTATUS
RtlpGetSddlSidStringFromSid(
    OUT PSTR* ppszSddlSidString,
    IN PSID pSid
    )
{
    NTSTATUS status = 0;
    PSTR pszSddlSidString = NULL;
    ULONG ulRid = 0;
    PSTR pszSid = NULL;
    // Do not free
    PCSTR pszSddlSid = NULL;


    status = RtlAllocateCStringFromSid(&pszSid,
                                       pSid);
    GOTO_CLEANUP_ON_STATUS(status);

    // Deal with SECURITY_CREATOR_OWNER_RID and SECURITY_WORLD_RID first
    // Due to the rid value both being zero
    if (LwRtlCStringIsEqual(pszSid, SECURITY_WORLD_PREFIX, FALSE))
    {
        pszSddlSid = SDDL_EVERYONE;

    }
    else if (LwRtlCStringIsEqual(pszSid, SECURITY_CREATOR_OWNER_PREFIX, FALSE))
    {
        pszSddlSid = SDDL_CREATOR_OWNER;
    }
    else
    {
        status = RtlGetRidSid(&ulRid,
                              pSid);
        GOTO_CLEANUP_ON_STATUS(status);

        pszSddlSid = RtlpRidToSddl(ulRid);
        // Exclude SDDL_EVERYONE and SDDL_CREATOR_OWNER
        // As it is handled above
        if (pszSddlSid == NULL ||
            (pszSddlSid &&
            (LwRtlCStringIsEqual(pszSddlSid, SDDL_EVERYONE, FALSE) ||
            LwRtlCStringIsEqual(pszSddlSid, SDDL_CREATOR_OWNER, FALSE)))
            )
        {
            pszSddlSid = pszSid;
        }
    }

    status  = LwRtlCStringDuplicate(&pszSddlSidString,
                                    pszSddlSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = STATUS_SUCCESS;

cleanup:

    RTL_FREE(&pszSid);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszSddlSidString);
    }

    *ppszSddlSidString = pszSddlSidString;

    return status;
}

static
NTSTATUS
RtlpGetSddlAceStringFromSecurityDescriptor(
    OUT PSTR* ppszSddlAceString,
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptorAbs,
    IN RTL_GET_ACL_CALLBACK pGetAclCallBack
    )
{
    NTSTATUS status = 0;
    // Do not free
    PACL pAcl = NULL;
    BOOLEAN bPresent = FALSE;
    BOOLEAN bDefaulted = FALSE;
    PVOID pAce = NULL;
    ULONG ulAceIndex = 0;

    PSTR pszSddlSidString = NULL;
    PSTR pszAceAcl = NULL;
    PSTR pszAceFlag = NULL;
    PSTR* ppszAceStrings = NULL;
    size_t sAceStringLength = 0;
    USHORT usAceCount = 0;
    size_t sFullLength = 0;
    PSTR pszFullAceString = NULL;

    status = pGetAclCallBack(
                  pSecurityDescriptorAbs,
                  &bPresent,
                  &pAcl,
                  &bDefaulted);
    GOTO_CLEANUP_ON_STATUS(status);

    if (!bPresent || !pAcl)
    {
        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    usAceCount = RtlGetAclAceCount(pAcl);

    if (!usAceCount)
    {
        GOTO_CLEANUP();
    }

    status = RTL_ALLOCATE(&ppszAceStrings, PSTR, sizeof(*ppszAceStrings) * usAceCount);
    GOTO_CLEANUP_ON_STATUS(status);

    for (ulAceIndex = 0; ulAceIndex < usAceCount; ulAceIndex++)
    {
        PACE_HEADER pAceHeader = NULL;
        PCSTR pszAceType = NULL;

        status = RtlGetAce(pAcl, ulAceIndex, OUT_PPVOID(&pAce));
        GOTO_CLEANUP_ON_STATUS(status);

        pAceHeader = (PACE_HEADER)pAce;

        switch (pAceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            case ACCESS_DENIED_ACE_TYPE:
            case SYSTEM_AUDIT_ACE_TYPE:
            {
                // These are all isomorphic.
                PACCESS_ALLOWED_ACE pAllowedAce = (PACCESS_ALLOWED_ACE) pAce;


                status = RtlpMapAccessMaskToSddlRights(pAllowedAce->Mask,
                                                       &pszAceAcl);
                GOTO_CLEANUP_ON_STATUS(status);

                status = RtlpGetSddlSidStringFromSid(&pszSddlSidString,
                                                    (PSID)&pAllowedAce->SidStart);
                GOTO_CLEANUP_ON_STATUS(status);
                break;
            }
            default:
                status = STATUS_NOT_SUPPORTED;
                GOTO_CLEANUP_ON_STATUS(status);
        }

        // Get the AceFlag string for the currently supported ACE
        pszAceType = RtlpMapAceTypeToSddlType(pAceHeader->AceType);

        // An Ace has to have a valid supported type
        if (LwRtlCStringIsNullOrEmpty(pszAceType))
        {
            status = STATUS_NOT_SUPPORTED;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        status = RtlpMapAceFlagToSddlFlag(pAceHeader->AceFlags,
                                          &pszAceFlag);
        GOTO_CLEANUP_ON_STATUS(status);

        // Construct ACE string
        // Object ACE is currently not supported
        // ace_type;ace_flags;rights;;;account_sid
        // A sample ACE string (A;;RPWPCCDCLCSWRCWDWOGA;;;S-1-0-0)
        sAceStringLength = 1; // '('
        // ACE_TYPE + ";"
        sAceStringLength += LwRtlCStringNumChars(pszAceType) + 1;
        // ACE_FLAG + ";"
        sAceStringLength += (!LwRtlCStringIsNullOrEmpty(pszAceFlag) ? LwRtlCStringNumChars(pszAceFlag) : 0) + 1;
        // ACE_ACCESS + ";"
        sAceStringLength += (!LwRtlCStringIsNullOrEmpty(pszAceAcl) ? LwRtlCStringNumChars(pszAceAcl) : 0) + 1;
        // ;; unsupported guid, guid_inherited
        sAceStringLength += 1 + 1;
        // ACE_SID and ")"

        // An Ace has to have a valid Sid
        if (LwRtlCStringIsNullOrEmpty(pszSddlSidString))
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        sAceStringLength += LwRtlCStringNumChars(pszSddlSidString) + 1;

        status = RTL_ALLOCATE(&ppszAceStrings[ulAceIndex], CHAR, sAceStringLength+1);
        GOTO_CLEANUP_ON_STATUS(status);

        strcat(ppszAceStrings[ulAceIndex], "(");

        strcat(ppszAceStrings[ulAceIndex], pszAceType);
        strcat(ppszAceStrings[ulAceIndex], ";");

        if (!LwRtlCStringIsNullOrEmpty(pszAceFlag))
        {
            strcat(ppszAceStrings[ulAceIndex], pszAceFlag);
        }
        strcat(ppszAceStrings[ulAceIndex], ";");

        if (!LwRtlCStringIsNullOrEmpty(pszAceAcl))
        {
            strcat(ppszAceStrings[ulAceIndex], pszAceAcl);
        }
        strcat(ppszAceStrings[ulAceIndex], ";");

        strcat(ppszAceStrings[ulAceIndex], ";");
        strcat(ppszAceStrings[ulAceIndex], ";");

        strcat(ppszAceStrings[ulAceIndex], pszSddlSidString);
        strcat(ppszAceStrings[ulAceIndex], ")");

        RTL_FREE(&pszSddlSidString);
        RTL_FREE(&pszAceAcl);
        RTL_FREE(&pszAceFlag);
        sAceStringLength = 0;
    }

    if (usAceCount != ulAceIndex)
    {
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    for (ulAceIndex = 0; ulAceIndex < usAceCount; ulAceIndex++)
    {
        if (!LwRtlCStringIsNullOrEmpty(ppszAceStrings[ulAceIndex]))
        {
            sFullLength += LwRtlCStringNumChars(ppszAceStrings[ulAceIndex]);
        }
    }

    status = RTL_ALLOCATE(&pszFullAceString, CHAR, sFullLength+1);
    GOTO_CLEANUP_ON_STATUS(status);

    for (ulAceIndex = 0; ulAceIndex < usAceCount; ulAceIndex++)
    {
        if (!LwRtlCStringIsNullOrEmpty(ppszAceStrings[ulAceIndex]))
        {
            strcat(pszFullAceString, ppszAceStrings[ulAceIndex]);
        }
    }

    status = STATUS_SUCCESS;

cleanup:

    RTL_FREE(&pszSddlSidString);
    RTL_FREE(&pszAceAcl);
    RTL_FREE(&pszAceFlag);

    RtlpFreeStringArray(ppszAceStrings, usAceCount);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszFullAceString);
    }

    *ppszSddlAceString = pszFullAceString;

    return status;
}

static
PCSTR
RtlpMapAceTypeToSddlType(
    UCHAR AceType
    )
{
    switch (AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            return SDDL_ACCESS_ALLOWED;
        case ACCESS_DENIED_ACE_TYPE:
            return SDDL_ACCESS_DENIED;
        case SYSTEM_AUDIT_ACE_TYPE:
            return  SDDL_AUDIT;
        // Currently not supported ACE types
#if 0
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            return SDDL_OBJECT_ACCESS_ALLOWED;
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
            return SDDL_OBJECT_ACCESS_DENIED;
        case SYSTEM_ALARM_ACE_TYPE:
            return  SDDL_ALARM;
        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
            return SDDL_OBJECT_AUDIT;
        case SYSTEM_ALARM_OBJECT_ACE_TYPE:
            return  SDDL_OBJECT_ALARM;
        case SYSTEM_MANDATORY_LABEL_ACE:
            return SDDL_MANDATORY_LABEL;
#endif

        default:
            return NULL;
    }
}

static
UCHAR
RtlpMapSddlTypeToAceType(
    PCSTR pszAceType
    )
{
    if (LwRtlCStringIsEqual(pszAceType, SDDL_ACCESS_ALLOWED, TRUE))
    {
        return ACCESS_ALLOWED_ACE_TYPE;
    }
    if (LwRtlCStringIsEqual(pszAceType, SDDL_ACCESS_DENIED, TRUE))
    {
        return ACCESS_DENIED_ACE_TYPE;
    }
    if (LwRtlCStringIsEqual(pszAceType, SDDL_AUDIT, TRUE))
    {
        return SYSTEM_AUDIT_ACE_TYPE;
    }

    return ACCESS_UNKNOWN_ACE_TYPE;
}


static
NTSTATUS
RtlpMapAceFlagToSddlFlag(
    IN UCHAR AceFlag,
    OUT PSTR* ppszAceFlag
    )
{
    NTSTATUS status = 0;
    PSTR pszAceFlag = NULL;

    status = RTL_ALLOCATE(&pszAceFlag, CHAR, SDDL_ACEFLAG_LENGTH);
    GOTO_CLEANUP_ON_STATUS(status);

    if (AceFlag & CONTAINER_INHERIT_ACE)
    {
        strcat(pszAceFlag, SDDL_CONTAINER_INHERIT);
    }

    if (AceFlag & OBJECT_INHERIT_ACE)
    {
        strcat(pszAceFlag, SDDL_OBJECT_INHERIT);
    }

    if (AceFlag & NO_PROPAGATE_INHERIT_ACE)
    {
        strcat(pszAceFlag, SDDL_NO_PROPAGATE);
    }

    if (AceFlag & INHERIT_ONLY_ACE)
    {
        strcat(pszAceFlag, SDDL_INHERIT_ONLY);
    }

    if (AceFlag & INHERITED_ACE)
    {
        strcat(pszAceFlag, SDDL_INHERITED);
    }

    if (AceFlag & SUCCESSFUL_ACCESS_ACE_FLAG)
    {
        strcat(pszAceFlag, SDDL_AUDIT_SUCCESS);
    }

    if (AceFlag & FAILED_ACCESS_ACE_FLAG)
    {
        strcat(pszAceFlag, SDDL_AUDIT_FAILURE);
    }

    status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszAceFlag);
    }

    *ppszAceFlag = pszAceFlag;

    return status;
}

static
UCHAR
RtlpMapSddlFlagToAceFlag(
    IN PCSTR pszAceFlag
    )
{
    UCHAR AceFlag = 0;
    size_t sLength = LwRtlCStringNumChars(pszAceFlag);
    int i = 0;
    CHAR szAceFlag[3] = {0};

    if (!sLength)
    {
        return AceFlag;
    }

    for (i = 0; i < sLength/SDDL_ACEFLAG_SIZE; i++)
    {
        memset(szAceFlag, 0, 3);
        memcpy(szAceFlag, pszAceFlag+i*SDDL_ACEFLAG_SIZE, SDDL_ACEFLAG_SIZE);

        if (LwRtlCStringIsEqual(szAceFlag, SDDL_CONTAINER_INHERIT, TRUE))
        {
            SetFlag(AceFlag, CONTAINER_INHERIT_ACE);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_OBJECT_INHERIT, TRUE))
        {
            SetFlag(AceFlag, OBJECT_INHERIT_ACE);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_NO_PROPAGATE, TRUE))
        {
            SetFlag(AceFlag, NO_PROPAGATE_INHERIT_ACE);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_INHERIT_ONLY, TRUE))
        {
            SetFlag(AceFlag, INHERIT_ONLY_ACE);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_INHERITED, TRUE))
        {
            SetFlag(AceFlag, INHERITED_ACE);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_AUDIT_SUCCESS, TRUE))
        {
            SetFlag(AceFlag, SUCCESSFUL_ACCESS_ACE_FLAG);
            continue;
        }
        if (LwRtlCStringIsEqual(szAceFlag, SDDL_AUDIT_FAILURE, TRUE))
        {
            SetFlag(AceFlag, FAILED_ACCESS_ACE_FLAG);
            continue;
        }
    }

    return AceFlag;
}

static
NTSTATUS
RtlpMapAccessMaskToSddlRights(
    IN ACCESS_MASK Access,
    OUT PSTR* ppszMask
    )
{
    PSTR pszMask = NULL;
    NTSTATUS status = 0;

    status = RTL_ALLOCATE(&pszMask, CHAR, SDDL_ACCESS_LENGTH);
    GOTO_CLEANUP_ON_STATUS(status);

    // Generic access rights
    if (IsSetFlag(Access, GENERIC_ALL))
    {
        strcat(pszMask, SDDL_GENERIC_ALL);
    }

    if (IsSetFlag(Access, GENERIC_READ))
    {
        strcat(pszMask, SDDL_GENERIC_READ);
    }

    if (IsSetFlag(Access, GENERIC_WRITE))
    {
        strcat(pszMask, SDDL_GENERIC_WRITE);
    }

    if (IsSetFlag(Access, GENERIC_EXECUTE))
    {
        strcat(pszMask, SDDL_GENERIC_EXECUTE);
    }

    // Standard access rights
    if (IsSetFlag(Access, READ_CONTROL))
    {
        strcat(pszMask, SDDL_READ_CONTROL);
    }

    if (IsSetFlag(Access, DELETE))
    {
        strcat(pszMask, SDDL_STANDARD_DELETE);
    }

    if (IsSetFlag(Access, WRITE_DAC))
    {
        strcat(pszMask, SDDL_WRITE_DAC);
    }

    if (IsSetFlag(Access, WRITE_OWNER))
    {
        strcat(pszMask, SDDL_WRITE_OWNER);
    }

    // File access rights
    if ((Access & FILE_ALL_ACCESS) == FILE_ALL_ACCESS)
    {
        strcat(pszMask, SDDL_FILE_ALL);
    }

    if ((Access & FILE_GENERIC_READ) == FILE_GENERIC_READ)
    {
        strcat(pszMask, SDDL_FILE_READ);
    }

    if ((Access & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE)
    {
        strcat(pszMask, SDDL_FILE_WRITE);
    }

    if ((Access & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE)
    {
        strcat(pszMask, SDDL_FILE_EXECUTE);
    }

    // Registry key access rights
    if ((Access & KEY_ALL_ACCESS) == KEY_ALL_ACCESS)
    {
        strcat(pszMask, SDDL_KEY_ALL);
    }

    if ((Access & KEY_READ) == KEY_READ)
    {
        strcat(pszMask, SDDL_KEY_READ);
    }

    if ((Access & KEY_WRITE) == KEY_WRITE)
    {
        strcat(pszMask, SDDL_KEY_WRITE);
    }

    if ((Access & KEY_EXECUTE) == KEY_EXECUTE)
    {
        strcat(pszMask, SDDL_KEY_EXECUTE);
    }

    // Mandatory label rights
    if (IsSetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_READ_UP))
    {
        strcat(pszMask, SDDL_NO_READ_UP);
    }

    if (IsSetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP))
    {
        strcat(pszMask, SDDL_NO_WRITE_UP);
    }

    if (IsSetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP))
    {
        strcat(pszMask, SDDL_NO_EXECUTE_UP);
    }

    // Directory service object access rights
    // Currently Not supported
#if 0
    if (Access & ADS_RIGHT_DS_READ_PROP)
    {
        strcat(pszMask, SDDL_READ_PROPERTY);
    }

    if (Access & ADS_RIGHT_DS_WRITE_PROP)
    {
        strcat(pszMask, SDDL_WRITE_PROPERTY);
    }

    if (Access & ADS_RIGHT_DS_CREATE_CHILD)
    {
        strcat(pszMask, SDDL_CREATE_CHILD);
    }

    if (Access & ADS_RIGHT_DS_DELETE_CHILD)
    {
        strcat(pszMask, SDDL_DELETE_CHILD);
    }

    if (Access & ADS_RIGHT_ACTRL_DS_LIST)
    {
        strcat(pszMask, SDDL_LIST_CHILDREN);
    }

    if (Access & ADS_RIGHT_DS_SELF)
    {
        strcat(pszMask, SDDL_SELF_WRITE);
    }

    if (Access & ADS_RIGHT_DS_LIST_OBJECT)
    {
        strcat(pszMask, SDDL_LIST_OBJECT);
    }

    if (Access & ADS_RIGHT_DS_DELETE_TREE)
    {
        strcat(pszMask, SDDL_DELETE_TREE);
    }

    if (Access & ADS_RIGHT_DS_CONTROL_ACCESS)
    {
        strcat(pszMask, SDDL_CONTROL_ACCESS);
    }
#endif

    status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszMask);
    }

    *ppszMask = pszMask;

    return status;
}

static
ACCESS_MASK
RtlpMapSddlRightsToAccessMask(
    IN PCSTR pszMask
    )
{
    ACCESS_MASK Access = 0;
    size_t sLength = LwRtlCStringNumChars(pszMask);
    int i = 0;
    CHAR szRight[3] = {0};

    if (!sLength)
    {
        return Access;
    }

    for (i = 0; i < sLength/SDDL_RIGHT_SIZE; i++)
    {
        memset(szRight, 0, 3);
        memcpy(szRight, pszMask+i*SDDL_RIGHT_SIZE, 2);

        // Generic access rights
        if (LwRtlCStringIsEqual(szRight, SDDL_GENERIC_ALL, TRUE))
        {
            SetFlag(Access, GENERIC_ALL);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_GENERIC_READ, TRUE))
        {
            SetFlag(Access, GENERIC_READ);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_GENERIC_WRITE, TRUE))
        {
            SetFlag(Access, GENERIC_WRITE);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_GENERIC_EXECUTE, TRUE))
        {
            SetFlag(Access, GENERIC_EXECUTE);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_READ_CONTROL, TRUE))
        {
            SetFlag(Access, READ_CONTROL);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_STANDARD_DELETE, TRUE))
        {
            SetFlag(Access, DELETE);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_WRITE_DAC, TRUE))
        {
            SetFlag(Access, WRITE_DAC);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_WRITE_OWNER, TRUE))
        {
            SetFlag(Access, WRITE_OWNER);
            continue;
        }

        // Directory service object access rights
        // Currently Not supported
    #if 0
        if (Access & ADS_RIGHT_DS_READ_PROP)
        {
            strcat(pszMask, SDDL_READ_PROPERTY);
        }

        if (Access & ADS_RIGHT_DS_WRITE_PROP)
        {
            strcat(pszMask, SDDL_WRITE_PROPERTY);
        }

        if (Access & ADS_RIGHT_DS_CREATE_CHILD)
        {
            strcat(pszMask, SDDL_CREATE_CHILD);
        }

        if (Access & ADS_RIGHT_DS_DELETE_CHILD)
        {
            strcat(pszMask, SDDL_DELETE_CHILD);
        }

        if (Access & ADS_RIGHT_ACTRL_DS_LIST)
        {
            strcat(pszMask, SDDL_LIST_CHILDREN);
        }

        if (Access & ADS_RIGHT_DS_SELF)
        {
            strcat(pszMask, SDDL_SELF_WRITE);
        }

        if (Access & ADS_RIGHT_DS_LIST_OBJECT)
        {
            SetFlag(Access, WRITE_OWNER);
            continue;
        }
            strcat(pszMask, SDDL_LIST_OBJECT);
        }

        if (Access & ADS_RIGHT_DS_DELETE_TREE)
        {
            strcat(pszMask, SDDL_DELETE_TREE);
        }

        if (Access & ADS_RIGHT_DS_CONTROL_ACCESS)
        {
            strcat(pszMask, SDDL_CONTROL_ACCESS);
        }
    #endif
        // File access rights
        if (LwRtlCStringIsEqual(szRight, SDDL_FILE_ALL, TRUE))
        {
            SetFlag(Access, FILE_ALL_ACCESS);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_FILE_READ, TRUE))
        {
            SetFlag(Access, FILE_GENERIC_READ);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_FILE_WRITE, TRUE))
        {
            SetFlag(Access, FILE_GENERIC_WRITE);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_FILE_EXECUTE, TRUE))
        {
            SetFlag(Access, FILE_GENERIC_EXECUTE);
            continue;
        }

        // Registry key access rights
        if (LwRtlCStringIsEqual(szRight, SDDL_KEY_ALL, TRUE))
        {
            SetFlag(Access, KEY_ALL_ACCESS);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_KEY_READ, TRUE))
        {
            SetFlag(Access, KEY_READ);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_KEY_WRITE, TRUE))
        {
            SetFlag(Access, KEY_WRITE);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_KEY_EXECUTE, TRUE))
        {
            SetFlag(Access, KEY_EXECUTE);
            continue;
        }

        // Mandatory label rights
        if (LwRtlCStringIsEqual(szRight, SDDL_NO_READ_UP, TRUE))
        {
            SetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_READ_UP);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_NO_WRITE_UP, TRUE))
        {
            SetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_WRITE_UP);
            continue;
        }

        if (LwRtlCStringIsEqual(szRight, SDDL_NO_EXECUTE_UP, TRUE))
        {
            SetFlag(Access, SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP);
            continue;
        }
    }

    return Access;
}


static
void
RtlpFreeStringArray(
    PSTR * ppStringArray,
    size_t sSize
    )
{
    size_t i = 0;

    if ( ppStringArray )
    {
        for(i = 0; i < sSize; i++)
        {
            if (ppStringArray[i])
            {
                LwRtlCStringFree(&ppStringArray[i]);
            }
        }

        LwRtlMemoryFree(ppStringArray);
    }

    return;
}

static
NTSTATUS
RtlpParseSddlString(
    OUT PSTR* ppszOwner,
    OUT PSTR* ppszGroup,
    OUT PSTR* ppszDacl,
    OUT PSTR* ppszSacl,
    IN PCSTR pszSddlString
    )
{
    NTSTATUS status = 0;
    PSTR pszOwner = NULL;
    PSTR pszGroup = NULL;
    PSTR pszDacl = NULL;
    PSTR pszSacl = NULL;
    // Do not free
    PSTR pszTmp = NULL;
    PSTR pszstrtok_rSav = NULL;

    PSTR pszSddlStringTmp = NULL;
    PSTR pszSddlStringTmp1 = NULL;
    PSTR* ppszTokens = NULL;
    PSTR pszFormattedToken = NULL;
    PSTR pszTmpToken = NULL;
    int iTokenCount = 0;
    int i = 0;

    status  = LwRtlCStringDuplicate(&pszSddlStringTmp,
                                    pszSddlString);
    GOTO_CLEANUP_ON_STATUS(status);

    status  = LwRtlCStringDuplicate(&pszSddlStringTmp1,
                                    pszSddlString);
    GOTO_CLEANUP_ON_STATUS(status);

    pszTmp = strtok_r(pszSddlStringTmp, SDDL_DELIMINATOR_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        iTokenCount++;
        pszTmp = strtok_r(NULL, SDDL_DELIMINATOR_S, &pszstrtok_rSav);
    }

    status = RTL_ALLOCATE(&ppszTokens, PSTR, sizeof(*ppszTokens) * iTokenCount);
    GOTO_CLEANUP_ON_STATUS(status);

    pszTmp = strtok_r(pszSddlStringTmp1, SDDL_DELIMINATOR_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        status  = LwRtlCStringDuplicate(&ppszTokens[i++],
                                        pszTmp);
        GOTO_CLEANUP_ON_STATUS(status);

        pszTmp = strtok_r(NULL, SDDL_DELIMINATOR_S, &pszstrtok_rSav);
    }

    for (i = 0; i < iTokenCount-1; i++)
    {
        int iTokenLength = strlen(ppszTokens[i+1]);

        status = RTL_ALLOCATE(&pszTmpToken,
                              CHAR,
                              sizeof(*pszTmpToken) * iTokenLength);
        GOTO_CLEANUP_ON_STATUS(status);

        memcpy(pszTmpToken, ppszTokens[i+1], iTokenLength-1);

        status = RtlCStringAllocatePrintf(
                      &pszFormattedToken,
                      "%c%c%s",
                      ppszTokens[i][strlen(ppszTokens[i])-1],
                      SDDL_DELIMINATOR_C,
                      pszTmpToken);
        GOTO_CLEANUP_ON_STATUS(status);

        if (!strncmp(pszFormattedToken, SDDL_OWNER, LwRtlCStringNumChars(SDDL_OWNER)))
        {
            size_t sOwnerPrefix = LwRtlCStringNumChars(SDDL_OWNER);
            if (pszFormattedToken[sOwnerPrefix] != SDDL_DELIMINATOR_C)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS(status);
            }

            status  = LwRtlCStringDuplicate(&pszOwner,
                                            &pszFormattedToken[sOwnerPrefix+1]);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else if (!strncmp(pszFormattedToken, SDDL_GROUP, LwRtlCStringNumChars(SDDL_GROUP)))
        {
            size_t sGroupPrefix = LwRtlCStringNumChars(SDDL_GROUP);
            if (pszFormattedToken[sGroupPrefix] != SDDL_DELIMINATOR_C)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS(status);
            }

            status  = LwRtlCStringDuplicate(&pszGroup,
                                            &pszFormattedToken[sGroupPrefix+1]);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else if (!strncmp(pszFormattedToken, SDDL_DACL, LwRtlCStringNumChars(SDDL_DACL)))
        {
            size_t sDaclPrefix = LwRtlCStringNumChars(SDDL_DACL);
            if (pszFormattedToken[sDaclPrefix] != SDDL_DELIMINATOR_C)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS(status);
            }

            status  = LwRtlCStringDuplicate(&pszDacl,
                                            &pszFormattedToken[sDaclPrefix+1]);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else if (!strncmp(pszFormattedToken, SDDL_SACL, LwRtlCStringNumChars(SDDL_SACL)))
        {
            size_t sSaclPrefix = LwRtlCStringNumChars(SDDL_SACL);
            if (pszFormattedToken[sSaclPrefix] != SDDL_DELIMINATOR_C)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS(status);
            }

            status  = LwRtlCStringDuplicate(&pszSacl,
                                            &pszFormattedToken[sSaclPrefix+1]);
            GOTO_CLEANUP_ON_STATUS(status);
        }

        RTL_FREE(&pszFormattedToken);
        RTL_FREE(&pszTmpToken);
    }

    status = STATUS_SUCCESS;

cleanup:

    RTL_FREE(&pszSddlStringTmp);
    RTL_FREE(&pszSddlStringTmp1);
    RTL_FREE(&pszFormattedToken);
    RTL_FREE(&pszTmpToken);

    RtlpFreeStringArray(ppszTokens, iTokenCount);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszOwner);
        RTL_FREE(&pszGroup);
        RTL_FREE(&pszDacl);
        RTL_FREE(&pszSacl);
    }

    *ppszOwner = pszOwner;
    *ppszGroup = pszGroup;
    *ppszDacl = pszDacl;
    *ppszSacl = pszSacl;

    return status;
}

static
NTSTATUS
RtlpParseSddlAclString(
    OUT PSTR** pppszAceStrings,
    OUT size_t* psCount,
    IN PCSTR pszSddlAclString
    )
{
    NTSTATUS status = 0;
    PSTR* ppszAceStrings = NULL;
    size_t sCount = 0;
    size_t sIndex = 0;
    PSTR pszSddlAclString1 = NULL;
    PSTR pszSddlAclString2 = NULL;
    // Do not free
    PSTR pszTmp = NULL;
    PSTR pszstrtok_rSav = NULL;

    status  = LwRtlCStringDuplicate(&pszSddlAclString1,
                                    pszSddlAclString);
    GOTO_CLEANUP_ON_STATUS(status);

    status  = LwRtlCStringDuplicate(&pszSddlAclString2,
                                    pszSddlAclString);
    GOTO_CLEANUP_ON_STATUS(status);

    pszTmp = strtok_r((PSTR)pszSddlAclString1, SDDL_ACE_END_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        sCount++;
        pszTmp = strtok_r(NULL, SDDL_ACE_END_S, &pszstrtok_rSav);
    }

    if (!sCount)
    {
        GOTO_CLEANUP();
    }

    status = RTL_ALLOCATE(&ppszAceStrings, PSTR, sizeof(*ppszAceStrings) * sCount);
    GOTO_CLEANUP_ON_STATUS(status);

    pszTmp = strtok_r((PSTR)pszSddlAclString2, SDDL_ACE_END_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        if (pszTmp[0] != SDDL_ACE_BEGIN_C)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        status  = LwRtlCStringDuplicate(&ppszAceStrings[sIndex++],
                                        &pszTmp[1]);
        GOTO_CLEANUP_ON_STATUS(status);

        pszTmp = strtok_r(NULL, SDDL_ACE_END_S, &pszstrtok_rSav);
    }

    status = STATUS_SUCCESS;

cleanup:

    RTL_FREE(&pszSddlAclString1);
    RTL_FREE(&pszSddlAclString2);

    if (!NT_SUCCESS(status))
    {
        RtlpFreeStringArray(ppszAceStrings, sCount);
        sCount = 0;
    }

    *pppszAceStrings = ppszAceStrings;
    *psCount = sCount;

    return status;
}

static
void
RtlpSafeFreeSddlAce(
    IN OUT PSDDL_ACE* ppAce
    )
{
    PSDDL_ACE pAce = NULL;

    if ((ppAce == NULL) || (*ppAce == NULL)) {
        return;
    }

    pAce = *ppAce;

    LW_RTL_FREE(&pAce->pszObjectGuid);
    LW_RTL_FREE(&pAce->pszObjectInheritedGuid);
    LW_RTL_FREE(&pAce->pSid);
    LW_RTL_FREE(&pAce);

    *ppAce = NULL;

    return;
}

static
void
RtlpSafeFreeSddlAceArray(
    PSDDL_ACE** pppAces,
    size_t sSize
    )
{
    PSDDL_ACE* ppAces = *pppAces;

    if (ppAces != NULL)
    {
        size_t i = 0;
        for (i = 0; i < sSize; i++)
        {
            RtlpSafeFreeSddlAce(&ppAces[i]);
        }

        RTL_FREE(&ppAces);
    }

    return;
}

// ace_type;ace_flags;rights;;;account_sid
static
NTSTATUS
RtlpParseSddlAceString(
    OUT PSDDL_ACE* ppSddlAce,
    IN PCSTR pszSddlAceString
    )
{
    NTSTATUS status = 0;
    PSDDL_ACE pSddlAce = NULL;
    //Do not free
    PSTR pszSid = NULL;

    PSTR* ppszAceParts = NULL;
    size_t sCount = 0;
    size_t sIndex = 0;

    PCSTR pszSddlAceStringTmp = pszSddlAceString;
    PSTR pszAceCursor = NULL;

    pszAceCursor = strchr(pszSddlAceStringTmp, SDDL_SEPERATOR_C);
    if (!pszAceCursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }
    do
    {
        sCount++;

        pszSddlAceStringTmp = pszAceCursor+1;
        pszAceCursor = strchr(pszSddlAceStringTmp, SDDL_SEPERATOR_C);

    }while(pszAceCursor);

    if (SDDL_ACE_PART_NUM != ++sCount)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&ppszAceParts, PSTR, sizeof(*ppszAceParts) * sCount);
    GOTO_CLEANUP_ON_STATUS(status);

    pszSddlAceStringTmp = pszSddlAceString;
    pszAceCursor = NULL;

    pszAceCursor = strchr(pszSddlAceStringTmp, SDDL_SEPERATOR_C);
    if (!pszAceCursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }
    do
    {
        size_t sLength = pszAceCursor - pszSddlAceStringTmp;

        if (sLength > 0)
        {
            status = RTL_ALLOCATE(&ppszAceParts[sIndex], CHAR, sLength + 1);
            GOTO_CLEANUP_ON_STATUS(status);

            memcpy(ppszAceParts[sIndex], pszSddlAceStringTmp, sLength);
        }

        sLength = 0;
        pszSddlAceStringTmp = pszAceCursor+1;
        pszAceCursor = strchr(pszSddlAceStringTmp, SDDL_SEPERATOR_C);
        sIndex++;
    }while(pszAceCursor);

    if (!LwRtlCStringIsNullOrEmpty(pszSddlAceStringTmp))
    {
        status  = LwRtlCStringDuplicate(&ppszAceParts[sIndex],

                                        pszSddlAceStringTmp);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pSddlAce, SDDL_ACE, sizeof(*pSddlAce));
    GOTO_CLEANUP_ON_STATUS(status);

    // ace_type;ace_flags;rights;;;account_sid
    // ace_type ppszAceParts[0]
    if (ppszAceParts[0])
    {
        pSddlAce->AceType = RtlpMapSddlTypeToAceType((PCSTR)ppszAceParts[0]);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER; // an ace needs a type
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ACCESS_UNKNOWN_ACE_TYPE ==  pSddlAce->AceType)
    {
        status = STATUS_NOT_SUPPORTED;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // ace_flag ppszAceParts[1]
    if (ppszAceParts[1])
    {
        pSddlAce->AceFlag = RtlpMapSddlFlagToAceFlag((PCSTR)ppszAceParts[1]);
    }

    // ace_access ppszAceParts[2]
    if (ppszAceParts[2])
    {
        pSddlAce->Access = RtlpMapSddlRightsToAccessMask((PCSTR)ppszAceParts[2]);
    }

    // ace_guid ppszAceParts[3] (currently NULL)


    // ace_inherit_guid ppszAceParts[4]  (currently NULL)


    // ace_sid ppszAceParts[5]
    if (ppszAceParts[5])
    {
        pszSid = (PSTR)RtlpSddlToSidString((PCSTR)ppszAceParts[5]);

        if (LwRtlCStringIsNullOrEmpty(pszSid))
        {
            if (LwRtlCStringIsEqual(ppszAceParts[5], SDDL_EVERYONE, FALSE))
            {
                pszSid = SECURITY_WORLD_PREFIX;

            }
            else if (LwRtlCStringIsEqual(ppszAceParts[5], SDDL_CREATOR_OWNER, FALSE))
            {
                pszSid = SECURITY_CREATOR_OWNER_PREFIX;
            }
            else
            {
                pszSid = ppszAceParts[5];
            }
        }

        status = RtlAllocateSidFromCString(&pSddlAce->pSid,
                                           pszSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER; // an ace needs a sid
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = STATUS_SUCCESS;

cleanup:

    RtlpFreeStringArray(ppszAceParts, sCount);

    if (!NT_SUCCESS(status))
    {
        RtlpSafeFreeSddlAce(&pSddlAce);
    }

    *ppSddlAce = pSddlAce;


    return status;
}

// D:dacl_flags(string_ace1)(string_ace2)... (string_acen)
static
NTSTATUS
RtlpGetAclFromSddlAclString(
    OUT PACL* ppAcl,
    OUT PSECURITY_DESCRIPTOR_CONTROL pControl,
    IN PSTR pszAclString,
    IN BOOLEAN bIsDacl
    )
{
    NTSTATUS status = 0;
    PSTR* ppszAceStrings = NULL;
    size_t sAceCount = 0;
    size_t sIndex = 0;
    PACL pAcl = NULL;
    DWORD dwSizeAcl = 0;
    CHAR szAclFlag[SDDL_CONTROL_LENGTH+1] = {0};
    // Do not free
    PSTR pszAceBegin = NULL;
    PSDDL_ACE* ppSddlAces = NULL;
    PACCESS_ALLOWED_ACE pAce = NULL;


    if (LwRtlCStringIsNullOrEmpty(pszAclString))
    {
        GOTO_CLEANUP();
    }

    // Obtain Dacl_flags/Sacl_flags in front of the first '('
    pszAceBegin = strchr(pszAclString, SDDL_ACE_BEGIN_C);
    if (pszAceBegin-pszAclString)
    {
        memcpy(szAclFlag, pszAclString, pszAceBegin-pszAclString);
    }

    status = RtlpMapSddlControlToAclControl(szAclFlag,
                                            bIsDacl,
                                            pControl);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpParseSddlAclString(&ppszAceStrings,
                                    &sAceCount,
                                    pszAceBegin);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RTL_ALLOCATE(&ppSddlAces, PSDDL_ACE, sizeof(*ppSddlAces) * sAceCount);
    GOTO_CLEANUP_ON_STATUS(status);

    for (sIndex = 0; sIndex < sAceCount; sIndex++)
    {
        status = RtlpParseSddlAceString(
            &ppSddlAces[sIndex],
            ppszAceStrings[(sIndex+sAceCount-1)%sAceCount]
            );
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Calculate memory needed for pACL
    dwSizeAcl = ACL_HEADER_SIZE;

    for (sIndex = 0; sIndex < sAceCount; sIndex++)
    {
        if (!ppSddlAces[sIndex])
        {
            status = STATUS_INTERNAL_ERROR;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        switch (ppSddlAces[sIndex]->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
                dwSizeAcl += RtlLengthAccessAllowedAce(ppSddlAces[sIndex]->pSid);
                break;

            case ACCESS_DENIED_ACE_TYPE:
                dwSizeAcl += RtlLengthAccessDeniedAce(ppSddlAces[sIndex]->pSid);
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                dwSizeAcl += RtlLengthAccessAuditAce(ppSddlAces[sIndex]->pSid);
                break;
            default:
                status = STATUS_NOT_SUPPORTED;
                GOTO_CLEANUP_ON_STATUS(status);
        }
    }

    status= LW_RTL_ALLOCATE(&pAcl, VOID, dwSizeAcl);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAcl(pAcl, dwSizeAcl, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    // Add each ACE to PACL
    for (sIndex = 0; sIndex < sAceCount; sIndex++)
    {
        USHORT aceSize = 0;

        if (!ppSddlAces[sIndex])
        {
            status = STATUS_INTERNAL_ERROR;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        switch (ppSddlAces[sIndex]->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
                aceSize =  RtlLengthAccessAllowedAce(ppSddlAces[sIndex]->pSid);

                status = RTL_ALLOCATE(&pAce, VOID, sizeof(*pAce)+aceSize);
                GOTO_CLEANUP_ON_STATUS(status);

                pAce->Header.AceSize = aceSize;
                pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
                break;

            case ACCESS_DENIED_ACE_TYPE:
                aceSize =  RtlLengthAccessDeniedAce(ppSddlAces[sIndex]->pSid);

                status = RTL_ALLOCATE(&pAce, VOID, sizeof(*pAce)+aceSize);
                GOTO_CLEANUP_ON_STATUS(status);

                pAce->Header.AceSize = aceSize;
                pAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                aceSize =  RtlLengthAccessAuditAce(ppSddlAces[sIndex]->pSid);

                status = RTL_ALLOCATE(&pAce, VOID, sizeof(*pAce)+aceSize);
                GOTO_CLEANUP_ON_STATUS(status);

                pAce->Header.AceSize = aceSize;
                pAce->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
                break;
            default:
                status = STATUS_NOT_SUPPORTED;
                GOTO_CLEANUP_ON_STATUS(status);
        }

        pAce->Header.AceFlags = ppSddlAces[sIndex]->AceFlag;
        pAce->Mask = ppSddlAces[sIndex]->Access;
        // We already know the size is sufficient
        RtlCopyMemory(&pAce->SidStart,
                      ppSddlAces[sIndex]->pSid,
                      RtlLengthSid(ppSddlAces[sIndex]->pSid));

        status = RtlAddAce(pAcl,
                           ACL_REVISION,
                           sIndex ? sIndex-1 : (ULONG)-1,
                           (PVOID)pAce,
                           //sizeof(*pAce)+pAce->Header.AceSize);
                           pAce->Header.AceSize);
        GOTO_CLEANUP_ON_STATUS(status);

        RTL_FREE(&pAce);
    }

    status = STATUS_SUCCESS;

cleanup:

    RTL_FREE(&pAce);
    RtlpSafeFreeSddlAceArray(&ppSddlAces, sAceCount);
    RtlpFreeStringArray(ppszAceStrings, sAceCount);

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pAcl);
    }

    *ppAcl = pAcl;

    return status;
}

static
NTSTATUS
RtlpMapAclControlToSddlControl(
    IN SECURITY_DESCRIPTOR_CONTROL Control,
    IN BOOLEAN bIsDacl,
    PSTR* ppszControl
    )
{
    NTSTATUS status = 0;
    PSTR pszControl = NULL;

    status = RTL_ALLOCATE(&pszControl, CHAR, SDDL_CONTROL_LENGTH);
    GOTO_CLEANUP_ON_STATUS(status);

    if (Control & (bIsDacl ? SE_DACL_PROTECTED : SE_SACL_PROTECTED))
    {
        strcat(pszControl, SDDL_PROTECTED);
    }

    if (Control & (bIsDacl ? SE_DACL_AUTO_INHERIT_REQ : SE_SACL_AUTO_INHERIT_REQ))
    {
        strcat(pszControl, SDDL_AUTO_INHERIT_REQ);
    }

    if (Control & (bIsDacl ? SE_DACL_AUTO_INHERITED : SE_SACL_AUTO_INHERITED))
    {
        strcat(pszControl, SDDL_AUTO_INHERITED);
    }

    status = STATUS_SUCCESS;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&pszControl);
    }

    *ppszControl = pszControl;

    return status;
}

static
NTSTATUS
RtlpMapSddlControlToAclControl(
    IN PCSTR pszControl,
    IN BOOLEAN bIsDacl,
    SECURITY_DESCRIPTOR_CONTROL* pControl
    )
{
    NTSTATUS status = 0;
    SECURITY_DESCRIPTOR_CONTROL Control = *pControl;
    size_t sLength = LwRtlCStringNumChars(pszControl);
    int i = 0;
    CHAR szControl[3] = {0};
    BOOLEAN bIsFirstSddlProtected = FALSE;

    if (!sLength)
    {
        GOTO_CLEANUP();
    }

    bIsFirstSddlProtected = !strncmp(pszControl, SDDL_PROTECTED, LwRtlCStringNumChars(SDDL_PROTECTED));

    if ((sLength%2 == 0 && bIsFirstSddlProtected) ||
         (sLength == 1 && !bIsFirstSddlProtected) ||
         sLength > SDDL_CONTROL_LENGTH)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // sLength == 2, 4, 1, 3, 5 with a leading 'P'
    if (sLength%2 == 0 || bIsFirstSddlProtected)
    {
        if (bIsFirstSddlProtected)
        {
            SetFlag(Control, bIsDacl? SE_DACL_PROTECTED : SE_SACL_PROTECTED);
        }

        for (i = 0; i < sLength/SDDL_CONTROL_SIZE; i++)
        {
            int iOffset = bIsFirstSddlProtected ? LwRtlCStringNumChars(SDDL_PROTECTED) : 0;

            memset(szControl, 0, 3);
            memcpy(szControl,
                   pszControl+iOffset+i*SDDL_CONTROL_SIZE,
                   2);

            if (LwRtlCStringIsEqual(szControl, SDDL_AUTO_INHERIT_REQ, TRUE))
            {
                SetFlag(Control, bIsDacl ? SE_DACL_AUTO_INHERIT_REQ : SE_SACL_AUTO_INHERIT_REQ);
                continue;
            }

            if (LwRtlCStringIsEqual(szControl, SDDL_AUTO_INHERITED, TRUE))
            {
                SetFlag(Control, bIsDacl ? SE_DACL_AUTO_INHERITED : SE_SACL_AUTO_INHERITED);
                continue;
            }
        }
    }
    // sLength == 3, 5 && !bIsFirstSddlProtected
    else if (sLength == SDDL_CONTROL_LENGTH-SDDL_CONTROL_SIZE ||
             sLength == SDDL_CONTROL_LENGTH)
    {
        if (strncmp(pszControl+SDDL_CONTROL_SIZE, SDDL_PROTECTED, LwRtlCStringNumChars(SDDL_PROTECTED)))
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else
        {
            SetFlag(Control, bIsDacl? SE_DACL_PROTECTED : SE_SACL_PROTECTED);

            for (i = 0; i < sLength/SDDL_CONTROL_SIZE; i++)
            {
                memset(szControl, 0, 3);
                memcpy(szControl,
                       pszControl + i*SDDL_CONTROL_SIZE + LwRtlCStringNumChars(SDDL_PROTECTED),
                       SDDL_CONTROL_SIZE);

                if (LwRtlCStringIsEqual(szControl, SDDL_AUTO_INHERIT_REQ, TRUE))
                {
                    SetFlag(Control, bIsDacl ? SE_DACL_AUTO_INHERIT_REQ : SE_SACL_AUTO_INHERIT_REQ);
                }
                else if (LwRtlCStringIsEqual(szControl, SDDL_AUTO_INHERITED, TRUE))
                {
                    SetFlag(Control, bIsDacl ? SE_DACL_AUTO_INHERITED : SE_SACL_AUTO_INHERITED);
                }
            }
        }
    }

    status = STATUS_SUCCESS;

cleanup:

    *pControl = Control;

    return status;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
