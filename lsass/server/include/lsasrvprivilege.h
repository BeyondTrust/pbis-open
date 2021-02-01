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
 *        lsasrvprivilege.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        Local Privilege Server API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef __LSASRV_PRIVILEGE_H__
#define __LSASRV_PRIVILEGE_H__


struct _LSA_ACCOUNT_CONTEXT;
typedef struct _LSA_ACCOUNT_CONTEXT
LSA_ACCOUNT_CONTEXT, *PLSA_ACCOUNT_CONTEXT;


DWORD
LsaSrvInitPrivileges(
    VOID
    );


VOID
LsaSrvFreePrivileges(
    VOID
    );


DWORD
LsaSrvPrivsOpenAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    );


DWORD
LsaSrvPrivsCreateAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID Sid,
    IN ACCESS_MASK AccessRights,
    OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    );


VOID
LsaSrvPrivsCloseAccount(
    IN OUT PLSA_ACCOUNT_CONTEXT *pAccountContext
    );


DWORD
LsaSrvPrivsEnumAccounts(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PSID **ppAccountSids,
    OUT PDWORD pCount
    );


DWORD
LsaSrvPrivsEnumAccountsWithUserRight(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PWSTR UserRight,
    OUT PSID **ppAccountSids,
    OUT PDWORD pCount
    );


DWORD
LsaSrvPrivsAddAccountRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID AccountSid,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    );


DWORD
LsaSrvPrivsRemoveAccountRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID AccountSid,
    IN BOOLEAN RemoveAll,
    IN PWSTR *ppwszAccountRights,
    IN DWORD NumAccountRights
    );


DWORD
LsaSrvPrivsEnumAccountRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PSID AccountSid,
    OUT PWSTR **ppwszAccountRights,
    OUT PDWORD pNumAccountRights
    );


DWORD
LsaSrvPrivsEnumPrivileges(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PDWORD pResume,
    IN DWORD PreferredMaxSize,
    OUT PWSTR **ppPrivilegeNames,
    OUT PLUID *ppPrivilegeValues,
    OUT PDWORD pCount
    );


DWORD
LsaSrvPrivsLookupPrivilegeValue(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PCWSTR pwszPrivilegeName,
    OUT PLUID pPrivilegeValue
    );


DWORD
LsaSrvPrivsLookupPrivilegeName(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLUID pPrivilegeValue,
    OUT PWSTR *ppwszPrivilegeName
    );


DWORD
LsaSrvPrivsLookupPrivilegeDescription(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PCWSTR PrivilegeName,
    IN SHORT ClientLanguageId,
    IN SHORT ClientSystemLanguageId,
    OUT PWSTR *pPrivilegeDescription,
    OUT PUSHORT pLanguageId
    );


DWORD
LsaSrvPrivsAddPrivilegesToAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN PPRIVILEGE_SET pPrivilegeSet
    );


DWORD
LsaSrvPrivsRemovePrivilegesFromAccount(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN BOOLEAN RemoveAll,
    IN PPRIVILEGE_SET pPrivilegeSet
    );


DWORD
LsaSrvPrivsEnumAccountPrivileges(
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    OUT PPRIVILEGE_SET *ppPrivileges
    );


DWORD
LsaSrvPrivsGetSystemAccessRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    OUT PDWORD pSystemAccessRights
    );


DWORD
LsaSrvPrivsSetSystemAccessRights(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN DWORD SystemAccessRights
    );


DWORD
LsaSrvPrivsGetAccountSecurity(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppSecurityDescRelative,
    OUT PDWORD pSecurityDescRelativeSize
    );


DWORD
LsaSrvPrivsSetAccountSecurity(
    IN OPTIONAL HANDLE hServer,
    IN OPTIONAL PACCESS_TOKEN AccessToken,
    IN PLSA_ACCOUNT_CONTEXT pAccountContext,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescRelative,
    IN DWORD SecurityDescRelativeSize
    );


DWORD
LsaSrvPrivsMarkAccountDeleted(
    IN PLSA_ACCOUNT_CONTEXT pAccountContext
    );


#endif /* __LSASRV_PRIVILEGE_H__ */
