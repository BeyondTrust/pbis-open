/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2011
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
 *        lsa_setsecurity.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaSetSecurity function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
LsaSrvSetAccountSecurity(
    PLSAR_ACCOUNT_CONTEXT pAccountContext,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative,
    DWORD SecurityDescRelativeSize
    );


NTSTATUS
LsaSrvSetSecurity(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PVOID hObject,
    /* [in] */ SECURITY_INFORMATION SecurityInformation,
    /* [in] */ PLSA_SECURITY_DESCRIPTOR_BUFFER pSecurityDescBuffer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLSA_GENERIC_CONTEXT pContext = NULL;
    DWORD securityDescRelativeSize = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative = NULL;

    securityDescRelativeSize = pSecurityDescBuffer->BufferLen;
    pSecurityDescRelative
        = (PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescBuffer->pBuffer;

    pContext = (PLSA_GENERIC_CONTEXT)(hObject);

    switch (pContext->Type)
    {
    case LsaContextPolicy:
        ntStatus = STATUS_ACCESS_DENIED;
        break;

    case LsaContextAccount:
        ntStatus = LsaSrvSetAccountSecurity(
                        (PLSAR_ACCOUNT_CONTEXT)pContext,
                        SecurityInformation,
                        pSecurityDescRelative,
                        securityDescRelativeSize);
        break;

    default:
        /* Something is seriously wrong if we get a context
           we haven't created */
        ntStatus = STATUS_INTERNAL_ERROR;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

error:
    return ntStatus;
}


static
NTSTATUS
LsaSrvSetAccountSecurity(
    PLSAR_ACCOUNT_CONTEXT pAccountContext,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative,
    DWORD SecurityDescRelativeSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD err = ERROR_SUCCESS;

    err = LsaSrvPrivsSetAccountSecurity(
                        NULL,
                        pAccountContext->pPolicyCtx->pUserToken,
                        pAccountContext->pAccountContext,
                        SecurityInformation,
                        pSecurityDescRelative,
                        SecurityDescRelativeSize);
    BAIL_ON_LSA_ERROR(err);

error:
    if (ntStatus == STATUS_SUCCESS &&
        err != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(err);
    }

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
