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
 *
 *        lsa_contexthandle.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa context handles
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSA_CONTEXT_HANDLE_H_
#define _LSA_CONTEXT_HANDLE_H_


enum LsaContextType
{
    LsaContextPolicy = 0,
    LsaContextAccount
};


typedef struct lsa_generic_context
{
    enum LsaContextType  Type;
    LONG                 refcount;

} LSA_GENERIC_CONTEXT, *PLSA_GENERIC_CONTEXT;


typedef struct lsa_policy_context
{
    enum LsaContextType  Type;
    LONG                 refcount;

    PACCESS_TOKEN        pUserToken;
    PBYTE                pSessionKey;
    DWORD                dwSessionKeyLen;
    DWORD                dwAccessGranted;

    SAMR_BINDING         hSamrBinding;
    CONNECT_HANDLE       hConn;
    DOMAIN_HANDLE        hBuiltinDomain;
    DOMAIN_HANDLE        hLocalDomain;
    PSID                 pLocalDomainSid;
    PWSTR                pwszLocalDomainName;
    PWSTR                pwszDomainName;
    PSID                 pDomainSid;
    PWSTR                pwszDcName;
    
    PLW_HASH_TABLE      pDomains;
    DWORD                dwDomainsNum;

} POLICY_CONTEXT, *PPOLICY_CONTEXT;


typedef struct _LSAR_ACCOUNT_CONTEXT
{
    enum LsaContextType  Type;
    LONG                 refcount;

    PLSA_ACCOUNT_CONTEXT pAccountContext;

    PPOLICY_CONTEXT      pPolicyCtx;

} LSAR_ACCOUNT_CONTEXT, *PLSAR_ACCOUNT_CONTEXT;


NTSTATUS
LsaSrvPolicyContextClose(
    PPOLICY_CONTEXT  pContext
    );


VOID
LsaSrvPolicyContextFree(
    PPOLICY_CONTEXT  pContext
    );


VOID
LsaSrvAccountContextFree(
    PLSAR_ACCOUNT_CONTEXT  pAccountCtx
    );


void
POLICY_HANDLE_rundown(
    void *hContext
    );


void
LSAR_ACCOUNT_HANDLE_rundown(
    void *hContext
    );


#endif /* _LSA_CONTEXT_HANDLE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
