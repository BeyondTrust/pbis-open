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
