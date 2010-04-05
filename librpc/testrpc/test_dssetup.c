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


handle_t
CreateDsrBinding(
    handle_t *binding,
    const wchar16_t *host
    )
{
    RPCSTATUS status = RPC_S_OK;
    size_t hostname_size = 0;
    char *hostname = NULL;
    PIO_CREDS creds = NULL;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;
    wc16stombs(hostname, host, hostname_size);

    if (LwIoGetActiveCreds(NULL, &creds) != STATUS_SUCCESS)
    {
        return NULL;
    }

    status = InitDsrBindingDefault(binding, hostname, creds);
    if (status != RPC_S_OK) {
        int result;
        unsigned char errmsg[dce_c_error_string_len];

        dce_error_inq_text(status, errmsg, &result);
        if (result == 0) {
            printf("Error: %s\n", errmsg);
        } else {
            printf("Unknown error: %08lx\n", (unsigned long int)status);
        }

        return NULL;
    }

    SAFE_FREE(hostname);
    return *binding;
}


int
TestDsrRoleGetPrimaryDomainInformation(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    const DWORD dwDefLevel = (WORD)(-1);

    BOOLEAN bRet = TRUE;
    WINERR err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    handle_t hBinding = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {DS_ROLE_BASIC_INFORMATION,
                                DS_ROLE_UPGRADE_STATUS,
                                DS_ROLE_OP_STATUS};
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD i = 0;
    PDS_ROLE_INFO pInfo = NULL;

    perr = fetch_value(options, optcount, "level", pt_uint32, &dwLevel,
                       &dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("level", pt_uint32, &dwLevel);

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    if (dwLevel == (WORD)(-1))
    {
        pdwLevels   = dwAvailableLevels;
        dwNumLevels = sizeof(dwAvailableLevels)/sizeof(dwAvailableLevels[0]);
    }
    else
    {
        dwSelectedLevels[0] = dwLevel;
        pdwLevels   = dwSelectedLevels;
        dwNumLevels = sizeof(dwSelectedLevels)/sizeof(dwSelectedLevels[0]);
    }

    CreateDsrBinding(&hBinding, hostname);
    if (!hBinding)
    {
        bRet = FALSE;
        goto done;
    }

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        INPUT_ARG_UINT(dwLevel);

        CALL_NETAPI(err, DsrRoleGetPrimaryDomainInformation(
                                    hBinding,
                                    dwLevel,
                                    &pInfo));
        BAIL_ON_WIN_ERROR(err);

        switch (dwLevel)
        {
        case DS_ROLE_BASIC_INFORMATION:
            ASSERT_TEST((pInfo->basic.dwRole >= DS_ROLE_STANDALONE_WORKSTATION &&
                         pInfo->basic.dwRole <= DS_ROLE_PRIMARY_DC));
            ASSERT_TEST(pInfo->basic.pwszDomain != NULL);
            ASSERT_TEST(pInfo->basic.pwszDnsDomain != NULL);
            ASSERT_TEST(pInfo->basic.pwszForest != NULL);
            break;

        case DS_ROLE_UPGRADE_STATUS:
            ASSERT_TEST((pInfo->upgrade.swUpgradeStatus == DS_ROLE_NOT_UPGRADING ||
                         pInfo->upgrade.swUpgradeStatus == DS_ROLE_UPGRADING));
            ASSERT_TEST((pInfo->upgrade.dwPrevious >= DS_ROLE_PREVIOUS_UNKNOWN &&
                         pInfo->upgrade.dwPrevious <= DS_ROLE_PREVIOUS_BACKUP));
            break;

        case DS_ROLE_OP_STATUS:
            ASSERT_TEST(pInfo->opstatus.swStatus <= DS_ROLE_NEEDS_REBOOT);
            break;
        }

        if (pInfo)
        {
            DsrFreeMemory(pInfo);
            pInfo = NULL;
        }
    }

done:
error:
    FreeDsrBinding(&hBinding);

    RELEASE_SESSION_CREDS;

    if (pInfo)
    {
        DsrFreeMemory(pInfo);
    }

    if (err != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


void
SetupDsrTests(
    struct test *t
    )
{
    AddTest(t, "DSR-ROLE-GET-PDC-INFO", TestDsrRoleGetPrimaryDomainInformation);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/


