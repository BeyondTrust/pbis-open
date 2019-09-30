/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *
 *        builtin.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Builtin Privileges table and database init function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


typedef struct _LSA_BUILTIN_PRIVILEGE
{
    PSTR     pszName;
    PSTR     pszDescription;
    LONG     Value;
    BOOLEAN  EnabledByDefault;

} LSA_BUILTIN_PRIVILEGE, *PLSA_BUILTIN_PRIVILEGE;


static LSA_BUILTIN_PRIVILEGE BuiltinPrivileges[] = {
    {
        .pszName = "SeAuditPrivilege",
        .pszDescription = "Generate security audits.",
        .Value = SE_AUDIT_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeBackupPrivilege",
        .pszDescription = "Backup files and directories.",
        .Value = SE_BACKUP_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeChangeNotifyPrivilege",
        .pszDescription = "Bypass traverse checking.",
        .Value = SE_CHANGE_NOTIFY_PRIVILEGE,
        .EnabledByDefault = TRUE
    },
    {
        .pszName = "SeCreateSymbolicLinkPrivilege",
        .pszDescription = "Create symbolic links.",
        .Value = SE_CREATE_SYMBOLIC_LINK_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeLoadDriverPrivilege",
        .pszDescription = "Load and unload device drivers.",
        .Value = SE_LOAD_DRIVER_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeMachineAccountPrivilege",
        .pszDescription = "Add workstations to domain.",
        .Value = SE_MACHINE_ACCOUNT_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeManageVolumePrivilege",
        .pszDescription = "Manage the files on a volume.",
        .Value = SE_MANAGE_VOLUME_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeRemoteShutdownPrivilege",
        .pszDescription = "Force shutdown from a remote system.",
        .Value = SE_REMOTE_SHUTDOWN_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeRestorePrivilege",
        .pszDescription = "Restore files and directories.",
        .Value = SE_RESTORE_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeSecurityPrivilege",
        .pszDescription = "Manage auditing and security log.",
        .Value = SE_SECURITY_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeShutdownPrivilege",
        .pszDescription = "Shut down the system.",
        .Value = SE_SHUTDOWN_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeSystemTimePrivilege",
        .pszDescription = "Change system time.",
        .Value = SE_SYSTEM_TIME_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeTakeOwnershipPrivilege",
        .pszDescription = "Take ownership of files or other objects.",
        .Value = SE_TAKE_OWNERSHIP_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeTcbPrivilege",
        .pszDescription = "Act as part of the operating system.",
        .Value = SE_TCB_PRIVILEGE,
        .EnabledByDefault = FALSE
    },
    {
        .pszName = "SeTimeZonePrivilege",
        .pszDescription = "Change time zone.",
        .Value = SE_TIME_ZONE_PRIVILEGE,
        .EnabledByDefault = FALSE
    }
};


DWORD
LsaSrvPrivsAddBuiltinPrivileges(
    PLW_HASH_TABLE pPrivilegesTable
    )
{
    DWORD err = ERROR_SUCCESS;
    DWORD i = 0;
    DWORD numPrivileges = sizeof(BuiltinPrivileges)/sizeof(BuiltinPrivileges[0]);
    PLSA_PRIVILEGE pPrivilege = NULL;
    PSTR pszKey = NULL;

    LSA_LOG_VERBOSE("Loading builtin privileges (%u privileges)",
                    numPrivileges);
    for (i = 0; i < numPrivileges; i++)
    {
        PLSA_BUILTIN_PRIVILEGE pBuiltin = &BuiltinPrivileges[i];
        PLSA_PRIVILEGE pExistingEntry = NULL;

        err = LwAllocateMemory(
                        sizeof(*pPrivilege),
                        OUT_PPVOID(&pPrivilege));
        BAIL_ON_LSA_ERROR(err);

        LSA_LOG_VERBOSE("Loading privilege %s",
                        LSA_SAFE_LOG_STRING(pBuiltin->pszName));

        err = LwAllocateString(
                        pBuiltin->pszName,
                        &pPrivilege->pszName);
        BAIL_ON_LSA_ERROR(err);

        err = LwMbsToWc16s(
                        pBuiltin->pszDescription,
                        &pPrivilege->pwszDescription);
        BAIL_ON_LSA_ERROR(err);

        pPrivilege->Luid = RtlConvertUlongToLuid(pBuiltin->Value);
        pPrivilege->EnabledByDefault = pBuiltin->EnabledByDefault;

        err = LwAllocateString(
                        pBuiltin->pszName,
                        &pszKey);
        BAIL_ON_LSA_ERROR(err);

        err = LwHashGetValue(
                        pPrivilegesTable,
                        pszKey,
                        (PVOID)&pExistingEntry);
        if (err == ERROR_SUCCESS)
        {
            LSA_LOG_ERROR("Duplicate %s privilege entry found", pszKey);

            err = ERROR_ALREADY_EXISTS;
            BAIL_ON_LSA_ERROR(err);
        }
        else if (err == ERROR_NOT_FOUND)
        {
            err = ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(err);
        }

        err = LwHashSetValue(
                        pPrivilegesTable,
                        pszKey,
                        pPrivilege);
        BAIL_ON_LSA_ERROR(err);

        pszKey = NULL;
        pPrivilege = NULL;
    }

error:
    if (err)
    {
        if (pPrivilege)
        {
            LW_SAFE_FREE_MEMORY(pPrivilege->pszName);
            LW_SAFE_FREE_MEMORY(pPrivilege->pwszDescription);
            LW_SAFE_FREE_MEMORY(pPrivilege);
        }
    }

    return err;
}
