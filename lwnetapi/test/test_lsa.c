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


typedef struct _TEST_LOOKUP
{
    PSTR    pszDomainName;
    PSTR    pszName;
    PSTR    pszSid;
    DWORD   dwRid;
    DWORD   dwType;
} TEST_LOOKUP, *PTEST_LOOKUP;


static
DWORD
TestFormatNT4Name(
    PWSTR  *ppwszNT4Name,
    PWSTR   pwszDomain,
    PWSTR   pwszName
    );

static
DWORD
TestGetLookupTestSet(
    PTEST_LOOKUP  *ppTestSet,
    PDWORD         pdwNumNames,
    PCSTR          pszTestSetName
    );

static
DWORD
TestLsaLookupNames(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaLookupNames2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaLookupNames3(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaLookupSids(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaQueryInfoPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaQueryInfoPolicy2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaInfoPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestLsaOpenPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
BOOLEAN
CallLsaOpenPolicy(
    LSA_BINDING     hBinding,
    PWSTR           pwszSysName,
    DWORD           dwAccessRights,
    POLICY_HANDLE  *phPolicy,
    PWSTR          *ppwszDomainName,
    PWSTR          *ppwszDnsDomainName,
    PSID           *ppDomainSid
    );


static
BOOLEAN
CallLsaLookupNames(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    );


static
BOOLEAN
CallLsaLookupNames2(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    );


static
BOOLEAN
CallLsaLookupNames3(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    );


static
BOOLEAN
CallLsaLookupSids(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumSids,
    PSID          *ppSids,
    DWORD          dwLevel,
    PWSTR        **pppwszNames,
    PDWORD        *ppdwTypes
    );


static
BOOLEAN
CallLsaClosePolicy(
    LSA_BINDING    hBinding,
    POLICY_HANDLE *phPolicy
    );


static
TEST_LOOKUP StandaloneNames[] = {
    {
        .pszDomainName  = NULL,
        .pszName        = "Administrator",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_ADMIN,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Guest",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_GUEST,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "_NonExistent",
        .pszSid         = NULL,
        .dwRid          = 0,
        .dwType         = SID_TYPE_UNKNOWN,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "Administrators",
        .pszSid         = "S-1-5-32-544",
        .dwRid          = DOMAIN_ALIAS_RID_ADMINS,
        .dwType         = SID_TYPE_ALIAS,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "Users",
        .pszSid         = "S-1-5-32-545",
        .dwRid          = DOMAIN_ALIAS_RID_USERS,
        .dwType         = SID_TYPE_ALIAS,
    },
};


static
TEST_LOOKUP DomainNames[] = {
    {
        .pszDomainName  = NULL,
        .pszName        = "_NonExistent",
        .pszSid         = NULL,
        .dwRid          = 0,
        .dwType         = SID_TYPE_UNKNOWN,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Administrator",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_ADMIN,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Guest",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_GUEST,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Domain Admins",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_GROUP_RID_ADMINS,
        .dwType         = SID_TYPE_DOM_GRP,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "Users",
        .pszSid         = "S-1-5-32-545",
        .dwRid          = DOMAIN_ALIAS_RID_USERS,
        .dwType         = SID_TYPE_ALIAS,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Domain Users",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_GROUP_RID_USERS,
        .dwType         = SID_TYPE_DOM_GRP,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "Guests",
        .pszSid         = "S-1-5-32-546",
        .dwRid          = DOMAIN_ALIAS_RID_GUESTS,
        .dwType         = SID_TYPE_ALIAS,
    },
};


static
TEST_LOOKUP DuplicateNames[] = {
    {
        .pszDomainName  = NULL,
        .pszName        = "Administrator",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_ADMIN,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Administrator",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_ADMIN,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Domain Users",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_GROUP_RID_ADMINS,
        .dwType         = SID_TYPE_DOM_GRP,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Administrator",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_USER_RID_ADMIN,
        .dwType         = SID_TYPE_USER,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Domain Users",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_GROUP_RID_ADMINS,
        .dwType         = SID_TYPE_DOM_GRP,
    },
    {
        .pszDomainName  = NULL,
        .pszName        = "Domain Users",
        .pszSid         = NULL,
        .dwRid          = DOMAIN_GROUP_RID_ADMINS,
        .dwType         = SID_TYPE_DOM_GRP,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "_NotExisting",
        .pszSid         = NULL,
        .dwRid          = 0,
        .dwType         = SID_TYPE_UNKNOWN,
    },
    {
        .pszDomainName  = "BUILTIN",
        .pszName        = "_NotExisting",
        .pszSid         = NULL,
        .dwRid          = 0,
        .dwType         = SID_TYPE_UNKNOWN,
    },
};


static
DWORD
TestLsaOpenPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwAccessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsa = NULL;
    POLICY_HANDLE hPolicy = NULL;

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    INPUT_ARG_PTR(hLsa);
    INPUT_ARG_WSTR(pwszHostname);
    INPUT_ARG_UINT(dwAccessRights);

    CALL_MSRPC(ntStatus, LsaOpenPolicy2(hLsa, pwszHostname, NULL,
                                        dwAccessRights, &hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    OUTPUT_ARG_PTR(hLsa);
    OUTPUT_ARG_PTR(hPolicy);

    ntStatus = LsaClose(hLsa, hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    LsaFreeBinding(&hLsa);

error:
    return bRet;
}


static
DWORD
TestFormatNT4Name(
    PWSTR  *ppwszNT4Name,
    PWSTR   pwszDomain,
    PWSTR   pwszName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sDomainLen = 0;
    size_t sNameLen = 0;
    DWORD dwNT4NameSize = 0;
    PWSTR pwszNT4Name = NULL;

    dwError = LwWc16sLen(pwszDomain, &sDomainLen);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sLen(pwszName, &sNameLen);
    BAIL_ON_WIN_ERROR(dwError);

    dwNT4NameSize = sizeof(WCHAR) * (sDomainLen + 1 + sNameLen);

    dwError = LwAllocateMemory(dwNT4NameSize + sizeof(WCHAR),
                               OUT_PPVOID(&pwszNT4Name));
    BAIL_ON_WIN_ERROR(dwError);

    if (sDomainLen > 0 && sNameLen > 0)
    {
        if (sw16printfw(pwszNT4Name, dwNT4NameSize, L"%ws\\%ws",
                        pwszDomain, pwszName) < 0)
        {
            dwError = LwErrnoToWin32Error(errno);
        }
    }
    else if (sDomainLen == 0 && sNameLen > 0)
    {
        if (sw16printfw(pwszNT4Name, dwNT4NameSize, L"%ws",
                        pwszName) < 0)
        {
            dwError = LwErrnoToWin32Error(errno);
        }
    }
    else if (sDomainLen > 0 && sNameLen == 0)
    {
        if (sw16printfw(pwszNT4Name, dwNT4NameSize, L"%ws",
                        pwszDomain) < 0)
        {
            dwError = LwErrnoToWin32Error(errno);
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    *ppwszNT4Name = pwszNT4Name;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszNT4Name);

    *ppwszNT4Name = NULL;

    goto cleanup;
}


static
DWORD
TestGetLookupTestSet(
    PTEST_LOOKUP   *ppTestSet,
    PDWORD          pdwNumNames,
    PCSTR           pszTestSetName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST_LOOKUP pTestSet = NULL;
    DWORD dwNumNames = 0;

    if (strcmp(pszTestSetName, "standalone") == 0 ||
        strcmp(pszTestSetName, "STANDALONE") == 0)
    {
        pTestSet = StandaloneNames;
        dwNumNames = (sizeof(StandaloneNames)
                      /sizeof(StandaloneNames[0]));
    }
    else if (strcmp(pszTestSetName, "domain") == 0 ||
             strcmp(pszTestSetName, "DOMAIN") == 0)
    {
        pTestSet = DomainNames;
        dwNumNames = (sizeof(DomainNames)
                      /sizeof(DomainNames[0]));
    }
    else if (strcmp(pszTestSetName, "duplicate") == 0 ||
             strcmp(pszTestSetName, "DUPLICATE") == 0)
    {
        pTestSet = DuplicateNames;
        dwNumNames = (sizeof(DuplicateNames)
                      /sizeof(DuplicateNames[0]));
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN_ERROR(dwError);

    *ppTestSet   = pTestSet;
    *pdwNumNames = dwNumNames;

error:
    return dwError;
}


static
DWORD
TestLsaLookupNames(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "\\\\";
    const DWORD dwDefLevel = -1;
    const BOOLEAN bDefRevLookup = TRUE;
    PCSTR pszDefTestSetName = "standalone";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    LSA_BINDING hLsa = NULL;
    PWSTR pwszSysName = NULL;
    BOOLEAN bRevLookup = TRUE;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {LSA_LOOKUP_NAMES_ALL,
                                 LSA_LOOKUP_NAMES_DOMAINS_ONLY,
                                 LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY,
                                 LSA_LOOKUP_NAMES_FOREST_TRUSTS,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2};
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD iLevel = 0;
    DWORD dwNumNames = 0;
    DWORD iName = 0;
    PWSTR *ppwszNames = NULL;
    PWSTR pwszNT4Name = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    PWSTR *ppwszRetNames = NULL;
    PTEST_LOOKUP pTestNames = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       (UINT32*)&pszTestSetName, (UINT32*)&pszDefTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "revlookup", pt_uint32,
                       (UINT32*)&bRevLookup, (UINT32*)&bDefRevLookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = TestGetLookupTestSet(&pTestNames, &dwNumNames, pszTestSetName);
    BAIL_ON_WIN_ERROR(dwError);

    TESTINFO(pTest, pwszHostname);

    PARAM_INFO("systemname", pt_w16string, &pwszSysName);
    PARAM_INFO("level", pt_uint32, &dwLevel);

    if (dwLevel == (DWORD)(-1))
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

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, NULL, 0, &hPolicy,
                              &pwszDomainName, NULL, NULL);

    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        dwLevel = pdwLevels[iLevel];

        dwError = LwAllocateMemory(sizeof(PWSTR) * dwNumNames,
                                   OUT_PPVOID(&ppwszNames));
        BAIL_ON_WIN_ERROR(dwError);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PWSTR pwszDomain = NULL;
            PWSTR pwszName = NULL;

            if (pTestNames[iName].pszDomainName)
            {
                dwError = LwMbsToWc16s(pTestNames[iName].pszDomainName,
                                       &pwszDomain);
            }
            else
            {
                dwError = LwAllocateWc16String(&pwszDomain,
                                               pwszDomainName);
            }
            BAIL_ON_WIN_ERROR(dwError);

            dwError = LwMbsToWc16s(pTestNames[iName].pszName,
                                   &pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = TestFormatNT4Name(&pwszNT4Name, pwszDomain, pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            ppwszNames[iName] = pwszNT4Name;

            LW_SAFE_FREE_MEMORY(pwszDomain);
            LW_SAFE_FREE_MEMORY(pwszName);

            pwszDomain = NULL;
            pwszName   = NULL;
        }

        bRet &= CallLsaLookupNames(hLsa, hPolicy, dwNumNames, ppwszNames, dwLevel,
                                   &ppSids, &pdwTypes);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PSID pSid = NULL;
            DWORD dwReturnedRid = 0;
            DWORD dwReturnedType = pdwTypes[iName];

            if (dwReturnedType == SID_TYPE_USE_NONE ||
                dwReturnedType == SID_TYPE_INVALID ||
                dwReturnedType == SID_TYPE_UNKNOWN)
            {
                continue;
            }

            if (pTestNames[iName].pszSid)
            {
                ntStatus = RtlAllocateSidFromCString(
                                        &pSid,
                                        pTestNames[iName].pszSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ASSERT_TEST_MSG((ppSids[iName] != NULL), ("(i = %u)\n", iName));

                ASSERT_SID_EQUAL_MSG(pSid, ppSids[iName], ("(i = %u)\n", iName));

                if (ppSids[iName])
                {
                    dwReturnedRid =
                        ppSids[iName]->SubAuthority[ppSids[iName]->SubAuthorityCount - 1];

                    ASSERT_TEST_MSG(dwReturnedRid == pTestNames[iName].dwRid,
                                    ("(i = %u)\n", iName));
                }
            }

            ASSERT_TEST_MSG(dwReturnedType == pTestNames[iName].dwType,
                            ("(i = %u)\n", iName));

            RTL_FREE(&pSid);
        }

        LW_SAFE_FREE_MEMORY(pdwTypes);
        pdwTypes = NULL;

        if (bRevLookup)
        {
            bRet &= CallLsaLookupSids(hLsa, hPolicy, dwNumNames, ppSids, dwLevel,
                                      &ppwszRetNames, &pdwTypes);
        }

        for (iName = 0; iName < dwNumNames; iName++)
        {
            LW_SAFE_FREE_MEMORY(ppwszNames[iName]);
            RTL_FREE(&ppSids[iName]);
        }

        LW_SAFE_FREE_MEMORY(ppwszNames);
        LW_SAFE_FREE_MEMORY(ppSids);
        LW_SAFE_FREE_MEMORY(pdwTypes);

        ppwszNames = NULL;
        ppSids     = NULL;
        pdwTypes   = NULL;
    }

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

error:
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


DWORD
TestLsaLookupNames2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "\\\\";
    const DWORD dwDefLevel = -1;
    const BOOLEAN bDefRevLookup = TRUE;
    PCSTR pszDefTestSetName = "standalone";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    LSA_BINDING hLsa = NULL;
    PWSTR pwszSysName = NULL;
    BOOLEAN bRevLookup = TRUE;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {LSA_LOOKUP_NAMES_ALL,
                                 LSA_LOOKUP_NAMES_DOMAINS_ONLY,
                                 LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY,
                                 LSA_LOOKUP_NAMES_FOREST_TRUSTS,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2};
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD iLevel = 0;
    DWORD dwNumNames = 0;
    DWORD iName = 0;
    PWSTR *ppwszNames = NULL;
    PWSTR pwszNT4Name = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    PWSTR *ppwszRetNames = NULL;
    PTEST_LOOKUP pTestNames = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       (UINT32*)&pszTestSetName, (UINT32*)&pszDefTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "revlookup", pt_uint32,
                       (UINT32*)&bRevLookup, (UINT32*)&bDefRevLookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = TestGetLookupTestSet(&pTestNames, &dwNumNames, pszTestSetName);
    BAIL_ON_WIN_ERROR(dwError);

    TESTINFO(pTest, pwszHostname);

    PARAM_INFO("systemname", pt_w16string, &pwszSysName);
    PARAM_INFO("level", pt_uint32, &dwLevel);

    if (dwLevel == (DWORD)(-1))
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

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, NULL, 0, &hPolicy,
                              &pwszDomainName, NULL, NULL);

    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        dwLevel = pdwLevels[iLevel];

        dwError = LwAllocateMemory(sizeof(PWSTR) * dwNumNames,
                                   OUT_PPVOID(&ppwszNames));
        BAIL_ON_WIN_ERROR(dwError);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PWSTR pwszDomain = NULL;
            PWSTR pwszName = NULL;

            if (pTestNames[iName].pszDomainName)
            {
                dwError = LwMbsToWc16s(pTestNames[iName].pszDomainName,
                                       &pwszDomain);
            }
            else
            {
                dwError = LwAllocateWc16String(&pwszDomain,
                                               pwszDomainName);
            }
            BAIL_ON_WIN_ERROR(dwError);

            dwError = LwMbsToWc16s(pTestNames[iName].pszName,
                                   &pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = TestFormatNT4Name(&pwszNT4Name, pwszDomain, pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            ppwszNames[iName] = pwszNT4Name;

            LW_SAFE_FREE_MEMORY(pwszDomain);
            LW_SAFE_FREE_MEMORY(pwszName);

            pwszDomain = NULL;
            pwszName   = NULL;
        }

        bRet &= CallLsaLookupNames2(hLsa, hPolicy, dwNumNames, ppwszNames, dwLevel,
                                   &ppSids, &pdwTypes);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PSID pSid = NULL;
            DWORD dwReturnedRid = 0;
            DWORD dwReturnedType = pdwTypes[iName];

            if (dwReturnedType == SID_TYPE_USE_NONE ||
                dwReturnedType == SID_TYPE_INVALID ||
                dwReturnedType == SID_TYPE_UNKNOWN)
            {
                continue;
            }

            if (pTestNames[iName].pszSid)
            {
                ntStatus = RtlAllocateSidFromCString(
                                        &pSid,
                                        pTestNames[iName].pszSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ASSERT_TEST_MSG((ppSids[iName] != NULL), ("(i = %u)\n", iName));

                ASSERT_SID_EQUAL_MSG(pSid, ppSids[iName], ("(i = %u)\n", iName));

                if (ppSids[iName])
                {
                    dwReturnedRid =
                        ppSids[iName]->SubAuthority[ppSids[iName]->SubAuthorityCount - 1];

                    ASSERT_TEST_MSG(dwReturnedRid == pTestNames[iName].dwRid,
                                    ("(i = %u)\n", iName));
                }
            }

            ASSERT_TEST_MSG(dwReturnedType == pTestNames[iName].dwType,
                            ("(i = %u)\n", iName));

            RTL_FREE(&pSid);
        }

        LW_SAFE_FREE_MEMORY(pdwTypes);
        pdwTypes = NULL;

        if (bRevLookup)
        {
            bRet &= CallLsaLookupSids(hLsa, hPolicy, dwNumNames, ppSids, dwLevel,
                                      &ppwszRetNames, &pdwTypes);
        }

        for (iName = 0; iName < dwNumNames; iName++)
        {
            LW_SAFE_FREE_MEMORY(ppwszNames[iName]);
            RTL_FREE(&ppSids[iName]);
        }

        LW_SAFE_FREE_MEMORY(ppwszNames);
        LW_SAFE_FREE_MEMORY(ppSids);
        LW_SAFE_FREE_MEMORY(pdwTypes);

        ppwszNames = NULL;
        ppSids     = NULL;
        pdwTypes   = NULL;
    }

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

error:
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


DWORD
TestLsaLookupNames3(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "\\\\";
    const DWORD dwDefLevel = -1;
    const BOOLEAN bDefRevLookup = TRUE;
    PCSTR pszDefTestSetName = "standalone";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    LSA_BINDING hLsa = NULL;
    PWSTR pwszSysName = NULL;
    BOOLEAN bRevLookup = TRUE;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {LSA_LOOKUP_NAMES_ALL,
                                 LSA_LOOKUP_NAMES_DOMAINS_ONLY,
                                 LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY,
                                 LSA_LOOKUP_NAMES_FOREST_TRUSTS,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2};
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD iLevel = 0;
    DWORD dwNumNames = 0;
    DWORD iName = 0;
    PWSTR *ppwszNames = NULL;
    PWSTR pwszNT4Name = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    PWSTR *ppwszRetNames = NULL;
    PTEST_LOOKUP pTestNames = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       (UINT32*)&pszTestSetName, (UINT32*)&pszDefTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "revlookup", pt_uint32,
                       (UINT32*)&bRevLookup, (UINT32*)&bDefRevLookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = TestGetLookupTestSet(&pTestNames, &dwNumNames, pszTestSetName);
    BAIL_ON_WIN_ERROR(dwError);

    TESTINFO(pTest, pwszHostname);

    PARAM_INFO("systemname", pt_w16string, &pwszSysName);
    PARAM_INFO("level", pt_uint32, &dwLevel);

    if (dwLevel == (DWORD)(-1))
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

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, NULL, 0, &hPolicy,
                              &pwszDomainName, NULL, NULL);

    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        dwLevel = pdwLevels[iLevel];

        dwError = LwAllocateMemory(sizeof(PWSTR) * dwNumNames,
                                   OUT_PPVOID(&ppwszNames));
        BAIL_ON_WIN_ERROR(dwError);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PWSTR pwszDomain = NULL;
            PWSTR pwszName = NULL;

            if (pTestNames[iName].pszDomainName)
            {
                dwError = LwMbsToWc16s(pTestNames[iName].pszDomainName,
                                       &pwszDomain);
            }
            else
            {
                dwError = LwAllocateWc16String(&pwszDomain,
                                               pwszDomainName);
            }
            BAIL_ON_WIN_ERROR(dwError);

            dwError = LwMbsToWc16s(pTestNames[iName].pszName,
                                   &pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = TestFormatNT4Name(&pwszNT4Name, pwszDomain, pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            ppwszNames[iName] = pwszNT4Name;

            LW_SAFE_FREE_MEMORY(pwszDomain);
            LW_SAFE_FREE_MEMORY(pwszName);

            pwszDomain = NULL;
            pwszName   = NULL;
        }

        bRet &= CallLsaLookupNames3(hLsa, hPolicy, dwNumNames, ppwszNames, dwLevel,
                                   &ppSids, &pdwTypes);

        for (iName = 0; iName < dwNumNames; iName++)
        {
            PSID pSid = NULL;
            DWORD dwReturnedRid = 0;
            DWORD dwReturnedType = pdwTypes[iName];

            if (dwReturnedType == SID_TYPE_USE_NONE ||
                dwReturnedType == SID_TYPE_INVALID ||
                dwReturnedType == SID_TYPE_UNKNOWN)
            {
                continue;
            }

            if (pTestNames[iName].pszSid)
            {
                ntStatus = RtlAllocateSidFromCString(
                                        &pSid,
                                        pTestNames[iName].pszSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ASSERT_TEST_MSG((ppSids[iName] != NULL), ("(i = %u)\n", iName));

                ASSERT_SID_EQUAL_MSG(pSid, ppSids[iName], ("(i = %u)\n", iName));

                if (ppSids[iName])
                {
                    dwReturnedRid =
                        ppSids[iName]->SubAuthority[ppSids[iName]->SubAuthorityCount - 1];

                    ASSERT_TEST_MSG(dwReturnedRid == pTestNames[iName].dwRid,
                                    ("(i = %u)\n", iName));
                }
            }

            ASSERT_TEST_MSG(dwReturnedType == pTestNames[iName].dwType,
                            ("(i = %u)\n", iName));

            RTL_FREE(&pSid);
        }

        LW_SAFE_FREE_MEMORY(pdwTypes);
        pdwTypes = NULL;

        if (bRevLookup)
        {
            bRet &= CallLsaLookupSids(hLsa, hPolicy, dwNumNames, ppSids, dwLevel,
                                      &ppwszRetNames, &pdwTypes);
        }

        for (iName = 0; iName < dwNumNames; iName++)
        {
            LW_SAFE_FREE_MEMORY(ppwszNames[iName]);
            RTL_FREE(&ppSids[iName]);
        }

        LW_SAFE_FREE_MEMORY(ppwszNames);
        LW_SAFE_FREE_MEMORY(ppSids);
        LW_SAFE_FREE_MEMORY(pdwTypes);

        ppwszNames = NULL;
        ppSids     = NULL;
        pdwTypes   = NULL;
    }

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

error:
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestLsaLookupSids(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "\\\\";
    const DWORD dwDefLevel = -1;
    const BOOLEAN bDefRevLookup = TRUE;
    PCSTR pszDefTestSetName = "standalone";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    LSA_BINDING hLsa = NULL;
    PWSTR pwszSysName = NULL;
    BOOLEAN bRevLookup = TRUE;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszDomainName = NULL;
    PSID pDomainSid = NULL;
    DWORD dwSelectedLevels[] = {0};
    DWORD dwAvailableLevels[] = {LSA_LOOKUP_NAMES_ALL,
                                 LSA_LOOKUP_NAMES_DOMAINS_ONLY,
                                 LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY,
                                 LSA_LOOKUP_NAMES_FOREST_TRUSTS,
                                 LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2};
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD iLevel = 0;
    DWORD dwNumSids = 0;
    DWORD iSid = 0;
    PSID *ppSids = NULL;
    PWSTR *ppwszNames = NULL;
    PDWORD pdwTypes = NULL;
    PTEST_LOOKUP pTestSids = NULL;
    PWSTR pwszNT4Name = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       (UINT32*)&pszTestSetName, (UINT32*)&pszDefTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "revlookup", pt_uint32,
                       (UINT32*)&bRevLookup, (UINT32*)&bDefRevLookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    dwError = TestGetLookupTestSet(&pTestSids, &dwNumSids, pszTestSetName);
    BAIL_ON_WIN_ERROR(dwError);

    TESTINFO(pTest, pwszHostname);

    PARAM_INFO("systemname", pt_w16string, &pwszSysName);
    PARAM_INFO("level", pt_uint32, &dwLevel);

    if (dwLevel == (DWORD)(-1))
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

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, NULL, 0, &hPolicy,
                              &pwszDomainName, NULL, &pDomainSid);

    for (iLevel = 0; iLevel < dwNumLevels; iLevel++)
    {
        dwLevel = pdwLevels[iLevel];

        dwError = LwAllocateMemory(sizeof(PSID) * dwNumSids,
                                   OUT_PPVOID(&ppSids));
        BAIL_ON_WIN_ERROR(dwError);

        for (iSid = 0; iSid < dwNumSids; iSid++)
        {
            PSID pSid = NULL;
            DWORD dwSidSize = 0;

            if (pTestSids[iSid].pszSid)
            {
                ntStatus = RtlAllocateSidFromCString(&pSid,
                                                     pTestSids[iSid].pszSid);
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                dwSidSize = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

                dwError = LwAllocateMemory(dwSidSize, OUT_PPVOID(&pSid));
                BAIL_ON_WIN_ERROR(dwError);

                ntStatus = RtlCopySid(dwSidSize, pSid, pDomainSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = RtlAppendRidSid(dwSidSize, pSid, 
                                           pTestSids[iSid].dwRid);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ppSids[iSid] = pSid;
        }

        bRet &= CallLsaLookupSids(hLsa, hPolicy, dwNumSids, ppSids, dwLevel,
                                  &ppwszNames, &pdwTypes);

        for (iSid = 0; iSid < dwNumSids; iSid++)
        {
            PWSTR pwszDomain = NULL;
            PWSTR pwszName = ppwszNames[iSid];
            DWORD dwReturnedType = pdwTypes[iSid];

            if (dwReturnedType == SID_TYPE_USE_NONE ||
                dwReturnedType == SID_TYPE_INVALID ||
                dwReturnedType == SID_TYPE_UNKNOWN)
            {
                continue;
            }

            if (pTestSids[iSid].pszDomainName)
            {
                dwError = LwMbsToWc16s(pTestSids[iSid].pszDomainName,
                                       &pwszDomain);
                BAIL_ON_WIN_ERROR(dwError);
            }
            else
            {
                dwError = LwAllocateWc16String(&pwszDomain,
                                               pwszDomainName);
                BAIL_ON_WIN_ERROR(dwError);
            }

            dwError = LwMbsToWc16s(pTestSids[iSid].pszName, &pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = TestFormatNT4Name(&pwszNT4Name, pwszDomain, pwszName);
            BAIL_ON_WIN_ERROR(dwError);

            ASSERT_TEST_MSG(ppwszNames[iSid] != NULL, ("(i = %u)\n", iSid));

            if (ppwszNames[iSid])
            {
                ASSERT_TEST_MSG(RtlWC16StringIsEqual(ppwszNames[iSid],
                                                     pwszNT4Name,
                                                     FALSE),
                                ("(i = %u)\n", iSid));
            }

            ASSERT_TEST_MSG(dwReturnedType == pTestSids[iSid].dwType,
                            ("(i = %u)\n", iSid));

            LW_SAFE_FREE_MEMORY(pwszDomain);
            LW_SAFE_FREE_MEMORY(pwszName);
            LW_SAFE_FREE_MEMORY(pwszNT4Name);

            pwszDomain   = NULL;
            pwszName     = NULL;
            pwszNT4Name  = NULL;
        }

        for (iSid = 0; iSid < dwNumSids; iSid++)
        {
            LW_SAFE_FREE_MEMORY(ppwszNames[iSid]);
        }

        LW_SAFE_FREE_MEMORY(ppwszNames);
        LW_SAFE_FREE_MEMORY(pdwTypes);

        ppwszNames = NULL;
        pdwTypes   = NULL;
    }

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

error:
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
DWORD
TestLsaQueryInfoPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                 LSA_ACCESS_VIEW_POLICY_INFO;

    const UINT32 def_level = 0;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    LSA_BINDING hLsa = NULL;
    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *info = NULL;
    UINT32 level = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    ntStatus = LsaOpenPolicy2(hLsa,
                              pwszHostname,
                              NULL,
                              access_rights,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1)
        {
            DISPLAY_ERROR(("Level %u unsupported. Exiting...\n", level));
            bRet = FALSE;
            goto error;
        }

        INPUT_ARG_PTR(hLsa);
        INPUT_ARG_PTR(hPolicy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(ntStatus, LsaQueryInfoPolicy(hLsa, hPolicy,
                                              level, &info));
        OUTPUT_ARG_PTR(&info);

        if (info) {
            LsaRpcFreeMemory(info);
        }

    } else {
        for (level = 1; level <= 12; level++) {
            if (level == 1) continue;

            INPUT_ARG_PTR(hLsa);
            INPUT_ARG_PTR(hPolicy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(&info);

            CALL_MSRPC(ntStatus, LsaQueryInfoPolicy(hLsa, hPolicy,
                                                  level, &info));
            OUTPUT_ARG_PTR(&info);

            if (info) {
                LsaRpcFreeMemory(info);
            }

            info = NULL;
        }
    }

    ntStatus = LsaClose(hLsa, hPolicy);
    LsaFreeBinding(&hLsa);

error:
    return bRet;
}


static
DWORD
TestLsaQueryInfoPolicy2(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                 LSA_ACCESS_ENABLE_LSA |
                                 LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS |
                                 LSA_ACCESS_CHANGE_SYS_AUDIT_REQS |
                                 LSA_ACCESS_SET_DEFAULT_QUOTA |
                                 LSA_ACCESS_CREATE_PRIVILEGE |
                                 LSA_ACCESS_CREATE_SECRET_OBJECT |
                                 LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS |
                                 LSA_ACCESS_CHANGE_DOMTRUST_RELATION |
                                 LSA_ACCESS_GET_SENSITIVE_POLICY_INFO |
                                 LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                                 LSA_ACCESS_VIEW_POLICY_INFO;

    const UINT32 def_level = 0;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    LSA_BINDING hLsa = NULL;
    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *info = NULL;
    UINT32 level = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    ntStatus = LsaOpenPolicy2(hLsa,
                              pwszHostname,
                              NULL,
                              access_rights,
                              &hPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1)
        {
            DISPLAY_ERROR(("Level %u unsupported. Exiting...\n", level));
            bRet = FALSE;
            goto error;
        }

        INPUT_ARG_PTR(hLsa);
        INPUT_ARG_PTR(hPolicy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(ntStatus, LsaQueryInfoPolicy2(hLsa, hPolicy,
                                               level, &info));
        OUTPUT_ARG_PTR(&info);

        if (info) {
            LsaRpcFreeMemory(info);
        }

    } else {
        for (level = 1; level <= 12; level++) {
            if (level == 1) continue;

            INPUT_ARG_PTR(hLsa);
            INPUT_ARG_PTR(hPolicy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(info);

            CALL_MSRPC(ntStatus, LsaQueryInfoPolicy2(hLsa, hPolicy,
                                                   level, &info));
            OUTPUT_ARG_PTR(info);

            if (info) {
                LsaRpcFreeMemory(info);
            }

            info = NULL;
        }
    }

    ntStatus = LsaClose(hLsa, hPolicy);
    LsaFreeBinding(&hLsa);

error:
    return bRet;
}


static
BOOLEAN
CallLsaLookupNames(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RefDomainList *pDomains = NULL;
    TranslatedSid *pTransSids = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD iSid = 0;

    CALL_MSRPC(ntStatus, LsaLookupNames(hBinding,
                                        hPolicy,
                                        dwNumNames,
                                        ppwszNames,
                                        &pDomains,
                                        &pTransSids,
                                        dwLevel,
                                        &dwCount));
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

    dwError = LwAllocateMemory(sizeof(PSID) * dwNumNames,
                               OUT_PPVOID(&ppSids));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(DWORD) * dwNumNames,
                               OUT_PPVOID(&pdwTypes));
    BAIL_ON_WIN_ERROR(dwError);

    for (iSid = 0; iSid < dwCount; iSid++)
    {
        DWORD iDomain = pTransSids[iSid].index;
        DWORD dwRid = pTransSids[iSid].rid;
        DWORD dwType = pTransSids[iSid].type;
        PSID pDomainSid = NULL;
        DWORD dwSidSize = 0;
        PSID pSid = NULL;

        if (dwType == SID_TYPE_USE_NONE ||
            dwType == SID_TYPE_INVALID ||
            dwType == SID_TYPE_UNKNOWN)
        {
            continue;
        }

        ASSERT_TEST_MSG(iDomain < pDomains->count,
                        ("invalid domain index = %u (i = %u)\n",
                         iDomain, iSid));

        if (iDomain >= 0 && iDomain < pDomains->count)
        {
            pDomainSid = pDomains->domains[iDomain].sid;

            ASSERT_TEST_MSG(pDomainSid != NULL && RtlValidSid(pDomainSid),
                            ("invalid domain SID (i = %u)\n", iSid));

            if (RtlValidSid(pDomainSid))
            {
                dwSidSize = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

                dwError = LwAllocateMemory(dwSidSize, OUT_PPVOID(&pSid));
                BAIL_ON_WIN_ERROR(dwError);

                ntStatus = RtlCopySid(dwSidSize, pSid, pDomainSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = RtlAppendRidSid(dwSidSize, pSid, dwRid);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

        ASSERT_TEST_MSG(dwRid > 0,
                        ("invalid RID = %u (i = %u)\n", dwRid, iSid));

        ASSERT_TEST_MSG((dwType >= SID_TYPE_USE_NONE &&
                         dwType <= SID_TYPE_LABEL),
                        ("invalid type = %u (i = %u)\n", dwType, iSid));


        pdwTypes[iSid] = dwType;
        ppSids[iSid]   = pSid;
    }

    *pppSids   = ppSids;
    *ppdwTypes = pdwTypes;

cleanup:
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }

    if (pTransSids)
    {
        LsaRpcFreeMemory(pTransSids);
    }
    
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;

error:
    for (iSid = 0; iSid < dwCount; iSid++)
    {
        RTL_FREE(&(ppSids[iSid]));
    }

    LW_SAFE_FREE_MEMORY(ppSids);
    LW_SAFE_FREE_MEMORY(pdwTypes);

    *pppSids   = NULL;
    *ppdwTypes = NULL;

    goto cleanup;
}


static
BOOLEAN
CallLsaLookupNames2(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RefDomainList *pDomains = NULL;
    TranslatedSid2 *pTransSids = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD iSid = 0;

    CALL_MSRPC(ntStatus, LsaLookupNames2(hBinding,
                                         hPolicy,
                                         dwNumNames,
                                         ppwszNames,
                                         &pDomains,
                                         &pTransSids,
                                         dwLevel,
                                         &dwCount));
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

    dwError = LwAllocateMemory(sizeof(PSID) * dwNumNames,
                               OUT_PPVOID(&ppSids));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(DWORD) * dwNumNames,
                               OUT_PPVOID(&pdwTypes));
    BAIL_ON_WIN_ERROR(dwError);

    for (iSid = 0; iSid < dwCount; iSid++)
    {
        DWORD iDomain = pTransSids[iSid].index;
        DWORD dwRid = pTransSids[iSid].rid;
        DWORD dwType = pTransSids[iSid].type;
        PSID pDomainSid = NULL;
        DWORD dwSidSize = 0;
        PSID pSid = NULL;

        if (dwType == SID_TYPE_USE_NONE ||
            dwType == SID_TYPE_INVALID ||
            dwType == SID_TYPE_UNKNOWN)
        {
            continue;
        }

        ASSERT_TEST_MSG(iDomain < pDomains->count,
                        ("invalid domain index = %u (i = %u)\n",
                         iDomain, iSid));

        if (iDomain >= 0 && iDomain < pDomains->count)
        {
            pDomainSid = pDomains->domains[iDomain].sid;

            ASSERT_TEST_MSG(pDomainSid != NULL && RtlValidSid(pDomainSid),
                            ("invalid domain SID (i = %u)\n", iSid));

            if (RtlValidSid(pDomainSid))
            {
                dwSidSize = RtlLengthRequiredSid(pDomainSid->SubAuthorityCount + 1);

                dwError = LwAllocateMemory(dwSidSize, OUT_PPVOID(&pSid));
                BAIL_ON_WIN_ERROR(dwError);

                ntStatus = RtlCopySid(dwSidSize, pSid, pDomainSid);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = RtlAppendRidSid(dwSidSize, pSid, dwRid);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

        ASSERT_TEST_MSG(dwRid > 0,
                        ("invalid RID = %u (i = %u)\n", dwRid, iSid));

        ASSERT_TEST_MSG((dwType >= SID_TYPE_USE_NONE &&
                         dwType <= SID_TYPE_LABEL),
                        ("invalid type = %u (i = %u)\n", dwType, iSid));


        pdwTypes[iSid] = dwType;
        ppSids[iSid]   = pSid;
    }

    *pppSids   = ppSids;
    *ppdwTypes = pdwTypes;

cleanup:
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }

    if (pTransSids)
    {
        LsaRpcFreeMemory(pTransSids);
    }
    
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;

error:
    for (iSid = 0; iSid < dwCount; iSid++)
    {
        RTL_FREE(&(ppSids[iSid]));
    }

    LW_SAFE_FREE_MEMORY(ppSids);
    LW_SAFE_FREE_MEMORY(pdwTypes);

    *pppSids   = NULL;
    *ppdwTypes = NULL;

    goto cleanup;
}


static
BOOLEAN
CallLsaLookupNames3(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumNames,
    PWSTR         *ppwszNames,
    DWORD          dwLevel,
    PSID         **pppSids,
    PDWORD        *ppdwTypes
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RefDomainList *pDomains = NULL;
    TranslatedSid3 *pTransSids = NULL;
    PSID *ppSids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD iSid = 0;

    CALL_MSRPC(ntStatus, LsaLookupNames3(hBinding,
                                         hPolicy,
                                         dwNumNames,
                                         ppwszNames,
                                         &pDomains,
                                         &pTransSids,
                                         dwLevel,
                                         &dwCount));
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

    dwError = LwAllocateMemory(sizeof(PSID) * dwNumNames,
                               OUT_PPVOID(&ppSids));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(DWORD) * dwNumNames,
                               OUT_PPVOID(&pdwTypes));
    BAIL_ON_WIN_ERROR(dwError);

    for (iSid = 0; iSid < dwCount; iSid++)
    {
        DWORD iDomain = pTransSids[iSid].index;
        DWORD dwType = pTransSids[iSid].type;
        PSID pSid = pTransSids[iSid].sid;
        PSID pDomainSid = NULL;
        PSID pRetSid = NULL;

        if (dwType == SID_TYPE_USE_NONE ||
            dwType == SID_TYPE_INVALID ||
            dwType == SID_TYPE_UNKNOWN)
        {
            continue;
        }

        ASSERT_TEST_MSG(iDomain < pDomains->count,
                        ("invalid domain index = %u (i = %u)\n",
                         iDomain, iSid));

        if (iDomain >= 0 && iDomain < pDomains->count)
        {
            pDomainSid = pDomains->domains[iDomain].sid;

            ASSERT_TEST_MSG(pDomainSid != NULL && RtlValidSid(pDomainSid),
                            ("invalid domain SID (i = %u)\n", iSid));
        }

        ASSERT_TEST_MSG(pSid != NULL && RtlValidSid(pSid),
                        ("invalid SID (i = %u)\n", iSid));

        if (RtlValidSid(pSid))
        {
            ntStatus = RtlDuplicateSid(&pRetSid, pSid);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ASSERT_TEST_MSG((dwType >= SID_TYPE_USE_NONE &&
                         dwType <= SID_TYPE_LABEL),
                        ("invalid type = %u (i = %u)\n", dwType, iSid));

        pdwTypes[iSid] = dwType;
        ppSids[iSid]   = pRetSid;
    }

    *pppSids   = ppSids;
    *ppdwTypes = pdwTypes;

cleanup:
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }

    if (pTransSids)
    {
        LsaRpcFreeMemory(pTransSids);
    }
    
    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;

error:
    for (iSid = 0; iSid < dwCount; iSid++)
    {
        RTL_FREE(&(ppSids[iSid]));
    }

    LW_SAFE_FREE_MEMORY(ppSids);
    LW_SAFE_FREE_MEMORY(pdwTypes);

    *pppSids   = NULL;
    *ppdwTypes = NULL;

    goto cleanup;
}


static
BOOLEAN
CallLsaLookupSids(
    LSA_BINDING    hBinding,
    POLICY_HANDLE  hPolicy,
    DWORD          dwNumSids,
    PSID          *ppSids,
    DWORD          dwLevel,
    PWSTR        **pppwszNames,
    PDWORD        *ppdwTypes
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    SID_ARRAY Sids = {0};
    RefDomainList *pDomains = NULL;
    TranslatedName *pTransNames = NULL;
    PWSTR *ppwszNames = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwCount = 0;
    DWORD iSid = 0;
    DWORD iName = 0;
    PSID pInvalidSid = NULL;

    Sids.dwNumSids = dwNumSids;

    dwError = LwAllocateMemory(sizeof(SID_PTR) * dwNumSids,
                               OUT_PPVOID(&Sids.pSids));
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = RtlAllocateSidFromCString(&pInvalidSid,
                                         "S-1-5-32-0");
    BAIL_ON_NT_STATUS(ntStatus);

    for (iSid = 0; iSid < dwNumSids; iSid++)
    {
        /* Replace NULL SIDs with an invalid SID because CallLookupSids
           may be called in sequence of other lookup calls, so keeping
           "invalid" names/SIDs in the same indices is important for
           further validation */
        Sids.pSids[iSid].pSid = ppSids[iSid] ? ppSids[iSid] : pInvalidSid;
    }

    CALL_MSRPC(ntStatus, LsaLookupSids(hBinding,
                                       hPolicy,
                                       &Sids,
                                       &pDomains,
                                       &pTransNames,
                                       dwLevel,
                                       &dwCount));
    if (ntStatus == STATUS_SOME_NOT_MAPPED ||
        ntStatus == STATUS_NONE_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }

    dwError = LwAllocateMemory(sizeof(PWSTR) * dwNumSids,
                               OUT_PPVOID(&ppwszNames));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(DWORD) * dwNumSids,
                               OUT_PPVOID(&pdwTypes));
    BAIL_ON_WIN_ERROR(dwError);

    for (iName = 0; iName < dwCount; iName++)
    {
        DWORD iDomain = pTransNames[iName].sid_index;
        DWORD dwType = pTransNames[iName].type;
        PWSTR pwszDomainName = NULL;
        PWSTR pwszTranslatedName = NULL;
        PWSTR pwszName = NULL;

        if (dwType == SID_TYPE_USE_NONE ||
            dwType == SID_TYPE_INVALID ||
            dwType == SID_TYPE_UNKNOWN)
        {
            continue;
        }

        ASSERT_TEST_MSG(iDomain < pDomains->count,
                        ("invalid domain index = %u (i = %u)\n",
                         iDomain, iName));

        ASSERT_TEST_MSG((dwType >= SID_TYPE_USE_NONE &&
                         dwType <= SID_TYPE_LABEL),
                        ("invalid type = %u (i = %u)\n", dwType, iName));

        if (iDomain >= 0 && iDomain < pDomains->count)
        {
            ASSERT_UNICODE_STRING_VALID_MSG(&(pDomains->domains[iDomain].name),
                                          ("invalid unicode name (i = %u)\n",
                                           iName));

            ASSERT_UNICODE_STRING_VALID_MSG(&(pTransNames[iName].name),
                                          ("invalid unicode name (i = %u)\n",
                                           iName));

            dwError = LwAllocateWc16StringFromUnicodeString(
                            &pwszDomainName,
                            (PUNICODE_STRING)&pDomains->domains[iDomain].name);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = LwAllocateWc16StringFromUnicodeString(
                            &pwszTranslatedName,
                            (PUNICODE_STRING)&pTransNames[iName].name);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = TestFormatNT4Name(&pwszName,
                                        pwszDomainName, pwszTranslatedName);
            BAIL_ON_WIN_ERROR(dwError);
        }

        ppwszNames[iName] = pwszName;
        pdwTypes[iName]   = dwType;

        LW_SAFE_FREE_MEMORY(pwszDomainName);
        LW_SAFE_FREE_MEMORY(pwszTranslatedName);

        pwszDomainName = NULL;
        pwszTranslatedName = NULL;
    }

    *pppwszNames = ppwszNames;
    *ppdwTypes   = pdwTypes;

cleanup:
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }

    if (pTransNames)
    {
        LsaRpcFreeMemory(pTransNames);
    }
    
    LW_SAFE_FREE_MEMORY(Sids.pSids);
    RTL_FREE(&pInvalidSid);

    if (ntStatus != STATUS_SUCCESS ||
        dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;

error:
    for (iName = 0; iName < dwCount; iName++)
    {
        LW_SAFE_FREE_MEMORY(ppwszNames[iName]);
    }

    LW_SAFE_FREE_MEMORY(ppwszNames);
    LW_SAFE_FREE_MEMORY(pdwTypes);

    *pppwszNames = NULL;
    *ppdwTypes   = NULL;

    goto cleanup;
}


static
BOOLEAN
CallLsaOpenPolicy(
    LSA_BINDING      hBinding,
    PWSTR            pwszSysName,
    DWORD            dwAccessRights,
    POLICY_HANDLE   *phPolicy,
    PWSTR           *ppwszDomainName,
    PWSTR           *ppwszDnsDomainName,
    PSID            *ppDomainSid
    )
{
    BOOL bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwMaxAccessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                              LSA_ACCESS_ENABLE_LSA |
                              LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS |
                              LSA_ACCESS_CHANGE_SYS_AUDIT_REQS |
                              LSA_ACCESS_SET_DEFAULT_QUOTA |
                              LSA_ACCESS_CREATE_PRIVILEGE |
                              LSA_ACCESS_CREATE_SECRET_OBJECT |
                              LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS |
                              LSA_ACCESS_CHANGE_DOMTRUST_RELATION |
                              LSA_ACCESS_GET_SENSITIVE_POLICY_INFO |
                              LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                              LSA_ACCESS_VIEW_POLICY_INFO;
    DWORD dwMinAccessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    DWORD dwDefAccessRights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                              LSA_ACCESS_VIEW_POLICY_INFO;

    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *pPolicyInfo = NULL;
    LsaPolicyInformation *pDomainInfo = NULL;
    LsaPolicyInformation *pDnsDomainInfo = NULL;
    DWORD i = 0;

    DISPLAY_COMMENT(("Testing LsaOpenPolicy\n"));

    CALL_MSRPC(ntStatus, LsaOpenPolicy2(hBinding,
                                        pwszSysName,
                                        NULL,
                                        dwMaxAccessRights,
                                        &hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 1; i <= 12; i++)
    {
        if (i == 1) continue;

        DISPLAY_COMMENT(("Testing LsaQueryInfoPolicy (level = %u)\n", i));

        CALL_MSRPC(ntStatus, LsaQueryInfoPolicy(hBinding,
                                                hPolicy,
                                                i,
                                                &pPolicyInfo));

        if (ntStatus != STATUS_SUCCESS) continue;

        switch (i)
        {
        case LSA_POLICY_INFO_DOMAIN:
            pDomainInfo = pPolicyInfo;
            break;

        case LSA_POLICY_INFO_DNS:
            pDnsDomainInfo = pPolicyInfo;
            break;

        default:
            if (pPolicyInfo)
            {
                LsaRpcFreeMemory(pPolicyInfo);
                pPolicyInfo = NULL;
            }
            break;
        }
    }

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&pDomainInfo->domain.name,
                                (PUNICODE_STRING)&pDnsDomainInfo->dns.name);

    ASSERT_SID_EQUAL(pDomainInfo->domain.sid, pDnsDomainInfo->dns.sid);

    DISPLAY_COMMENT(("Testing LsaClose\n"));
    CALL_MSRPC(ntStatus, LsaClose(hBinding, hPolicy));

    hPolicy = NULL;

    DISPLAY_COMMENT(("Testing LsaOpenPolicy2\n"));

    CALL_MSRPC(ntStatus, LsaOpenPolicy2(hBinding,
                                        pwszSysName,
                                        NULL,
                                        dwMinAccessRights,
                                        &hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 1; i <= 12; i++)
    {
        if (i == 1) continue;

        DISPLAY_COMMENT(("Testing LsaQueryInfoPolicy2 (level = %u)\n", i));

        ntStatus = LsaQueryInfoPolicy2(hBinding, hPolicy, i, &pPolicyInfo);

        if (i == LSA_POLICY_INFO_DB ||
            i == LSA_POLICY_INFO_AUDIT_FULL_SET ||
            i == LSA_POLICY_INFO_AUDIT_FULL_QUERY)
        {
            /* ignore audit-related info levels */
            continue;
        }
        ASSERT_TEST_MSG((ntStatus == STATUS_ACCESS_DENIED),
                        ("Expected: STATUS_ACCESS_DENIED, "
                         "Received: %s (0x%08x)\n",
                         LwNtStatusToName(ntStatus), ntStatus));

        ASSERT_TEST((pPolicyInfo == NULL));
    }

    DISPLAY_COMMENT(("Testing LsaClose\n"));
    CALL_MSRPC(ntStatus, LsaClose(hBinding, hPolicy));

    hPolicy = NULL;

    DISPLAY_COMMENT(("Testing LsaOpenPolicy2\n"));

    if (dwAccessRights == 0)
    {
        dwAccessRights = dwDefAccessRights;
    }

    CALL_MSRPC(ntStatus, LsaOpenPolicy2(hBinding,
                                        pwszSysName,
                                        NULL,
                                        dwAccessRights,
                                        &hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    *phPolicy = hPolicy;

    if (ppwszDomainName)
    {
        dwError = LwAllocateWc16StringFromUnicodeString(
                                  ppwszDomainName,
                                  (PUNICODE_STRING)&pDomainInfo->domain.name);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppwszDnsDomainName)
    {
        dwError = LwAllocateWc16StringFromUnicodeString(
                                  ppwszDnsDomainName,
                                  (PUNICODE_STRING)&pDnsDomainInfo->dns.name);
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (ppDomainSid)
    {
        ntStatus = RtlDuplicateSid(ppDomainSid, pDomainInfo->domain.sid);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:
    if (pDomainInfo)
    {
        LsaRpcFreeMemory(pDomainInfo);
    }

    if (pDnsDomainInfo)
    {
        LsaRpcFreeMemory(pDnsDomainInfo);
    }

    if (dwError != ERROR_SUCCESS ||
        ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
BOOLEAN
CallLsaClosePolicy(
    LSA_BINDING    hBinding,
    POLICY_HANDLE *phPolicy
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    POLICY_HANDLE hPolicy = NULL;

    DISPLAY_COMMENT(("Testing LsaClose\n"));

    hPolicy = *phPolicy;

    CALL_MSRPC(ntStatus, LsaClose(hBinding, hPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

error:
    return bRet;
}


static
DWORD
TestLsaInfoPolicy(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "";

    BOOL bRet = TRUE;
    enum param_err perr = perr_success;
    LSA_BINDING hLsa = NULL;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszSysName = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, pwszSysName, 0, &hPolicy, NULL, NULL, NULL);

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

    LsaFreeBinding(&hLsa);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hLsa),
                             RPC_LSA_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);

    bRet &= CallLsaOpenPolicy(hLsa, pwszSysName, 0, &hPolicy, NULL, NULL, NULL);

    bRet &= CallLsaClosePolicy(hLsa, &hPolicy);

    LsaFreeBinding(&hLsa);

    LW_SAFE_FREE_MEMORY(pwszSysName);

    return (int)bRet;
}


VOID
SetupLsaTests(PTEST t)
{
    AddTest(t, "LSA-OPEN-POLICY", TestLsaOpenPolicy);
    AddTest(t, "LSA-LOOKUP-NAMES", TestLsaLookupNames);
    AddTest(t, "LSA-LOOKUP-NAMES2", TestLsaLookupNames2);
    AddTest(t, "LSA-LOOKUP-NAMES3", TestLsaLookupNames3);
    AddTest(t, "LSA-LOOKUP-SIDS", TestLsaLookupSids);
    AddTest(t, "LSA-QUERY-INFO-POL", TestLsaQueryInfoPolicy);
    AddTest(t, "LSA-QUERY-INFO-POL2", TestLsaQueryInfoPolicy2);
    AddTest(t, "LSA-INFO-POLICY", TestLsaInfoPolicy);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
