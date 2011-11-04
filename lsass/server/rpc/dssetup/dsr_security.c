/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        dsr_security.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup server security descriptor functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
DsrSrvCreateServerDacl(
    PACL *ppDacl
    );


static
DWORD
DsrSrvCreateDacl(
    PACL *ppDacl,
    PACCESS_LIST pList
    );


DWORD
DsrSrvInitServerSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;

    BAIL_ON_INVALID_PTR(ppSecDesc);

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                                    pSecDesc,
                                    pOwnerSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(
                                    pSecDesc,
                                    pGroupSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = DsrSrvCreateServerDacl(&pDacl);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);

    *ppSecDesc = pSecDesc;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDesc);

    *ppSecDesc = NULL;
    goto cleanup;
}


static
DWORD
DsrSrvCreateServerDacl(
    PACL *ppDacl
    )
{
    ACCESS_MASK SystemAccessMask = STANDARD_RIGHTS_REQUIRED |
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

    ACCESS_MASK AuthenticatedAccessMask = STANDARD_RIGHTS_READ |
                                          LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                          LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                                          LSA_ACCESS_VIEW_POLICY_INFO;

    ACCESS_MASK WorldAccessMask = STANDARD_RIGHTS_READ;

    DWORD dwError = ERROR_SUCCESS;
    DWORD dwSystemSidLen = 0;
    DWORD dwBuiltinAdminsSidLen = 0;
    DWORD dwAuthenticatedSidLen = 0;
    DWORD dwWorldSidLen = 0;
    PSID pSystemSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pAuthenticatedSid = NULL;
    PSID pWorldSid = NULL;
    PACL pDacl = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pSystemSid,
            .AccessMask   = SystemAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pAuthenticatedSid,
            .AccessMask   = AuthenticatedAccessMask,
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

    /* create local system sid */
    dwError = LwAllocateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &pSystemSid,
                                   &dwSystemSidLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* create administrators sid */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   &dwBuiltinAdminsSidLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* create authenticated users sid */
    dwError = LwAllocateWellKnownSid(WinAuthenticatedUserSid,
                                   NULL,
                                   &pAuthenticatedSid,
                                   &dwAuthenticatedSidLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* create world (everyone) sid */
    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   &dwWorldSidLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvCreateDacl(&pDacl,
                               AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pSystemSid);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pAuthenticatedSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

    return dwError;

error:
    *ppDacl = NULL;

    goto cleanup;
}


static
DWORD
DsrSrvCreateDacl(
    PACL *ppDacl,
    PACCESS_LIST pList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;
    DWORD i = 0;
    ULONG ulSidSize = 0;

    dwDaclSize += ACL_HEADER_SIZE;

    for (i = 0; pList[i].ppSid && (*pList[i].ppSid); i++)
    {
        ulSidSize = RtlLengthSid(*(pList[i].ppSid));

        if (pList[i].ulAccessType == ACCESS_ALLOWED_ACE_TYPE)
        {
            dwDaclSize += ulSidSize + sizeof(ACCESS_ALLOWED_ACE);
        }
        else if (pList[i].ulAccessType == ACCESS_DENIED_ACE_TYPE)
        {
            dwDaclSize += ulSidSize + sizeof(ACCESS_DENIED_ACE);
        }
    }

    dwError = LwAllocateMemory(dwDaclSize,
                               OUT_PPVOID(&pDacl));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

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

        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppDacl = pDacl;

cleanup:
    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pDacl);
    *ppDacl = NULL;

    goto cleanup;
}


DWORD
DsrSrvDestroyServerSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
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

    BAIL_ON_INVALID_PTR(ppSecDesc);

    pSecDesc = *ppSecDesc;
    if (pSecDesc == NULL)
    {
        dwError = ERROR_SUCCESS;
        goto cleanup;
    }

    ntStatus = RtlGetOwnerSecurityDescriptor(pSecDesc,
                                             &pOwnerSid,
                                             &bOwnerDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetGroupSecurityDescriptor(pSecDesc,
                                             &pPrimaryGroupSid,
                                             &bPrimaryGroupDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc,
                                            &bDaclPresent,
                                            &pDacl,
                                            &bDaclDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = RtlGetSaclSecurityDescriptor(pSecDesc,
                                            &bSaclPresent,
                                            &pSacl,
                                            &bSaclDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
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
    *ppSecDesc = NULL;

    return dwError;

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
