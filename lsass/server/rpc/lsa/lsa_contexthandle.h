/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
