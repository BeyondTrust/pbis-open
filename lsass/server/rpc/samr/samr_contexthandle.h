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
 *        samr_contexthandle.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr context handles
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMR_CONTEXT_HANDLE_H_
#define _SAMR_CONTEXT_HANDLE_H_


enum SamrContextType
{
    SamrContextConnect = 0,
    SamrContextDomain,
    SamrContextAccount
};


typedef struct samr_generic_context
{
    enum SamrContextType    Type;
    LONG                    refcount;

} SAMR_GENERIC_CONTEXT, *PSAMR_GENERIC_CONTEXT;


typedef struct samr_connect_context
{
    enum SamrContextType    Type;
    LONG                    refcount;

    PACCESS_TOKEN           pUserToken;
    PBYTE                   pSessionKey;
    DWORD                   dwSessionKeyLen;
    DWORD                   dwAccessGranted;

    DWORD                   dwConnectVersion;
    DWORD                   dwLevel;
    SamrConnectInfo         Info;                   

    HANDLE                  hDirectory;

} CONNECT_CONTEXT, *PCONNECT_CONTEXT;


typedef struct samr_domain_context
{
    enum SamrContextType Type;
    LONG                 refcount;

    DWORD                dwAccessGranted;

    PWSTR                pwszDn;
    PWSTR                pwszDomainName;
    PSID                 pDomainSid;
    LONG64               ntMinPasswordAge;
    LONG64               ntMaxPasswordAge;
    DWORD                dwMinPasswordLen;
    LONG64               ntPasswordPromptTime;
    DWORD                dwPasswordProperties;

    PCONNECT_CONTEXT     pConnCtx;

} DOMAIN_CONTEXT, *PDOMAIN_CONTEXT;


typedef struct samr_account_context
{
    enum SamrContextType Type;
    LONG                 refcount;

    DWORD                dwAccessGranted;

    PWSTR                pwszDn;
    PWSTR                pwszName;
    DWORD                dwRid;
    DWORD                dwAccountType;
    PSID                 pSid;

    PDOMAIN_CONTEXT      pDomCtx;

} ACCOUNT_CONTEXT, *PACCOUNT_CONTEXT;


VOID
SamrSrvConnectContextFree(
    PCONNECT_CONTEXT  pConnCtx
    );


VOID
SamrSrvDomainContextFree(
    PDOMAIN_CONTEXT  pDomCtx
    );


VOID
SamrSrvAccountContextFree(
    PACCOUNT_CONTEXT  pAcctCtx
    );


void
CONNECT_HANDLE_rundown(
    void *hContext
    );


void
DOMAIN_HANDLE_rundown(
    void *hContext
    );


void
ACCOUNT_HANDLE_rundown(
    void *hContext
    );


#endif /* _SAMR_CONTEXT_HANDLE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
