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

extern int verbose_mode;


#define DISPLAY_DOMAINS(names, num)                             \
    {                                                           \
        UINT32 i;                                               \
        for (i = 0; i < num; i++) {                             \
            wchar16_t *name = names[i];                         \
            w16printfw(L"Domain name: [%ws]\n", name);          \
        }                                                       \
    }



typedef struct _TEST_USER
{
    PSTR   pszAccountName;
    PSTR   pszFullName;
    PSTR   pszHomeDirectory;
    PSTR   pszHomeDrive;
    PSTR   pszLogonScript;
    PSTR   pszProfilePath;
    PSTR   pszDescription;
    PSTR   pszWorkstations;
    PSTR   pszComment;
    PSTR   pszParameters;
    DWORD  dwPrimaryGroup;
    DWORD  dwAccountFlags;
    DWORD  dwCountryCode;
    DWORD  dwCodePage;
    PSTR   pszPassword;
} TEST_USER, *PTEST_USER;


static
SAMR_BINDING
CreateSamrBinding(
    SAMR_BINDING *binding,
    const wchar16_t *host
    );

static
void
GetSessionKey(
    SAMR_BINDING binding,
    unsigned char** sess_key,
    unsigned16* sess_key_len,
    unsigned32* st);

NTSTATUS
GetSamDomainName(
    wchar16_t **domname,
    const wchar16_t *hostname
    );

NTSTATUS
GetSamDomainSid(
    PSID* sid,
    const wchar16_t *hostname
    );

static
NTSTATUS
EnsureUserAccount(
    PCWSTR pwszHostname,
    PWSTR  pwszUsername,
    PBOOL  pbCreated
    );

static
NTSTATUS
EnsureAlias(
    PCWSTR pwszHostname,
    PWSTR  pwszAliasname,
    PBOOL  pbCreated
    );

static
NTSTATUS
CleanupAlias(
    PCWSTR pwszHostname,
    PWSTR  pwszUsername
    );

static
DWORD
TestGetUserAccountTestSet(
    PTEST_USER     *ppTestSet,
    PDWORD          pdwNumNames,
    PCSTR           pszTestSetName
    );

static
BOOLEAN
CallSamrConnect(
    SAMR_BINDING hBinding,
    const wchar16_t *system_name,
    CONNECT_HANDLE *phConn
    );

static
BOOLEAN
CallSamrClose(
    SAMR_BINDING hBinding,
    CONNECT_HANDLE *phConn
    );

static
BOOLEAN
CallSamrEnumDomains(
    SAMR_BINDING        hBinding,
    CONNECT_HANDLE  hConn,
    PSID           *ppDomainSid,
    PSID           *ppBuiltinSid
    );

static
BOOLEAN
CallSamrOpenDomains(
    SAMR_BINDING        hBinding,
    CONNECT_HANDLE  hConn,
    PSID            pDomainSid,
    PSID            pBuiltinSid,
    DWORD           dwAccessRights,
    DOMAIN_HANDLE  *phDomain,
    DOMAIN_HANDLE  *phBuiltin
    );

static
BOOLEAN
CallSamrEnumDomainUsers(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PDWORD        *ppdwUserRids,
    PDWORD         pdwNumUsers
    );

static
BOOLEAN
CallSamrEnumDomainAliases(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PDWORD        *ppdwAliasRids,
    PDWORD         pdwNumAliases
    );

static
BOOLEAN
CallSamrQueryDomainUsers(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwRids,
    DWORD           dwNumRids,
    UserInfo     ***pppInfo
    );

static
BOOLEAN
CallSamrQueryDomainAliases(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwRids,
    DWORD           dwNumRids
    );

static
BOOLEAN
CallSamrGetAliasMemberships(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PSID            pDomainSid
    );

static
BOOLEAN
CallSamrGetMembersInAlias(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwAliasRids,
    DWORD           dwNumRids
    );

static
BOOLEAN
CallCreateUserAccounts(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PSTR            pszTestSetName,
    PDWORD         *ppdwNewAccountRids,
    PDWORD          pdwNumAccounts,
    UserInfo     ***pppNewAccountInfo
    );

static
BOOLEAN
CallCleanupAccounts(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PWSTR         *ppwszUsernames,
    DWORD          dwNumUsernames
    );

static
BOOLEAN
CallSamrDeleteDomainUsers(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwUserRids,
    DWORD           dwNumUsers
    );

static
BOOLEAN
TestValidateDomainInfo(
    DomainInfo    **ppDomainInfo
    );

static
BOOLEAN
TestValidateUserInfo(
    UserInfo      **ppUserInfo,
    DWORD           dwRid
    );

static
BOOLEAN
TestValidateAliasInfo(
    AliasInfo **ppAliasInfo,
    DWORD       dwRid
    );

static
BOOLEAN
TestVerifyUsers(
    UserInfo   **ppNewUserInfo,
    UserInfo  ***pppUserInfo,
    DWORD        dwNumUsers
    );

static
DWORD
TestSamrConnect(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrDomains(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrDomainsQuery(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrUsers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrQueryUsers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrAliases(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrQueryAliases(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrAlias(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrUsersInAliases(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrEnumUsers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrQueryDisplayInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrCreateAlias(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrCreateGroup(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrMultipleConnections(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );

static
DWORD
TestSamrQuerySecurity(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    );


static
TEST_USER SimpleStandaloneAccounts[] = {
    {
        .pszAccountName      = "testuser1",
        .pszFullName         = "xxx FullName aaa",
        .pszHomeDirectory    = "xxxHomeDiraaa",
        .pszHomeDrive        = "K:",
        .pszLogonScript      = "xxxLogonScriptaaa",
        .pszProfilePath      = "xxxProfilePathaaa",
        .pszDescription      = "xxx Description aaa",
        .pszWorkstations     = NULL,
        .pszComment          = "xxx Comment aaa",
        .pszParameters       = NULL,
        .dwPrimaryGroup      = 0,
        .dwAccountFlags      = ACB_NORMAL | ACB_DISABLED,
        .dwCountryCode       = 0,
        .dwCodePage          = 0,
        .pszPassword         = NULL
    },
    {
        .pszAccountName      = "testuser2",
        .pszFullName         = "xxx FullName bbb",
        .pszHomeDirectory    = "xxxHomeDirbbb",
        .pszHomeDrive        = "L:",
        .pszLogonScript      = "xxxLogonScriptbbb",
        .pszProfilePath      = "xxxProfilePathbbb",
        .pszDescription      = "xxx Description bbb",
        .pszWorkstations     = NULL,
        .pszComment          = "xxx Comment bbb",
        .pszParameters       = NULL,
        .dwPrimaryGroup      = 0,
        .dwAccountFlags      = ACB_NORMAL | ACB_DISABLED,
        .dwCountryCode       = 0,
        .dwCodePage          = 0,
        .pszPassword         = NULL
    },
    {
        .pszAccountName      = "testuser3",
        .pszFullName         = "xxx FullName ccc",
        .pszHomeDirectory    = "xxxHomeDirccc",
        .pszHomeDrive        = "M:",
        .pszLogonScript      = "xxxLogonScriptccc",
        .pszProfilePath      = "xxxProfilePathccc",
        .pszDescription      = "xxx Description ccc",
        .pszWorkstations     = NULL,
        .pszComment          = "xxx Comment ccc",
        .pszParameters       = NULL,
        .dwPrimaryGroup      = 0,
        .dwAccountFlags      = ACB_NORMAL | ACB_DISABLED,
        .dwCountryCode       = 0,
        .dwCodePage          = 0,
        .pszPassword         = NULL
    }
};


static
SAMR_BINDING
CreateSamrBinding(
    SAMR_BINDING *binding,
    const wchar16_t *host
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LW_PIO_CREDS creds = NULL;

    if (binding == NULL) return NULL;

    if (LwIoGetActiveCreds(NULL, &creds) != STATUS_SUCCESS)
    {
        *binding = NULL;
        goto error;
    }

    ntStatus = SamrInitBindingDefault(binding, host, creds);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    if (creds)
    {
        LwIoDeleteCreds(creds);
    }

    return *binding;
}

static
void
GetSessionKey(
    SAMR_BINDING binding,
    unsigned char** sess_key,
    unsigned16* sess_key_len,
    unsigned32* st)
{
    rpc_transport_info_handle_t info = NULL;

    rpc_binding_inq_transport_info((handle_t)binding, &info, st);
    if (*st)
    {
        goto error;
    }

    rpc_smb_transport_info_inq_session_key(info, sess_key,
                                           sess_key_len);

cleanup:
    return;

error:
    *sess_key     = NULL;
    *sess_key_len = 0;
    goto cleanup;
}


/*
  Utility function for getting SAM domain name given a hostname
*/
NTSTATUS
GetSamDomainName(
    wchar16_t **domname,
    const wchar16_t *hostname
    )
{
    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const UINT32 enum_size = 32;
    const char *builtin = "Builtin";

    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret = STATUS_SUCCESS;
    SAMR_BINDING samr_binding = NULL;
    CONNECT_HANDLE hConn = NULL;
    UINT32 resume = 0;
    UINT32 count = 0;
    UINT32 i = 0;
    wchar16_t **dom_names = NULL;

    if (domname == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return STATUS_UNSUCCESSFUL;

    status = SamrConnect2(samr_binding, hostname, conn_access, &hConn);
    if (status != 0) rpc_fail(status);

    dom_names = NULL;

    do {
        status = SamrEnumDomains(samr_binding, hConn, &resume,
                                 enum_size, &dom_names, &count);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES) rpc_fail(status);

        for (i = 0; i < count; i++) {
            char n[32] = {0};

            wc16stombs(n, dom_names[i], sizeof(n));
            if (strcasecmp(n, builtin)) {
                *domname = (wchar16_t*) wc16sdup(dom_names[i]);
                ret = STATUS_SUCCESS;

                SamrFreeMemory((void*)dom_names);
                goto found;
            }
        }

        SamrFreeMemory((void*)dom_names);

    } while (status == STATUS_MORE_ENTRIES);

    *domname = NULL;
    ret = STATUS_NOT_FOUND;

found:
    status = SamrClose(samr_binding, hConn);
    if (status != 0) return status;

done:
    SamrFreeBinding(&samr_binding);

    return status;
}


/*
  Utility function for getting SAM domain SID given a hostname
*/
NTSTATUS
GetSamDomainSid(
    PSID* sid,
    const wchar16_t *hostname
    )
{
    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    NTSTATUS status = STATUS_SUCCESS;
    SAMR_BINDING samr_b = NULL;
    CONNECT_HANDLE hConn = NULL;
    wchar16_t *domname = NULL;
    PSID out_sid = NULL;

    if (sid == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) goto done;

    samr_b = CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) rpc_fail(STATUS_UNSUCCESSFUL);

    status = SamrConnect2(samr_b, hostname, conn_access, &hConn);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_b, hConn, domname, &out_sid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, hConn);
    if (status != 0) rpc_fail(status);

    /* Allocate a copy of sid so it can be freed clean by the caller */
    status = RtlDuplicateSid(sid, out_sid);

done:
    SamrFreeBinding(&samr_b);

    if (out_sid) {
        SamrFreeMemory((void*)out_sid);
    }

    LW_SAFE_FREE_MEMORY(domname);

    return status;
}


static
NTSTATUS
EnsureUserAccount(
    PCWSTR pwszHostname,
    PWSTR  pwszUsername,
    PBOOL  pbCreated
    )
{

    const UINT32 account_flags = ACB_NORMAL;
    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const UINT32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    SAMR_BINDING samr_b = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    UINT32 user_rid = 0;
    BOOL bCreated = FALSE;

    status = GetSamDomainSid(&sid, pwszHostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, pwszHostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, pwszHostname, conn_access, &hConn);
    if (status != 0) rpc_fail(status)

    status = SamrOpenDomain(samr_b, hConn, domain_access, sid, &hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrCreateUser(samr_b, hDomain, pwszUsername, account_flags,
                            &hUser, &user_rid);
    if (status == STATUS_SUCCESS)
    {
        bCreated = TRUE;

        status = SamrClose(samr_b, hUser);
        if (status != 0) rpc_fail(status);

    } else if (status != 0 &&
               status != STATUS_USER_EXISTS) rpc_fail(status);

    status = SamrClose(samr_b, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, hConn);
    if (status != 0) rpc_fail(status);

    if (pbCreated)
    {
        *pbCreated = bCreated;
    }

done:

    SamrFreeBinding(&samr_b);
    RTL_FREE(&sid);

    return status;
}


static
NTSTATUS
EnsureAlias(
    PCWSTR pwszHostname,
    PWSTR  pwszAliasname,
    PBOOL  pbCreated
    )
{

    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const UINT32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_ALIAS |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;
    const UINT32 alias_access = ALIAS_ACCESS_LOOKUP_INFO |
                                ALIAS_ACCESS_SET_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    SAMR_BINDING samr_b = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    UINT32 alias_rid = 0;
    BOOL bCreated = FALSE;

    status = GetSamDomainSid(&sid, pwszHostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, pwszHostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, pwszHostname, conn_access, &hConn);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_b, hConn, domain_access, sid, &hDomain);
    if (status != 0) return status;

    status = SamrCreateDomAlias(samr_b, hDomain, pwszAliasname, alias_access,
                                &hAlias, &alias_rid);
    if (status == STATUS_SUCCESS) {
        /* Let caller know that new alias have been created */
        bCreated = TRUE;

        status = SamrClose(samr_b, hAlias);
        if (status != 0) rpc_fail(status);

    } else if (status != 0 &&
               status != STATUS_ALIAS_EXISTS) rpc_fail(status);

    status = SamrClose(samr_b, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, hConn);
    if (status != 0) rpc_fail(status);

    if (pbCreated)
    {
        *pbCreated = bCreated;
    }

done:
    SamrFreeBinding(&samr_b);
    RTL_FREE(&sid);

    return status;
}


static
NTSTATUS
CleanupAlias(
    PCWSTR pwszHostname,
    PWSTR  pwszUsername
    )
{
    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const UINT32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT;
    const UINT32 alias_access = DELETE;

    SAMR_BINDING samr_b = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    PSID sid = NULL;
    wchar16_t *names[1] = {0};
    UINT32 *rids = NULL;
    UINT32 *types = NULL;
    UINT32 rids_count = 0;

    status = GetSamDomainSid(&sid, pwszHostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, pwszHostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, pwszHostname, conn_access, &hConn);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_b, hConn, domain_access, sid, &hDomain);
    if (status != 0) rpc_fail(status);

    names[0] = pwszUsername;
    status = SamrLookupNames(samr_b, hDomain, 1, names, &rids, &types,
                             &rids_count);

    /* if no account has been found return success */
    if (status == STATUS_NONE_MAPPED) {
        status = STATUS_SUCCESS;
        goto done;

    } else if (status != 0) {
        rpc_fail(status)
    }

    status = SamrOpenAlias(samr_b, hDomain, alias_access, rids[0], &hAlias);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteDomAlias(samr_b, hAlias);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_b);

done:
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    LW_SAFE_FREE_MEMORY(sid);

    return status;
}


static
DWORD
TestGetUserAccountTestSet(
    PTEST_USER     *ppTestSet,
    PDWORD          pdwNumNames,
    PCSTR           pszTestSetName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST_USER pTestSet = NULL;
    DWORD dwNumNames = 0;

    if (strcmp(pszTestSetName, "standalone-simple") == 0 ||
        strcmp(pszTestSetName, "STANDALONE-SIMPLE") == 0)
    {
        pTestSet = SimpleStandaloneAccounts;
        dwNumNames = (sizeof(SimpleStandaloneAccounts)
                      /sizeof(SimpleStandaloneAccounts[0]));
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
BOOLEAN
CallSamrConnect(
    SAMR_BINDING        hBinding,
    PCWSTR          pwszSystemName,
    CONNECT_HANDLE *phConn
    )
{
    BOOL bRet = TRUE;
    BOOL bConnected = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwAccessMask = SAMR_ACCESS_OPEN_DOMAIN |
                         SAMR_ACCESS_ENUM_DOMAINS |
                         SAMR_ACCESS_CONNECT_TO_SERVER;
    DWORD dwLevelIn = 0;
    DWORD dwLevelOut = 0;
    SamrConnectInfo InfoIn;
    SamrConnectInfo InfoOut;
    CONNECT_HANDLE hConn = NULL;
    CONNECT_HANDLE h = NULL;

    memset(&InfoIn, 0, sizeof(InfoIn));
    memset(&InfoOut, 0, sizeof(InfoOut));

    DISPLAY_COMMENT(("Testing SamrConnect2\n"));

    ntStatus = SamrConnect2(hBinding, pwszSystemName, dwAccessMask, &h);
    if (ntStatus)
    {
        DISPLAY_ERROR(("SamrConnect2 error %s\n", LwNtStatusToName(ntStatus)));
        bRet = FALSE;
    }
    else
    {
        bConnected = TRUE;
        hConn      = h;
    }

    DISPLAY_COMMENT(("Testing SamrConnect3\n"));

    ntStatus = SamrConnect3(hBinding, pwszSystemName, dwAccessMask, &h);
    if (ntStatus)
    {
        DISPLAY_ERROR(("SamrConnect3 error %s\n", LwNtStatusToName(ntStatus)));
        bRet = FALSE;
    }
    else
    {
        if (bConnected)
        {
            CallSamrClose(hBinding, &hConn);
        }

        bConnected = TRUE;
        hConn      = h;
    }

    DISPLAY_COMMENT(("Testing SamrConnect4\n"));

    ntStatus = SamrConnect4(hBinding, pwszSystemName, 0, dwAccessMask, &h);
    if (ntStatus)
    {
        DISPLAY_ERROR(("SamrConnect4 error %s\n", LwNtStatusToName(ntStatus)));
        bRet = FALSE;
    }
    else
    {
        if (bConnected)
        {
            CallSamrClose(hBinding, &hConn);
        }

        bConnected = TRUE;
        hConn      = h;
    }

    DISPLAY_COMMENT(("Testing SamrConnect5\n"));

    dwLevelIn = 1;
    InfoIn.info1.client_version = SAMR_CONNECT_POST_WIN2K;

    ntStatus = SamrConnect5(hBinding, pwszSystemName, dwAccessMask, dwLevelIn,
                            &InfoIn, &dwLevelOut, &InfoOut, &h);
    if (ntStatus)
    {
        DISPLAY_ERROR(("SamrConnect5 error %s\n", LwNtStatusToName(ntStatus)));
        bRet = FALSE;
    }
    else
    {
        if (bConnected)
        {
            CallSamrClose(hBinding, &hConn);
        }

        bConnected = TRUE;
        hConn      = h;
    }

    *phConn = hConn;

    return bRet;
}


static
BOOLEAN
CallSamrClose(
    SAMR_BINDING        hBinding,
    CONNECT_HANDLE *phConn
    )
{
    BOOL bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CONNECT_HANDLE hConn = NULL;

    DISPLAY_COMMENT(("Testing SamrClose\n"));

    hConn = *phConn;

    ntStatus = SamrClose(hBinding, hConn);
    if (ntStatus)
    {
        DISPLAY_ERROR(("SamrClose error %s\n", LwNtStatusToName(ntStatus)));
        bRet = FALSE;
    }

    return bRet;
}


static
BOOLEAN
CallSamrEnumDomains(
    SAMR_BINDING        hBinding,
    CONNECT_HANDLE  hConn,
    PSID           *ppDomainSid,
    PSID           *ppBuiltinSid
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwResume = 0;
    DWORD dwMaxSize = 16;
    PWSTR *ppwszDomains = NULL;
    DWORD dwNumEntries = 0;
    DWORD i = 0;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;

    DISPLAY_COMMENT(("Testing SamrEnumDomains with max_size = %u\n", dwMaxSize));

    do
    {
        CALL_MSRPC(ntStatus, SamrEnumDomains(hBinding, hConn, &dwResume,
                                             dwMaxSize, &ppwszDomains,
                                             &dwNumEntries));
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomains error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_DOMAINS(ppwszDomains, dwNumEntries);

            for (i = 0; i < dwNumEntries; i++)
            {
                ASSERT_TEST_MSG((ppwszDomains[i] != NULL), ("(i = %u)\n", i));
            }

            if (ppwszDomains)
            {
                SamrFreeMemory(ppwszDomains);
                ppwszDomains = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume    = 0;
    dwMaxSize  *= 2;

    DISPLAY_COMMENT(("Testing SamrEnumDomains with max_size = %u\n", dwMaxSize));

    do
    {
        CALL_MSRPC(ntStatus, SamrEnumDomains(hBinding, hConn, &dwResume,
                                             dwMaxSize, &ppwszDomains,
                                             &dwNumEntries));
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomains error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_DOMAINS(ppwszDomains, dwNumEntries);

            for (i = 0; i < dwNumEntries; i++)
            {
                ASSERT_TEST_MSG((ppwszDomains[i] != NULL), ("(i = %u)\n", i));
            }

            if (ppwszDomains)
            {
                SamrFreeMemory(ppwszDomains);
                ppwszDomains = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume    = 0;
    dwMaxSize  *= 2;

    DISPLAY_COMMENT(("Testing SamrEnumDomains with max_size = %u\n", dwMaxSize));

    do
    {
        CALL_MSRPC(ntStatus, SamrEnumDomains(hBinding, hConn, &dwResume,
                                             dwMaxSize, &ppwszDomains,
                                             &dwNumEntries));
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomains error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_DOMAINS(ppwszDomains, dwNumEntries);

            for (i = 0; i < dwNumEntries; i++)
            {
                ASSERT_TEST_MSG((ppwszDomains[i] != NULL), ("(i = %u)\n", i));
            }

            if (ppwszDomains)
            {
                SamrFreeMemory(ppwszDomains);
                ppwszDomains = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume   = 0;
    dwMaxSize  = (DWORD)(-1);

    DISPLAY_COMMENT(("Testing SamrEnumDomains with max_size = %u\n", dwMaxSize));

    do
    {
        CALL_MSRPC(ntStatus, SamrEnumDomains(hBinding, hConn, &dwResume,
                                             dwMaxSize, &ppwszDomains,
                                             &dwNumEntries));
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomains error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_DOMAINS(ppwszDomains, dwNumEntries);

            for (i = 0; i < dwNumEntries; i++)
            {
                PWSTR pwszDomain = ppwszDomains[i];

                ASSERT_TEST_MSG((ppwszDomains[i] != NULL), ("(i = %u)\n", i));

                DISPLAY_COMMENT(("Testing SamrLookupDomain\n"));

                ntStatus = SamrLookupDomain(hBinding, hConn, pwszDomain, &pSid);
                if (ntStatus)
                {
                    DISPLAY_ERROR(("SamrLookupDomain error %s\n",
                                   LwNtStatusToName(ntStatus)));
                    bRet = FALSE;
                }
                else
                {
                    ASSERT_TEST(pSid != NULL && RtlValidSid(pSid));

                    if (pSid->SubAuthorityCount > 1)
                    {
                        DISPLAY_COMMENT(("Found domain SID\n"));
                        pDomainSid = pSid;
                    }
                    else
                    {
                        DISPLAY_COMMENT(("Found builtin domain SID\n"));
                        pBuiltinSid = pSid;
                    }
                }
            }

            if (ppwszDomains)
            {
                SamrFreeMemory(ppwszDomains);
                ppwszDomains = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    ntStatus = RtlDuplicateSid(ppDomainSid, pDomainSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    ntStatus = RtlDuplicateSid(ppBuiltinSid, pBuiltinSid);
    if (ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    if (pDomainSid)
    {
        SamrFreeMemory(pDomainSid);
    }

    if (pBuiltinSid)
    {
        SamrFreeMemory(pBuiltinSid);
    }

    return bRet;
}


static
BOOLEAN
CallSamrOpenDomains(
    SAMR_BINDING        hBinding,
    CONNECT_HANDLE  hConn,
    PSID            pDomainSid,
    PSID            pBuiltinSid,
    DWORD           dwAccessRights,
    DOMAIN_HANDLE  *phDomain,
    DOMAIN_HANDLE  *phBuiltin
    )
{
    BOOL bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 dwDefaultAccessMask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                 DOMAIN_ACCESS_LOOKUP_INFO_1;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    DomainInfo *pDomainInfo[14] = {NULL};
    DWORD i = 0;

    if (dwAccessRights == 0)
    {
        dwAccessRights = dwDefaultAccessMask;
    }

    if (pDomainSid)
    {
        DISPLAY_COMMENT(("Testing SamrOpenDomain\n"));

        CALL_MSRPC(ntStatus, SamrOpenDomain(hBinding, hConn, dwAccessRights,
                                            pDomainSid, &hDomain));
        if (ntStatus)
        {
            DISPLAY_ERROR(("SamrOpenDomain error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            for (i = 1; i <= 13; i++)
            {
                if (i == 10)
                {
                    /* domain info level 10 doesn't exist */
                    continue;
                }

                DISPLAY_COMMENT(("Testing SamrQueryDomainInfo (level=%u)\n", i));

                CALL_MSRPC(ntStatus, SamrQueryDomainInfo(hBinding,
                                                         hDomain, i, &pDomainInfo[i]));
                if (ntStatus)
                {
                    DISPLAY_ERROR(("SamrQueryDomainInfo error %s\n", LwNtStatusToName(ntStatus)));
                    bRet = FALSE;
                }
            }

            bRet &= TestValidateDomainInfo(pDomainInfo);
        }
    }
    else
    {
        DISPLAY_COMMENT(("Domain skipped\n"));
    }

    for (i = 1; i <= 13; i++)
    {
        if (pDomainInfo[i])
        {
            SamrFreeMemory(pDomainInfo[i]);
            pDomainInfo[i] = NULL;
        }
    }

    if (pBuiltinSid)
    {
        DISPLAY_COMMENT(("Testing SamrOpenDomain\n"));

        CALL_MSRPC(ntStatus, SamrOpenDomain(hBinding, hConn, dwAccessRights,
                                            pBuiltinSid, &hBuiltin));
        if (ntStatus)
        {
            DISPLAY_ERROR(("SamrOpenDomain error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            for (i = 1; i <= 13; i++)
            {
                if (i == 10)
                {
                    /* domain info level 10 doesn't exist */
                    continue;
                }

                DISPLAY_COMMENT(("Testing SamrQueryDomainInfo (level=%u)\n", i));

                CALL_MSRPC(ntStatus, SamrQueryDomainInfo(hBinding,
                                                         hBuiltin, i, &pDomainInfo[i]));
                if (ntStatus)
                {
                    DISPLAY_ERROR(("SamrQueryDomainInfo error %s\n", LwNtStatusToName(ntStatus)));
                    bRet = FALSE;
                }
            }

            bRet &= TestValidateDomainInfo(pDomainInfo);
        }
    }
    else
    {
        DISPLAY_COMMENT(("Builtin domain skipped\n"));
    }

    *phDomain  = hDomain;
    *phBuiltin = hBuiltin;

    for (i = 1; i <= 13; i++)
    {
        if (pDomainInfo[i])
        {
            SamrFreeMemory(pDomainInfo[i]);
        }
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        bRet = FALSE;
    }

    return bRet;
}


static
BOOLEAN
TestValidateDomainInfo(
    DomainInfo    **ppDomainInfo
    )
{
    BOOLEAN bRet = TRUE;

    ASSERT_UNICODE_STRING_VALID(&ppDomainInfo[2]->info2.comment);
    ASSERT_UNICODE_STRING_VALID(&ppDomainInfo[2]->info2.domain_name);
    ASSERT_UNICODE_STRING_VALID(&ppDomainInfo[2]->info2.primary);
    ASSERT_TEST(ppDomainInfo[2]->info2.role >= SAMR_ROLE_STANDALONE &&
                ppDomainInfo[2]->info2.role <= SAMR_ROLE_DOMAIN_PDC);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[4]->info4.comment,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.comment);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[5]->info5.domain_name,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.domain_name);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[6]->info6.primary,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.primary);
    ASSERT_TEST(ppDomainInfo[7]->info7.role ==
                ppDomainInfo[2]->info2.role);
    ASSERT_TEST(ppDomainInfo[11]->info11.info2.role >= SAMR_ROLE_STANDALONE &&
                ppDomainInfo[11]->info11.info2.role <= SAMR_ROLE_DOMAIN_PDC);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[11]->info11.info2.comment,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.comment);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[11]->info11.info2.domain_name,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.domain_name);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppDomainInfo[11]->info11.info2.primary,
                                (PUNICODE_STRING)&ppDomainInfo[2]->info2.primary);
    ASSERT_TEST(ppDomainInfo[11]->info11.info2.role ==
                ppDomainInfo[2]->info2.role);
    ASSERT_TEST(ppDomainInfo[12]->info12.lockout_duration ==
                ppDomainInfo[11]->info11.lockout_duration);
    ASSERT_TEST(ppDomainInfo[12]->info12.lockout_window ==
                ppDomainInfo[11]->info11.lockout_window);
    ASSERT_TEST(ppDomainInfo[12]->info12.lockout_threshold ==
                ppDomainInfo[11]->info11.lockout_threshold);

    return bRet;
}


#define DISPLAY_USERS(names, num)                               \
    {                                                           \
        UINT32 i;                                               \
        for (i = 0; i < num; i++) {                             \
            wchar16_t *name = names[i];                         \
            w16printfw(L"User name: [%ws]\n", name);            \
        }                                                       \
    }


static
BOOLEAN
CallSamrEnumDomainUsers(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PDWORD        *ppdwUserRids,
    PDWORD         pdwNumUsers
    )
{
    BOOL bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwResume = 0;
    DWORD dwMaxSize = 16;
    DWORD dwAccountFlags = 0;
    PWSTR *ppwszUsers = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwRidsCount = 0;
    DWORD i = 0;
    PWSTR ppwszNames[2] = {0};
    PDWORD pdwUserRids = NULL;

    DISPLAY_COMMENT(("Testing SamrEnumDomainUsers with max_size = %u\n", dwMaxSize));

    dwResume       = 0;
    dwAccountFlags = ACB_NORMAL;

    do
    {
        ntStatus = SamrEnumDomainUsers(hBinding, hDomain, &dwResume, dwAccountFlags,
                                       dwMaxSize, &ppwszUsers, &pdwRids, &dwNumEntries);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainUsers error %s\n", LwNtStatusToName(ntStatus)));
        }
        else
        {
            DISPLAY_USERS(ppwszUsers, dwNumEntries);

            if (ppwszUsers)
            {
                SamrFreeMemory(ppwszUsers);
                ppwszUsers = NULL;
            }

            if (pdwRids)
            {
                SamrFreeMemory(pdwRids);
                pdwRids = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume    = 0;
    dwMaxSize  *= 2;

    DISPLAY_COMMENT(("Testing SamrEnumDomainUsers with max_size = %u\n", dwMaxSize));

    do
    {
        ntStatus = SamrEnumDomainUsers(hBinding, hDomain, &dwResume, dwAccountFlags,
                                       dwMaxSize, &ppwszUsers, &pdwRids, &dwNumEntries);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainUsers error %s\n", LwNtStatusToName(ntStatus)));
        }
        else
        {
            DISPLAY_USERS(ppwszUsers, dwNumEntries);

            if (ppwszUsers)
            {
                SamrFreeMemory(ppwszUsers);
                ppwszUsers = NULL;
            }

            if (pdwRids)
            {
                SamrFreeMemory(pdwRids);
                pdwRids = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume    = 0;
    dwMaxSize  *= 2;

    DISPLAY_COMMENT(("Testing SamrEnumDomainUsers with max_size = %u\n", dwMaxSize));

    do
    {
        ntStatus = SamrEnumDomainUsers(hBinding, hDomain, &dwResume, dwAccountFlags,
                                       dwMaxSize, &ppwszUsers, &pdwRids, &dwNumEntries);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainUsers error %s\n", LwNtStatusToName(ntStatus)));
        }
        else
        {
            DISPLAY_USERS(ppwszUsers, dwNumEntries);

            if (ppwszUsers)
            {
                SamrFreeMemory(ppwszUsers);
                ppwszUsers = NULL;
            }

            if (pdwRids)
            {
                SamrFreeMemory(pdwRids);
                pdwRids = NULL;
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwResume   = 0;
    dwMaxSize  = (UINT32)(-1);

    DISPLAY_COMMENT(("Testing SamrEnumDomainUsers with max_size = %u\n", dwMaxSize));

    do
    {
        ntStatus = SamrEnumDomainUsers(hBinding, hDomain, &dwResume, dwAccountFlags,
                                       dwMaxSize, &ppwszUsers, &pdwRids, &dwNumEntries);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainUsers error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_USERS(ppwszUsers, dwNumEntries);

            dwError = LwAllocateMemory(sizeof(DWORD) * dwNumEntries,
                                       OUT_PPVOID(&pdwUserRids));
            BAIL_ON_WIN_ERROR(dwError);

            memcpy(pdwUserRids, pdwRids, sizeof(DWORD) * dwNumEntries);

            SamrFreeMemory(pdwRids);
            pdwRids = NULL;

            for (i = 0; i < dwNumEntries; i++)
            {
                PWSTR pwszUser = ppwszUsers[i];

                ppwszNames[0] = pwszUser;
                ppwszNames[1] = NULL;

                DISPLAY_COMMENT(("Testing SamrLookupNames\n"));

                ntStatus = SamrLookupNames(hBinding, hDomain, 1, ppwszNames,
                                           &pdwRids, &pdwTypes, &dwRidsCount);
                if (ntStatus)
                {
                    DISPLAY_ERROR(("SamrLookupNames error %s\n",
                                   LwNtStatusToName(ntStatus)));
                }

                if (pdwRids)
                {
                    SamrFreeMemory(pdwRids);
                    pdwRids = NULL;
                }

                if (pdwTypes)
                {
                    SamrFreeMemory(pdwTypes);
                    pdwTypes = NULL;
                }
            }
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    if (ppdwUserRids)
    {
        *ppdwUserRids = pdwUserRids;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pdwUserRids);
    }

    if (pdwNumUsers)
    {
        *pdwNumUsers  = dwNumEntries;
    }

error:
    if (ppwszUsers)
    {
        SamrFreeMemory(ppwszUsers);
    }

    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    return bRet;
}


static
BOOLEAN
CallSamrEnumDomainAliases(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PDWORD        *ppdwAliasRids,
    PDWORD         pdwNumAliases
    )
{
    BOOL bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS ntEnumStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwResume = 0;
    DWORD dwAccountFlags = 0;
    PWSTR *ppwszAliases = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwResumeIndex = 0;
    DWORD dwTotalNumAliases = 0;
    DWORD dwRidsCount = 0;
    DWORD i = 0;
    PWSTR ppwszNames[2] = {0};
    PDWORD pdwAliasRids = NULL;

    dwResume = 0;
    dwAccountFlags = ACB_NORMAL;

    DISPLAY_COMMENT(("Testing SamrEnumDomainAliases\n"));

    do
    {
        ntStatus = SamrEnumDomainAliases(hBinding, hDomain, &dwResume, dwAccountFlags,
                                         &ppwszAliases, &pdwRids, &dwNumEntries);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainAliases error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
            goto error;
        }

        if (ppwszAliases)
        {
            SamrFreeMemory(ppwszAliases);
            ppwszAliases = NULL;
        }

        if (pdwRids)
        {
            SamrFreeMemory(pdwRids);
            pdwRids = NULL;
        }

        dwTotalNumAliases += dwNumEntries;
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    dwError = LwAllocateMemory(sizeof(pdwAliasRids[0]) * dwTotalNumAliases,
                               OUT_PPVOID(&pdwAliasRids));
    BAIL_ON_WIN_ERROR(dwError);

    do
    {
        ntEnumStatus = SamrEnumDomainAliases(hBinding, hDomain,
                                             &dwResume, dwAccountFlags,
                                             &ppwszAliases, &pdwRids,
                                             &dwNumEntries);

        if (ntEnumStatus != STATUS_SUCCESS &&
            ntEnumStatus != STATUS_MORE_ENTRIES)
        {
            DISPLAY_ERROR(("SamrEnumDomainAliases error %s\n", LwNtStatusToName(ntStatus)));
            bRet = FALSE;
        }
        else
        {
            DISPLAY_USERS(ppwszAliases, dwNumEntries);

            memcpy(&pdwAliasRids[dwResumeIndex], pdwRids,
                   sizeof(pdwAliasRids[0]) * dwNumEntries);
            dwResumeIndex += dwNumEntries;

            if (pdwRids)
            {
                SamrFreeMemory(pdwRids);
                pdwRids = NULL;
            }

            for (i = 0; i < dwNumEntries; i++)
            {
                PWSTR pwszAlias = ppwszAliases[i];

                ppwszNames[0] = pwszAlias;
                ppwszNames[1] = NULL;

                DISPLAY_COMMENT(("Testing SamrLookupNames\n"));

                ntStatus = SamrLookupNames(hBinding, hDomain, 1, ppwszNames,
                                           &pdwRids, &pdwTypes, &dwRidsCount);
                if (ntStatus)
                {
                    DISPLAY_ERROR(("SamrLookupNames error %s\n",
                                   LwNtStatusToName(ntStatus)));
                }

                if (pdwRids)
                {
                    SamrFreeMemory(pdwRids);
                    pdwRids = NULL;
                }

                if (pdwTypes)
                {
                    SamrFreeMemory(pdwTypes);
                    pdwTypes = NULL;
                }
            }

            if (ppwszAliases)
            {
                SamrFreeMemory(ppwszAliases);
                ppwszAliases = NULL;
            }
        }
    }
    while (ntEnumStatus == STATUS_MORE_ENTRIES);

    if (ppdwAliasRids)
    {
        *ppdwAliasRids = pdwAliasRids;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pdwAliasRids);
    }

    if (pdwNumAliases)
    {
        *pdwNumAliases = dwTotalNumAliases;
    }

error:
    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    if (ppwszAliases)
    {
        SamrFreeMemory(ppwszAliases);
    }

    return bRet;
}


static
BOOLEAN
CallSamrQueryDomainUsers(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwRids,
    DWORD           dwNumRids,
    UserInfo     ***pppInfo
    )
{
    const DWORD dwAccessMask = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES;
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwRid = 0;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD iUser = 0;
    UserInfo **ppUserInfo = NULL;
    DWORD iLevel = 0;

    for (iUser = 0; iUser < dwNumRids; iUser++)
    {
        dwError = LwAllocateMemory(sizeof(ppUserInfo[0]) * 22,
                                   OUT_PPVOID(&ppUserInfo));
        BAIL_ON_WIN_ERROR(dwError);

        dwRid = pdwRids[iUser];
        CALL_MSRPC(ntStatus, SamrOpenUser(hBinding, hDomain, dwAccessMask,
                                          dwRid, &hUser));
        if (ntStatus)
        {
            bRet = FALSE;
        }
        else
        {
            for (iLevel = 1; iLevel <= 21; iLevel++)
            {
                if (iLevel == 5)      /* This level doesn't work at the moment */
                {
                    continue;
                }

                CALL_MSRPC(ntStatus, SamrQueryUserInfo(hBinding, hUser,
                                                       iLevel, &ppUserInfo[iLevel]));
                if (ntStatus == STATUS_INVALID_INFO_CLASS &&
                    (iLevel == 15 ||
                     iLevel == 18 ||
                     iLevel == 19 ))
                {
                    ntStatus = STATUS_SUCCESS;
                }

                if (ntStatus)
                {
                    bRet = FALSE;
                }
            }

            bRet &= TestValidateUserInfo(ppUserInfo, dwRid);
        }
                
        CALL_MSRPC(ntStatus, SamrClose(hBinding, hUser));

        if (pppInfo)
        {
            pppInfo[iUser] = ppUserInfo;
        }
        else
        {
            for (iLevel = 1; iLevel <= 21; iLevel++)
            {
                if (ppUserInfo[iLevel])
                {
                    SamrFreeMemory(ppUserInfo[iLevel]);
                    ppUserInfo[iLevel] = NULL;
                }
            }

            LW_SAFE_FREE_MEMORY(ppUserInfo);
            ppUserInfo = NULL;
        }
    }

error:
    return bRet;
}


static
BOOLEAN
CallSamrQueryDomainAliases(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwRids,
    DWORD           dwNumRids
    )
{
    const DWORD dwAccessMask = ALIAS_ACCESS_LOOKUP_INFO;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwRid = 0;
    ACCOUNT_HANDLE hAlias = NULL;
    DWORD iAlias = 0;
    AliasInfo *ppAliasInfo[4] = {NULL};
    DWORD iLevel = 0;

    for (iAlias = 0; iAlias < dwNumRids; iAlias++)
    {
        dwRid = pdwRids[iAlias];

        CALL_MSRPC(ntStatus, SamrOpenAlias(hBinding, hDomain, dwAccessMask,
                                           dwRid, &hAlias));
        if (ntStatus)
        {
            bRet = FALSE;
        }
        else
        {
            for (iLevel = 1; iLevel <= 3; iLevel++)
            {
                CALL_MSRPC(ntStatus, SamrQueryAliasInfo(hBinding, hAlias,
                                                        iLevel, &ppAliasInfo[iLevel]));
                if (ntStatus)
                {
                    bRet = FALSE;
                }
            }

            bRet &= TestValidateAliasInfo(ppAliasInfo, dwRid);
        }

        CALL_MSRPC(ntStatus, SamrClose(hBinding, hAlias));

        for (iLevel = 1; iLevel <= 3; iLevel++)
        {
            if (ppAliasInfo[iLevel])
            {
                SamrFreeMemory(ppAliasInfo[iLevel]);
                ppAliasInfo[iLevel] = NULL;
            }
        }
    }

    for (iLevel = 1; iLevel <= 3; iLevel++)
    {
        if (ppAliasInfo[iLevel])
        {
            SamrFreeMemory(ppAliasInfo[iLevel]);
        }
    }

    return bRet;
}


static
BOOLEAN
CallSamrGetAliasMemberships(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PSID            pDomainSid
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSID *ppWellKnownSids = NULL;
    DWORD iSid = 0;
    DWORD dwNumWellKnownSids = 4;
    PDWORD pdwRids = NULL;
    DWORD dwRidsCount = 0;

    dwError = LwAllocateMemory(sizeof(ppWellKnownSids[0]) * dwNumWellKnownSids,
                               OUT_PPVOID(&ppWellKnownSids));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinBuiltinUsersSid,
                                     NULL,
                                     &(ppWellKnownSids[iSid++]),
                                     NULL);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinBuiltinAdministratorsSid,
                                     NULL,
                                     &(ppWellKnownSids[iSid++]),
                                     NULL);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinAccountDomainUsersSid,
                                     pDomainSid,
                                     &(ppWellKnownSids[iSid++]),
                                     NULL);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateWellKnownSid(WinAccountDomainAdminsSid,
                                     pDomainSid,
                                     &(ppWellKnownSids[iSid++]),
                                     NULL);
    BAIL_ON_WIN_ERROR(dwError);

    CALL_MSRPC(ntStatus, SamrGetAliasMembership(hBinding,
                                                hDomain,
                                                ppWellKnownSids,
                                                dwNumWellKnownSids,
                                                &pdwRids,
                                                &dwRidsCount));
    if (ntStatus)
    {
        bRet = FALSE;
    }

    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
        pdwRids = NULL;
    }

    for (iSid = 0; iSid < dwNumWellKnownSids; iSid++)
    {
        CALL_MSRPC(ntStatus, SamrGetAliasMembership(hBinding,
                                                    hDomain,
                                                    &(ppWellKnownSids[iSid]),
                                                    1,
                                                    &pdwRids,
                                                    &dwRidsCount));
        if (ntStatus)
        {
            bRet = FALSE;
        }

        if (pdwRids)
        {
            SamrFreeMemory(pdwRids);
            pdwRids = NULL;
        }
    }

error:
    for (iSid = 0; iSid < dwNumWellKnownSids; iSid++)
    {
        LW_SAFE_FREE_MEMORY(ppWellKnownSids[iSid]);
    }

    return bRet;
}


static
BOOLEAN
CallSamrGetMembersInAlias(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwAliasRids,
    DWORD           dwNumRids
    )
{
    const DWORD dwAccessMask = ALIAS_ACCESS_LOOKUP_INFO |
                               ALIAS_ACCESS_GET_MEMBERS;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwRid = 0;
    ACCOUNT_HANDLE hAlias = NULL;
    DWORD iAlias = 0;
    PSID *ppSids = NULL;
    DWORD dwSidsCount = 0;
    DWORD iSid = 0;

    for (iAlias = 0; iAlias < dwNumRids; iAlias++)
    {
        dwRid = pdwAliasRids[iAlias];

        CALL_MSRPC(ntStatus, SamrOpenAlias(hBinding,
                                           hDomain,
                                           dwAccessMask,
                                           dwRid,
                                           &hAlias));
        if (ntStatus)
        {
            bRet = FALSE;
            goto cleanup;
        }

        CALL_MSRPC(ntStatus, SamrGetMembersInAlias(hBinding,
                                                   hAlias,
                                                   &ppSids,
                                                   &dwSidsCount));
        if (ntStatus)
        {
            bRet = FALSE;
            goto cleanup;
        }

        for (iSid = 0; iSid < dwSidsCount; iSid++)
        {
            ASSERT_TEST_MSG((ppSids[iSid] != NULL && RtlValidSid(ppSids[iSid])),
                            ("(i = %u)\n", iSid));
        }

cleanup:
        CALL_MSRPC(ntStatus, SamrClose(hBinding, hAlias));

        if (ppSids)
        {
            SamrFreeMemory(ppSids);
            ppSids = NULL;
        }

        hAlias = NULL;
    }

    return bRet;
}


static
BOOLEAN
CallCreateUserAccounts(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PSTR            pszTestSetName,
    PDWORD         *ppdwNewAccountRids,
    PDWORD          pdwNumAccounts,
    UserInfo     ***pppNewAccountInfo
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PTEST_USER pTestSet = NULL;
    DWORD dwNumUsers = 0;
    DWORD iUser = 0;
    PWSTR pwszAccountName = NULL;
    DWORD dwCreateUserAccessMask = USER_ACCESS_GET_ATTRIBUTES |
                                   USER_ACCESS_SET_ATTRIBUTES |
                                   USER_ACCESS_SET_PASSWORD;
    PDWORD pdwRids = NULL;
    DWORD dwNumAccounts = 0;
    UserInfo **ppUserInfo = NULL;

    dwError = TestGetUserAccountTestSet(&pTestSet, &dwNumUsers, pszTestSetName);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(pdwRids[0]) * dwNumUsers,
                               OUT_PPVOID(&pdwRids));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(ppUserInfo[0]) * dwNumUsers,
                               OUT_PPVOID(&ppUserInfo));
    BAIL_ON_WIN_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        DWORD dwAccessGranted = 0;
        PTEST_USER pUser = &(pTestSet[iUser]);
        ACCOUNT_HANDLE hUser = NULL;
        DWORD dwRid = 0;
        DWORD dwLevel = 0;
        UserInfo Info8;
        UserInfo Info9;
        UserInfo Info10;
        UserInfo Info11;
        UserInfo Info12;
        UserInfo Info13;
        UserInfo Info14;
        UserInfo Info16;
        UserInfo *pInfo21 = NULL;

        memset(&Info8, 0, sizeof(Info8));
        memset(&Info9, 0, sizeof(Info9));
        memset(&Info10, 0, sizeof(Info10));
        memset(&Info11, 0, sizeof(Info11));
        memset(&Info12, 0, sizeof(Info12));
        memset(&Info13, 0, sizeof(Info13));
        memset(&Info14, 0, sizeof(Info14));
        memset(&Info16, 0, sizeof(Info16));

        dwError = LwMbsToWc16s(pUser->pszAccountName, &pwszAccountName);
        BAIL_ON_WIN_ERROR(dwError);

        bRet &= CallCleanupAccounts(hBinding,
                                    hDomain,
                                    &pwszAccountName,
                                    1);

        CALL_MSRPC(ntStatus, SamrCreateUser2(hBinding,
                                             hDomain,
                                             pwszAccountName,
                                             ACB_NORMAL,
                                             dwCreateUserAccessMask,
                                             &hUser,
                                             &dwAccessGranted,
                                             &dwRid));
        if (ntStatus)
        {
            bRet = FALSE;
            goto cleanup;
        }

        pdwRids[iUser] = dwRid;
        dwNumAccounts++;

        CALL_MSRPC(ntStatus, SamrClose(hBinding, hUser));

        CALL_MSRPC(ntStatus, SamrOpenUser(hBinding,
                                          hDomain,
                                          dwCreateUserAccessMask,
                                          dwRid,
                                          &hUser));
        if (ntStatus)
        {
            bRet = FALSE;
            goto cleanup;
        }

        if (pUser->pszFullName)
        {
            dwLevel = 8;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info8.info8.full_name,
                                          pUser->pszFullName);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info8));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->pszHomeDirectory ||
            pUser->pszHomeDrive)
        {
            dwLevel = 10;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info10.info10.home_directory,
                                          pUser->pszHomeDirectory);
            BAIL_ON_WIN_ERROR(dwError);

            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info10.info10.home_drive,
                                          pUser->pszHomeDrive);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info10));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->pszLogonScript)
        {
            dwLevel = 11;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info11.info11.logon_script,
                                          pUser->pszLogonScript);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info11));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->pszProfilePath)
        {
            dwLevel = 12;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info12.info12.profile_path,
                                          pUser->pszProfilePath);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info12));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->pszDescription)
        {
            dwLevel = 13;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info13.info13.description,
                                          pUser->pszDescription);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info13));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->pszWorkstations)
        {
            dwLevel = 14;
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&Info14.info14.workstations,
                                          pUser->pszWorkstations);
            BAIL_ON_WIN_ERROR(dwError);

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info14));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        if (pUser->dwPrimaryGroup)
        {
            dwLevel = 9;
            Info9.info9.primary_gid = pUser->dwPrimaryGroup;

            CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, &Info9));
            if (ntStatus)
            {
                bRet = FALSE;
            }
        }

        dwLevel = 21;
        dwError = LwAllocateMemory(sizeof(*pInfo21), OUT_PPVOID(&pInfo21));
        BAIL_ON_WIN_ERROR(dwError);

        if (pUser->pszFullName)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.full_name,
                                          pUser->pszFullName);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_FULL_NAME;
        }

        if (pUser->pszHomeDirectory)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.home_directory,
                                          pUser->pszHomeDirectory);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_HOME_DIRECTORY;
        }

        if (pUser->pszHomeDrive)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.home_drive,
                                          pUser->pszHomeDrive);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_HOME_DRIVE;
        }

        if (pUser->pszLogonScript)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.logon_script,
                                          pUser->pszLogonScript);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_LOGON_SCRIPT;
        }

        if (pUser->pszProfilePath)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.profile_path,
                                          pUser->pszProfilePath);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_PROFILE_PATH;
        }

        if (pUser->pszDescription)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.description,
                                          pUser->pszDescription);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_DESCRIPTION;
        }

        if (pUser->pszWorkstations)
        {
            dwError = LwAllocateUnicodeStringFromCString(
                                          (PUNICODE_STRING)&pInfo21->info21.workstations,
                                          pUser->pszWorkstations);
            BAIL_ON_WIN_ERROR(dwError);

            pInfo21->info21.fields_present |= SAMR_FIELD_WORKSTATIONS;
        }

        if (pUser->dwPrimaryGroup)
        {
            pInfo21->info21.primary_gid = pUser->dwPrimaryGroup;
            pInfo21->info21.fields_present |= SAMR_FIELD_PRIMARY_GID;
        }

        CALL_MSRPC(ntStatus, SamrSetUserInfo2(hBinding, hUser, dwLevel, pInfo21));
        if (ntStatus)
        {
            bRet = FALSE;
        }

        ppUserInfo[iUser] = pInfo21;

cleanup:
        CALL_MSRPC(ntStatus, SamrClose(hBinding, hUser));

        LW_SAFE_FREE_MEMORY(pwszAccountName);
        LwFreeUnicodeString((PUNICODE_STRING)&Info8.info8.full_name);
        LwFreeUnicodeString((PUNICODE_STRING)&Info10.info10.home_directory);
        LwFreeUnicodeString((PUNICODE_STRING)&Info10.info10.home_drive);
        LwFreeUnicodeString((PUNICODE_STRING)&Info11.info11.logon_script);
        LwFreeUnicodeString((PUNICODE_STRING)&Info12.info12.profile_path);
        LwFreeUnicodeString((PUNICODE_STRING)&Info13.info13.description);
        LwFreeUnicodeString((PUNICODE_STRING)&Info14.info14.workstations);

        pwszAccountName = NULL;
        hUser = NULL;
    }

    *ppdwNewAccountRids = pdwRids;
    *pdwNumAccounts     = dwNumAccounts;
    *pppNewAccountInfo  = ppUserInfo;

error:
    LW_SAFE_FREE_MEMORY(pwszAccountName);

    return bRet;
}


static
BOOLEAN
CallCleanupAccounts(
    SAMR_BINDING       hBinding,
    DOMAIN_HANDLE  hDomain,
    PWSTR         *ppwszUsernames,
    DWORD          dwNumUsernames
    )
{
    DWORD dwUserAccess = DELETE;

    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ACCOUNT_HANDLE hAccount = NULL;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    DWORD dwRidsCount = 0;
    DWORD iRid = 0;

    CALL_MSRPC(ntStatus, SamrLookupNames(hBinding,
                                         hDomain,
                                         dwNumUsernames,
                                         ppwszUsernames,
                                         &pdwRids,
                                         &pdwTypes,
                                         &dwRidsCount));
    /* if no account has been found return success */
    if (ntStatus == STATUS_NONE_MAPPED)
    {
        goto done;
    }
    /* if some accounts haven't been found keep going */
    else if (ntStatus == STATUS_SOME_NOT_MAPPED)
    {
        ntStatus = STATUS_SUCCESS;
    }
    else if (ntStatus)
    {
        bRet = FALSE;
        goto done;
    }

    for (iRid = 0; iRid < dwRidsCount; iRid++)
    {
        ASSERT_TEST_MSG((pdwTypes[iRid] >= SID_TYPE_USE_NONE &&
                         pdwTypes[iRid] <= SID_TYPE_LABEL),
                        ("(iRid = %u)\n", iRid));

        if (pdwTypes[iRid] == SID_TYPE_UNKNOWN)
        {
            continue;
        }

        CALL_MSRPC(ntStatus, SamrOpenUser(hBinding,
                                          hDomain,
                                          dwUserAccess,
                                          pdwRids[iRid],
                                          &hAccount));
        if (ntStatus)
        {
            bRet = FALSE;
            continue;
        }

        CALL_MSRPC(ntStatus, SamrDeleteUser(hBinding, hAccount));
        if (ntStatus)
        {
            bRet = FALSE;
            continue;
        }
    }

done:
    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    return bRet;
}


static
BOOLEAN
TestValidateUserInfo(
    UserInfo      **ppUserInfo,
    DWORD           dwRid
    )
{
    BOOLEAN bRet = TRUE;

    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[1]->info1.account_name);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[1]->info1.full_name);
    ASSERT_TEST(ppUserInfo[1]->info1.primary_gid > 0);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[1]->info1.description);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[1]->info1.comment);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[2]->info2.comment,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.comment);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[3]->info3.account_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.account_name);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[3]->info3.full_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.full_name);
    ASSERT_TEST(ppUserInfo[3]->info3.rid == dwRid);
    ASSERT_TEST(ppUserInfo[3]->info3.primary_gid == ppUserInfo[1]->info1.primary_gid);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[3]->info3.home_directory);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[3]->info3.home_drive);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[3]->info3.logon_script);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[3]->info3.profile_path);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[3]->info3.workstations);
    ASSERT_TEST((ppUserInfo[3]->info3.account_flags & ACB_NORMAL));

    ASSERT_TEST((ppUserInfo[4]->info4.logon_hours.units_per_week / 8) <= 1260);

#if 0
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.account_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.account_name);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.full_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.full_name);
    ASSERT_TEST(ppUserInfo[5]->info5.rid == ppUserInfo[3]->info3.rid);
    ASSERT_TEST(ppUserInfo[5]->info5.primary_gid == ppUserInfo[1]->info1.primary_gid);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.home_directory,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.home_directory);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.home_drive,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.home_drive);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.logon_script,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.logon_script);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.profile_path,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.profile_path);
    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[5]->info5.description);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[5]->info5.workstations,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.workstations);
    ASSERT_TEST(ppUserInfo[5]->info5.logon_hours.units_per_week ==
                ppUserInfo[4]->info4.logon_hours.units_per_week);
    ASSERT_TEST(memcmp(ppUserInfo[5]->info5.logon_hours.units,
                       ppUserInfo[4]->info4.logon_hours.units,
                       ppUserInfo[5]->info5.logon_hours.units_per_week / 8) == 0);
    ASSERT_TEST(ppUserInfo[5]->info5.account_flags == ppUserInfo[3]->info3.account_flags);
#endif

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[6]->info6.account_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.account_name);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[6]->info6.full_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.full_name);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[7]->info7.account_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.account_name);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[8]->info8.full_name,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.full_name);

    ASSERT_TEST(ppUserInfo[9]->info9.primary_gid == ppUserInfo[1]->info1.primary_gid);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[10]->info10.home_directory,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.home_directory);
    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[10]->info10.home_drive,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.home_drive);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[11]->info11.logon_script,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.logon_script);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[12]->info12.profile_path,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.profile_path);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[13]->info13.description,
                                (PUNICODE_STRING)&ppUserInfo[1]->info1.description);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[14]->info14.workstations,
                                (PUNICODE_STRING)&ppUserInfo[3]->info3.workstations);

    ASSERT_TEST(ppUserInfo[16]->info16.account_flags == ppUserInfo[3]->info3.account_flags);

#if 0
    ASSERT_TEST(ppUserInfo[17]->info17.account_expiry == ppUserInfo[5]->info5.account_expiry);
#endif

    ASSERT_UNICODE_STRING_VALID(&ppUserInfo[20]->info20.parameters);

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_ACCOUNT_NAME)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.account_name,
                                    (PUNICODE_STRING)&ppUserInfo[1]->info1.account_name);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_FULL_NAME)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.full_name,
                                    (PUNICODE_STRING)&ppUserInfo[1]->info1.full_name);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_RID)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.rid == ppUserInfo[3]->info3.rid);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_PRIMARY_GID)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.primary_gid == ppUserInfo[3]->info3.primary_gid);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_DESCRIPTION)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.description,
                                    (PUNICODE_STRING)&ppUserInfo[1]->info1.description);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_COMMENT)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.comment,
                                    (PUNICODE_STRING)&ppUserInfo[1]->info1.comment);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_HOME_DIRECTORY)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.home_directory,
                                    (PUNICODE_STRING)&ppUserInfo[3]->info3.home_directory);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_HOME_DRIVE)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.home_drive,
                                    (PUNICODE_STRING)&ppUserInfo[3]->info3.home_drive);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_LOGON_SCRIPT)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.logon_script,
                                    (PUNICODE_STRING)&ppUserInfo[3]->info3.logon_script);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_PROFILE_PATH)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.profile_path,
                                    (PUNICODE_STRING)&ppUserInfo[3]->info3.profile_path);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_WORKSTATIONS)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.workstations,
                                    (PUNICODE_STRING)&ppUserInfo[3]->info3.workstations);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_LAST_LOGON)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.last_logon == ppUserInfo[3]->info3.last_logon);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_LAST_LOGOFF)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.last_logoff == ppUserInfo[3]->info3.last_logoff);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_LOGON_HOURS)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.logon_hours.units_per_week ==
                    ppUserInfo[4]->info4.logon_hours.units_per_week);
        ASSERT_TEST(memcmp(ppUserInfo[21]->info21.logon_hours.units,
                           ppUserInfo[4]->info4.logon_hours.units,
                           ppUserInfo[21]->info21.logon_hours.units_per_week / 8) == 0);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_BAD_PWD_COUNT)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.bad_password_count ==
                    ppUserInfo[3]->info3.bad_password_count);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_NUM_LOGONS)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.logon_count == ppUserInfo[3]->info3.logon_count);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_ALLOW_PWD_CHANGE)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.allow_password_change ==
                    ppUserInfo[3]->info3.allow_password_change);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_FORCE_PWD_CHANGE)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.force_password_change ==
                    ppUserInfo[3]->info3.force_password_change);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_LAST_PWD_CHANGE)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.last_password_change ==
                    ppUserInfo[3]->info3.last_password_change);
    }

#if 0
    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_ACCT_EXPIRY)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.account_expiry == ppUserInfo[5]->info5.account_expiry);
    }
#endif

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_ACCT_FLAGS)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.account_flags == ppUserInfo[3]->info3.account_flags);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_PARAMETERS)
    {
        ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppUserInfo[21]->info21.parameters,
                                    (PUNICODE_STRING)&ppUserInfo[20]->info20.parameters);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_COUNTRY_CODE)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.country_code == ppUserInfo[2]->info2.country_code);
    }

    if (ppUserInfo[21]->info21.fields_present & SAMR_FIELD_CODE_PAGE)
    {
        ASSERT_TEST(ppUserInfo[21]->info21.code_page == ppUserInfo[2]->info2.code_page);
    }

    return bRet;
}


static
BOOLEAN
TestValidateAliasInfo(
    AliasInfo **ppAliasInfo,
    DWORD       dwRid
    )
{
    BOOLEAN bRet = TRUE;

    ASSERT_UNICODE_STRING_VALID(&ppAliasInfo[1]->all.name);
    ASSERT_UNICODE_STRING_VALID(&ppAliasInfo[1]->all.description);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppAliasInfo[2]->name,
                                (PUNICODE_STRING)&ppAliasInfo[1]->all.name);

    ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppAliasInfo[3]->description,
                                (PUNICODE_STRING)&ppAliasInfo[1]->all.description);

    return bRet;
}


static
BOOLEAN
TestVerifyUsers(
    UserInfo   **ppNewUserInfo,
    UserInfo  ***pppUserInfo,
    DWORD        dwNumUsers
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwFieldsPresent1 = 0;
    DWORD dwFieldsPresent2 = 0;
    DWORD i = 0;

    for (i = 0; i < dwNumUsers; i++)
    {
        dwFieldsPresent1 = ppNewUserInfo[i]->info21.fields_present;
        dwFieldsPresent2 = pppUserInfo[i][21]->info21.fields_present;

        DISPLAY_COMMENT(("Verifying user (i = %u)\n", i));

        if ((dwFieldsPresent1 & SAMR_FIELD_ACCOUNT_NAME) &&
            (dwFieldsPresent2 & SAMR_FIELD_ACCOUNT_NAME))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.account_name,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.account_name);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_FULL_NAME) &&
            (dwFieldsPresent2 & SAMR_FIELD_FULL_NAME))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.full_name,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.full_name);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_HOME_DIRECTORY) &&
            (dwFieldsPresent2 & SAMR_FIELD_HOME_DIRECTORY))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.home_directory,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.home_directory);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_HOME_DRIVE) &&
            (dwFieldsPresent2 & SAMR_FIELD_HOME_DRIVE))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.home_drive,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.home_drive);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_LOGON_SCRIPT) &&
            (dwFieldsPresent2 & SAMR_FIELD_LOGON_SCRIPT))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.logon_script,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.logon_script);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_PROFILE_PATH) &&
            (dwFieldsPresent2 & SAMR_FIELD_PROFILE_PATH))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.profile_path,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.profile_path);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_DESCRIPTION) &&
            (dwFieldsPresent2 & SAMR_FIELD_DESCRIPTION))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.description,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.description);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_WORKSTATIONS) &&
            (dwFieldsPresent2 & SAMR_FIELD_WORKSTATIONS))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.workstations,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.workstations);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_COMMENT) &&
            (dwFieldsPresent2 & SAMR_FIELD_COMMENT))
        {
            ASSERT_UNICODE_STRING_EQUAL((PUNICODE_STRING)&ppNewUserInfo[i]->info21.comment,
                                        (PUNICODE_STRING)&pppUserInfo[i][21]->info21.comment);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_RID) &&
            (dwFieldsPresent2 & SAMR_FIELD_RID))
        {
            ASSERT_TEST(ppNewUserInfo[i]->info21.rid == pppUserInfo[i][21]->info21.rid);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_PRIMARY_GID) &&
            (dwFieldsPresent2 & SAMR_FIELD_PRIMARY_GID))
        {
            ASSERT_TEST(ppNewUserInfo[i]->info21.primary_gid ==
                        pppUserInfo[i][21]->info21.primary_gid);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_ACCT_FLAGS) &&
            (dwFieldsPresent2 & SAMR_FIELD_ACCT_FLAGS))
        {
            ASSERT_TEST(ppNewUserInfo[i]->info21.account_flags ==
                        pppUserInfo[i][21]->info21.account_flags);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_COUNTRY_CODE) &&
            (dwFieldsPresent2 & SAMR_FIELD_COUNTRY_CODE))
        {
            ASSERT_TEST(ppNewUserInfo[i]->info21.country_code ==
                        pppUserInfo[i][21]->info21.country_code);
        }

        if ((dwFieldsPresent1 & SAMR_FIELD_CODE_PAGE) &&
            (dwFieldsPresent2 & SAMR_FIELD_CODE_PAGE))
        {
            ASSERT_TEST(ppNewUserInfo[i]->info21.code_page ==
                        pppUserInfo[i][21]->info21.code_page);
        }
    }

    return bRet;
}


static
DWORD
TestSamrConnect(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = NULL;

    BOOLEAN bRet = TRUE;
    enum param_err perr = perr_success;
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PWSTR pwszSysName = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrClose(hSamr,
                          &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pwszSysName);

    return bRet;
}


static
DWORD
TestSamrDomains(
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
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pwszSysName);

    return bRet;
}


static
DWORD
TestSamrDomainsQuery(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "";

    BOOLEAN bRet = TRUE;
    enum param_err perr = perr_success;
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    DWORD dwDomainReadRights = DOMAIN_ACCESS_LOOKUP_INFO_2 |
                               DOMAIN_ACCESS_LOOKUP_INFO_1;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    bRet &= CallSamrOpenDomains(hSamr,
                                hConn,
                                pDomainSid,
                                pBuiltinSid,
                                dwDomainReadRights,
                                &hDomain,
                                &hBuiltin);

    bRet &= CallSamrClose(hSamr, &hDomain);

    bRet &= CallSamrClose(hSamr, &hBuiltin);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pwszSysName);

    return bRet;
}


static
DWORD
TestSamrUsers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefSysName = "";
    PCSTR pszDefTestSetName = "standalone-simple";

    BOOL bRet = TRUE;
    enum param_err perr = perr_success;
    SAMR_BINDING hSamr = NULL;
    DWORD dwError = ERROR_SUCCESS;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    DWORD dwDomainReadRights = DOMAIN_ACCESS_LOOKUP_INFO_2 |
                               DOMAIN_ACCESS_LOOKUP_INFO_1;
    DWORD dwCreateAccountRights = DOMAIN_ACCESS_CREATE_USER |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  dwDomainReadRights;
    DWORD dwReadOnlyAccountRights = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                    DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                    dwDomainReadRights;
    BOOLEAN bCreate = FALSE;
    PSTR pszTestSetName = NULL;
    PDWORD pdwNewAccountRids = NULL;
    DWORD dwNumNewAccounts = 0;
    UserInfo **ppNewAccountInfo = NULL;
    UserInfo ***pppUserInfo = NULL;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "testsetname", pt_string,
                       &pszTestSetName, &pszDefTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    if (CallSamrOpenDomains(hSamr,
                            hConn,
                            pDomainSid,
                            pBuiltinSid,
                            dwCreateAccountRights,
                            &hDomain,
                            &hBuiltin))
    {
        bRet    &= TRUE;
        bCreate  = TRUE;
    }
    else
    {
        bRet &= CallSamrOpenDomains(hSamr,
                                    hConn,
                                    pDomainSid,
                                    pBuiltinSid,
                                    dwReadOnlyAccountRights,
                                    &hDomain,
                                    &hBuiltin);
    }

    bRet &= CallSamrEnumDomainUsers(hSamr, hDomain, NULL, NULL);

    bRet &= CallSamrEnumDomainUsers(hSamr, hBuiltin, NULL, NULL);

    if (bCreate)
    {
        bRet &= CallCreateUserAccounts(hSamr,
                                       hDomain,
                                       pszTestSetName,
                                       &pdwNewAccountRids,
                                       &dwNumNewAccounts,
                                       &ppNewAccountInfo);

        dwError = LwAllocateMemory(sizeof(pppUserInfo[0]) * dwNumNewAccounts,
                                   OUT_PPVOID(&pppUserInfo));
        BAIL_ON_WIN_ERROR(dwError);

        bRet &= CallSamrQueryDomainUsers(hSamr,
                                         hDomain,
                                         pdwNewAccountRids,
                                         dwNumNewAccounts,
                                         pppUserInfo);

        bRet &= TestVerifyUsers(ppNewAccountInfo,
                                pppUserInfo,
                                dwNumNewAccounts);

        bRet &= CallSamrDeleteDomainUsers(hSamr,
                                          hDomain,
                                          pdwNewAccountRids,
                                          dwNumNewAccounts);
    }

    bRet &= CallSamrGetAliasMemberships(hSamr,
                                        hDomain,
                                        pDomainSid);

    bRet &= CallSamrGetAliasMemberships(hSamr,
                                        hBuiltin,
                                        pDomainSid);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pwszSysName);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pBuiltinSid);

    return bRet;
}


static
DWORD
TestSamrQueryUsers(
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
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    PDWORD pdwDomainRids = NULL;
    DWORD dwNumDomainUsers = 0;
    PDWORD pdwBuiltinRids = NULL;
    DWORD dwNumBuiltinUsers = 0;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    bRet &= CallSamrOpenDomains(hSamr,
                                hConn,
                                pDomainSid,
                                pBuiltinSid,
                                0,
                                &hDomain,
                                &hBuiltin);

    bRet &= CallSamrEnumDomainUsers(hSamr,
                                    hDomain,
                                    &pdwDomainRids,
                                    &dwNumDomainUsers);

    bRet &= CallSamrQueryDomainUsers(hSamr,
                                     hDomain,
                                     pdwDomainRids,
                                     dwNumDomainUsers,
                                     NULL);

    bRet &= CallSamrEnumDomainUsers(hSamr,
                                    hBuiltin,
                                    &pdwBuiltinRids,
                                    &dwNumBuiltinUsers);

    bRet &= CallSamrQueryDomainUsers(hSamr,
                                     hBuiltin,
                                     pdwBuiltinRids,
                                     dwNumBuiltinUsers,
                                     NULL);

    bRet &= CallSamrClose(hSamr, &hDomain);

    bRet &= CallSamrClose(hSamr, &hBuiltin);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pdwDomainRids);
    LW_SAFE_FREE_MEMORY(pdwBuiltinRids);
    LW_SAFE_FREE_MEMORY(pwszSysName);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pBuiltinSid);

    return bRet;
}


static
BOOLEAN
CallSamrDeleteDomainUsers(
    SAMR_BINDING        hBinding,
    DOMAIN_HANDLE   hDomain,
    PDWORD          pdwUserRids,
    DWORD           dwNumUsers
    )
{
    BOOLEAN bRet = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD iUser = 0;
    DWORD dwAccessRights = DELETE;
    ACCOUNT_HANDLE hUser = NULL;

    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        DWORD dwRid = pdwUserRids[iUser];

        CALL_MSRPC(ntStatus, SamrOpenUser(hBinding,
                                          hDomain,
                                          dwAccessRights,
                                          dwRid,
                                          &hUser));
        if (ntStatus)
        {
            bRet = FALSE;
            continue;
        }

        CALL_MSRPC(ntStatus, SamrDeleteUser(hBinding, hUser));
        if (ntStatus)
        {
            bRet = FALSE;
            continue;
        }
    }

    return bRet;
}


static
DWORD
TestSamrAliases(
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
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    PDWORD pdwDomainRids = NULL;
    DWORD dwNumDomainAliases = 0;
    PDWORD pdwBuiltinRids = NULL;
    DWORD dwNumBuiltinAliases = 0;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    bRet &= CallSamrOpenDomains(hSamr,
                                hConn,
                                pDomainSid,
                                pBuiltinSid,
                                0,
                                &hDomain,
                                &hBuiltin);

    bRet &= CallSamrEnumDomainAliases(hSamr,
                                      hDomain,
                                      &pdwDomainRids,
                                      &dwNumDomainAliases);

    bRet &= CallSamrEnumDomainAliases(hSamr,
                                      hBuiltin,
                                      &pdwBuiltinRids,
                                      &dwNumBuiltinAliases);

    bRet &= CallSamrGetMembersInAlias(hSamr,
                                      hDomain,
                                      pdwDomainRids,
                                      dwNumDomainAliases);

    bRet &= CallSamrGetMembersInAlias(hSamr,
                                      hBuiltin,
                                      pdwBuiltinRids,
                                      dwNumBuiltinAliases);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pwszSysName);
    LW_SAFE_FREE_MEMORY(pdwDomainRids);
    LW_SAFE_FREE_MEMORY(pdwBuiltinRids);

    return bRet;
}


static
DWORD
TestSamrQueryAliases(
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
    SAMR_BINDING hSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    PSID pDomainSid = NULL;
    PSID pBuiltinSid = NULL;
    PWSTR pwszSysName = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    DOMAIN_HANDLE hBuiltin = NULL;
    PDWORD pdwDomainRids = NULL;
    DWORD dwNumDomainUsers = 0;
    PDWORD pdwBuiltinRids = NULL;
    DWORD dwNumBuiltinUsers = 0;

    perr = fetch_value(pOptions, dwOptcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(pTest, pwszHostname);

    bRet &= CreateRpcBinding(OUT_PPVOID(&hSamr),
                             RPC_SAMR_BINDING,
                             pwszHostname,
                             pwszBindingString,
                             pCreds);
    if (!bRet)
    {
        goto error;
    }

    bRet &= CallSamrConnect(hSamr,
                            pwszSysName,
                            &hConn);

    bRet &= CallSamrEnumDomains(hSamr,
                                hConn,
                                &pDomainSid,
                                &pBuiltinSid);

    bRet &= CallSamrOpenDomains(hSamr,
                                hConn,
                                pDomainSid,
                                pBuiltinSid,
                                0,
                                &hDomain,
                                &hBuiltin);

    bRet &= CallSamrEnumDomainAliases(hSamr,
                                      hDomain,
                                      &pdwDomainRids,
                                      &dwNumDomainUsers);

    bRet &= CallSamrQueryDomainAliases(hSamr,
                                       hDomain,
                                       pdwDomainRids,
                                       dwNumDomainUsers);

    bRet &= CallSamrEnumDomainAliases(hSamr,
                                      hBuiltin,
                                      &pdwBuiltinRids,
                                      &dwNumBuiltinUsers);

    bRet &= CallSamrQueryDomainAliases(hSamr,
                                       hBuiltin,
                                       pdwBuiltinRids,
                                       dwNumBuiltinUsers);

    bRet &= CallSamrClose(hSamr, &hDomain);

    bRet &= CallSamrClose(hSamr, &hBuiltin);

    bRet &= CallSamrClose(hSamr, &hConn);

    SamrFreeBinding(&hSamr);

error:
    LW_SAFE_FREE_MEMORY(pdwDomainRids);
    LW_SAFE_FREE_MEMORY(pdwBuiltinRids);
    LW_SAFE_FREE_MEMORY(pwszSysName);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pBuiltinSid);

    return bRet;
}


#define DISPLAY_ALIAS_INFO(info, level)                           \
    if (verbose_mode ) {                                          \
        switch ((level)) {                                        \
        case 1:                                                   \
            OUTPUT_ARG_UNICODE_STRING(&(info)->all.name);         \
            OUTPUT_ARG_UNICODE_STRING(&(info)->all.description);  \
            OUTPUT_ARG_UINT((info)->all.num_members);             \
            break;                                                \
                                                                  \
        case 2:                                                   \
            OUTPUT_ARG_UNICODE_STRING(&(info)->name);             \
            break;                                                \
                                                                  \
        case 3:                                                   \
            OUTPUT_ARG_UNICODE_STRING(&(info)->description);      \
            break;                                                \
        }                                                         \
    }


static
DWORD
TestSamrAlias(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const UINT32 alias_access_mask = ALIAS_ACCESS_LOOKUP_INFO |
                                     ALIAS_ACCESS_SET_INFO |
                                     ALIAS_ACCESS_ADD_MEMBER |
                                     ALIAS_ACCESS_REMOVE_MEMBER |
                                     ALIAS_ACCESS_GET_MEMBERS |
                                     DELETE;

    const char *testalias = "TestAlias";
    const char *testuser = "TestUser";
    const char *testalias_desc = "TestAlias Comment";

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    SAMR_BINDING samr_binding = NULL;
    DWORD dwSidSize = 0;
    UINT32 user_rid = 0;
    UINT32 *rids = NULL;
    UINT32 *types = NULL;
    UINT32 num_rids = 0;
    wchar16_t *aliasname = NULL;
    wchar16_t *aliasdesc = NULL;
    wchar16_t *username = NULL;
    wchar16_t *domname = NULL;
    int i = 0;
    UINT32 rids_count = 0;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    wchar16_t *names[1] = {0};
    PSID sid = NULL;
    PSID user_sid = NULL;
    AliasInfo *aliasinfo = NULL;
    AliasInfo setaliasinfo;
    PSID* member_sids = NULL;
    UINT32 members_num = 0;
    BOOL bAliasCreated = FALSE;
    BOOL bUserCreated = FALSE;

    TESTINFO(pTest, pwszHostname);

    samr_binding = CreateSamrBinding(&samr_binding, pwszHostname);
    if (samr_binding == NULL) return false;

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string, &aliasname, &testalias);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "aliasdesc", pt_w16string, &aliasdesc, &testalias_desc);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string, &username, &testuser);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("aliasdesc", pt_w16string, aliasdesc);
    PARAM_INFO("username", pt_w16string, username);


    /*
     * Creating and querying/setting alias (in the host domain)
     */

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask, &hConn);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, pwszHostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, hConn, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, dom_access_mask, sid,
                            &hDomain);
    if (status != 0) rpc_fail(status);

    /*
     * Ensure alias to perform tests on
     */
    status = EnsureAlias(pwszHostname, aliasname, &bAliasCreated);
    if (status != 0) rpc_fail(status);

    names[0] = aliasname;

    status = SamrLookupNames(samr_binding, hDomain, 1, names,
                             &rids, &types, &rids_count);
    if (status != 0) rpc_fail(status);

    if (rids_count == 0 || rids_count != 1) {
        printf("Incosistency found when looking for alias name\n");
        rpc_fail(STATUS_UNSUCCESSFUL);
    }

    status = SamrOpenAlias(samr_binding, hDomain, alias_access_mask,
                           rids[0], &hAlias);
    if (status != 0) rpc_fail(status);

    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    rids  = NULL;
    types = NULL;

    /*
     * Ensure a user account which will soon become alias member
     */
    status = EnsureUserAccount(pwszHostname, username, &bUserCreated);
    if (status != 0) rpc_fail(status);

    names[0] = username;

    status = SamrLookupNames(samr_binding, hDomain, 1, names, &rids, &types,
                             &rids_count);
    if (status != 0) rpc_fail(status);

    if (rids_count == 0 || rids_count != 1) {
        printf("Incosistency found when looking for alias name\n");
        rpc_fail(STATUS_UNSUCCESSFUL);
    }

    user_rid = rids[0];

    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    rids  = NULL;
    types = NULL;

    for (i = 3; i > 1; i--) {
        INPUT_ARG_PTR(samr_binding);
        INPUT_ARG_PTR(hAlias);
        INPUT_ARG_UINT(i);

        CALL_MSRPC(status, SamrQueryAliasInfo(samr_binding, hAlias,
                                              (UINT16)i, &aliasinfo));
        if (status != 0) rpc_fail(status);

        if (aliasinfo) SamrFreeMemory((void*)aliasinfo);
    }

    status = SamrQueryAliasInfo(samr_binding, hAlias, 1, &aliasinfo);
    if (status != 0) rpc_fail(status);

    dwError = LwAllocateUnicodeStringFromWc16String(
                            &setaliasinfo.description,
                            aliasdesc);
    if (dwError != 0) netapi_fail(dwError);

    status = SamrSetAliasInfo(samr_binding, hAlias, 3, &setaliasinfo);
    if (status != 0) rpc_fail(status);

    dwSidSize = RtlLengthRequiredSid(sid->SubAuthorityCount + 1);
    dwError = LwAllocateMemory(dwSidSize, OUT_PPVOID(&user_sid));
    if (dwError != 0) netapi_fail(dwError);

    status = RtlCopySid(dwSidSize, user_sid, sid);
    if (status != 0) rpc_fail(status);

    status = RtlAppendRidSid(dwSidSize, user_sid, user_rid);
    if (status != 0) rpc_fail(status);

    status = SamrGetAliasMembership(samr_binding, hDomain, &user_sid, 1,
                                    &rids, &num_rids);
    if (status != 0) rpc_fail(status);


    /*
     * Adding, deleting and querying members in alias
     */

    status = SamrAddAliasMember(samr_binding, hAlias, user_sid);
    if (status != 0) rpc_fail(status);

    status = SamrGetMembersInAlias(samr_binding, hAlias, &member_sids,
                                   &members_num);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteAliasMember(samr_binding, hAlias, user_sid);
    if (status != 0) rpc_fail(status);


    /*
     * Cleanup
     */

    if (bAliasCreated) {
        status = SamrDeleteDomAlias(samr_binding, hAlias);
        if (status != 0) rpc_fail(status);

    } else {
        status = SamrClose(samr_binding, hAlias);
        if (status != 0) rpc_fail(status);
    }

    if (bUserCreated) {
        status = SamrDeleteUser(samr_binding, hUser);
        if (status != 0) rpc_fail(status);
    }

    status = SamrClose(samr_binding, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    if (aliasinfo) SamrFreeMemory((void*)aliasinfo);
    if (rids) SamrFreeMemory((void*)rids);
    if (member_sids) SamrFreeMemory((void*)member_sids);
    if (domname) SamrFreeMemory((void*)domname);
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    LwFreeUnicodeString(&setaliasinfo.description);
    LW_SAFE_FREE_MEMORY(user_sid);

    LW_SAFE_FREE_MEMORY(aliasname);
    LW_SAFE_FREE_MEMORY(aliasdesc);
    LW_SAFE_FREE_MEMORY(username);

    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrUsersInAliases(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 builtin_dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                           DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                           DOMAIN_ACCESS_LOOKUP_INFO_2;
    const char *btin_sidstr = "S-1-5-32";

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    SAMR_BINDING samr_binding = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hBtinDomain = NULL;
    char *sidstr = NULL;
    PSID sid = NULL;
    DWORD dwSidSize = 0;
    DomainInfo *dominfo = NULL;

    TESTINFO(pTest, pwszHostname);

    CreateSamrBinding(&samr_binding, pwszHostname);
    if (!samr_binding)
    {
        goto done;
    }

    /*
     * Querying user membership in aliases
     */

    perr = fetch_value(pOptions, dwOptcount, "sid", pt_string, &sidstr,
                       &btin_sidstr);
    if (!perr_is_ok(perr)) perr_fail(perr);

    RtlAllocateSidFromCString(&sid, sidstr);

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask,
                          &hConn);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, builtin_dom_access_mask,
                            sid, &hBtinDomain);
    if (status == 0) {
        UINT32 acct_flags = ACB_NORMAL;
        UINT32 resume = 0;
        wchar16_t **names = NULL;
        UINT32 *rids = NULL;
        UINT32 num_entries = 0;
        UINT32 i = 0;
        PSID alias_sid = NULL;

        do {
            status = SamrEnumDomainAliases(samr_binding, hBtinDomain,
                                           &resume, acct_flags, &names, &rids,
                                           &num_entries);

            for (i = 0; i < num_entries; i++) {
                UINT32 *member_rids = NULL;
                UINT32 count = 0;

                dwSidSize = RtlLengthRequiredSid(sid->SubAuthorityCount + 1);
                dwError = LwAllocateMemory(dwSidSize, OUT_PPVOID(&alias_sid));
                if (dwError != 0) netapi_fail(dwError);

                status = RtlCopySid(dwSidSize, alias_sid, sid);
                if (status != 0) rpc_fail(status);

                status = RtlAppendRidSid(dwSidSize, alias_sid, rids[i]);
                if (status != 0) rpc_fail(status);

                /* there's actually no need to check status code here */
                status = SamrGetAliasMembership(samr_binding,
                                                hBtinDomain,
                                                &alias_sid, 1, &member_rids,
                                                &count);
                LW_SAFE_FREE_MEMORY(alias_sid);

                if (member_rids) SamrFreeMemory((void*)member_rids);
            }

            if (names) SamrFreeMemory((void*)names);
            if (rids) SamrFreeMemory((void*)rids);
            names = NULL;
            rids  = NULL;

        } while (status == STATUS_MORE_ENTRIES);

        status = SamrQueryDomainInfo(samr_binding, hBtinDomain,
                                     (UINT16)2, &dominfo);

        status = SamrClose(samr_binding, hBtinDomain);
    }

    status = SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    RTL_FREE(&sid);
    LW_SAFE_FREE_MEMORY(sidstr);

    return (status == STATUS_SUCCESS);
}


#define DISPLAY_ACCOUNTS(type, flags, names, rids, num)         \
    {                                                           \
        UINT32 i;                                               \
        for (i = 0; i < num; i++) {                             \
            wchar16_t *name = enum_names[i];                    \
            UINT32 rid = enum_rids[i];                          \
                                                                \
            w16printfw(L"%hhs (flags=0x%08x): %ws (rid=0x%x)\n",\
                       (type), (flags), (name), (rid));         \
        }                                                       \
    }

static
DWORD
TestSamrEnumUsers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const int def_specifydomain = 0;
    const char *def_domainname = "BUILTIN";
    const int def_acb_flags = 0;

    NTSTATUS status = STATUS_SUCCESS;
    SAMR_BINDING samr_binding = NULL;
    enum param_err perr = perr_success;
    UINT32 resume = 0;
    UINT32 num_entries = 0;
    UINT32 max_size = 0;
    UINT32 acb_flags = 0;
    UINT32 account_flags = 0;
    wchar16_t **enum_names = NULL;
    wchar16_t *domname = NULL;
    wchar16_t *domainname = NULL;
    UINT32 *enum_rids = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    PSID sid = NULL;
    int specifydomain = 0;

    perr = fetch_value(pOptions, dwOptcount, "specifydomain", pt_int32,
                       &specifydomain, &def_specifydomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string,
                       &domainname, &def_domainname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "acbflags", pt_uint32,
                       &acb_flags, &def_acb_flags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    CreateSamrBinding(&samr_binding, pwszHostname);
    if (!samr_binding)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask,
                          &hConn);
    if (status != 0) rpc_fail(status);

    if (specifydomain) {
        domname = wc16sdup(domainname);

    } else {
        status = GetSamDomainName(&domname, pwszHostname);
        if (status != 0) rpc_fail(status);
    }

    status = SamrLookupDomain(samr_binding, hConn, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, dom_access_mask,
                            sid, &hDomain);
    if (status != 0) rpc_fail(status);

    /*
     * Enumerating domain users
     */

    max_size = 128;
    resume = 0;
    account_flags = ACB_NORMAL;
    do {
        status = SamrEnumDomainUsers(samr_binding, hDomain, &resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);

        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("User", account_flags, enum_names, enum_rids,
                             num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = ACB_DOMTRUST;
    do {
        status = SamrEnumDomainUsers(samr_binding, hDomain, &resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Domain trust", account_flags, enum_names,
                             enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = ACB_WSTRUST;
    do {
        status = SamrEnumDomainUsers(samr_binding, hDomain, &resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Workstation", account_flags, enum_names,
                             enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = acb_flags;
    do {
        status = SamrEnumDomainUsers(samr_binding, hDomain, &resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Account", account_flags,
                             enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);


    SamrClose(samr_binding, hDomain);
    if (status != 0) rpc_fail(status);

    SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    if (sid) {
        SamrFreeMemory((void*)sid);
    }

    LW_SAFE_FREE_MEMORY(domname);
    LW_SAFE_FREE_MEMORY(domainname);

    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrQueryDisplayInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const int def_specifydomain = 0;
    const char *def_domainname = "BUILTIN";
    const int def_level = 0;
    const int def_max_entries = 64;
    const int def_buf_size = 512;

    NTSTATUS status = STATUS_SUCCESS;
    SAMR_BINDING samr_binding = NULL;
    enum param_err perr = perr_success;
    wchar16_t *domname = NULL;
    wchar16_t *domainname = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    PSID sid = NULL;
    int specifydomain = 0;
    INT32 level = 0;
    UINT32 start_idx = 0;
    UINT32 max_entries = 0;
    UINT32 buf_size = 0;
    UINT32 total_size = 0;
    UINT32 returned_size = 0;
    SamrDisplayInfo *info = NULL;

    perr = fetch_value(pOptions, dwOptcount, "specifydomain", pt_int32,
                       &specifydomain, &def_specifydomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string,
                       &domainname, &def_domainname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_int32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "maxentries", pt_int32, &max_entries,
                       &def_max_entries);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "maxsize", pt_int32, &buf_size,
                       &def_buf_size);
    if (!perr_is_ok(perr)) perr_fail(perr);

    CreateSamrBinding(&samr_binding, pwszHostname);
    if (!samr_binding)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask,
                          &hConn);
    if (status != 0) rpc_fail(status);

    if (specifydomain) {
        domname = wc16sdup(domainname);

    } else {
        status = GetSamDomainName(&domname, pwszHostname);
        if (status != 0) rpc_fail(status);
    }

    status = SamrLookupDomain(samr_binding, hConn, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, dom_access_mask,
                            sid, &hDomain);
    if (status != 0) rpc_fail(status);

    if (level == 0) {
        for (level = 1; level <= 5; level++) {
            start_idx = 0;

            do {
                CALL_MSRPC(status, SamrQueryDisplayInfo(samr_binding,
                                                        hDomain,
                                                        (UINT16)level,
                                                        start_idx,
                                                        max_entries,
                                                        buf_size,
                                                        &total_size,
                                                        &returned_size,
                                                        &info));
                switch (level) {
                case 1:
                    if (info && info->info1.count) {
                        start_idx =
                            info->info1.entries[info->info1.count - 1].idx + 1;
                    }
                    break;
                case 2:
                    if (info && info->info2.count) {
                        start_idx =
                            info->info2.entries[info->info2.count - 1].idx + 1;
                    }
                    break;
                case 3:
                    if (info && info->info3.count) {
                        start_idx =
                            info->info3.entries[info->info3.count - 1].idx + 1;
                    }
                    break;
                case 4:
                    if (info && info->info4.count) {
                        start_idx =
                            info->info4.entries[info->info4.count - 1].idx + 1;
                    }
                    break;
                case 5:
                    if (info && info->info5.count) {
                        start_idx =
                            info->info5.entries[info->info5.count - 1].idx + 1;
                    }
                    break;
                }

                if (info) {
                    SamrFreeMemory(info);
                }
            } while (status == STATUS_MORE_ENTRIES);
        }

    } else {
        do {
            CALL_MSRPC(status, SamrQueryDisplayInfo(samr_binding, hDomain,
                                                    (UINT16)level, start_idx,
                                                    max_entries, buf_size,
                                                    &total_size, &returned_size,
                                                    &info));
            switch (level) {
            case 1:
                if (info && info->info1.count) {
                    start_idx =
                        info->info1.entries[info->info1.count - 1].idx + 1;
                }
                break;
            case 2:
                if (info && info->info2.count) {
                    start_idx =
                        info->info2.entries[info->info2.count - 1].idx + 1;
                }
                break;
            case 3:
                if (info && info->info3.count) {
                    start_idx =
                        info->info3.entries[info->info3.count - 1].idx + 1;
                }
                break;
            case 4:
                if (info && info->info4.count) {
                    start_idx =
                        info->info4.entries[info->info4.count - 1].idx + 1;
                }
                break;
            case 5:
                if (info && info->info5.count) {
                    start_idx =
                        info->info5.entries[info->info5.count - 1].idx + 1;
                }
                break;
            }

            if (info) {
                SamrFreeMemory(info);
            }

        } while (status == STATUS_MORE_ENTRIES);
    }

    SamrClose(samr_binding, hDomain);
    if (status != 0) rpc_fail(status);

    SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    if (sid) {
        SamrFreeMemory((void*)sid);
    }

    LW_SAFE_FREE_MEMORY(domname);
    LW_SAFE_FREE_MEMORY(domainname);

    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrCreateAlias(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const UINT32 alias_access = ALIAS_ACCESS_LOOKUP_INFO |
                                ALIAS_ACCESS_SET_INFO |
                                DELETE;

    const char *def_aliasname = "Testalias";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    SAMR_BINDING samr_binding = NULL;
    wchar16_t *newaliasname = NULL;
    wchar16_t *domname = NULL;
    UINT32 rid = 0;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAccount = NULL;
    PSID sid = NULL;

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string,
                       &newaliasname, &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    status = CleanupAlias(pwszHostname, newaliasname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_binding, pwszHostname);
    if (!samr_binding)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask,
                          &hConn);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, pwszHostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, hConn, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, dom_access_mask,
                            sid, &hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrCreateDomAlias(samr_binding, hDomain, newaliasname,
                                alias_access, &hAccount, &rid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrOpenAlias(samr_binding, hDomain, alias_access, rid,
                           &hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteDomAlias(samr_binding, hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    if (sid) SamrFreeMemory((void*)sid);

    LW_SAFE_FREE_MEMORY(domname);
    LW_SAFE_FREE_MEMORY(newaliasname);

    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrCreateGroup(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const UINT32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_CREATE_GROUP |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const UINT32 group_access = GROUP_ACCESS_LOOKUP_INFO |
                                GROUP_ACCESS_SET_INFO |
                                DELETE;

    const char *def_groupname = "Testgroup";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    SAMR_BINDING samr_binding = NULL;
    wchar16_t *newgroupname = NULL;
    wchar16_t *domname = NULL;
    UINT32 rid = 0;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAccount = NULL;
    PSID sid = NULL;

    perr = fetch_value(pOptions, dwOptcount, "groupname", pt_w16string,
                       &newgroupname, &def_groupname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    CreateSamrBinding(&samr_binding, pwszHostname);
    if (!samr_binding)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding, pwszHostname, conn_access_mask,
                          &hConn);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, pwszHostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, hConn, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, hConn, dom_access_mask,
                            sid, &hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrCreateDomGroup(samr_binding, hDomain, newgroupname,
                                group_access, &hAccount, &rid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrOpenGroup(samr_binding, hDomain, group_access, rid,
                           &hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteDomGroup(samr_binding, hAccount);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hDomain);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, hConn);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding);

done:
    if (sid) SamrFreeMemory((void*)sid);

    LW_SAFE_FREE_MEMORY(domname);
    LW_SAFE_FREE_MEMORY(newgroupname);

    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrMultipleConnections(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const UINT32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS |
                               SAMR_ACCESS_CONNECT_TO_SERVER;
    NTSTATUS status = STATUS_SUCCESS;
    SAMR_BINDING samr_binding1 = NULL;
    SAMR_BINDING samr_binding2 = NULL;
    CONNECT_HANDLE hConn1 = NULL;
    CONNECT_HANDLE hConn2 = NULL;
    unsigned char *key1 = NULL;
    unsigned char *key2 = NULL;
    unsigned16 key_len1, key_len2;
    unsigned32 st = 0;

    samr_binding1 = NULL;
    samr_binding2 = NULL;

    CreateSamrBinding(&samr_binding1, pwszHostname);
    if (!samr_binding1)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding1, pwszHostname, conn_access, &hConn1);
    if (status != 0) rpc_fail(status);

    GetSessionKey(samr_binding1, &key1, &key_len1, &st);
    if (st != 0) return false;

    CreateSamrBinding(&samr_binding2, pwszHostname);
    if (!samr_binding2)
    {
        goto done;
    }

    status = SamrConnect2(samr_binding2, pwszHostname, conn_access, &hConn2);
    if (status != 0) rpc_fail(status);

    GetSessionKey(samr_binding2, &key2, &key_len2, &st);
    if (st != 0) return false;

    status = SamrClose(samr_binding1, hConn1);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding2, hConn2);
    if (status != 0) rpc_fail(status);

    SamrFreeBinding(&samr_binding1);
    SamrFreeBinding(&samr_binding2);

done:
    return (status == STATUS_SUCCESS);
}


static
DWORD
TestSamrQuerySecurity(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const PSTR pszDefSidStr = "S-1-5-32-544";
    const ULONG ulDefSecurityInfo = OWNER_SECURITY_INFORMATION;

    const DWORD dwConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const DWORD dwDomainAccess = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    const DWORD dwUserAccess = READ_CONTROL |
                               USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszSidStr = NULL;
    ULONG ulRid = 0;
    ULONG ulSecurityInfo = 0;
    SAMR_BINDING bSamr = NULL;
    CONNECT_HANDLE hConn = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    UINT32 ulSecDescRelLen = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    ULONG ulSecDescLen = 1024;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 1024;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 1024;
    PSID pOwnerSid = NULL;
    ULONG ulOwnerSidLen = 1024;
    PSID pGroupSid = NULL;
    ULONG ulGroupSidLen = 1024;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "sid", pt_sid,
                       &pSid, &pszDefSidStr);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "security_info", pt_uint32,
                       &ulSecurityInfo, &ulDefSecurityInfo);
    if (!perr_is_ok(perr)) perr_fail(perr);

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSidStr,
                                            pSid);
    if (ntStatus != 0) rpc_fail(ntStatus);

    PARAM_INFO("sid", pt_w16string, pwszSidStr);
    PARAM_INFO("security_info", pt_uint32, &ulSecurityInfo);

    CreateSamrBinding(&bSamr, pwszHostname);
    if (!bSamr)
    {
        goto done;
    }

    ntStatus = SamrConnect2(bSamr,
                            pwszHostname,
                            dwConnAccess,
                            &hConn);
    if (ntStatus != 0) rpc_fail(ntStatus);

    /* Create domain SID from account SID */
    ntStatus = RtlDuplicateSid(&pDomainSid,
                               pSid);
    if (ntStatus != 0) rpc_fail(ntStatus);

    pDomainSid->SubAuthorityCount--;

    ntStatus = SamrOpenDomain(bSamr,
                              hConn,
                              dwDomainAccess,
                              pDomainSid,
                              &hDomain);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulRid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];

    ntStatus = SamrOpenUser(bSamr,
                            hDomain,
                            dwUserAccess,
                            ulRid,
                            &hUser);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ntStatus = SamrQuerySecurity(bSamr,
                                 hUser,
                                 ulSecurityInfo,
                                 &pSecDescRel,
                                 &ulSecDescRelLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulSecDescLen = 1024;
    ntStatus = RTL_ALLOCATE(&pSecDesc,
                            VOID,
                            ulSecDescLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulDaclLen = 1024;
    ntStatus = RTL_ALLOCATE(&pDacl,
                            VOID,
                            ulDaclLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulSaclLen = 1024;
    ntStatus = RTL_ALLOCATE(&pSacl,
                            VOID,
                            ulSaclLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulOwnerSidLen = 1024;
    ntStatus = RTL_ALLOCATE(&pOwnerSid,
                            VOID,
                            ulOwnerSidLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ulGroupSidLen = 1024;
    ntStatus = RTL_ALLOCATE(&pGroupSid,
                            VOID,
                            ulGroupSidLen);
    if (ntStatus != 0) rpc_fail(ntStatus);


    ntStatus = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                           pSecDesc,
                                           &ulSecDescLen,
                                           pDacl,
                                           &ulDaclLen,
                                           pSacl,
                                           &ulSaclLen,
                                           pOwnerSid,
                                           &ulOwnerSidLen,
                                           pGroupSid,
                                           &ulGroupSidLen);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ntStatus = SamrClose(bSamr, hUser);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ntStatus = SamrClose(bSamr, hDomain);
    if (ntStatus != 0) rpc_fail(ntStatus);

    ntStatus = SamrClose(bSamr, hConn);
    if (ntStatus != 0) rpc_fail(ntStatus);

done:
    if (pSecDesc)
    {
        SamrFreeMemory(pSecDesc);
    }

    RTL_FREE(&pwszSidStr);
    RTL_FREE(&pSid);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pSecDesc);

    return (ntStatus == STATUS_SUCCESS);
}


VOID
SetupSamrTests(PTEST t)
{
    AddTest(t, "SAMR-ALIAS", TestSamrAlias);
    AddTest(t, "SAMR-ALIAS-MEMBERS", TestSamrUsersInAliases);
    AddTest(t, "SAMR-ENUM-USERS", TestSamrEnumUsers);
    AddTest(t, "SAMR-CREATE-ALIAS", TestSamrCreateAlias);
    AddTest(t, "SAMR-CREATE-GROUP", TestSamrCreateGroup);
    AddTest(t, "SAMR-MULTIPLE-CONNECTION", TestSamrMultipleConnections);
    AddTest(t, "SAMR-QUERY-DISPLAY-INFO", TestSamrQueryDisplayInfo);
    AddTest(t, "SAMR-CONNECT", TestSamrConnect);
    AddTest(t, "SAMR-DOMAINS", TestSamrDomains);
    AddTest(t, "SAMR-DOMAINS-QUERY", TestSamrDomainsQuery);
    AddTest(t, "SAMR-USERS", TestSamrUsers);
    AddTest(t, "SAMR-USERS-QUERY", TestSamrQueryUsers);
    AddTest(t, "SAMR-ALIASES-QUERY", TestSamrQueryAliases);
    AddTest(t, "SAMR-ALIASES", TestSamrAliases);
    AddTest(t, "SAMR-QUERY-SECURITY", TestSamrQuerySecurity);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
