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
 *        lsa_domaincache.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SAM domains cache for use with samr rpc client calls
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSASRV_DOMAINCACHE_H_
#define _LSASRV_DOMAINCACHE_H_


NTSTATUS
LsaSrvCreateDomainsTable(
    PLW_HASH_TABLE *ppDomains
    );


NTSTATUS
LsaSrvGetDomainByName(
    PPOLICY_CONTEXT pPolCtx,
    PCWSTR pwszDomainName,
    PDOMAIN_ENTRY *ppDomain
    );


NTSTATUS
LsaSrvGetDomainBySid(
    PPOLICY_CONTEXT pPolCtx,
    const PSID pSid,
    PDOMAIN_ENTRY *ppDomain
    );


NTSTATUS
LsaSrvSetDomain(
    PPOLICY_CONTEXT pPolCtx,
    const PDOMAIN_ENTRY pDomain
    );


VOID
LsaSrvDomainEntryFree(
    PDOMAIN_ENTRY *ppEntry
    );


VOID
LsaSrvDestroyDomainsTable(
    PLW_HASH_TABLE  pDomains,
    BOOLEAN          bCleanClose
    );


NTSTATUS
LsaSrvConnectDomainByName(
    PPOLICY_CONTEXT   pPolCtx,
    PCWSTR            pwszDomainName,
    PDOMAIN_ENTRY    *ppDomEntry
    );


NTSTATUS
LsaSrvConnectDomainBySid(
    PPOLICY_CONTEXT   pPolCtx,
    PSID              pDomainSid,
    PDOMAIN_ENTRY    *ppDomEntry
    );


#endif /* _LSASRV_DOMAINCACHE_H_ */
