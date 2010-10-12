/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
