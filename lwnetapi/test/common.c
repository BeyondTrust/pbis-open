/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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

#include "includes.h"
#include <lsa/ad.h>


BOOLEAN
CreateRpcBinding(
    PVOID            *phBinding,
    RPC_BINDING_TYPE  eBindingType,
    PCWSTR            pwszHostname,
    PCWSTR            pwszBinding,
    PCREDENTIALS      pCredentials
    )
{
    PCSTR pszNtlmsspAuth = "ntlmssp";
    PCSTR pszSign = "sign";
    PCSTR pszSeal = "seal";

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    LSA_BINDING hBinding = NULL;
    LW_PIO_CREDS pIoCreds = NULL;
    PWSTR pwszBindingString = NULL;
    size_t sBindingStringLen = 0;
    PSTR pszBindingString = NULL;
    PSTR *ppszOptions = NULL;
    DWORD iOpt = 0;
    DWORD i = 0;
    DWORD dwNumValidOptions = 0;
    unsigned32 AuthType = 0;
    unsigned32 ProtectionLevel = 0;
    SEC_WINNT_AUTH_IDENTITY *pNtlmSspAuthInfo = NULL;
    PVOID pAuthInfo = NULL;
    unsigned char *pszUuid = NULL;
    unsigned char *pszProtSeq = NULL;
    unsigned char *pszNetworkAddr = NULL;
    unsigned char *pszEndpoint = NULL;
    unsigned char *pszOptions = NULL;
    PSTR pszHostname = NULL;

    if (phBinding == NULL)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pwszBinding)
    {
        dwError = LwAllocateWc16String(&pwszBindingString,
                                       pwszBinding);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwWc16sLen(pwszBindingString, &sBindingStringLen);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwWc16sToMbs(pwszBindingString,
                               &pszBindingString);
        BAIL_ON_WIN_ERROR(dwError);

        ppszOptions = get_string_list(pszBindingString, ':');
        if (ppszOptions == NULL)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /*
         * Find and identify valid options
         */
        while (ppszOptions[iOpt])
        {
            if (!strcasecmp(pszNtlmsspAuth, ppszOptions[iOpt]))
            {
                AuthType = rpc_c_authn_winnt;
                dwNumValidOptions++;
            }
            else if (!strcasecmp(pszSign, ppszOptions[iOpt]))
            {
                ProtectionLevel = rpc_c_authn_level_pkt_integrity;
                dwNumValidOptions++;
            }
            else if (!strcasecmp(pszSeal, ppszOptions[iOpt]))
            {
                ProtectionLevel = rpc_c_authn_level_pkt_privacy;
                dwNumValidOptions++;
            }

            iOpt++;
        }
    }

    /*
     * Cut off the options from the binding string so it can
     * be passed to rpc routines
     */
    if (dwNumValidOptions > 0)
    {
        i = sBindingStringLen;
        while (dwNumValidOptions &&
               pwszBindingString[--i])
        {
            if (pwszBindingString[i] == (WCHAR)':')
            {
                dwNumValidOptions--;
            }
        }

        pwszBindingString[i] = (WCHAR)'\0';
        pszBindingString[i] = '\0';
    }

    ntStatus = LwIoGetActiveCreds(NULL, &pIoCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszBindingString)
    {
        ntStatus = RpcInitBindingFromBindingString(
                                         (handle_t*)&hBinding,
                                         pwszBindingString,
                                         pIoCreds);
    }
    else
    {
        switch (eBindingType)
        {
        case RPC_LSA_BINDING:
            ntStatus = LsaInitBindingDefault(&hBinding,
                                             pwszHostname,
                                             pIoCreds);
            break;

        case RPC_SAMR_BINDING:
            ntStatus = SamrInitBindingDefault(&hBinding,
                                              pwszHostname,
                                              pIoCreds);
            break;

        case RPC_NETLOGON_BINDING:
            ntStatus = NetrInitBindingDefault(&hBinding,
                                              pwszHostname,
                                              pIoCreds);
            break;

        case RPC_DSSETUP_BINDING:
            ntStatus = DsrInitBindingDefault(&hBinding,
                                             pwszHostname,
                                             pIoCreds);
            break;

        case RPC_WKSSVC_BINDING:
            ntStatus = WkssInitBindingDefault(&hBinding,
                                              pwszHostname,
                                              pIoCreds);
            break;

        default:
            ntStatus = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (AuthType)
    {
    case rpc_c_authn_winnt:
        dwError = LwAllocateMemory(sizeof(*pNtlmSspAuthInfo),
                                   OUT_PPVOID(&pNtlmSspAuthInfo));
        BAIL_ON_WIN_ERROR(dwError);

        if (pCredentials->Ntlm.pwszDomain)
        {
            dwError = LwWc16sToMbs(pCredentials->Ntlm.pwszDomain,
                                   &pNtlmSspAuthInfo->Domain);
            BAIL_ON_WIN_ERROR(dwError);

            pNtlmSspAuthInfo->DomainLength = strlen(pNtlmSspAuthInfo->Domain);
        }

        dwError = LwWc16sToMbs(pCredentials->Ntlm.pwszUsername,
                               &pNtlmSspAuthInfo->User);
        BAIL_ON_WIN_ERROR(dwError);

        pNtlmSspAuthInfo->UserLength = strlen(pNtlmSspAuthInfo->User);

        dwError = LwWc16sToMbs(pCredentials->Ntlm.pwszPassword,
                               &pNtlmSspAuthInfo->Password);
        BAIL_ON_WIN_ERROR(dwError);

        pNtlmSspAuthInfo->PasswordLength = strlen(pNtlmSspAuthInfo->Password);

        pAuthInfo = (PVOID)pNtlmSspAuthInfo;
        break;

    default:
        pAuthInfo = NULL;
        break;
    }

    if (pwszHostname)
    {
        dwError = LwWc16sToMbs(pwszHostname,
                               &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }
    else
    {
        rpc_string_binding_parse((unsigned char*)pszBindingString,
                                 &pszUuid,
                                 &pszProtSeq,
                                 &pszNetworkAddr,
                                 &pszEndpoint,
                                 &pszOptions,
                                 &rpcStatus);
        if (rpcStatus)
        {
            ntStatus = LwRpcStatusToNtStatus(rpcStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        dwError = LwAllocateString((PSTR)pszNetworkAddr,
                                   &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (AuthType)
    {
        rpc_binding_set_auth_info(hBinding,
                                  (unsigned char*)pszHostname,
                                  ProtectionLevel,
                                  AuthType,
                                  (rpc_auth_identity_handle_t)pAuthInfo,
                                  rpc_c_authz_name, /* authz_protocol */
                                  &rpcStatus);
        if (rpcStatus)
        {
            ntStatus = LwRpcStatusToNtStatus(rpcStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *phBinding = hBinding;

cleanup:
    if (pIoCreds)
    {
        LwIoDeleteCreds(pIoCreds);
    }

    LW_SAFE_FREE_MEMORY(pwszBindingString);
    LW_SAFE_FREE_MEMORY(pszBindingString);
    LW_SAFE_FREE_MEMORY(pszHostname);

    rpc_string_free(&pszUuid, &rpcStatus);
    rpc_string_free(&pszProtSeq, &rpcStatus);
    rpc_string_free(&pszNetworkAddr, &rpcStatus);
    rpc_string_free(&pszEndpoint, &rpcStatus);
    rpc_string_free(&pszOptions, &rpcStatus);

    if (ppszOptions)
    {
        free_string_list(ppszOptions);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;

error:
    *phBinding = NULL;
    bRet       = FALSE;

    goto cleanup;
}


VOID
FreeRpcBinding(
    PVOID            *phBinding,
    RPC_BINDING_TYPE  eBindingType
    )
{
    unsigned32 rpcStatus = RPC_S_OK;
    PVOID hBinding = NULL;

    if (phBinding) return;

    hBinding = *phBinding;

    switch (eBindingType)
    {
    case RPC_LSA_BINDING:
        LsaFreeBinding(hBinding);
        break;

    case RPC_SAMR_BINDING:
        SamrFreeBinding(hBinding);
        break;

    case RPC_NETLOGON_BINDING:
        NetrFreeBinding(hBinding);
        break;

    case RPC_DSSETUP_BINDING:
        DsrFreeBinding(hBinding);
        break;

    case RPC_WKSSVC_BINDING:
        WkssFreeBinding(hBinding);
        break;

    default:
        if (phBinding && *phBinding)
        {
            rpc_binding_free((handle_t*)phBinding, &rpcStatus);
        }
        break;
    }

    *phBinding = NULL;
}


DWORD
GetMachinePassword(
    OUT OPTIONAL PWSTR* ppwszDnsDomainName,
    OUT OPTIONAL PWSTR* ppwszDomainName,
    OUT OPTIONAL PWSTR* ppwszMachineSamAccountName,
    OUT OPTIONAL PWSTR* ppwszMachinePassword,
    OUT OPTIONAL PWSTR* ppwszComputerName
    )
{
    DWORD dwError = 0;
    PWSTR pwszDnsDomainName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszMachineSamAccountName = NULL;
    PWSTR pwszMachinePassword = NULL;
    PWSTR pwszComputerName = NULL;
    HANDLE hLsaConnection = NULL;
    PLSA_MACHINE_PASSWORD_INFO_A pPasswordInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LsaAdGetMachinePasswordInfo(hLsaConnection,
                                          NULL,
                                          &pPasswordInfo);
    BAIL_ON_WIN_ERROR(dwError);

    if (ppwszDnsDomainName)
    {
        dwError = LwMbsToWc16s(pPasswordInfo->Account.DnsDomainName,
                               &pwszDnsDomainName);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppwszDomainName)
    {
        dwError = LwMbsToWc16s(pPasswordInfo->Account.NetbiosDomainName,
                               &pwszDomainName);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppwszMachineSamAccountName)
    {
        dwError = LwMbsToWc16s(pPasswordInfo->Account.SamAccountName,
                               &pwszMachineSamAccountName);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppwszMachinePassword)
    {
        dwError = LwMbsToWc16s(pPasswordInfo->Password,
                               &pwszMachinePassword);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppwszComputerName)
    {
        dwError = LwMbsToWc16s(pPasswordInfo->Account.SamAccountName,
                               &pwszComputerName);
        BAIL_ON_WIN_ERROR(dwError);

        // Remove $ from account name
        pwszComputerName[wc16slen(pwszComputerName) - 1] = 0;
    }

error:
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
        LW_SAFE_FREE_MEMORY(pwszDomainName);
        LW_SAFE_FREE_MEMORY(pwszMachineSamAccountName);
        LW_SECURE_FREE_WSTRING(pwszMachinePassword);
        LW_SAFE_FREE_MEMORY(pwszComputerName);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pPasswordInfo)
    {
        LsaAdFreeMachinePasswordInfo(pPasswordInfo);
    }

    if (ppwszDnsDomainName)
    {
        LW_SAFE_FREE_MEMORY(*ppwszDnsDomainName);
        *ppwszDnsDomainName = pwszDnsDomainName;
    }
    if (ppwszDomainName)
    {
        LW_SAFE_FREE_MEMORY(*ppwszDomainName);
        *ppwszDomainName = pwszDomainName;
    }
    if (ppwszMachineSamAccountName)
    {
        LW_SAFE_FREE_MEMORY(*ppwszMachineSamAccountName);
        *ppwszMachineSamAccountName = pwszMachineSamAccountName;
    }
    if (ppwszMachinePassword)
    {
        LW_SAFE_FREE_MEMORY(*ppwszMachinePassword);
        *ppwszMachinePassword = pwszMachinePassword;
    }
    if (ppwszComputerName)
    {
        LW_SAFE_FREE_MEMORY(*ppwszComputerName);
        *ppwszComputerName = pwszComputerName;
    }

    return dwError;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
