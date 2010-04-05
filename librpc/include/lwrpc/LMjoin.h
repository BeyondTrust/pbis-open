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
 * Abstract: NetAPI domain join functions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _LM_JOIN_H_
#define _LM_JOIN_H_

#include <lwps/lwps.h>
#include <lwrpc/types.h>


/* (un)join domain flags */
#define NETSETUP_JOIN_DOMAIN                          0x00000001
#define NETSETUP_ACCT_CREATE                          0x00000002
#define NETSETUP_ACCT_DELETE                          0x00000004
#define NETSETUP_WIN9X_UPGRADE                        0x00000010
#define NETSETUP_DOMAIN_JOIN_IF_JOINED                0x00000020
#define NETSETUP_JOIN_UNSECURE                        0x00000040
#define NETSETUP_MACHINE_PWD_PASSED                   0x00000080
#define NETSETUP_DEFER_SPN_SET                        0x00000100


NET_API_STATUS
NetJoinDomain(
    const wchar16_t *hostname,
    const wchar16_t *domain,
    const wchar16_t *account_ou,
    const wchar16_t *account,
    const wchar16_t *password,
    UINT32 options
    );


NET_API_STATUS
NetJoinDomainLocal(
    const wchar16_t *machine,
    const wchar16_t *machine_dns_domain,
    const wchar16_t *domain,
    const wchar16_t *account_ou,
    const wchar16_t *account,
    const wchar16_t *password,
    UINT32 options,
    const wchar16_t *osname,
    const wchar16_t *osver,
    const wchar16_t *ospack
    );


NET_API_STATUS
NetUnjoinDomain(
    const wchar16_t *hostname,
    const wchar16_t *account,
    const wchar16_t *password,
    UINT32 options
    );


NET_API_STATUS
NetUnjoinDomainLocal(
    const wchar16_t *machine, 
    const wchar16_t *domain,
    const wchar16_t *account,
    const wchar16_t *password,
    UINT32 options
    );


NET_API_STATUS
NetMachineChangePassword(
    void
    );


#endif /* _LM_JOIN_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
