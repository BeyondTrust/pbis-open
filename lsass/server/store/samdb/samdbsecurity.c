/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samdbsecurity.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Security Descriptor handling routines
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#include "includes.h"


typedef struct _ACCESS_LIST
{
    PSID        *ppSid;
    ACCESS_MASK AccessMask;
    ULONG       ulAccessType;

} ACCESS_LIST, *PACCESS_LIST;


static
DWORD
SamDbCreateLocalDomainDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateBuiltinDomainDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateLocalUserDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateLocalGroupDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateBuiltinGroupDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateNewLocalAccountDacl(
    PSID pSid,
    PACL *ppDacl
    );


static
DWORD
SamDbCreateDacl(
    PACL *ppDacl,
    PACCESS_LIST pList
    );


DWORD
SamDbCreateLocalDomainSecDesc(
    PSID pSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner to SYSTEM
     */
    dwError = LwAllocateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                                    pSecDesc,
                                    pOwnerSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group to BUILTIN\Administrators
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(
                                    pSecDesc,
                                    pGroupSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create default DACL */
    dwError = SamDbCreateLocalDomainDacl(pSid,
                                         &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


DWORD
SamDbCreateBuiltinDomainSecDesc(
    PSID pSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner to LOCAL_SYSTEM
     */
    dwError = LwAllocateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                                    pSecDesc,
                                    pOwnerSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group to BUILTIN\Administrators
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(
                                    pSecDesc,
                                    pGroupSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create default DACL */
    dwError = SamDbCreateBuiltinDomainDacl(pSid,
                                           &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


DWORD
SamDbCreateLocalUserSecDesc(
    PSID pUserSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner (BUITLIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                             pOwnerSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(pSecDesc,
                                             pGroupSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* create default DACL */
    dwError = SamDbCreateLocalUserDacl(pUserSid,
                                       &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


DWORD
SamDbCreateLocalGroupSecDesc(
    PSID pSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                             pOwnerSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(pSecDesc,
                                             pGroupSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* create default DACL */
    dwError = SamDbCreateLocalGroupDacl(pSid,
                                        &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


DWORD
SamDbCreateBuiltinGroupSecDesc(
    PSID pSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                             pOwnerSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(pSecDesc,
                                             pGroupSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* create default DACL */
    dwError = SamDbCreateBuiltinGroupDacl(pSid,
                                          &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


DWORD
SamDbCreateNewLocalAccountSecDesc(
    PSID pSid,
    PSECURITY_DESCRIPTOR_RELATIVE *ppSecDescRel,
    PULONG pulSecDescLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDescRel = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;

    dwError = LwAllocateMemory(SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
                               OUT_PPVOID(&pSecDesc));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                                    pSecDesc,
                                    SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set owner (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                             pOwnerSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Set group (BUILTIN\Administrators)
     */
    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(pSecDesc,
                                             pGroupSid,
                                             FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* create default DACL */
    dwError = SamDbCreateNewLocalAccountDacl(pSid,
                                             &pDacl);
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlSetDaclSecurityDescriptor(
                                    pSecDesc,
                                    TRUE,
                                    pDacl,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        dwError = LwReallocMemory(pSecDescRel,
                                  OUT_PPVOID(&pNewSecDescRel),
                                  ulSecDescLen);
        BAIL_ON_SAMDB_ERROR(dwError);

        pSecDescRel    = pNewSecDescRel;
        pNewSecDescRel = NULL;

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                                    pSecDesc,
                                    pSecDescRel,
                                    &ulSecDescLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ulSecDescLen *= 2;
        }

    } while (ntStatus != STATUS_SUCCESS &&
             ulSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);

    *ppSecDescRel  = pSecDescRel;
    *pulSecDescLen = ulSecDescLen;

cleanup:
    LW_SAFE_FREE_MEMORY(pSecDesc);
    LW_SAFE_FREE_MEMORY(pDacl);

    LW_SAFE_FREE_MEMORY(pOwnerSid);
    LW_SAFE_FREE_MEMORY(pGroupSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pSecDescRel);

    *ppSecDescRel  = NULL;
    *pulSecDescLen = 0;

    goto cleanup;
}


static
DWORD
SamDbCreateLocalDomainDacl(
    PSID pSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                  DOMAIN_ACCESS_SET_INFO_1 |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                  DOMAIN_ACCESS_SET_INFO_2 |
                                  DOMAIN_ACCESS_CREATE_USER |
                                  DOMAIN_ACCESS_CREATE_GROUP |
                                  DOMAIN_ACCESS_CREATE_ALIAS |
                                  DOMAIN_ACCESS_LOOKUP_ALIAS |
                                  DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_SET_INFO_3;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                DOMAIN_ACCESS_LOOKUP_ALIAS |
                                DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                DOMAIN_ACCESS_OPEN_ACCOUNT;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    ULONG ulLocalAdminSidLen = 0;
    PSID pLocalAdminSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pLocalAdminSid,
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

    ulLocalAdminSidLen = RtlLengthRequiredSid(pSid->SubAuthorityCount + 1);
    dwError = LwAllocateMemory(ulLocalAdminSidLen,
                               OUT_PPVOID(&pLocalAdminSid));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCopySid(ulLocalAdminSidLen,
                          pLocalAdminSid,
                          pSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAppendRidSid(ulLocalAdminSidLen,
                               pLocalAdminSid,
                               DOMAIN_USER_RID_ADMIN);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);
    LW_SAFE_FREE_MEMORY(pLocalAdminSid);

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


static
DWORD
SamDbCreateBuiltinDomainDacl(
    PSID pSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_READ |
                                  DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                  DOMAIN_ACCESS_SET_INFO_1 |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                  DOMAIN_ACCESS_SET_INFO_2 |
                                  DOMAIN_ACCESS_CREATE_USER |
                                  DOMAIN_ACCESS_CREATE_GROUP |
                                  DOMAIN_ACCESS_CREATE_ALIAS |
                                  DOMAIN_ACCESS_LOOKUP_ALIAS |
                                  DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_SET_INFO_3;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                DOMAIN_ACCESS_LOOKUP_ALIAS |
                                DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                DOMAIN_ACCESS_OPEN_ACCOUNT;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
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

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

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


static
DWORD
SamDbCreateLocalUserDacl(
    PSID pAccountSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminsAccessMask = STANDARD_RIGHTS_READ |
                                   WRITE_DAC |
                                   WRITE_OWNER |
                                   DELETE |
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
                                    USER_ACCESS_SET_LOC_COM |
                                    USER_ACCESS_CHANGE_PASSWORD;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                USER_ACCESS_GET_NAME_ETC |
                                USER_ACCESS_GET_LOCALE |
                                USER_ACCESS_GET_LOGONINFO |
                                USER_ACCESS_GET_ATTRIBUTES |
                                USER_ACCESS_CHANGE_PASSWORD |
                                USER_ACCESS_GET_GROUPS |
                                USER_ACCESS_GET_GROUP_MEMBERSHIP;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pWorldSid,
            .AccessMask   = AllAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pBuiltinAdminsSid,
            .AccessMask   = AdminsAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = &pAccountSid,
            .AccessMask   = AccountAccessMask,
            .ulAccessType = ACCESS_ALLOWED_ACE_TYPE
        },
        {
            .ppSid        = NULL,
            .AccessMask   = 0,
            .ulAccessType = 0
        }
    };

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, &AccessList[0]);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

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


static
DWORD
SamDbCreateLocalGroupDacl(
    PSID pSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  ALIAS_ACCESS_ADD_MEMBER |
                                  ALIAS_ACCESS_REMOVE_MEMBER |
                                  ALIAS_ACCESS_GET_MEMBERS |
                                  ALIAS_ACCESS_LOOKUP_INFO |
                                  ALIAS_ACCESS_SET_INFO;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                ALIAS_ACCESS_GET_MEMBERS |
                                ALIAS_ACCESS_LOOKUP_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    ULONG ulLocalAdminSidLen = 0;
    PSID pLocalAdminSid = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
        {
            .ppSid        = &pLocalAdminSid,
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


    ulLocalAdminSidLen = RtlLengthRequiredSid(pSid->SubAuthorityCount + 1);
    dwError = LwAllocateMemory(ulLocalAdminSidLen,
                               OUT_PPVOID(&pLocalAdminSid));
    BAIL_ON_SAMDB_ERROR(dwError);

    ntStatus = RtlCopySid(ulLocalAdminSidLen,
                          pLocalAdminSid,
                          pSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAppendRidSid(ulLocalAdminSidLen,
                               pLocalAdminSid,
                               DOMAIN_USER_RID_ADMIN);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);
    LW_SAFE_FREE_MEMORY(pLocalAdminSid);

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


static
DWORD
SamDbCreateBuiltinGroupDacl(
    PSID pSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  ALIAS_ACCESS_ADD_MEMBER |
                                  ALIAS_ACCESS_REMOVE_MEMBER |
                                  ALIAS_ACCESS_GET_MEMBERS |
                                  ALIAS_ACCESS_LOOKUP_INFO |
                                  ALIAS_ACCESS_SET_INFO;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                ALIAS_ACCESS_GET_MEMBERS |
                                ALIAS_ACCESS_LOOKUP_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
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

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

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


static
DWORD
SamDbCreateNewLocalAccountDacl(
    PSID pSid,
    PACL *ppDacl
    )
{
    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
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
                                  USER_ACCESS_CHANGE_GROUP_MEMBERSHIP |
                                  ALIAS_ACCESS_ADD_MEMBER |
                                  ALIAS_ACCESS_REMOVE_MEMBER |
                                  ALIAS_ACCESS_GET_MEMBERS |
                                  ALIAS_ACCESS_LOOKUP_INFO |
                                  ALIAS_ACCESS_SET_INFO;

    ACCESS_MASK AllAccessMask = STANDARD_RIGHTS_READ |
                                USER_ACCESS_GET_NAME_ETC |
                                USER_ACCESS_GET_LOCALE |
                                USER_ACCESS_GET_LOGONINFO |
                                USER_ACCESS_GET_ATTRIBUTES |
                                USER_ACCESS_GET_GROUPS |
                                USER_ACCESS_GET_GROUP_MEMBERSHIP |
                                ALIAS_ACCESS_GET_MEMBERS |
                                ALIAS_ACCESS_LOOKUP_INFO;

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACL pDacl = NULL;
    PSID pBuiltinAdminsSid = NULL;
    PSID pWorldSid = NULL;

    ACCESS_LIST AccessList[] = {
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

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pBuiltinAdminsSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinWorldSid,
                                   NULL,
                                   &pWorldSid,
                                   NULL);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbCreateDacl(&pDacl, AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);
    LW_SAFE_FREE_MEMORY(pWorldSid);

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


static
DWORD
SamDbCreateDacl(
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
