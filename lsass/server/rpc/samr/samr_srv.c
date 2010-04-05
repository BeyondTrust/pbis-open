/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        samr_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server module initialisation and shutdown
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


static
DWORD
SamrSrvCreateServerDacl(
    PACL *ppDacl
    );


DWORD
LsaInitializeRpcSrv(
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_init(&gSamrSrvDataMutex, NULL);

    dwError = RpcSvcRegisterRpcInterface(samr_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRpcSrvName = (PSTR)gpszSamrRpcSrvName;
    *ppFnTable      = &gSamrRpcFuncTable;

    dwError = SamrSrvInitialiseConfig(&gSamrSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvReadRegistry(&gSamrSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvInitServerSecurityDescriptor(&gpSamrSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    bSamrSrvInitialised = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaShutdownRpcSrv(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnregisterRpcInterface(samr_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvDestroyServerSecurityDescriptor(&gpSamrSecDesc);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_destroy(&gSamrSrvDataMutex);

    bSamrSrvInitialised = FALSE;

error:
    return dwError;
}


DWORD
SamrRpcStartServer(
    void
    )
{
    PCSTR pszDescription = "Security Accounts Manager";
    ENDPOINT EndPoints[] = {
        { "ncacn_np",      "\\\\pipe\\\\samr" },
        { "ncacn_ip_tcp",  NULL },
        { "ncalrpc",       NULL },  /* endpoint is fetched from config parameter */
        { NULL,            NULL }
    };

    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszLpcSocketPath = NULL;

    dwError = SamrSrvConfigGetLpcSocketPath(&pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    while (EndPoints[i].pszProtocol) {
        if (strcmp(EndPoints[i].pszProtocol, "ncalrpc") == 0 &&
            pszLpcSocketPath) {
            EndPoints[i].pszEndpoint = pszLpcSocketPath;
        }

        i++;
    }

    dwError = RpcSvcBindRpcInterface(gpSamrSrvBinding,
                                     samr_v1_0_s_ifspec,
                                     EndPoints,
                                     pszDescription);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}


DWORD
SamrRpcStopServer(
    void
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnbindRpcInterface(gpSamrSrvBinding,
                                       samr_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}


DWORD
SamrSrvInitServerSecurityDescriptor(
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

    dwError = LwCreateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &pOwnerSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                                    pSecDesc,
                                    pOwnerSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwCreateWellKnownSid(WinBuiltinAdministratorsSid,
                                   NULL,
                                   &pGroupSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlSetGroupSecurityDescriptor(
                                    pSecDesc,
                                    pGroupSid,
                                    FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SamrSrvCreateServerDacl(&pDacl);
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


DWORD
SamrSrvDestroyServerSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PACL pDacl = NULL;
    BOOLEAN bDaclPresent = FALSE;
    BOOLEAN bDaclDefaulted = FALSE;

    BAIL_ON_INVALID_PTR(ppSecDesc);

    pSecDesc = *ppSecDesc;
    if (pSecDesc == NULL)
    {
        dwError = ERROR_SUCCESS;
        goto cleanup;
    }

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc,
                                            &bDaclPresent,
                                            &pDacl,
                                            &bDaclDefaulted);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (bDaclPresent)
    {
        LW_SAFE_FREE_MEMORY(pDacl);
    }

    LW_SAFE_FREE_MEMORY(pSecDesc);

    *ppSecDesc = NULL;

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamrSrvCreateServerDacl(
    PACL *ppDacl
    )
{
    ACCESS_MASK SystemAccessMask = STANDARD_RIGHTS_REQUIRED |
                                   SAMR_ACCESS_CONNECT_TO_SERVER |
                                   SAMR_ACCESS_SHUTDOWN_SERVER |
                                   SAMR_ACCESS_INITIALIZE_SERVER |
                                   SAMR_ACCESS_CREATE_DOMAIN |
                                   SAMR_ACCESS_ENUM_DOMAINS |
                                   SAMR_ACCESS_OPEN_DOMAIN;

    ACCESS_MASK AdminAccessMask = STANDARD_RIGHTS_REQUIRED |
                                  SAMR_ACCESS_CONNECT_TO_SERVER |
                                  SAMR_ACCESS_SHUTDOWN_SERVER |
                                  SAMR_ACCESS_INITIALIZE_SERVER |
                                  SAMR_ACCESS_ENUM_DOMAINS |
                                  SAMR_ACCESS_OPEN_DOMAIN;

    ACCESS_MASK AuthenticatedAccessMask = STANDARD_RIGHTS_READ |
                                          SAMR_ACCESS_CONNECT_TO_SERVER |
                                          SAMR_ACCESS_ENUM_DOMAINS |
                                          SAMR_ACCESS_OPEN_DOMAIN;

    ACCESS_MASK WorldAccessMask = STANDARD_RIGHTS_READ;

    NTSTATUS ntStatus = STATUS_SUCCESS;
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
    dwSystemSidLen = RtlLengthRequiredSid(1);
    dwError = LwAllocateMemory(dwSystemSidLen,
                               OUT_PPVOID(&pSystemSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateWellKnownSid(WinLocalSystemSid,
                                     NULL,
                                     pSystemSid,
                                     &dwSystemSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* create administrators sid */
    dwBuiltinAdminsSidLen = RtlLengthRequiredSid(2);
    dwError = LwAllocateMemory(dwBuiltinAdminsSidLen,
                               OUT_PPVOID(&pBuiltinAdminsSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateWellKnownSid(WinBuiltinAdministratorsSid,
                                     NULL,
                                     pBuiltinAdminsSid,
                                     &dwBuiltinAdminsSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* create authenticated users sid */
    dwAuthenticatedSidLen = RtlLengthRequiredSid(1);
    dwError = LwAllocateMemory(dwAuthenticatedSidLen,
                               OUT_PPVOID(&pAuthenticatedSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateWellKnownSid(WinAuthenticatedUserSid,
                                     NULL,
                                     pAuthenticatedSid,
                                     &dwAuthenticatedSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /* create world (everyone) sid */
    dwWorldSidLen = RtlLengthRequiredSid(1);
    dwError = LwAllocateMemory(dwWorldSidLen,
                               OUT_PPVOID(&pWorldSid));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlCreateWellKnownSid(WinWorldSid,
                                     NULL,
                                     pWorldSid,
                                     &dwWorldSidLen);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = SamrSrvCreateDacl(&pDacl,
                                AccessList);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDacl = pDacl;

cleanup:
    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
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
