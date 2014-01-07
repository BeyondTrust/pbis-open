/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
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
 *        lsa_openpolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        LsaOpenPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaOpenPolicy2(
    IN  LSA_BINDING      hBinding,
    IN  PCWSTR           pwszSysName,
    IN  PVOID            attrib,
    IN  UINT32           AccessMask,
    OUT POLICY_HANDLE   *phPolicy
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR wszDefaultSysName[] = LSA_DEFAULT_SYSNAME;
    PWSTR pwszSystemName = NULL;
    POLICY_HANDLE hPolicy = NULL;
    SECURITY_QUALITY_OF_SERVICE SecQos = {0};
    ObjectAttribute ObjAttribute = {0};

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(phPolicy, ntStatus);

    ntStatus = RtlWC16StringDuplicate(
                        &pwszSystemName,
                        (pwszSysName) ? pwszSysName : &(wszDefaultSysName[0]));
    BAIL_ON_NT_STATUS(ntStatus);

    /* ObjectAttribute argument is not used, so just
       pass an empty structure */

    SecQos.Length              = 0;
    SecQos.ImpersonationLevel  = SecurityImpersonation;
    SecQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecQos.EffectiveOnly       = 0;

    ObjAttribute.len          = 0;
    ObjAttribute.root_dir     = NULL;
    ObjAttribute.object_name  = NULL;
    ObjAttribute.attributes   = 0;
    ObjAttribute.sec_desc     = NULL;
    // TODO-QOS field in object attributes should probaby be NULL.
    ObjAttribute.sec_qos      = &SecQos;

    DCERPC_CALL(ntStatus, cli_LsaOpenPolicy2(
                              (handle_t)hBinding,
                              pwszSystemName,
                              &ObjAttribute,
                              AccessMask,
                              &hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    *phPolicy = hPolicy;

cleanup:
    RtlWC16StringFree(&pwszSystemName);

    return ntStatus;

error:
    if (phPolicy)
    {
        *phPolicy = NULL;
    }

    goto cleanup;
}
