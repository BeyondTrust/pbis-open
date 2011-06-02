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


static
BOOLEAN
CallOpenSchannel(
    NETR_BINDING hNetr,
    PWSTR pwszMachineAccount,
    PCWSTR pwszMachineName,
    PWSTR pwszServer,
    PWSTR pwszDomain,
    PWSTR pwszDnsDomain,
    PWSTR pwszComputer,
    PWSTR pwszMachpass,
    NetrCredentials *pCreds,
    NETR_BINDING *phSchn
    );

static
BOOLEAN
CallCloseSchannel(
    NETR_BINDING  hSchannel
    );

static
BOOLEAN
CallNetrSamLogonInteractive(
    NETR_BINDING           hSchannel,
    NetrCredentials       *pCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    DWORD                  logonLevel,
    PDWORD                 pValidationLevels,
    DWORD                  numValidationLevels,
    NetrValidationInfo  ***pppSamLogonInfo
    );

static
BOOLEAN
CallNetrSamLogonInteractiveEx(
    NETR_BINDING           hSchannel,
    NetrCredentials       *pCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    DWORD                  logonLevel,
    PDWORD                 pValidationLevels,
    DWORD                  numValidationLevels,
    NetrValidationInfo  ***pppSamLogonInfo
    );

static
BOOLEAN
CallNetrSamLogoff(
    NETR_BINDING      hSchannel,
    NetrCredentials  *pCreds,
    PWSTR             pwszServer,
    PWSTR             pwszDomain,
    PWSTR             pwszComputer,
    PWSTR             pwszUsername,
    PWSTR             pwszPassword,
    DWORD             dwLevels
    );

static
BOOLEAN
TestValidateSamLogonInfo(
    NetrValidationInfo ***pLogonInfo,
    DWORD                 dwNumLogonInfos
    );

static
BOOLEAN
TestValidateDomainTrusts(
    NetrDomainTrust      *pTrust,
    DWORD                 dwNumTrusts
    );

static
DWORD
TestNetlogonSamLogonInteractive(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestNetlogonEnumTrustedDomains(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestNetlogonEnumDomainTrusts(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestNetlogonGetDcName(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );


static
BOOLEAN
CallOpenSchannel(
    NETR_BINDING      hNetr,
    PWSTR             pwszMachineAccount,
    PCWSTR            pwszMachineName,
    PWSTR             pwszServer,
    PWSTR             pwszDomain,
    PWSTR             pwszDnsDomain,
    PWSTR             pwszComputer,
    PWSTR             pwszMachpass,
    NetrCredentials  *pCreds,
    PNETR_BINDING     phSchn
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NETR_BINDING hSchn = NULL;

    CALL_MSRPC(ntStatus, NetrOpenSchannel(hNetr,
                                          pwszMachineAccount,
                                          pwszMachineName,
                                          pwszServer,
                                          pwszDomain,
                                          pwszDnsDomain,
                                          pwszComputer,
                                          pwszMachpass,
                                          pCreds,
                                          &hSchn));
    if (ntStatus)
    {
        bRet = FALSE;
    }

    *phSchn = hSchn;

    return bRet;
}


static
BOOLEAN
CallCloseSchannel(
    NETR_BINDING  hSchannel
    )
{
    BOOLEAN bRet = TRUE;

    NetrFreeBinding(&hSchannel);

    LwIoSetThreadCreds(NULL);

    return bRet;
}


static
BOOLEAN
CallNetrSamLogonInteractive(
    NETR_BINDING           hSchannel,
    NetrCredentials       *pCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    DWORD                  logonLevel,
    PDWORD                 pValidationLevels,
    DWORD                  numValidationLevels,
    NetrValidationInfo  ***pppSamLogonInfo
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD validationLevel = 0;
    DWORD i = 0;
    NetrValidationInfo *pValidationInfo = NULL;
    NetrValidationInfo **ppSamLogonInfo = NULL;
    BYTE Authoritative = 0;
    DWORD totalNumLevels = 7;

    dwError = LwAllocateMemory(sizeof(ppSamLogonInfo[0]) * totalNumLevels,
                               OUT_PPVOID(&ppSamLogonInfo));
    BAIL_ON_WIN_ERROR(dwError);

    for (i = 0; i < numValidationLevels; i++)
    {
        validationLevel = pValidationLevels[i];

        CALL_MSRPC(ntStatus, NetrSamLogonInteractive(
                                          hSchannel,
                                          pCreds,
                                          pwszServer,
                                          pwszDomain,
                                          pwszComputer,
                                          pwszUsername,
                                          pwszPassword,
                                          logonLevel,
                                          validationLevel,
                                          &pValidationInfo,
                                          &Authoritative));
        if (ntStatus)
        {
            bRet = FALSE;
        }

        ppSamLogonInfo[validationLevel] = pValidationInfo;
    }

    *pppSamLogonInfo = ppSamLogonInfo;

error:
    return bRet;
}


static
BOOLEAN
CallNetrSamLogonInteractiveEx(
    NETR_BINDING           hSchannel,
    NetrCredentials      *pNetlogonCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    DWORD                  logonLevel,
    PDWORD                 pValidationLevels,
    DWORD                  numValidationLevels,
    NetrValidationInfo  ***pppSamLogonInfo
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD validationLevel = 0;
    DWORD i = 0;
    NetrValidationInfo *pValidationInfo = NULL;
    NetrValidationInfo **ppSamLogonInfo = NULL;
    BYTE Authoritative = 0;
    DWORD totalNumLevels = 7;

    dwError = LwAllocateMemory(sizeof(ppSamLogonInfo[0]) * totalNumLevels,
                               OUT_PPVOID(&ppSamLogonInfo));
    BAIL_ON_WIN_ERROR(dwError);

    for (i = 0; i < numValidationLevels; i++)
    {
        validationLevel = pValidationLevels[i];

        CALL_MSRPC(ntStatus, NetrSamLogonEx(
                                          hSchannel,
                                          pNetlogonCreds,
                                          pwszServer,
                                          pwszDomain,
                                          pwszComputer,
                                          pwszUsername,
                                          pwszPassword,
                                          logonLevel,
                                          validationLevel,
                                          &pValidationInfo,
                                          &Authoritative));
        if (ntStatus)
        {
            bRet = FALSE;
        }

        ppSamLogonInfo[validationLevel] = pValidationInfo;
    }

    *pppSamLogonInfo = ppSamLogonInfo;

error:    
    return bRet;
}
    

static
BOOLEAN
CallNetrSamLogoff(
    NETR_BINDING      hSchannel,
    NetrCredentials  *pCreds,
    PWSTR             pwszServer,
    PWSTR             pwszDomain,
    PWSTR             pwszComputer,
    PWSTR             pwszUsername,
    PWSTR             pwszPassword,
    DWORD             dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    CALL_MSRPC(ntStatus, NetrSamLogoff(
                                 hSchannel,
                                 pCreds,
                                 pwszServer,
                                 pwszDomain,
                                 pwszComputer,
                                 pwszUsername,
                                 pwszPassword,
                                 dwLevel));
    if (ntStatus)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
BOOLEAN
TestValidateSamLogonInfo(
    NetrValidationInfo ***pppLogonInfo,
    DWORD                 dwNumUsers
    )
{
    BOOLEAN bRet = TRUE;
    DWORD i = 0;

    for (i = 0; i < dwNumUsers; i++)
    {
        NetrValidationInfo **ppLogonInfo = pppLogonInfo[i];

        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.account_name, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.full_name, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.logon_script, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.profile_path, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.home_directory, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.home_drive, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.logon_server, ("(i = %u)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.domain, ("(i = %u)\n", i));
        ASSERT_TEST_MSG((ppLogonInfo[2]->sam2->base.domain_sid != NULL &&
                         RtlValidSid(ppLogonInfo[2]->sam2->base.domain_sid)), ("(i = %u)\n", i));
        ASSERT_TEST_MSG(ppLogonInfo[2]->sam2->base.acct_flags & ACB_NORMAL, ("(i = %u)\n", i));
    }

    return bRet;
}


static
BOOLEAN
TestValidateDomainTrusts(
    NetrDomainTrust      *pTrusts,
    DWORD                 dwNumTrusts
    )
{
    BOOLEAN bRet = TRUE;
    DWORD i = 0;

    for (i = 0; i < dwNumTrusts; i++)
    {
        NetrDomainTrust *pTrust = &(pTrusts[i]);

        ASSERT_TEST_MSG(pTrust->netbios_name != NULL, ("(i = %u)\n", i));
        ASSERT_TEST_MSG(pTrust->dns_name != NULL, ("(i = %u)\n", i));
        ASSERT_TEST_MSG((pTrust->trust_type >= 1 &&
                         pTrust->trust_type <= 4), ("(i = %u)\n", i));
        ASSERT_TEST_MSG((pTrust->sid != NULL &&
                         RtlValidSid(pTrust->sid)), ("(i = %u)\n", i));
    }

    return bRet;
}


static
DWORD
TestNetlogonSamLogonInteractive(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    const DWORD dwDefLogonLevel = 0;
    const DWORD dwDefValidationLevel = 2;

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    NETR_BINDING hSchn = NULL;
    enum param_err perr = perr_success;
    NetrCredentials Creds = {0};
    NetrValidationInfo **ppSamLogonInfo = NULL;
    PSTR pszComputer = NULL;
    PSTR pszMachpass = NULL;
    PWSTR pwszComputer = NULL;
    PWSTR pwszMachAcct = NULL;
    PWSTR pwszMachpass = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDnsDomain = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    DWORD pDefaultLogonLevels[] = { 1, 3, 4};
    DWORD numLogonLevels = (sizeof(pDefaultLogonLevels)
                            /sizeof(pDefaultLogonLevels[0]));
    PDWORD pLogonLevels = NULL;
    DWORD pDefaultValidationLevels[] = { 2, 3, 6 };
    DWORD numValidationLevels = (sizeof(pDefaultValidationLevels)
                                 /sizeof(pDefaultValidationLevels[0]));
    PDWORD pValidationLevels = NULL;
    DWORD iLogonLevel = 0;
    DWORD iValidationLevel = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "server", pt_w16string, &pwszServer,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domain", pt_w16string, &pwszDomain,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "dnsdomain", pt_w16string, &pwszDnsDomain,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string, &pwszUsername,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string, &pwszPassword,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "validation_level", pt_uint32,
                       &dwValidationLevel, &dwDefValidationLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = LwWc16sToMbs(pwszComputer, &pszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachpass, &pszMachpass);
    BAIL_ON_WIN_ERROR(dwError);

    if (strcmp(pszComputer, pszDefComputer) == 0 &&
        strcmp(pszMachpass, pszDefMachpass) == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszComputer);
        LW_SAFE_FREE_MEMORY(pwszMachpass);

        dwError = GetMachinePassword(
                        &pwszDnsDomain,
                        &pwszDomain,
                        &pwszMachAcct,
                        &pwszMachpass,
                        &pwszComputer);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("dnsdomain", pt_w16string, pwszDnsDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hNetr),
                             RPC_NETLOGON_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallOpenSchannel(hNetr,
                             pwszMachAcct,
                             pwszHostname,
                             pwszServer,
                             pwszDomain,
                             pwszDnsDomain,
                             pwszComputer,
                             pwszMachpass,
                             &Creds,
                             &hSchn);

    if (hSchn == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    if (dwLogonLevel)
    {
        pLogonLevels = &dwLogonLevel;
        numLogonLevels = 1;
    }
    else
    {
        pLogonLevels = pDefaultLogonLevels;
    }

    if (dwValidationLevel)
    {
        pValidationLevels = &dwValidationLevel;
        numValidationLevels = 1;
    }
    else
    {
        pValidationLevels = pDefaultValidationLevels;
    }

    for (iLogonLevel = 0; iLogonLevel < numLogonLevels; iLogonLevel++)
    {
        bRet &= CallNetrSamLogonInteractive(
                              hSchn,
                              &Creds,
                              pwszServer,
                              pwszDomain,
                              pwszComputer,
                              pwszUsername,
                              pwszPassword,
                              pLogonLevels[iLogonLevel],
                              pValidationLevels,
                              numValidationLevels,
                              &ppSamLogonInfo);

        if (bRet)
        {
            bRet &= TestValidateSamLogonInfo(&ppSamLogonInfo, 1);

            bRet &= CallNetrSamLogoff(
                                  hSchn,
                                  &Creds,
                                  pwszServer,
                                  pwszDomain,
                                  pwszComputer,
                                  pwszUsername,
                                  pwszPassword,
                                  pLogonLevels[iLogonLevel]);

            for (iValidationLevel = 0;
                 ppSamLogonInfo && iValidationLevel <= 6;
                 iValidationLevel++)
            {
                if (ppSamLogonInfo[iValidationLevel])
                {
                    NetrFreeMemory(ppSamLogonInfo[iValidationLevel]);
                    ppSamLogonInfo[iValidationLevel] = NULL;
                }
            }
        }
    }

    bRet &= CallCloseSchannel(hSchn);

done:
error:
    for (iValidationLevel = 0;
         ppSamLogonInfo && iValidationLevel <= 6;
         iValidationLevel++)
    {
        if (ppSamLogonInfo[iValidationLevel])
        {
            NetrFreeMemory(ppSamLogonInfo[iValidationLevel]);
            ppSamLogonInfo[iValidationLevel] = NULL;
        }
    }

    NetrFreeBinding(&hNetr);

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SECURE_FREE_WSTRING(pwszMachpass);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pszComputer);
    LW_SAFE_FREE_MEMORY(pszMachpass);

    return (int)bRet;
}


static
DWORD
TestNetlogonSamLogonInteractiveEx(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    const DWORD dwDefLogonLevel = 0;
    const DWORD dwDefValidationLevel = 2;

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    NETR_BINDING hSchn = NULL;
    enum param_err perr = perr_success;
    NetrCredentials NetlogonCreds = {0};
    NetrValidationInfo **ppSamLogonInfo = NULL;
    PSTR pszComputer = NULL;
    PSTR pszMachpass = NULL;
    PWSTR pwszComputer = NULL;
    PWSTR pwszMachAcct = NULL;
    PWSTR pwszMachpass = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDnsDomain = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    DWORD pDefaultLogonLevels[] = { 1, 3, 4};
    DWORD numLogonLevels = (sizeof(pDefaultLogonLevels)
                            /sizeof(pDefaultLogonLevels[0]));
    PDWORD pLogonLevels = NULL;
    DWORD pDefaultValidationLevels[] = { 2, 3, 6 };
    DWORD numValidationLevels = (sizeof(pDefaultValidationLevels)
                                 /sizeof(pDefaultValidationLevels[0]));
    PDWORD pValidationLevels = NULL;
    DWORD iLogonLevel = 0;
    DWORD iValidationLevel = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "server", pt_w16string, &pwszServer,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domain", pt_w16string, &pwszDomain,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "dnsdomain", pt_w16string, &pwszDnsDomain,
                       NULL);
    if (perr_is_err(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string, &pwszUsername,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string, &pwszPassword,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "validation_level", pt_uint32,
                       &dwValidationLevel, &dwDefValidationLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = LwWc16sToMbs(pwszComputer, &pszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachpass, &pszMachpass);
    BAIL_ON_WIN_ERROR(dwError);

    if (strcmp(pszComputer, pszDefComputer) == 0 &&
        strcmp(pszMachpass, pszDefMachpass) == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszComputer);
        LW_SAFE_FREE_MEMORY(pwszMachpass);

        dwError = GetMachinePassword(
                        &pwszDnsDomain,
                        &pwszDomain,
                        &pwszMachAcct,
                        &pwszMachpass,
                        &pwszComputer);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("dnsdomain", pt_w16string, pwszDnsDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hNetr),
                             RPC_NETLOGON_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallOpenSchannel(hNetr,
                             pwszMachAcct,
                             pwszHostname,
                             pwszServer,
                             pwszDomain,
                             pwszDnsDomain,
                             pwszComputer,
                             pwszMachpass,
                             &NetlogonCreds,
                             &hSchn);

    if (hSchn == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    if (dwLogonLevel)
    {
        pLogonLevels = &dwLogonLevel;
        numLogonLevels = 1;
    }
    else
    {
        pLogonLevels = pDefaultLogonLevels;
    }

    if (dwValidationLevel)
    {
        pValidationLevels = &dwValidationLevel;
        numValidationLevels = 1;
    }
    else
    {
        pValidationLevels = pDefaultValidationLevels;
    }

    for (iLogonLevel = 0; iLogonLevel < numLogonLevels; iLogonLevel++)
    {
        bRet &= CallNetrSamLogonInteractiveEx(
                              hSchn,
                              &NetlogonCreds,
                              pwszServer,
                              pwszDomain,
                              pwszComputer,
                              pwszUsername,
                              pwszPassword,
                              pLogonLevels[iLogonLevel],
                              pValidationLevels,
                              numValidationLevels,
                              &ppSamLogonInfo);

        if (bRet)
        {
            bRet &= TestValidateSamLogonInfo(&ppSamLogonInfo, 1);

            for (iValidationLevel = 0;
                 ppSamLogonInfo && iValidationLevel <= 6;
                 iValidationLevel++)
            {
                if (ppSamLogonInfo[iValidationLevel])
                {
                    NetrFreeMemory(ppSamLogonInfo[iValidationLevel]);
                    ppSamLogonInfo[iValidationLevel] = NULL;
                }
            }
        }
    }

    bRet &= CallCloseSchannel(hSchn);

done:
error:
    for (iValidationLevel = 0;
         ppSamLogonInfo && iValidationLevel <= 6;
         iValidationLevel++)
    {
        if (ppSamLogonInfo[iValidationLevel])
        {
            NetrFreeMemory(ppSamLogonInfo[iValidationLevel]);
            ppSamLogonInfo[iValidationLevel] = NULL;
        }
    }

    NetrFreeBinding(&hNetr);

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SECURE_FREE_WSTRING(pwszMachpass);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pszComputer);
    LW_SAFE_FREE_MEMORY(pszMachpass);

    return (int)bRet;
}


static
DWORD
TestNetlogonEnumTrustedDomains(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefServer = "TEST";

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NETR_BINDING hNetr = NULL;
    enum param_err perr = perr_success;
    PWSTR pwszServer = NULL;
    DWORD dwCount = 0;
    NetrDomainTrust *pTrusts = NULL;
    DWORD iTrust = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "server", pt_w16string, &pwszServer,
                       &pszDefServer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("server", pt_w16string, pwszServer);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hNetr),
                             RPC_NETLOGON_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    CALL_MSRPC(ntStatus, NetrEnumerateTrustedDomainsEx(hNetr, pwszServer,
                                                     &pTrusts, &dwCount));
    BAIL_ON_NT_STATUS(ntStatus);

    bRet  &= TestValidateDomainTrusts(pTrusts, dwCount);

    for (iTrust = 0; iTrust < dwCount; iTrust++)
    {
        OUTPUT_ARG_WSTR(pTrusts[iTrust].netbios_name);
        OUTPUT_ARG_WSTR(pTrusts[iTrust].dns_name);
        OUTPUT_ARG_UINT(pTrusts[iTrust].trust_flags);
    }

error:
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    NetrFreeBinding(&hNetr);

    LW_SAFE_FREE_MEMORY(pwszServer);

    if (ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestNetlogonEnumDomainTrusts(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefTrustFlags = NETR_TRUST_FLAG_IN_FOREST |
                                  NETR_TRUST_FLAG_OUTBOUND |
                                  NETR_TRUST_FLAG_TREEROOT |
                                  NETR_TRUST_FLAG_PRIMARY |
                                  NETR_TRUST_FLAG_NATIVE |
                                  NETR_TRUST_FLAG_INBOUND;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    enum param_err perr = perr_success;
    DWORD dwTrustFlags = 0;
    DWORD dwCount = 0;
    NetrDomainTrust *pTrusts = NULL;
    DWORD iTrust = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "trustflags", pt_uint32, &dwTrustFlags,
                       &dwDefTrustFlags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("trustflags", pt_uint32, &dwTrustFlags);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hNetr),
                             RPC_NETLOGON_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    CALL_NETAPI(err, DsrEnumerateDomainTrusts(hNetr, pwszHostname, dwTrustFlags,
                                              &pTrusts, &dwCount));
    BAIL_ON_WIN_ERROR(err);

    bRet  &= TestValidateDomainTrusts(pTrusts, dwCount);

    for (iTrust = 0; iTrust < dwCount; iTrust++)
    {
        OUTPUT_ARG_WSTR(pTrusts[iTrust].netbios_name);
        OUTPUT_ARG_WSTR(pTrusts[iTrust].dns_name);
        OUTPUT_ARG_UINT(pTrusts[iTrust].trust_flags);
    }

error:
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    NetrFreeBinding(&hNetr);

    if (ntStatus != STATUS_SUCCESS ||
        err != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestNetlogonGetDcName(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefGetDcFlags = DS_FORCE_REDISCOVERY;
    const PSTR pszDefDomainName = "DOMAIN";

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERROR err = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    enum param_err perr = perr_success;
    DWORD dwGetDcFlags = 0;
    PWSTR pwszDomainName = NULL;
    DsrDcNameInfo *pInfo = NULL;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "getdcflags", pt_uint32, &dwGetDcFlags,
                       &dwDefGetDcFlags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string, &pwszDomainName,
                       &pszDefDomainName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("getdcflags", pt_uint32, &dwGetDcFlags);
    PARAM_INFO("domainname", pt_w16string, pwszDomainName);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hNetr),
                             RPC_NETLOGON_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    CALL_NETAPI(err, DsrGetDcName(hNetr, pwszHostname, pwszDomainName,
                                  NULL, NULL, dwGetDcFlags, &pInfo));
    BAIL_ON_WIN_ERROR(err);

error:
    if (pInfo)
    {
        NetrFreeMemory(pInfo);
    }

    NetrFreeBinding(&hNetr);

    if (ntStatus != STATUS_SUCCESS ||
        err != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


VOID
SetupNetlogonTests(PTEST t)
{
    AddTest(t, "NETR-ENUM-TRUSTED-DOM" , TestNetlogonEnumTrustedDomains);
    AddTest(t, "NETR-DSR-ENUM-DOMTRUSTS", TestNetlogonEnumDomainTrusts);
    AddTest(t, "NETR-SAM-LOGON-INTERACTIVE", TestNetlogonSamLogonInteractive);
    AddTest(t, "NETR-SAM-LOGON-INTERACTIVE-EX", TestNetlogonSamLogonInteractiveEx);
    AddTest(t, "NETR-DSR-GET-DC-NAME", TestNetlogonGetDcName);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
