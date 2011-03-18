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
 *        privilege.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */
#ifndef __LSACLIENT_PRIVILEGE_H__
#define __LSACLIENT_PRIVILEGE_H__

#include <lsa/lsa.h>
#include <sys/types.h>

//
// Privilege names for predefined unicode strings
//
#define SE_ASSIGN_AUDIT_NAME \
    {'S','e','A','u','d','i','t','P','r','i','v','i','l','e','g','e',0}

#define SE_BACKUP_NAME \
    {'S','e','B','a','c','k','u','p','P','r','i','v','i','l','e','g','e',0}

#define SE_CHANGE_NOTIFY_NAME \
    {'S','e','C','h','a','n','g','e','N','o','t','i','f','y','P','r','i','v','i','l','e','g','e',0}

#define SE_CREATE_SYMBOLIC_LINK_NAME \
    {'S','e','C','h','a','n','g','e','N','o','t','i','f','y','P','r','i','v','i','l','e','g','e',0}

#define SE_LOAD_DRIVER_NAME \
    {'S','e','L','o','a','d', 'D','r','i','v','e','r','P','r','i','v','i','l','e','g','e',0}

#define SE_MACHINE_ACCOUNT_NAME \
    {'S','e','M','a','c','h','i','n','e','A','c','c','o','u','n','t','P','r','i','v','i','l','e','g','e',0}

#define SE_MANAGE_VOLUME_NAME \
    {'S','e','M','a','n','a','g','e','V','o','l','u','m','e','P','r','i','v','i','l','e','g','e',0}

#define SE_REMOTE_SHUTDOWN_NAME \
    {'S','e','M','a','n','a','g','e','V','o','l','u','m','e','P','r','i','v','i','l','e','g','e',0}

#define SE_RESTORE_PRIVILEGE_NAME \
    {'S','e','R','e','s','t','o','r','e','P','r','i','v','i','l','e','g','e',0}

#define SE_SECURTY_PRIVILEGE \
    {'S','e','R','e','s','t','o','r','e','P','r','i','v','i','l','e','g','e',0}

#define SE_SHUTDOWN_NAME \
    {'S','e','S','h','u','t','d','o','w','n','P','r','i','v','i','l','e','g','e',0}

#define SE_SYSTEMTIME_NAME \
    {'S','e','S','y','s','t','e','m','T','i','m','e','P','r','i','v','i','l','e','g','e',0}

#define SE_TAKE_OWNERSHIP_NAME \
    {'S','e','T','a','k','e','O','w','n','e','r','s','h','i','p','P','r','i','v','i','l','e','g','e',0}

#define SE_TCB_NAME \
    {'S','e','T','c','b','P','r','i','v','i','l','e','g','e',0}

#define SE_TIME_ZONE_NAME \
    {'S','e','T','i','m','e','Z','o','n','e','P','r','i','v','i','l','e','g','e',0}


//
// System access right flags
//
#define POLICY_MODE_INTERACTIVE              (0x00000001)
#define POLICY_MODE_NETWORK                  (0x00000002)
#define POLICY_MODE_BATCH                    (0x00000004)
#define POLICY_MODE_SERVICE                  (0x00000010)
#define POLICY_MODE_DENY_INTERACTIVE         (0x00000040)
#define POLICY_MODE_DENY_NETWORK             (0x00000080)
#define POLICY_MODE_DENY_BATCH               (0x00000100)
#define POLICY_MODE_DENY_SERVICE             (0x00000200)
#define POLICY_MODE_REMOTE_INTERACTIVE       (0x00000400)
#define POLICY_MODE_DENY_REMOTE_INTERACTIVE  (0x00000800)

//
// System access right names for predefined unicode strings
//
#define SE_INTERACTIVE_LOGON_NAME \
    {'S','e','I','n','t','e','r','a','c','t','i','v','e','L','o','g','o','n','R','i','g','h','t',0}

#define SE_NETWORK_LOGON_NAME \
    {'S','e','N','e','t','w','o','r','k','L','o','g','o','n','R','i','g','h','t',0}

#define SE_BATCH_LOGON_NAME  \
    {'S','e','B','a','t','c','h','L','o','g','o','n','R','i','g','h','t',0}

#define SE_SERVICE_LOGON_NAME \
    {'S','e','S','e','r','v','i','c','e','L','o','g','o','n','R','i','g','h','t',0}

#define SE_DENY_INTERACTIVE_LOGON_NAME \
    {'S','e','D','e','n','y','I','n','t','e','r','a','c','t','i','v','e','L','o','g','o','n','R','i','g','h','t',0}

#define SE_DENY_NETWORK_LOGON_NAME \
    {'S','e','D','e','n','y','N','e','t','w','o','r','k','L','o','g','o','n','R','i','g','h','t',0}

#define SE_DENY_BATCH_LOGON_NAME \
    {'S','e','D','e','n','y','B','a','t','c','h','L','o','g','o','n','R','i','g','h','t',0}

#define SE_DENY_SERVICE_LOGON_NAME \
    {'S','e','D','e','n','y','S','e','r','v','i','c','e','L','o','g','o','n','R','i','g','h','t',0}

#define SE_REMOTE_INTERACTIVE_LOGON_NAME \
    {'S','e','R','e','m','o','t','e','I','n','t','e','r','a','c','t','i','v','e','L','o','g','o','n','R','i','g','h','t',0}

#define SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME \
    {'S','e','D','e','n','y','R','e','m','o','t','e','I','n','t','e','r','a','c','t','i','v','e','L','o','g','o','n','R','i','g','h','t',0}


DWORD
LsaPrivsAddAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    );


DWORD
LsaPrivsRemoveAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    IN BOOLEAN RemoveAll,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    );


DWORD
LsaPrivsLookupPrivilegeValue(
    IN HANDLE hLsaConnection,
    IN PCWSTR pwszPrivilegeName,
    OUT PLUID pPrivilegeValue
    );


DWORD
LsaPrivsLookupPrivilegeName(
    IN HANDLE hLsaConnection,
    IN PLUID pPrivilegeValue,
    OUT PWSTR *ppwszPrivilegeName
    );


DWORD
LsaPrivsEnumAccountRights(
    IN HANDLE hLsaConnection,
    IN PSID pAccountSid,
    OUT PWSTR **pppwszAccountRights,
    OUT PDWORD pNumAccountRights
    );


#endif /* __LSACLIENT_PRIVILEGE_H__ */
