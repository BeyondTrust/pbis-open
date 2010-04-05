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
 * Abstract: Netlogon interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _NETLOGON_H_
#define _NETLOGON_H_

#include <lwrpc/netlogonbinding.h>
#include <lwrpc/netrdefs.h>


NTSTATUS
NetrOpenSchannel(
    IN  handle_t          hNetrBinding,
    IN  PCWSTR            pwszMachineAccount,
    IN  PCWSTR            pwszHostname,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszComputer,
    IN  PCWSTR            pwszMachinePassword,
    IN  NetrCredentials  *pCreds,
    OUT handle_t         *phSchannelBinding
    );


VOID
NetrCloseSchannel(
    IN  handle_t hSchannelBinding
    );


BOOLEAN
NetrCredentialsCorrect(
    IN  NetrCredentials  *pCreds,
    IN  BYTE              SrvCreds[8]
    );


NTSTATUS
NetrServerReqChallenge(
    IN  handle_t  hNetrBinding,
    IN  PCWSTR    pwszServer,
    IN  PCWSTR    pwszComputer,
    IN  BYTE      CliChal[8],
    IN  BYTE      SrvChal[8]
    );


NTSTATUS
NetrServerAuthenticate(
    IN  handle_t hNetrBinding,
    IN  PCWSTR   pwszServer,
    IN  PCWSTR   pwszAccount,
    IN  UINT16   SchannelType,
    IN  PCWSTR   pwszComputer,
    IN  UINT8    CliCreds[8],
    IN  UINT8    SrvCreds[8]
    );


NTSTATUS
NetrServerAuthenticate2(
    IN  handle_t    hNetrBinding,
    IN  PCWSTR      pwszServer,
    IN  PCWSTR      pwszAccount,
    IN  UINT16      SchannelType,
    IN  PCWSTR      pwszComputer,
    IN  BYTE        CliCreds[8],
    IN  BYTE        SrvCreds[8],
    IN OUT PUINT32  pNegFlags
    );


NTSTATUS
NetrServerAuthenticate3(
    IN  handle_t   hNetrBinding,
    IN  PCWSTR     pwszServer,
    IN  PCWSTR     pwszAccount,
    IN  UINT16     SchannelType,
    IN  PCWSTR     pwszComputer,
    IN  BYTE       CliCreds[8],
    IN  BYTE       SrvCreds[8],
    IN OUT UINT32 *pNegFlags,
    IN OUT UINT32 *pRid
    );


NTSTATUS
NetrGetDomainInfo(
    IN  handle_t          hNetrBinding,
    IN  NetrCredentials  *pCreds,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszComputer,
    IN  UINT32            Level,
    IN  NetrDomainQuery  *pQuery,
    OUT NetrDomainInfo  **ppDomainInfo
    );


NTSTATUS
NetrSamLogonInteractive(
    IN  handle_t              hNetrBinding,
    IN  NetrCredentials      *pCreds,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PCWSTR                pwszPassword,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    );


NTSTATUS
NetrSamLogonNetwork(
    IN  handle_t               hNetrBinding,
    IN  NetrCredentials       *pCreds,
    IN  PCWSTR                 pwszServer,
    IN  PCWSTR                 pwszDomain,
    IN  PCWSTR                 pwszComputer,
    IN  PCWSTR                 pwszUsername,
    IN  PBYTE                  pChallenge,
    IN  PBYTE                  pLmResp,
    IN  UINT32                 LmRespLen,
    IN  PBYTE                  pNtResp,
    IN  UINT32                 NtRespLen,
    IN  UINT16                 LogonLevel,
    IN  UINT16                 ValidationLevel,
    OUT NetrValidationInfo   **ppValidationInfo,
    OUT PBYTE                  pAuthoritative
    );


NTSTATUS
NetrSamLogoff(
    IN  handle_t          hNetrBinding,
    IN  NetrCredentials  *pCreds,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszDomain,
    IN  PCWSTR            pwszComputer,
    IN  PCWSTR            pwszUsername,
    IN  PCWSTR            pwszPassword,
    IN  UINT32            LogonLevel
    );


NTSTATUS
NetrSamLogonEx(
    IN  handle_t              hNetrBinding,
    IN  NetrCredentials      *pCreds,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PCWSTR                pwszPassword,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    );

NTSTATUS
NetrSamLogonNetworkEx(
    IN  handle_t              hNetrBinding,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PBYTE                 pChallenge,
    IN  PBYTE                 pLmResp,
    IN  UINT32                LmRespLen,
    IN  PBYTE                 pNtResp,
    IN  UINT32                NtRespLen,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    );

NTSTATUS
NetrEnumerateTrustedDomainsEx(
    IN  handle_t          hNetrBinding,
    IN  PCWSTR            pwszServerName,
    OUT NetrDomainTrust **ppTrusts,
    OUT PUINT32           pCount
    );


WINERR
DsrEnumerateDomainTrusts(
    IN  handle_t           hNetrBinding,
    IN  PCWSTR             pwszServer,
    IN  UINT32             Flags,
    OUT NetrDomainTrust  **ppTrusts,
    OUT PUINT32            pCount
    );


WINERR
DsrGetDcName(
    IN  handle_t hNetrBinding,
    IN  PCWSTR pwszServerName,
    IN  PCWSTR pwszDomainName,
    IN  const Guid *pDomainGuid,
    IN  const Guid *pSiteGuid,
    IN  UINT32 GetDcFlags,
    OUT DsrDcNameInfo **ppInfo
    );


VOID
NetrCredentialsInit(
    OUT NetrCredentials *pCreds,
    IN  BYTE             CliChal[8],
    IN  BYTE             SrvChal[8],
    IN  BYTE             PassHash[16],
    IN  UINT32           NegFlags
    );


NTSTATUS
NetrInitMemory(
    VOID
    );


NTSTATUS
NetrDestroyMemory(
    VOID
    );


VOID
NetrFreeMemory(
    IN PVOID pPtr
    );


#endif /* _NETLOGON_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
