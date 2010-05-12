/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _NET_CONNECTION_H_
#define _NET_CONNECTION_H_


typedef enum _NET_CONN_TYPE
{
    NET_CONN_SAMR = 1,
    NET_CONN_LSA,
    NET_CONN_WKSSVC

} NET_CONN_TYPE, *PNET_CONN_TYPE;


typedef struct _NET_CONN {
    PWSTR pwszHostname;

    BYTE  SessionKey[16];
    DWORD dwSessionKeyLen;

    NET_CONN_TYPE eType;

    union
    {
        struct _NET_SAMR
        {
            SAMR_BINDING     hBinding;

            CONNECT_HANDLE   hConn;
            DWORD            dwConnAccess;

            DOMAIN_HANDLE    hDomain;
            DWORD            dwDomainAccess;
            PWSTR            pwszDomainName;
            PSID             pDomainSid;

            DOMAIN_HANDLE    hBuiltin;
            DWORD            dwBuiltinAccess;
        } Samr;

        struct _NET_LSA
        {
            LSA_BINDING      hBinding;

            POLICY_HANDLE    hPolicy;
            DWORD            dwPolicyAccess;
        } Lsa;

        struct _NET_WKSSVC
        {
            WKSS_BINDING     hBinding;
        } WksSvc;

    } Rpc;
} NET_CONN, *PNET_CONN;


DWORD
NetConnectSamr(
    PNET_CONN  *ppConn,
    PCWSTR      pwszHostname,
    DWORD       dwReqDomainAccess,
    DWORD       dwReqBuiltinAccess,
    PIO_CREDS   pCreds
    );


DWORD
NetConnectLsa(
    PNET_CONN  *pConn,
    PCWSTR      pwszHostname,
    DWORD       dwReqPolicyAccess,
    PIO_CREDS   pCreds
    );


DWORD
NetConnectWkssvc(
    PNET_CONN  *ppConn,
    PCWSTR      pwszHostname,
    PIO_CREDS   pCreds
    );


VOID
NetDisconnectSamr(
    PNET_CONN  *pConn
    );


VOID
NetDisconnectLsa(
    PNET_CONN  *pConn
    );


VOID
NetDisconnectWkssvc(
    PNET_CONN  *ppConn
    );


#endif /* _NET_CONNECTION_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
