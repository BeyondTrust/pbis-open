/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        security.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Lsa Accounts and Privileges security
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
DWORD
LsaSrvCreatePrivilegesDacl(
    OUT PACL *ppDacl
    );

static
DWORD
LsaSrvCreateDacl(
    OUT PACL *ppDacl,
    IN PACCESS_LIST pList
    );

static
DWORD
LsaSrvCreateAccountsDacl(
    OUT PACL *ppDacl
    );


DWORD
LsaSrvInitPrivilegesSecurity(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDescriptor
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;

    err = LwAllocateMemory(
                        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                        OUT_PPVOID(&pSecDesc));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                        pSecDesc,
                        SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwAllocateWellKnownSid(
                        WinBuiltinAdministratorsSid,
                        NULL,
                        &pOwnerSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                        pSecDesc,
                        pOwnerSid,
                        FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwAllocateWellKnownSid(
                        WinLocalSystemSid,
                        NULL,
                        &pGroupSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetGroupSecurityDescriptor(
                        pSecDesc,
                        pGroupSid,
                        FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LsaSrvCreatePrivilegesDacl(&pDacl);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecurityDescriptor = pSecDesc;

error:
    if (err || ntStatus)
    {
        LsaSrvFreeSecurityDescriptor(pSecDesc);

        *ppSecurityDescriptor = NULL;
    }

    if (err == ERROR_SUCCESS &&
        ntStatus == STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
LsaSrvCreatePrivilegesDacl(
    OUT PACL *ppDacl
    )
{
    ACCESS_MASK AnonymousAccessMaskDenied = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                  LSA_ACCESS_ENABLE_LSA |
                                  LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS |
                                  LSA_ACCESS_CHANGE_SYS_AUDIT_REQS |
                                  LSA_ACCESS_SET_DEFAULT_QUOTA |
                                  LSA_ACCESS_CREATE_PRIVILEGE |
                                  LSA_ACCESS_CREATE_SECRET_OBJECT |
                                  LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS |
                                  LSA_ACCESS_CHANGE_DOMTRUST_RELATION |
                                  LSA_ACCESS_GET_SENSITIVE_POLICY_INFO |
                                  LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                                  LSA_ACCESS_VIEW_POLICY_INFO;

    ACCESS_MASK WorldAccessMask = STANDARD_RIGHTS_READ |
                                  LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                  LSA_ACCESS_VIEW_POLICY_INFO;

    ACCESS_MASK AnonymousAccessMaskAllowed = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                             LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PSID pAnonymousSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;
    PACL pDacl = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pAnonymousSid,
            .AccessMask   = AnonymousAccessMaskDenied,
            .ulAccessType = ACCESS_DENIED_ACE_TYPE
        },
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pWorldSid,
            .AccessMask   = WorldAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pAnonymousSid,
            .AccessMask   = AnonymousAccessMaskAllowed,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = NULL,
            .AccessMask   = 0,
            .ulAccessType = 0
        }
    };

    // allocate anonymous sid
    err = LwAllocateWellKnownSid(
                        WinAnonymousSid,
                        NULL,
                        &pAnonymousSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    // allocate builtin adminis sid
    err = LwAllocateWellKnownSid(
                        WinBuiltinAdministratorsSid,
                        NULL,
                        &pBuiltinAdminsSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    // allocate world (everyone) sid
    err = LwAllocateWellKnownSid(
                        WinWorldSid,
                        NULL,
                        &pWorldSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaSrvCreateDacl(
                        &pDacl,
                        AccessList);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppDacl = pDacl;

error:
    if (err || ntStatus)
    {
        *ppDacl = NULL;
    }

    LW_SAFE_FREE_MEMORY(pAnonymousSid);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
LsaSrvCreateDacl(
    OUT PACL *ppDacl,
    IN PACCESS_LIST pList
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD daclSize = 0;
    PACL pDacl = NULL;
    DWORD i = 0;
    DWORD sidSize = 0;

    daclSize += ACL_HEADER_SIZE;

    for (i = 0; pList[i].ppSid && (*pList[i].ppSid); i++)
    {
        sidSize = RtlLengthSid(*(pList[i].ppSid));

        if (pList[i].ulAccessType == ACCESS_ALLOWED_ACE_TYPE)
        {
            daclSize += sidSize + sizeof(ACCESS_ALLOWED_ACE);
        }
        else if (pList[i].ulAccessType == ACCESS_DENIED_ACE_TYPE)
        {
            daclSize += sidSize + sizeof(ACCESS_DENIED_ACE);
        }
    }

    err = LwAllocateMemory(
                        daclSize,
                        OUT_PPVOID(&pDacl));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCreateAcl(pDacl, daclSize, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; pList[i].ppSid && (*pList[i].ppSid); i++)
    {
        if (pList[i].ulAccessType == ACCESS_ALLOWED_ACE_TYPE)
        {
            ntStatus = RtlAddAccessAllowedAceEx(pDacl,
                                                ACL_REVISION,
                                                0,
                                                pList[i].AccessMask,
                                                *(pList[i].ppSid));
        }
        else if (pList[i].ulAccessType == ACCESS_DENIED_ACE_TYPE)
        {
            ntStatus = RtlAddAccessDeniedAceEx(pDacl,
                                               ACL_REVISION,
                                               0,
                                               pList[i].AccessMask,
                                               *(pList[i].ppSid));
        }

        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppDacl = pDacl;

error:
    if (err || ntStatus)
    {
        LW_SAFE_FREE_MEMORY(pDacl);

        *ppDacl = NULL;
    }

    if (ntStatus == STATUS_SUCCESS &&
        err != ERROR_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvFreeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    BOOLEAN bOwnerDefaulted = FALSE;
    PSID pPrimaryGroupSid = NULL;
    BOOLEAN bPrimaryGroupDefaulted = FALSE;
    PACL pDacl = NULL;
    BOOLEAN bDaclPresent = FALSE;
    BOOLEAN bDaclDefaulted = FALSE;
    PACL pSacl = NULL;
    BOOLEAN bSaclPresent = FALSE;
    BOOLEAN bSaclDefaulted = FALSE;

    ntStatus = RtlGetOwnerSecurityDescriptor(
                        pSecDesc,
                        &pOwnerSid,
                        &bOwnerDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetGroupSecurityDescriptor(
                        pSecDesc,
                        &pPrimaryGroupSid,
                        &bPrimaryGroupDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetDaclSecurityDescriptor(
                        pSecDesc,
                        &bDaclPresent,
                        &pDacl,
                        &bDaclDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetSaclSecurityDescriptor(
                        pSecDesc,
                        &bSaclPresent,
                        &pSacl,
                        &bSaclDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    LW_SAFE_FREE_MEMORY(pOwnerSid);
    LW_SAFE_FREE_MEMORY(pPrimaryGroupSid);

    if (bDaclPresent)
    {
        LW_SAFE_FREE_MEMORY(pDacl);
    }

    if (bSaclPresent)
    {
        LW_SAFE_FREE_MEMORY(pSacl);
    }

    LW_SAFE_FREE_MEMORY(pSecDesc);

    err = LwNtStatusToWin32Error(ntStatus);

    return err;
}


DWORD
LsaSrvInitAccountsSecurity(
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecurityDescRelative,
    OUT PDWORD pSecurityDescRelativeSize
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative = NULL;
    DWORD secDescRelativeSize = 0;

    err = LwAllocateMemory(
                        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                        OUT_PPVOID(&pSecDesc));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                        pSecDesc,
                        SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwAllocateWellKnownSid(
                        WinBuiltinAdministratorsSid,
                        NULL,
                        &pOwnerSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                        pSecDesc,
                        pOwnerSid,
                        FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwAllocateWellKnownSid(
                        WinLocalSystemSid,
                        NULL,
                        &pGroupSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetGroupSecurityDescriptor(
                        pSecDesc,
                        pGroupSid,
                        FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LsaSrvCreateAccountsDacl(&pDacl);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRelative,
                                    &secDescRelativeSize);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LwAllocateMemory(
                        secDescRelativeSize,
                        OUT_PPVOID(&pSecDescRelative));
    BAIL_ON_LSA_ERROR(err);

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRelative,
                                    &secDescRelativeSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecurityDescRelative    = pSecDescRelative;
    *pSecurityDescRelativeSize = secDescRelativeSize;

error:
    if (err || ntStatus)
    {
        LW_SAFE_FREE_MEMORY(pSecDescRelative);

        *ppSecurityDescRelative    = NULL;
        *pSecurityDescRelativeSize = 0;
    }

    LsaSrvFreeSecurityDescriptor(pSecDesc);

    if (err == ERROR_SUCCESS &&
        ntStatus == STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


static
DWORD
LsaSrvCreateAccountsDacl(
    OUT PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  LSA_ACCOUNT_VIEW |
                                  LSA_ACCOUNT_ADJUST_PRIVILEGES |
                                  LSA_ACCOUNT_ADJUST_QUOTAS |
                                  LSA_ACCOUNT_ADJUST_SYSTEM_ACCESS;

    ACCESS_MASK WorldAccessMask = STANDARD_RIGHTS_READ;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;
    PACL pDacl = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pWorldSid,
            .AccessMask   = WorldAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = NULL,
            .AccessMask   = 0,
            .ulAccessType = 0
        }
    };

    // allocate builtin adminis sid
    err = LwAllocateWellKnownSid(
                        WinBuiltinAdministratorsSid,
                        NULL,
                        &pBuiltinAdminsSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    // allocate world (everyone) sid
    err = LwAllocateWellKnownSid(
                        WinWorldSid,
                        NULL,
                        &pWorldSid,
                        NULL);
    BAIL_ON_LSA_ERROR(err);

    ntStatus = LsaSrvCreateDacl(
                        &pDacl,
                        AccessList);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppDacl = pDacl;

error:
    if (err || ntStatus)
    {
        *ppDacl = NULL;
    }

    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    return err;
}


DWORD
LsaSrvAllocateAbsoluteFromSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative,
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecurityDesc
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDesc = NULL;
    DWORD securityDescSize = 0;
    PACL pDacl = NULL;
    DWORD daclSize = 0;
    PACL pSacl = NULL;
    DWORD saclSize = 0;
    PSID pOwnerSid = NULL;
    DWORD ownerSidSize = 0;
    PSID pGroupSid = NULL;
    DWORD groupSidSize = 0;

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                        pSecurityDescRelative,
                        pSecurityDesc,
                        &securityDescSize,
                        pDacl,
                        &daclSize,
                        pSacl,
                        &saclSize,
                        pOwnerSid,
                        &ownerSidSize,
                        pGroupSid,
                        &groupSidSize);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus != STATUS_SUCCESS)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    err = LwAllocateMemory(
                        securityDescSize,
                        OUT_PPVOID(&pSecurityDesc));
    BAIL_ON_LSA_ERROR(err);

    if (daclSize)
    {
        err = LwAllocateMemory(
                            daclSize,
                            OUT_PPVOID(&pDacl));
        BAIL_ON_LSA_ERROR(err);
    }

    if (saclSize)
    {
        err = LwAllocateMemory(
                            saclSize,
                            OUT_PPVOID(&pSacl));
        BAIL_ON_LSA_ERROR(err);
    }

    if (ownerSidSize)
    {
        err = LwAllocateMemory(
                            ownerSidSize,
                            OUT_PPVOID(&pOwnerSid));
        BAIL_ON_LSA_ERROR(err);
    }

    if (groupSidSize)
    {
        err = LwAllocateMemory(
                            groupSidSize,
                            OUT_PPVOID(&pGroupSid));
        BAIL_ON_LSA_ERROR(err);
    }

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                        pSecurityDescRelative,
                        pSecurityDesc,
                        &securityDescSize,
                        pDacl,
                        &daclSize,
                        pSacl,
                        &saclSize,
                        pOwnerSid,
                        &ownerSidSize,
                        pGroupSid,
                        &groupSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecurityDesc = pSecurityDesc;

error:
    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(ntStatus);
    }

    if (err)
    {
        LsaSrvFreeSecurityDescriptor(pSecurityDesc);
    }

    return err;
}


VOID
LsaSrvPrivsGetClientCreds(
    IN HANDLE hServer,
    OUT PDWORD pUid,
    OUT PDWORD pGid,
    OUT PDWORD pPid
    )
{
    PLSASRV_PRIVILEGE_CLIENT_STATE pServerState
        = (PLSASRV_PRIVILEGE_CLIENT_STATE)hServer;

    if (pUid)
    {
        *pUid = pServerState->peerUID;
    }

    if (pGid)
    {
        *pGid = pServerState->peerGID;
    }

    if (pPid)
    {
        *pPid = pServerState->peerPID;
    }
}


DWORD
LsaSrvPrivsGetAccessTokenFromServerHandle(
    HANDLE hServer,
    PACCESS_TOKEN *pAccessToken
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSASRV_PRIVILEGE_GLOBALS pGlobals = &gLsaPrivilegeGlobals;
    DWORD uid = -1;
    DWORD gid = -1;
    PACCESS_TOKEN accessToken = NULL;

    LsaSrvPrivsGetClientCreds(
                       hServer,
                       &uid,
                       &gid,
                       NULL);

    ntStatus = LwMapSecurityCreateAccessTokenFromUidGid(
                       pGlobals->pSecurityContext,
                       &accessToken,
                       uid,
                       gid);

    *pAccessToken = accessToken;

error:
    if (ntStatus)
    {
        err = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(err);

        *pAccessToken = NULL;
    }

    return err;
}
