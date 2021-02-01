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
 *        lsaipc-privilege.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS) Client API
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
