/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
 */

/*
 * Copyright (C) BeyondTrust Software. All rights reserved.
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

