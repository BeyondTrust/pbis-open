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

#ifndef _JOIN_LOCAL_H_
#define _JOIN_LOCAL_H_


NTSTATUS
ResetAccountPasswordTimer(
    handle_t samr_b,
    ACCOUNT_HANDLE hAccount,
    UINT32 account_flags
    );


NTSTATUS
ResetWksAccount(
    PNET_CONN       pConn,
    wchar16_t      *name,
    ACCOUNT_HANDLE  hAccount
    );


NTSTATUS
CreateWksAccount(
    PNET_CONN       pConn,
    wchar16_t      *samacct_name,
    ACCOUNT_HANDLE *phAccount
    );


NTSTATUS
SetMachinePassword(
    PNET_CONN       pConn,
    ACCOUNT_HANDLE  hAccount,
    UINT32          new,
    wchar16_t      *name,
    wchar16_t      *password
    );


NET_API_STATUS
DirectoryConnect(
    const wchar16_t *domain,
    LDAP **ldconn,
    wchar16_t **dn_context
    );

NET_API_STATUS
DirectoryDisconnect(
    LDAP *ldconn
    );

NET_API_STATUS
MachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    const wchar16_t *dns_domain_name,
    wchar16_t **samacct);

NET_API_STATUS
MachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **dn
    );

NET_API_STATUS
MachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    int rejoin
    );

NET_API_STATUS
MachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    );


#endif /* _JOIN_LOCAL_H_ */
