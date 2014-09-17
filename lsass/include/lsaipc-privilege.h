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
 *        lsaipc-privilege.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __LSAIPC_PRIVILEGE_H__
#define __LSAIPC_PRIVILEGE_H__


typedef struct _LSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_REQ {
    DWORD NumSids;
    PSTR *ppszSids;
} LSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_REQ,
*PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_REQ;


typedef struct _LSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP {
    PLUID_AND_ATTRIBUTES pPrivileges;
    DWORD NumPrivileges;
    DWORD SystemAccessRights;
} LSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP,
*PLSA_PRIVS_IPC_ENUM_PRIVILEGES_SIDS_RESP;


typedef struct _LSA_PRIVS_IPC_ADD_ACCOUNT_RIGHTS_REQ {
    PSTR pszSid;
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_PRIVS_IPC_ADD_ACCOUNT_RIGHTS_REQ,
*PLSA_PRIVS_IPC_ADD_ACCOUNT_RIGHTS_REQ;


typedef struct _LSA_PRIVS_IPC_REMOVE_ACCOUNT_RIGHTS_REQ {
    PSTR pszSid;
    BOOLEAN RemoveAll;
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_PRIVS_IPC_REMOVE_ACCOUNT_RIGHTS_REQ,
*PLSA_PRIVS_IPC_REMOVE_ACCOUNT_RIGHTS_REQ;


typedef struct _LSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_REQ {
    PSTR pszSid;
} LSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_REQ,
*PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_REQ;


typedef struct _LSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP {
    PWSTR *ppwszAccountRights;
    DWORD NumAccountRights;
} LSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP,
*PLSA_PRIVS_IPC_ENUM_ACCOUNT_RIGHTS_RESP;


#endif /* __LSAIPC_PRIVILEGE_H__ */
