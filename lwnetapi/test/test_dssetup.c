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

#include "includes.h"


static
DWORD
TestDsrRoleGetPrimaryDomainInformation(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefLevel = (WORD)(-1);

    BOOLEAN bRet = TRUE;
    WINERROR err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    DSR_BINDING hDsr = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = { DS_ROLE_BASIC_INFORMATION,
                                  DS_ROLE_UPGRADE_STATUS,
                                  DS_ROLE_OP_STATUS };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD i = 0;
    PDSR_ROLE_INFO pInfo = NULL;

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32, &dwLevel,
                       &dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("level", pt_uint32, &dwLevel);

    TESTINFO(pTest, pwszHostname);

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

    bRet &= CreateRpcBinding(OUT_PPVOID(&hDsr),
                             RPC_DSSETUP_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        INPUT_ARG_UINT(dwLevel);

        CALL_NETAPI(err, DsrRoleGetPrimaryDomainInformation(
                                    hDsr,
                                    dwLevel,
                                    &pInfo));
        BAIL_ON_WIN_ERROR(err);

        switch (dwLevel)
        {
        case DS_ROLE_BASIC_INFORMATION:
            ASSERT_TEST((pInfo->Basic.dwRole >= DS_ROLE_STANDALONE_WORKSTATION &&
                         pInfo->Basic.dwRole <= DS_ROLE_PRIMARY_DC));
            ASSERT_TEST(pInfo->Basic.pwszDomain != NULL);
            ASSERT_TEST(pInfo->Basic.pwszDnsDomain != NULL);
            ASSERT_TEST(pInfo->Basic.pwszForest != NULL);
            break;

        case DS_ROLE_UPGRADE_STATUS:
            ASSERT_TEST((pInfo->Upgrade.swUpgradeStatus == DS_ROLE_NOT_UPGRADING ||
                         pInfo->Upgrade.swUpgradeStatus == DS_ROLE_UPGRADING));
            ASSERT_TEST((pInfo->Upgrade.dwPrevious >= DS_ROLE_PREVIOUS_UNKNOWN &&
                         pInfo->Upgrade.dwPrevious <= DS_ROLE_PREVIOUS_BACKUP));
            break;

        case DS_ROLE_OP_STATUS:
            ASSERT_TEST(pInfo->OpStatus.swStatus <= DS_ROLE_NEEDS_REBOOT);
            break;
        }

        if (pInfo)
        {
            DsrFreeMemory(pInfo);
            pInfo = NULL;
        }
    }

    DsrFreeBinding(&hDsr);

error:
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


VOID
SetupDsrTests(PTEST t)
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


