/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2010
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
 *        lpsecurity.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Account security descriptor functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
LocalDirCreateLocalUserDacl(
    PSID   pDomainSid,
    DWORD  dwRid,
    PACL  *ppDacl
    );


static
DWORD
LocalDirCreateLocalGroupDacl(
    PSID   pDomainSid,
    DWORD  dwRid,
    PACL  *ppDacl
    );


static
DWORD
LocalDirCreateDacl(
    PACL *ppDacl,
    PACCESS_LIST pList
    );


DWORD
LocalDirCreateNewAccountSecurityDescriptor(
    PSID                           pDomainSid,
    DWORD                          dwRid,
    DWORD                          dwObjectClass,
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSID pAdminSid = NULL;
    DWORD dwAdminSidSize = 0;
    PSID pBuiltinAdminsSid = NULL;
    DWORD dwBuiltinAdminsSidSize = 0;
    PACL pDacl = NULL;

    BAIL_ON_INVALID_POINTER(pDomainSid);
    BAIL_ON_INVALID_POINTER(ppSecDesc);

    if (dwRid <= DOMAIN_USER_RID_MAX)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwObjectClass != DIR_OBJECT_CLASS_USER &&
        dwObjectClass != DIR_OBJECT_CLASS_LOCAL_GROUP)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner (Administrator)
     */
    dwError = LwAllocateWellKnownSid(WinAccountAdministratorSid,
                                   pDomainSid,
                                   &pAdminSid,
                                   &dwAdminSidSize);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                             pAdminSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   &dwBuiltinAdminsSidSize);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(pSecDesc,
                                             pBuiltinAdminsSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Create default DACL
     */
    if (dwObjectClass == DIR_OBJECT_CLASS_USER)
    {
        ntStatus = LocalDirCreateLocalUserDacl(pDomainSid,
					       dwRid,
					       &pDacl);
    }
    else if (dwObjectClass == DIR_OBJECT_CLASS_LOCAL_GROUP)
    {
        ntStatus = LocalDirCreateLocalGroupDacl(pDomainSid,
						dwRid,
						&pDacl);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecDesc = pSecDesc;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LocalDirFreeSecurityDescriptor(&pSecDesc);

    goto cleanup;
}


static
DWORD
LocalDirCreateLocalUserDacl(
    PSID   pDomainSid,
    DWORD  dwRid,
    PACL  *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_ALL |
                                  USER_ACCESS_GET_NAME_ETC |
                                  USER_ACCESS_GET_LOCALE |
                                  USER_ACCESS_SET_LOC_COM |
                                  USER_ACCESS_GET_LOGONINFO |
                                  USER_ACCESS_GET_ATTRIBUTES |
                                  USER_ACCESS_SET_ATTRIBUTES |
                                  USER_ACCESS_CHANGE_PASSWORD |
                                  USER_ACCESS_SET_PASSWORD |
                                  USER_ACCESS_GET_GROUPS |
                                  USER_ACCESS_GET_GROUP_MEMBERSHIP |
                                  USER_ACCESS_CHANGE_GROUP_MEMBERSHIP;

    ACCESS_MASK AccountAccessMask = STANDARD_RIGHTS_READ |
                                    USER_ACCESS_GET_NAME_ETC |
                                    USER_ACCESS_GET_LOCALE |
                                    USER_ACCESS_GET_LOGONINFO |
                                    USER_ACCESS_GET_ATTRIBUTES |
                                    USER_ACCESS_CHANGE_PASSWORD |
                                    USER_ACCESS_GET_GROUPS |
                                    USER_ACCESS_GET_GROUP_MEMBERSHIP;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                USER_ACCESS_GET_NAME_ETC |
                                USER_ACCESS_GET_LOCALE |
                                USER_ACCESS_GET_LOGONINFO |
                                USER_ACCESS_GET_ATTRIBUTES |
                                USER_ACCESS_CHANGE_PASSWORD |
                                USER_ACCESS_GET_GROUPS |
                                USER_ACCESS_GET_GROUP_MEMBERSHIP;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdminSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pAccountSid = NULL;
    DWORD dwAccountSidSize = 0;
    PSID pWorldSid = NULL;
    PACL pDacl = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pAdminSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pAccountSid,
            .AccessMask   = AccountAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pWorldSid,
            .AccessMask   = AllAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = NULL,
            .AccessMask   = 0,
            .ulAccessType = 0
        }
    };

    dwError = LwAllocateWellKnownSid(WinAccountAdministratorSid,
                                   pDomainSid,
                                   &pAdminSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwAccountSidSize = RtlLengthRequiredSid(
                                   pDomainSid->SubAuthorityCount + 1);
    dwError = LwAllocateMemory(dwAccountSidSize,
                               OUT_PPVOID(&pAccountSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCopySid(dwAccountSidSize,
                          pAccountSid,
                          pDomainSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAppendRidSid(dwAccountSidSize,
                               pAccountSid,
                               dwRid);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirCreateDacl(&pDacl,
                                 AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pAdminSid);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pAccountSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (ppDacl)
    {
        *ppDacl = NULL;
    }

    goto cleanup;
}


static
DWORD
LocalDirCreateLocalGroupDacl(
    PSID   pDomainSid,
    DWORD  dwRid,
    PACL  *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_ALL |
                                  ALIAS_ACCESS_ADD_MEMBER |
                                  ALIAS_ACCESS_REMOVE_MEMBER |
                                  ALIAS_ACCESS_GET_MEMBERS |
                                  ALIAS_ACCESS_LOOKUP_INFO |
                                  ALIAS_ACCESS_SET_INFO;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                ALIAS_ACCESS_GET_MEMBERS |
                                ALIAS_ACCESS_LOOKUP_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdminSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;
    PACL pDacl = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pAdminSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pWorldSid,
            .AccessMask   = AllAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = NULL,
            .AccessMask   = 0,
            .ulAccessType = 0
        }
    };

    dwError = LwAllocateWellKnownSid(WinAccountAdministratorSid,
                                   pDomainSid,
                                   &pAdminSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirCreateDacl(&pDacl,
                                 AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pAdminSid);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (ppDacl)
    {
        *ppDacl = NULL;
    }

    goto cleanup;
}


static
DWORD
LocalDirCreateDacl(
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

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pDacl);
    *ppDacl = NULL;

    goto cleanup;
}


VOID
LocalDirFreeSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
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

    if (ppSecDesc == NULL ||
        *ppSecDesc == NULL)
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntStatus = RtlGetOwnerSecurityDescriptor(pSecDesc,
                                             &pOwnerSid,
                                             &bOwnerDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetGroupSecurityDescriptor(pSecDesc,
                                             &pPrimaryGroupSid,
                                             &bPrimaryGroupDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc,
                                            &bDaclPresent,
                                            &pDacl,
                                            &bDaclDefaulted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetSaclSecurityDescriptor(pSecDesc,
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
    *ppSecDesc = NULL;
}
