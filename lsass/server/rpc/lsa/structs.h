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
 *        structs.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa rpc internal structures
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSASRV_STRUCTS_H_
#define _LSASRV_STRUCTS_H_


/*
 * Domain cache entry types
 */
typedef enum {
    eDomainSid   = 1,
    eDomainName

} DOMAIN_KEY_TYPE;


typedef struct _DOMAIN_KEY
{
    DOMAIN_KEY_TYPE  eType;
    PSID             pSid;
    PWSTR            pwszName;

} DOMAIN_KEY, *PDOMAIN_KEY;


typedef struct _DOMAIN_ENTRY
{
    PWSTR          pwszName;
    PSID           pSid;
    LSA_BINDING    hLsaBinding;
    POLICY_HANDLE  hPolicy;

} DOMAIN_ENTRY, *PDOMAIN_ENTRY;


/*
 * Account names to lookup
 */
typedef struct _ACCOUNT_NAMES
{
    PWSTR  *ppwszNames;
    PDWORD  pdwIndices;
    DWORD   dwCount;

} ACCOUNT_NAMES, *PACCOUNT_NAMES;


/*
 * Account sids to lookup
 */
typedef struct _ACCOUNT_SIDS
{
    PSID   *ppSids;
    PDWORD  pdwIndices;
    DWORD   dwCount;

} ACCOUNT_SIDS, *PACCOUNT_SIDS;


#endif /* _LSASRV_STRUCTS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
