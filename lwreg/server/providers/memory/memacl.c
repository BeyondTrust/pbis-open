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
 *        memacl.c
 *
 * Abstract:
 *        ACL functions for registry memory provider backend
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */
#include "includes.h"

NTSTATUS
MemSetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescRel
    )
{
    NTSTATUS status = STATUS_ACCESS_DENIED;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    PREG_KEY_CONTEXT pKeyCtx = NULL;
    ACCESS_MASK accessRequired = KEY_ALL_ACCESS;
    REG_DB_CONNECTION regDbConn = {0};
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)hNtRegConnection;

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;


    BAIL_ON_NT_INVALID_POINTER(pKeyHandle);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        accessRequired |= WRITE_OWNER;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        accessRequired |= WRITE_DAC;
    }

    status = RegSrvAccessCheckKey(pServerState->pToken,
                                  pSecDescRel,
                                  ulSecDescRel,
                                  accessRequired,
                                  &pKeyHandle->AccessGranted);
    if (STATUS_NO_TOKEN == status)
    {
        status = 0;
        pKeyHandle->AccessGranted = 0;
    }
    BAIL_ON_NT_STATUS(status);
    
    pKeyHandle->pKey->AccessGranted = pKeyHandle->AccessGranted;

    BAIL_ON_NT_STATUS(status);

    status = RegSrvAccessCheckKeyHandle(pKeyHandle, accessRequired);
    BAIL_ON_NT_STATUS(status);

    pKeyCtx = pKeyHandle->pKey;
    //BAIL_ON_INVALID_KEY_CONTEXT(pKeyCtx);

    /* Sanity checks */
    if (SecurityInformation == 0)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (!RtlValidRelativeSecurityDescriptor(pSecDescRel, ulSecDescRel, SecurityInformation))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        BAIL_ON_NT_STATUS(status);
    }

    status = MemDbSetKeyAcl(
                 hNtRegConnection,
                 &regDbConn,
                 pSecDescRel,
                 ulSecDescRel);
    BAIL_ON_NT_STATUS(status);


cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
MemGetKeySecurity(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN OUT PULONG pulSecDescRelLen
    )
{
    NTSTATUS status = 0;
    PREG_KEY_HANDLE pKeyHandle = (PREG_KEY_HANDLE)hKey;
    REG_DB_CONNECTION regDbConn = {0};

    BAIL_ON_NT_INVALID_POINTER(hNtRegConnection);

    regDbConn.pMemReg = pKeyHandle->pKey->hNode;
    status = MemDbGetKeyAcl(
                 hNtRegConnection,
                 &regDbConn,
                 pSecDescRel,
                 pulSecDescRelLen);

cleanup:
    return status;

error:
    goto cleanup;
}

