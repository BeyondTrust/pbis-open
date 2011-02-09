/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsalocalprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __LSALOCALPROVIDER_H__
#define __LSALOCALPROVIDER_H__

#include "lsautils.h"

#define LSA_LOCAL_IO_SETDOMAINNAME        1
#define LSA_LOCAL_IO_SETDOMAINSID         2
#define LSA_LOCAL_IO_GETDOMAINNAME        3
#define LSA_LOCAL_IO_GETDOMAINSID         4
#define LSA_LOCAL_IO_ENUMPRIVSIDS         5
#define LSA_LOCAL_IO_ADDACCOUNTRIGHTS     6
#define LSA_LOCAL_IO_REMOVEACCOUNTRIGHTS  7
#define LSA_LOCAL_IO_LOOKUPPRIVILEGEVALUE 8
#define LSA_LOCAL_IO_LOOKUPPRIVILEGENAME  9
#define LSA_LOCAL_IO_ENUMACCOUNTRIGHTS   10


typedef struct _LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ {
    DWORD NumSids;
    PCSTR *ppszSids;
} LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ,
*PLSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_REQ;


typedef struct _LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_RESP {
    PLUID_AND_ATTRIBUTES pPrivileges;
    DWORD NumPrivileges;
} LSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_RESP,
*PLSA_LOCAL_IPC_ENUM_PRIVILEGES_SIDS_RESP;


typedef struct _LSA_LOCAL_IPC_ADD_ACCOUNT_RIGHTS_REQ {
    PSTR pszSid;
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_LOCAL_IPC_ADD_ACCOUNT_RIGHTS_REQ,
*PLSA_LOCAL_IPC_ADD_ACCOUNT_RIGHTS_REQ;


typedef struct _LSA_LOCAL_IPC_REMOVE_ACCOUNT_RIGHTS_REQ {
    PSTR pszSid;
    BOOLEAN RemoveAll;
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_LOCAL_IPC_REMOVE_ACCOUNT_RIGHTS_REQ,
*PLSA_LOCAL_IPC_REMOVE_ACCOUNT_RIGHTS_REQ;


typedef struct _LSA_LOCAL_IPC_ENUM_ACCOUNT_RIGHTS_RESP {
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_LOCAL_IPC_ENUM_ACCOUNT_RIGHTS_RESP,
*PLSA_LOCAL_IPC_ENUM_ACCOUNT_RIGHTS_RESP;


LWMsgTypeSpec*
LsaLocalIpcGetEnumPrivilegesSidsReqSpec(
    VOID
    );


LWMsgTypeSpec*
LsaLocalIpcGetEnumPrivilegesSidsRespSpec(
    VOID
    );


LWMsgTypeSpec*
LsaLocalIpcGetAddAccountRightsReqSpec(
    VOID
    );


LWMsgTypeSpec*
LsaLocalIpcGetRemoveAccountRightsReqSpec(
    VOID
    );


LWMsgTypeSpec*
LsaLocalIpcGetEnumAccountRightsRespSpec(
    VOID
    );


#endif /* __LSALOCALPROVIDER_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
