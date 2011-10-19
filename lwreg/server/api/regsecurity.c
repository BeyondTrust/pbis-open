/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        regsecurity.c
 *
 * Abstract:
 *
 *        Likewise Registry Utilities
 *
 *        Security related routines such as ACLs, access checks, etc...
 *
 * Authors: Wei Fu (wfu@likewise.com)
 */

#include "api.h"

static
NTSTATUS
RegBuildDefaultDaclForKey(
    OUT PACL *ppDacl
    );

NTSTATUS
RegSrvAccessCheckKeyHandle(
    IN PREG_KEY_HANDLE pKeyHandle,
    IN ACCESS_MASK AccessRequired
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((pKeyHandle->AccessGranted & AccessRequired) == AccessRequired)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
    	status = STATUS_ACCESS_DENIED;
    }

    return status;
}

NTSTATUS
RegSrvCreateAccessToken(
    uid_t uid,
    gid_t gid,
    PACCESS_TOKEN* ppToken
    )
{
    NTSTATUS status = 0;
    PACCESS_TOKEN pToken = NULL;

	status = LwMapSecurityCreateAccessTokenFromUidGid(gpRegLwMapSecurityCtx,
    		                                          &pToken,
    		                                          uid,
    		                                          gid);
    if (status || !pToken)
    {
    	status = STATUS_NO_TOKEN;
    }
    BAIL_ON_NT_STATUS(status);

    *ppToken = pToken;

cleanup:

    return status;

error:

    if (pToken)
    {
        RtlReleaseAccessToken(&pToken);
    }

    *ppToken = NULL;

    goto cleanup;
}

NTSTATUS
RegSrvAccessCheckKey(
	IN PACCESS_TOKEN pToken,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescRelLen,
    IN ACCESS_MASK AccessDesired,
    OUT ACCESS_MASK *psamGranted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ACCESS_MASK AccessMask = 0;
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


    BAIL_ON_NT_INVALID_POINTER(psamGranted);

    if (!pToken)
    {
    	status = STATUS_NO_TOKEN;
    	BAIL_ON_NT_STATUS(status);
    }

    // Get sizes
    status = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                         pSecDescAbs, &ulSecDescAbsLen,
                                         pDacl, &ulDaclLen,
                                         pSacl, &ulSaclLen,
                                         pOwner, &ulOwnerLen,
                                         pGroup, &ulGroupLen);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE(&pSecDescAbs, VOID, ulSecDescAbsLen);
    BAIL_ON_NT_STATUS(status);

    if (ulOwnerLen)
    {
        status = LW_RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulGroupLen)
    {
        status = LW_RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulDaclLen)
    {
        status = LW_RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulSaclLen)
    {
        status = LW_RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
        BAIL_ON_NT_STATUS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                         pSecDescAbs, &ulSecDescAbsLen,
                                         pDacl, &ulDaclLen,
                                         pSacl, &ulSaclLen,
                                         pOwner, &ulOwnerLen,
                                         pGroup, &ulGroupLen);
    BAIL_ON_NT_STATUS(status);

    // Access check
    if (!RtlAccessCheck(pSecDescAbs,
                        pToken,
                        AccessDesired,
                        0,
                        &gRegKeyGenericMapping,
                        &AccessMask,
                        &status))
    {
        BAIL_ON_NT_STATUS(status);
    }

    *psamGranted = AccessMask;

cleanup:

    RegSrvFreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    return status;

error:
    *psamGranted = 0;

    goto cleanup;
}

VOID
RegSrvFreeAbsoluteSecurityDescriptor(
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

    LW_RTL_FREE(&pSecDesc);
    LW_RTL_FREE(&pOwner);
    LW_RTL_FREE(&pGroup);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}

NTSTATUS
RegSrvCreateDefaultSecDescRel(
	IN OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
	IN OUT PULONG pulSecDescLength
	)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;

    status = LW_RTL_ALLOCATE(&pSecDescAbs,
                            VOID,
                            SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(pSecDescAbs,
                                                 SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(status);

    // Owner: Root
    status = LwMapSecurityGetSidFromId(
                 gpRegLwMapSecurityCtx,
                 &pOwnerSid,
                 TRUE,
                 0);
    BAIL_ON_NT_STATUS(status);

    status = RtlSetOwnerSecurityDescriptor(
                 pSecDescAbs,
                 pOwnerSid,
                 FALSE);
    BAIL_ON_NT_STATUS(status);
    pOwnerSid = NULL;

    // Group Administrators

    status = RtlAllocateSidFromCString(&pGroupSid, "S-1-5-32-544");
    BAIL_ON_NT_STATUS(status);

    status = RtlSetGroupSecurityDescriptor(
                 pSecDescAbs,
                 pGroupSid,
                 FALSE);
    BAIL_ON_NT_STATUS(status);
    pGroupSid = NULL;


    // Do not set Sacl currently

    // DACL
    status = RegBuildDefaultDaclForKey(&pDacl);
    BAIL_ON_NT_STATUS(status);

    status = RtlSetDaclSecurityDescriptor(pSecDescAbs,
                                          TRUE,
                                          pDacl,
                                          FALSE);
    BAIL_ON_NT_STATUS(status);
    pDacl = NULL;

    if (!RtlValidSecurityDescriptor(pSecDescAbs))
    {
		status = STATUS_INVALID_SECURITY_DESCR;
		BAIL_ON_NT_STATUS(status);
    }

    do
    {
        status = NtRegReallocMemory(pSecDescRel,
                                    (PVOID*)&pSecDescRel,
                                    ulSecDescLen);
        BAIL_ON_NT_STATUS(status);

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
            BAIL_ON_NT_STATUS(status);
        }
    }
    while((status != STATUS_SUCCESS) &&
          (ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));

    *ppSecDescRel = pSecDescRel;
    *pulSecDescLength = ulSecDescLen;

cleanup:
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);

    RegSrvFreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pSecDescRel);
    ulSecDescLen = 0;

    goto cleanup;
}

static
NTSTATUS
RegBuildDefaultDaclForKey(
    OUT PACL *ppDacl
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwSizeDacl = 0;
    PSID pRootSid = NULL;
    PSID pEveryoneSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;

    status = LwMapSecurityGetSidFromId(gpRegLwMapSecurityCtx,
    		                           &pRootSid,
    		                           TRUE,
                                       0);
    BAIL_ON_NT_STATUS(status);
    dwSidCount++;

    status = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(status);
    dwSidCount++;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pRootSid) +
        RtlLengthSid(pEveryoneSid) -
        dwSidCount * sizeof(ULONG);

    status= LW_RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(status);

    status = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(status);

    status = RtlAddAccessAllowedAceEx(pDacl,
                                      ACL_REVISION,
                                      0,
                                      KEY_ALL_ACCESS |
                                      DELETE |
                                      WRITE_OWNER |
                                      WRITE_DAC |
                                      READ_CONTROL,
                                      pRootSid);
    BAIL_ON_NT_STATUS(status);

    status = RtlAddAccessAllowedAceEx(pDacl,
                                      ACL_REVISION,
                                      0,
                                      KEY_READ,
                                      pEveryoneSid);
    BAIL_ON_NT_STATUS(status);

    *ppDacl = pDacl;
    pDacl = NULL;

cleanup:

    LW_RTL_FREE(&pRootSid);
    LW_RTL_FREE(&pEveryoneSid);

    LW_RTL_FREE(&pDacl);

    return status;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

