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
    NETR_BINDING         hNetr,
    PWSTR            pwszMachineAccount,
    PCWSTR           pwszMachineName,
    PWSTR            pwszServer,
    PWSTR            pwszDomain,
    PWSTR            pwszComputer,
    PWSTR            pwszMachpass,
    NetrCredentials *pCreds,
    NETR_BINDING        *phSchn
    );


static
BOOLEAN
CallCloseSchannel(
    NETR_BINDING  hSchannel
    );


static
BOOLEAN
CallNetrSamLogonInteractive(
    NETR_BINDING               hSchannel,
    NetrCredentials       *pCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    PDWORD                 pdwLevels,
    DWORD                  dwNumLevels,
    NetrValidationInfo  ***pppSamLogonInfo
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


NETR_BINDING
CreateNetlogonBinding(
    NETR_BINDING *binding,
    const wchar16_t *host
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LW_PIO_CREDS creds = NULL;

    if (binding == NULL || host == NULL) return NULL;

    if (LwIoGetActiveCreds(NULL, &creds) != STATUS_SUCCESS) return NULL;

    ntStatus = NetrInitBindingDefault(binding, host, creds, FALSE);
    if (ntStatus)
    {
        *binding = NULL;
        goto error;
    }

error:
    if (creds)
    {
        LwIoDeleteCreds(creds);
    }

    return *binding;
}


NETR_BINDING
TestOpenSchannel(
    NETR_BINDING hNetr,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    wchar16_t *server,
    wchar16_t *domain,
    wchar16_t *computer,
    wchar16_t *machpass,
    UINT32 protection_level,
    NetrCredentials *creds
    )
{
    unsigned32 st = rpc_s_ok;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *machine_acct = NULL;
    NETR_BINDING schn_b = NULL;
    PIO_CREDS auth = NULL;
    UINT8 srv_cred[8];
    rpc_schannel_auth_info_t schnauth_info;

    memset((void*)srv_cred, 0, sizeof(srv_cred));
    memset((void*)&schnauth_info, 0, sizeof(schnauth_info));

    machine_acct = asw16printfw(L"%ws$", computer);
    if (machine_acct == NULL) goto error;

    status = NetrOpenSchannel(hNetr, machine_acct, hostname, server, domain,
                              computer, machpass, creds, &schn_b);
    BAIL_ON_NT_STATUS(status);

    if (!NetrCredentialsCorrect(creds, srv_cred))
    {
        status = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(schnauth_info.session_key, creds->session_key, 16);
    schnauth_info.domain_name  = (unsigned char*) awc16stombs(domain);
    schnauth_info.machine_name = (unsigned char*) awc16stombs(computer);
    schnauth_info.sender_flags = rpc_schn_initiator_flags;

    status = LwIoCreatePlainCredsW(user, domain, pass, &auth);
    BAIL_ON_NT_STATUS(status);

    status = LwIoSetThreadCreds(auth);
    BAIL_ON_NT_STATUS(status);

    LwIoDeleteCreds(auth);

done:
    LW_SAFE_FREE_MEMORY(machine_acct);

    return (st == rpc_s_ok &&
            status == STATUS_SUCCESS) ? schn_b : NULL;

error:
    goto done;
}


void TestCloseSchannel(NETR_BINDING schn_b)
{
    NetrFreeBinding(&schn_b);

    LwIoSetThreadCreds(NULL);
}


static
BOOLEAN
CallOpenSchannel(
    NETR_BINDING         hNetr,
    PWSTR            pwszMachineAccount,
    PCWSTR           pwszMachineName,
    PWSTR            pwszServer,
    PWSTR            pwszDomain,
    PWSTR            pwszComputer,
    PWSTR            pwszMachpass,
    NetrCredentials *pCreds,
    NETR_BINDING        *phSchn
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
                                          pwszComputer,
                                          pwszMachpass,
                                          pCreds, &hSchn));
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
    NETR_BINDING               hSchannel,
    NetrCredentials       *pCreds,
    PWSTR                  pwszServer,
    PWSTR                  pwszDomain,
    PWSTR                  pwszComputer,
    PWSTR                  pwszUsername,
    PWSTR                  pwszPassword,
    PDWORD                 pdwLevels,
    DWORD                  dwNumLevels,
    NetrValidationInfo  ***pppSamLogonInfo
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    DWORD dwLogonLevels[] = { 1, 3, 5 };
    DWORD i = 0;
    DWORD j = 0;
    NetrValidationInfo *pValidationInfo = NULL;
    NetrValidationInfo **ppSamLogonInfo = NULL;
    BYTE Authoritative = 0;
    DWORD dwTotalNumLevels = 7;

    dwError = LwAllocateMemory(sizeof(ppSamLogonInfo[0]) * dwTotalNumLevels,
                               OUT_PPVOID(&ppSamLogonInfo));
    BAIL_ON_WIN_ERROR(dwError);

    for (i = 0; i < (sizeof(dwLogonLevels)/sizeof(dwLogonLevels[0])); i++)
    {
        dwLogonLevel = dwLogonLevels[i];

        for (j = 0; j < dwTotalNumLevels; j++)
        {
            if (ppSamLogonInfo[j])
            {
                NetrFreeMemory(ppSamLogonInfo[j]);
            }
        }

        for (j = 0; j < dwNumLevels; j++)
        {
            dwValidationLevel = pdwLevels[j];

            CALL_MSRPC(ntStatus, NetrSamLogonInteractive(hSchannel,
                                                         pCreds,
                                                         pwszServer,
                                                         pwszDomain,
                                                         pwszComputer,
                                                         pwszUsername,
                                                         pwszPassword,
                                                         dwLogonLevel,
                                                         dwValidationLevel,
                                                         &pValidationInfo,
                                                         &Authoritative));
            if (ntStatus)
            {
                bRet = FALSE;
            }

            ppSamLogonInfo[dwValidationLevel] = pValidationInfo;
        }
    }

    *pppSamLogonInfo = ppSamLogonInfo;

error:    
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

        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.account_name, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.full_name, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.logon_script, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.profile_path, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.home_directory, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.home_drive, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.logon_server, ("(i = %d)\n", i));
        ASSERT_UNICODE_STRING_VALID_MSG(&ppLogonInfo[2]->sam2->base.domain, ("(i = %d)\n", i));
        ASSERT_TEST_MSG((ppLogonInfo[2]->sam2->base.domain_sid != NULL &&
                         RtlValidSid(ppLogonInfo[2]->sam2->base.domain_sid)), ("(i = %d)\n", i));
        ASSERT_TEST_MSG(ppLogonInfo[2]->sam2->base.acct_flags & ACB_NORMAL, ("(i = %d)\n", i));
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

        ASSERT_TEST_MSG(pTrust->netbios_name != NULL, ("(i = %d)\n", i));
        ASSERT_TEST_MSG(pTrust->dns_name != NULL, ("(i = %d)\n", i));
        ASSERT_TEST_MSG((pTrust->trust_type >= 1 &&
                         pTrust->trust_type <= 4), ("(i = %d)\n", i));
        ASSERT_TEST_MSG((pTrust->sid != NULL &&
                         RtlValidSid(pTrust->sid)), ("(i = %d)\n", i));
    }

    return bRet;
}


int
TestNetlogonSamLogonInteractive(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    PCSTR pszDefUsername = "user";
    PCSTR pszDefPassword = "pass";
    const DWORD dwDefLogonLevel = 2;
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
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    HANDLE hStore = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pPassInfo = NULL;
    DWORD pdwLevels[] = { 2, 3, 6 };
    DWORD dwNumLevels = sizeof(pdwLevels)/sizeof(pdwLevels[0]);
    DWORD iLevel = 0;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &pwszServer,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &pwszDomain,
                       NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &pwszUsername,
                       &pszDefUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &pwszPassword,
                       &pszDefPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
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

        dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwpsGetPasswordByCurrentHostName(hStore, &pPassInfo);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachAcct,
                                       pPassInfo->pwszMachineAccount);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachpass,
                                       pPassInfo->pwszMachinePassword);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszComputer,
                                       pPassInfo->pwszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    CreateNetlogonBinding(&hNetr, hostname);

    bRet &= CallOpenSchannel(hNetr,
                             pwszMachAcct,
                             hostname,
                             pwszServer,
                             pwszDomain,
                             pwszComputer,
                             pwszMachpass,
                             &Creds,
                             &hSchn);

    if (hSchn == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    bRet &= CallNetrSamLogonInteractive(hSchn,
                                        &Creds,
                                        pwszServer,
                                        pwszDomain,
                                        pwszComputer,
                                        pwszUsername,
                                        pwszPassword,
                                        pdwLevels,
                                        dwNumLevels,
                                        &ppSamLogonInfo);

    bRet &= TestValidateSamLogonInfo(&ppSamLogonInfo, 1);

    bRet &= CallCloseSchannel(hSchn);

done:
error:
    for (iLevel = 0; ppSamLogonInfo && iLevel <= 6; iLevel++)
    {
        if (ppSamLogonInfo[iLevel])
        {
            NetrFreeMemory(ppSamLogonInfo[iLevel]);
        }
    }

    LwpsFreePasswordInfo(hStore, pPassInfo);
    LwpsClosePasswordStore(hStore);

    NetrFreeBinding(&hNetr);

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pszComputer);
    LW_SAFE_FREE_MEMORY(pszMachpass);

    return (int)bRet;
}


int TestNetlogonSamLogon(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcount)
{
    PCSTR pszDefServer = "TEST";
    PCSTR pszDefDomain = NULL;
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    PCSTR pszDefUsername = "user";
    PCSTR pszDefPassword = "pass";
    const DWORD dwDefLogonLevel = 2;
    const DWORD dwDefValidationLevel = 2;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    NETR_BINDING hSchn = NULL;
    enum param_err perr = perr_success;
    PSTR pszComputer = NULL;
    PSTR pszMachpass = NULL;
    PWSTR pwszComputer = NULL;
    PWSTR pwszMachAcct = NULL;
    PWSTR pwszMachpass = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    PWSTR pwszSite = NULL;
    NetrCredentials Creds = {0};
    NetrValidationInfo **ppSamLogonInfo = NULL;
    NetrValidationInfo *pValidationInfo = NULL;
    NetrValidationInfo **ppValidationInfo = NULL;
    BYTE Authoritative = 0;
    HANDLE hStore = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pPassInfo = NULL;
    NetrDomainQuery Query = {0};
    NetrDomainQuery1 Query1;
    NetrDomainInfo *pInfo = NULL;

    memset(&Query1, 0, sizeof(Query1));

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &pwszServer,
                       &pszDefServer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &pwszDomain,
                       &pszDefDomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &pwszUsername,
                       &pszDefUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &pwszPassword,
                       &pszDefPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
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

        dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwpsGetPasswordByCurrentHostName(hStore, &pPassInfo);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachAcct,
                                       pPassInfo->pwszMachineAccount);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachpass,
                                       pPassInfo->pwszMachinePassword);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszComputer,
                                       pPassInfo->pwszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    ntStatus = NetrOpenSchannel(hNetr, pwszMachAcct, hostname, pwszServer,
                                pwszDomain, pwszComputer, pwszMachpass,
                                &Creds, &hSchn);
    BAIL_ON_NT_STATUS(ntStatus);

    memset(&Query1, 0, sizeof(Query1));

    dwError = LwMbsToWc16s("Default-First-Site-Name", &pwszSite);
    BAIL_ON_WIN_ERROR(dwError);

    Query1.workstation_domain = pwszDomain;
    Query1.workstation_site   = pwszSite;
    Query.query1 = &Query1;

    ntStatus = NetrGetDomainInfo(hSchn, &Creds, pwszServer, pwszComputer,
                                 1, &Query, &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pInfo)
    {
        NetrFreeMemory(pInfo);
        pInfo = NULL;
    }

    ntStatus = NetrGetDomainInfo(hSchn, &Creds, pwszServer, pwszComputer,
                                 1, &Query, &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(sizeof(ppSamLogonInfo[0]) * 7,
                               OUT_PPVOID(&ppSamLogonInfo));
    BAIL_ON_WIN_ERROR(dwError);

    for (dwLogonLevel = 2; dwLogonLevel <= 6; dwLogonLevel++)
    {
        CALL_MSRPC(ntStatus, NetrSamLogonInteractive(hSchn, &Creds, pwszServer,
                                                     pwszDomain, pwszComputer,
                                                     pwszUsername, pwszPassword,
                                                     dwLogonLevel,
                                                     dwValidationLevel,
                                                     &pValidationInfo,
                                                     &Authoritative));
        BAIL_ON_NT_STATUS(ntStatus);

        ppValidationInfo[dwLogonLevel] = pValidationInfo;
    }

done:
error:
    if (hSchn)
    {
        TestCloseSchannel(hSchn);
    }

    NetrFreeBinding(&hNetr);

    LwpsFreePasswordInfo(hStore, pPassInfo);
    LwpsClosePasswordStore(hStore);

    if (pInfo)
    {
        NetrFreeMemory(pInfo);
    }

    if (pValidationInfo)
    {
        NetrFreeMemory(pValidationInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pszComputer);
    LW_SAFE_FREE_MEMORY(pszMachpass);

    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


int TestNetlogonSamLogoff(struct test *t, const wchar16_t *hostname,
                          const wchar16_t *user, const wchar16_t *pass,
                          struct parameter *options, int optcount)
{
    PCSTR pszDefServer = "TEST";
    PCSTR pszDefDomain = "TESTNET";
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    PCSTR pszDefUsername = "user";
    PCSTR pszDefPassword = "pass";
    const DWORD dwDefLogonLevel = 2;
    const DWORD dwDefValidationLevel = 2;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    NETR_BINDING hSchn = NULL;
    enum param_err perr = perr_success;
    PSTR pszComputer = NULL;
    PSTR pszMachpass = NULL;
    PWSTR pwszComputer = NULL;
    PWSTR pwszMachAcct = NULL;
    PWSTR pwszMachpass = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    NetrCredentials Creds = {0};
    HANDLE hStore = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pPassInfo = NULL;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &pwszServer,
                       &pszDefServer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &pwszDomain,
                       &pszDefDomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &pwszUsername,
                       &pszDefUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &pwszPassword,
                       &pszDefPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
                       &dwValidationLevel, &dwDefValidationLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = LwWc16sToMbs(pwszComputer, &pszComputer);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszMachpass, &pszMachpass);
    BAIL_ON_WIN_ERROR(dwError);

    if (strcmp(pszComputer, pszDefComputer) == 0 &&
        strcmp(pszMachpass, pszDefMachpass) == 0) {

        LW_SAFE_FREE_MEMORY(pwszComputer);
        LW_SAFE_FREE_MEMORY(pwszMachpass);

        dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwpsGetPasswordByCurrentHostName(hStore, &pPassInfo);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachAcct,
                                       pPassInfo->pwszMachineAccount);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachpass,
                                       pPassInfo->pwszMachinePassword);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszComputer,
                                       pPassInfo->pwszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    ntStatus = NetrOpenSchannel(hNetr, pwszMachAcct, hostname, pwszServer,
                                pwszDomain, pwszComputer, pwszMachpass,
                                &Creds, &hSchn);
    BAIL_ON_NT_STATUS(ntStatus);

    CALL_MSRPC(ntStatus, NetrSamLogoff(hSchn, &Creds, pwszServer, pwszDomain,
                                       pwszComputer, pwszUsername, pwszPassword,
                                       dwLogonLevel));
    BAIL_ON_NT_STATUS(ntStatus);

done:
error:
    if (hSchn)
    {
        TestCloseSchannel(hSchn);
    }

    NetrFreeBinding(&hNetr);

    LwpsFreePasswordInfo(hStore, pPassInfo);
    LwpsClosePasswordStore(hStore);

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);

    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}



int TestNetlogonSamLogonEx(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    PCSTR pszDefServer = "TEST";
    PCSTR pszDefDomain = NULL;
    PCSTR pszDefComputer = "TestWks4";
    PCSTR pszDefMachpass = "secret01$";
    PCSTR pszDefUsername = "user";
    PCSTR pszDefPassword = "pass";
    const DWORD dwDefLogonLevel = 2;
    const DWORD dwDefValidationLevel = 2;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    NETR_BINDING hNetr = NULL;
    NETR_BINDING hSchn = NULL;
    enum param_err perr = perr_success;
    PSTR pszComputer = NULL;
    PSTR pszMachpass = NULL;
    PWSTR pwszComputer = NULL;
    PWSTR pwszMachAcct = NULL;
    PWSTR pwszMachpass = NULL;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwLogonLevel = 0;
    DWORD dwValidationLevel = 0;
    NetrCredentials Creds = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;
    HANDLE hStore = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pPassInfo = NULL;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &pwszComputer,
                       &pszDefComputer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &pwszMachpass,
                       &pszDefMachpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &pwszServer,
                       &pszDefServer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &pwszDomain,
                       &pszDefDomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &pwszUsername,
                       &pszDefUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &pwszPassword,
                       &pszDefPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &dwLogonLevel,
                       &dwDefLogonLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
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

        dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwpsGetPasswordByCurrentHostName(hStore, &pPassInfo);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachAcct,
                                       pPassInfo->pwszMachineAccount);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszMachpass,
                                       pPassInfo->pwszMachinePassword);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwAllocateWc16String(&pwszComputer,
                                       pPassInfo->pwszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    PARAM_INFO("computer", pt_w16string, pwszComputer);
    PARAM_INFO("machpass", pt_w16string, pwszMachpass);
    PARAM_INFO("server", pt_w16string, pwszServer);
    PARAM_INFO("domain", pt_w16string, pwszDomain);
    PARAM_INFO("username", pt_w16string, pwszUsername);
    PARAM_INFO("password", pt_w16string, pwszPassword);
    PARAM_INFO("logon_level", pt_int32, &dwLogonLevel);
    PARAM_INFO("validation_level", pt_int32, &dwValidationLevel);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    ntStatus = NetrOpenSchannel(hNetr, pwszMachAcct, hostname, pwszServer,
                                pwszDomain, pwszComputer, pwszMachpass,
                                &Creds, &hSchn);
    BAIL_ON_NT_STATUS(ntStatus);

    CALL_MSRPC(ntStatus, NetrSamLogonEx(hSchn, &Creds, pwszServer,
                                        pwszDomain, pwszComputer,
                                        pwszUsername,
                                        pwszPassword,
                                        dwLogonLevel,
                                        dwValidationLevel,
                                        &pValidationInfo,
                                        &Authoritative));
    BAIL_ON_NT_STATUS(ntStatus);

done:
error:
    if (hSchn)
    {
        TestCloseSchannel(hSchn);
    }

    NetrFreeBinding(&hNetr);

    LwpsFreePasswordInfo(hStore, pPassInfo);
    LwpsClosePasswordStore(hStore);

    if (pValidationInfo)
    {
        NetrFreeMemory(pValidationInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszMachAcct);
    LW_SAFE_FREE_MEMORY(pwszComputer);
    LW_SAFE_FREE_MEMORY(pwszServer);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pszComputer);
    LW_SAFE_FREE_MEMORY(pszMachpass);

    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}



int TestNetlogonEnumTrustedDomains(struct test *t, const wchar16_t *hostname,
                                   const wchar16_t *user, const wchar16_t *pass,
                                   struct parameter *options, int optcount)
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

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "server", pt_w16string, &pwszServer,
                       &pszDefServer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("server", pt_w16string, pwszServer);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    CALL_MSRPC(ntStatus, NetrEnumerateTrustedDomainsEx(hNetr, pwszServer,
                                                     &pTrusts, &dwCount));
    if (ntStatus != STATUS_SUCCESS) goto done;

    bRet  &= TestValidateDomainTrusts(pTrusts, dwCount);

    for (iTrust = 0; iTrust < dwCount; iTrust++)
    {
        OUTPUT_ARG_WSTR(pTrusts[iTrust].netbios_name);
        OUTPUT_ARG_WSTR(pTrusts[iTrust].dns_name);
        OUTPUT_ARG_UINT(pTrusts[iTrust].trust_flags);
    }

done:
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


int TestNetlogonEnumDomainTrusts(struct test *t, const wchar16_t *hostname,
                                 const wchar16_t *user, const wchar16_t *pass,
                                 struct parameter *options, int optcount)
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

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "trustflags", pt_uint32, &dwTrustFlags,
                       &dwDefTrustFlags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("trustflags", pt_uint32, &dwTrustFlags);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    CALL_NETAPI(err, DsrEnumerateDomainTrusts(hNetr, hostname, dwTrustFlags,
                                              &pTrusts, &dwCount));
    BAIL_ON_WIN_ERROR(err);

    bRet  &= TestValidateDomainTrusts(pTrusts, dwCount);

    for (iTrust = 0; iTrust < dwCount; iTrust++)
    {
        OUTPUT_ARG_WSTR(pTrusts[iTrust].netbios_name);
        OUTPUT_ARG_WSTR(pTrusts[iTrust].dns_name);
        OUTPUT_ARG_UINT(pTrusts[iTrust].trust_flags);
    }

done:
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


int TestNetlogonGetDcName(struct test *t, const wchar16_t *hostname,
                          const wchar16_t *user, const wchar16_t *pass,
                          struct parameter *options, int optcount)
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

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "getdcflags", pt_uint32, &dwGetDcFlags,
                       &dwDefGetDcFlags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domainname", pt_w16string, &pwszDomainName,
                       &pszDefDomainName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("getdcflags", pt_uint32, &dwGetDcFlags);
    PARAM_INFO("domainname", pt_w16string, &pwszDomainName);

    hNetr = CreateNetlogonBinding(&hNetr, hostname);
    if (hNetr == NULL)
    {
        bRet = FALSE;
        goto done;
    }

    CALL_NETAPI(err, DsrGetDcName(hNetr, hostname, pwszDomainName,
                                  NULL, NULL, dwGetDcFlags, &pInfo));
    BAIL_ON_WIN_ERROR(err);

done:
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


void SetupNetlogonTests(struct test *t)
{
    AddTest(t, "NETR-ENUM-TRUSTED-DOM" , TestNetlogonEnumTrustedDomains);
    AddTest(t, "NETR-DSR-ENUM-DOMTRUSTS", TestNetlogonEnumDomainTrusts);
    AddTest(t, "NETR-SAM-LOGON", TestNetlogonSamLogon);
    AddTest(t, "NETR-SAM-LOGON-INTERACTIVE", TestNetlogonSamLogonInteractive);
    AddTest(t, "NETR-SAM-LOGOFF", TestNetlogonSamLogoff);
    AddTest(t, "NETR-SAM-LOGON-EX", TestNetlogonSamLogonEx);
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
