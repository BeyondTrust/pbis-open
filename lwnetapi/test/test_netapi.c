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


typedef struct _TEST_NET_USER_ADD
{
    PSTR    pszUserName;
    PSTR    pszComment;
    PSTR    pszDescription;
    PSTR    pszHomeDir;
    PSTR    pszScriptPath;
    DWORD   dwPrivilege;
    DWORD   dwFlags;
    DWORD   dwExpectedFlags;
    PSTR    pszPassword;
    DWORD   dwLevel;
    BOOLEAN bPreCleanup;
    BOOLEAN bPostCleanup;
    DWORD   dwError;
    DWORD   dwParmError;

} TEST_NET_USER_ADD, *PTEST_NET_USER_ADD;


static
TEST_NET_USER_ADD LocalUserAddValidationTest[] = {
    {
        .pszUserName     = "testuser1",
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .dwExpectedFlags = UF_NORMAL_ACCOUNT |
                           UF_ACCOUNTDISABLE,
        .pszPassword     = NULL,
        .dwLevel         = 1,
        .bPreCleanup     = TRUE,
        .bPostCleanup    = TRUE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .pszUserName     = "testuser1",
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .dwExpectedFlags = UF_NORMAL_ACCOUNT,
        .pszPassword     = "toPs3Cret%",
        .dwLevel         = 1,
        .bPreCleanup     = FALSE,
        .bPostCleanup    = TRUE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .pszUserName     = "testuser1",
        .pszComment      = "### Comment testuser1 ###",
        .pszDescription  = "### Description testuser1 ###",
        .pszHomeDir      = "c:\\tmp",
        .pszScriptPath   = "n:\\netlogon\\logon.cmd",
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .dwExpectedFlags = UF_NORMAL_ACCOUNT |
                           UF_ACCOUNTDISABLE,
        .pszPassword     = NULL,
        .dwLevel         = 1,
        .bPreCleanup     = FALSE,
        .bPostCleanup    = TRUE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .pszUserName     = "testuser1",
        .pszComment      = "### Comment testuser1 ###",
        .pszDescription  = "### Description testuser1 ###",
        .pszHomeDir      = "c:\\tmp",
        .pszScriptPath   = "n:\\netlogon\\logon.cmd",
        .dwPrivilege     = 0,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .dwExpectedFlags = UF_NORMAL_ACCOUNT |
                           UF_ACCOUNTDISABLE,
        .pszPassword     = NULL,
        .dwLevel         = 1,
        .bPreCleanup     = FALSE,
        .bPostCleanup    = TRUE,
        .dwError         = ERROR_INVALID_PARAMETER,
        .dwParmError     = USER_PRIV_PARMNUM
    }
};


typedef struct _TEST_NET_USER_DEL
{
    PSTR    pszUserName;
    BOOLEAN bPrepare;
    BOOLEAN bCleanup;
    DWORD   dwError;

} TEST_NET_USER_DEL, *PTEST_NET_USER_DEL;


static
TEST_NET_USER_DEL LocalUserDelValidationTest[] = {
    {
        .pszUserName   = "testuser1",
        .bPrepare      = TRUE,
        .bCleanup      = TRUE,
        .dwError       = ERROR_SUCCESS
    },
    {
        .pszUserName   = "testuser2",
        .bPrepare      = TRUE,
        .bCleanup      = TRUE,
        .dwError       = ERROR_SUCCESS
    },
    {
        .pszUserName   = "testuser2",
        .bPrepare      = FALSE,
        .bCleanup      = TRUE,
        .dwError       = NERR_UserNotFound
    },
    {
        .pszUserName   = "testuser1",
        .bPrepare      = TRUE,
        .bCleanup      = TRUE,
        .dwError       = ERROR_SUCCESS
    },
    {
        .pszUserName   = "testuser1",
        .bPrepare      = TRUE,
        .bCleanup      = TRUE,
        .dwError       = ERROR_SUCCESS
    }
};


typedef struct _TEST_NET_USER_SET_INFO
{
    DWORD   dwLevel;
    PSTR    pszUserName;
    PSTR    pszFullName;
    PSTR    pszComment;
    PSTR    pszDescription;
    PSTR    pszHomeDir;
    PSTR    pszScriptPath;
    DWORD   dwPrivilege;
    DWORD   dwFlags;
    DWORD   dwExpectedFlags;
    PSTR    pszPassword;
    BOOLEAN bPrepare;
    BOOLEAN bCleanup;
    DWORD   dwError;
    DWORD   dwParmError;

} TEST_NET_USER_SET_INFO, *PTEST_NET_USER_SET_INFO;


static
TEST_NET_USER_SET_INFO LocalUserSetInfoValidationTest[] = {
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .pszPassword     = NULL,
        .bPrepare        = TRUE,
        .bCleanup        = FALSE,
        /*
         * dwFlags has to include UF_ACCOUNTDISABLE because there's
         * no password set on the account yet
         */
        .dwError         = ERROR_PASSWORD_RESTRICTION,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .pszPassword     = "newPas88-+word",
        .bPrepare        = TRUE,
        .bCleanup        = FALSE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 2,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = "###Testing comment###",
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .pszPassword     = NULL,
        .bPrepare        = FALSE,
        .bCleanup        = FALSE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 2,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = "###Testing description###",
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = 0,
        .pszPassword     = NULL,
        .bPrepare        = FALSE,
        .bCleanup        = FALSE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = "###Testing homedir###",
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT,
        .pszPassword     = NULL,
        .bPrepare        = FALSE,
        .bCleanup        = FALSE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = "###Testing script path###",
        .dwPrivilege     = USER_PRIV_USER,
        .dwFlags         = UF_NORMAL_ACCOUNT | UF_ACCOUNTDISABLE,
        .pszPassword     = NULL,
        .bPrepare        = FALSE,
        .bCleanup        = FALSE,
        .dwError         = ERROR_SUCCESS,
        .dwParmError     = 0
    },
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_GUEST,
        .dwFlags         = 0,
        .pszPassword     = NULL,
        .bPrepare        = FALSE,
        .bCleanup        = FALSE,
        /*
         * dwPrivilege has to be the same as returned from NetUserGetInfo
         * and would have to be USER_PRIV_USER in this case
         */
        .dwError         = ERROR_INVALID_PARAMETER,
        .dwParmError     = USER_PRIV_PARMNUM
    },
    {
        .dwLevel         = 1,
        .pszUserName     = "testuser1",
        .pszFullName     = NULL,
        .pszComment      = NULL,
        .pszDescription  = NULL,
        .pszHomeDir      = NULL,
        .pszScriptPath   = NULL,
        .dwPrivilege     = USER_PRIV_GUEST,
        .dwFlags         = 0,
        .pszPassword     = "newPas88-+word",
        .bPrepare        = FALSE,
        .bCleanup        = TRUE,
        /*
         * dwPrivilege has to be the same as returned from NetUserGetInfo
         * and would have to be USER_PRIV_USER in this case
         */
        .dwError         = ERROR_INVALID_PARAMETER,
        .dwParmError     = USER_PRIV_PARMNUM
    }
};


static
DWORD
TestGetNetUserAddTestSet(
    PCSTR                pszTestSetName,
    PTEST_NET_USER_ADD  *ppTestSet,
    PDWORD               pdwNumTests
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST_NET_USER_ADD pTestSet = NULL;
    DWORD dwNumTests = 0;

    if (strcasecmp(pszTestSetName, "validation") == 0)
    {
        pTestSet = LocalUserAddValidationTest;
        dwNumTests = (sizeof(LocalUserAddValidationTest)/
                      sizeof(LocalUserAddValidationTest[0]));
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN_ERROR(dwError);

    *ppTestSet   = pTestSet;
    *pdwNumTests = dwNumTests;

error:
    return dwError;
}


static
DWORD
TestGetNetUserDelTestSet(
    PCSTR                pszTestSetName,
    PTEST_NET_USER_DEL  *ppTestSet,
    PDWORD               pdwNumTests
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST_NET_USER_DEL pTestSet = NULL;
    DWORD dwNumTests = 0;

    if (strcasecmp(pszTestSetName, "validation") == 0)
    {
        pTestSet = LocalUserDelValidationTest;
        dwNumTests = (sizeof(LocalUserDelValidationTest)/
                      sizeof(LocalUserDelValidationTest[0]));
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN_ERROR(dwError);

    *ppTestSet   = pTestSet;
    *pdwNumTests = dwNumTests;

error:
    return dwError;
}


static
DWORD
TestGetNetUserSetInfoTestSet(
    PCSTR                    pszTestSetName,
    PTEST_NET_USER_SET_INFO *ppTestSet,
    PDWORD                   pdwNumTests
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PTEST_NET_USER_SET_INFO pTestSet = NULL;
    DWORD dwNumTests = 0;

    if (strcasecmp(pszTestSetName, "validation") == 0)
    {
        pTestSet = LocalUserSetInfoValidationTest;
        dwNumTests = (sizeof(LocalUserSetInfoValidationTest)/
                      sizeof(LocalUserSetInfoValidationTest[0]));
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN_ERROR(dwError);

    *ppTestSet   = pTestSet;
    *pdwNumTests = dwNumTests;

error:
    return dwError;
}


static
DWORD
CleanupAccount(
    PCWSTR  pwszHostname,
    PWSTR   pwszUsername
    )
{
    DWORD err = ERROR_SUCCESS;

    err = NetUserDel(pwszHostname,
                     pwszUsername);
    if (err == NERR_UserNotFound)
    {
        err = ERROR_SUCCESS;
    }

    return err;
}


static
DWORD
EnsureUserAccount(
    PCWSTR     pwszHostname,
    PWSTR      pwszUsername,
    PBOOLEAN   pbCreated
    )
{
    DWORD err = ERROR_SUCCESS;
    USER_INFO_1 Info = {0};
    DWORD dwParmError = 0;

    Info.usri1_name  = pwszUsername;
    Info.usri1_priv  = USER_PRIV_USER;
    Info.usri1_flags = UF_NORMAL_ACCOUNT;

    err = NetUserAdd(pwszHostname,
                     1,
                     (PVOID)&Info,
                     &dwParmError);
    if (err == ERROR_SUCCESS)
    {
        *pbCreated = TRUE;
    }
    else if (err == NERR_UserExists)
    {
        *pbCreated = FALSE;
        err = ERROR_SUCCESS;
    }

    return err;
}


static
DWORD
CleanupLocalGroup(
    PCWSTR  pwszHostname,
    PWSTR   pwszAliasname
    )
{
    DWORD err = ERROR_SUCCESS;

    err = NetLocalGroupDel(pwszHostname,
                           pwszAliasname);
    if (err == NERR_GroupNotFound)
    {
        err = ERROR_SUCCESS;
    }

    return err;
}


static
DWORD
EnsureLocalGroup(
    PCWSTR     pwszHostname,
    PWSTR      pwszAliasname,
    PBOOLEAN  pbCreated
    )
{
    DWORD err = ERROR_SUCCESS;
    LOCALGROUP_INFO_0 Info = {0};
    DWORD dwParmError = 0;

    Info.lgrpi0_name = pwszAliasname;

    err = NetLocalGroupAdd(pwszHostname,
                           0,
                           (PVOID)&Info,
                           &dwParmError);
    if (err == ERROR_SUCCESS)
    {
        *pbCreated = TRUE;
    }
    else if (err == NERR_GroupExists)
    {
        *pbCreated = FALSE;
        err = ERROR_SUCCESS;
    }

    return err;
}


#ifdef UNUSED
static
int GetUserLocalGroups(const wchar16_t *hostname, wchar16_t *username,
                       LOCALGROUP_USERS_INFO_0 *grpinfo, UINT32 *entries)
{
    const UINT32 level = 1;
    const UINT32 flags = 0;
    const UINT32 pref_maxlen = (UINT32)(-1);

    NET_API_STATUS err = ERROR_SUCCESS;
    UINT32 total, parm_err;
    int i = 0;

    grpinfo = NULL;
    *entries = total = parm_err = 0;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(username);
    INPUT_ARG_UINT(i);
    INPUT_ARG_UINT(flags);
    INPUT_ARG_PTR(grpinfo);
    INPUT_ARG_UINT(pref_maxlen);

    CALL_NETAPI(err, NetUserGetLocalGroups(hostname, username, level, flags,
                                           (void*)&grpinfo, pref_maxlen,
                                           entries, &total));

    OUTPUT_ARG_PTR(grpinfo);
    OUTPUT_ARG_UINT(*entries);
    OUTPUT_ARG_UINT(total);

    if (grpinfo != NULL && *entries > 0 && total > 0) {
        VERBOSE(printf("\tGroups found:\n"));

        for (i = 0; i < *entries; i++) {

            wchar16_t *name;

            name = grpinfo[i].lgrui0_name;

            if (name != NULL) {

                if(((UINT16) name[0]) == 0) {
                    w16printfw(L"\tERROR: LOCALGROUP_USERS_INFO_0[%2d]"
                              L".lgrui0_name = \"\" (empty string)\n", i);
                    return -1;

                } else {
                    VERBOSE(w16printfw(L"\tLOCALGROUP_USERS_INFO_0[%2d]"
                                      L".lgrui0_name = \"%ws\"\n", i, name));
                }
            } else {
                printf("\tERROR: LOCALGROUP_USERS_INFO_0[%2d].lgrui0_name = NULL\n", i);
                return -1;
            }


        }
    } else if (grpinfo != NULL && (*entries == 0 || total == 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is zero while"
               "buffer pointer is non-null\n");
        return -1;

    } else if (grpinfo == NULL && (*entries != 0 || total != 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is non-zero while"
               "buffer pointer is null\n");
        return -1;
    }

    return err;
}


static
int GetLocalGroupMembers(const wchar16_t *hostname, const wchar16_t *aliasname,
                         LOCALGROUP_MEMBERS_INFO_3* info, UINT32 *entries)
{
    const UINT32 level = 3;
    const UINT32 prefmaxlen = (UINT32)(-1);
    NET_API_STATUS err;
    UINT32 total, resume;
    int i = 0;

    resume = 0;
    info = NULL;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(aliasname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_PTR(info);
    INPUT_ARG_UINT(prefmaxlen);

    CALL_NETAPI(err, NetLocalGroupGetMembers(hostname, aliasname, level,
                                             (void*) &info,
                                             prefmaxlen, entries,
                                             &total, &resume));

    OUTPUT_ARG_PTR(info);
    OUTPUT_ARG_UINT(*entries);
    OUTPUT_ARG_UINT(total);
    OUTPUT_ARG_UINT(resume);

    if (info != NULL && entries > 0 && total > 0) {
        VERBOSE(printf("\tMembers found:\n"));

        for (i = 0; i < *entries; i++) {
            wchar16_t *name;

            name = info[i].lgrmi3_domainandname;
            if (name != NULL) {

                if(((UINT16) name[0]) == 0) {
                    w16printfw(L"\tERROR: LOCALGROUP_MEMBERS_INFO_3[%2d]"
                              L".lgrmi3_domainandname = \"\"  (empty string)\n", i);
                    return -1;
                }

                else if(name[wc16slen(name) - 1] == (wchar16_t)'\\') {
                    //this is a ghost entry from a user which has been removed.  Ignore it.
                }

                else {
                    VERBOSE(w16printfw(L"\tLOCALGROUP_MEMBERS_INFO_3[%2d]"
                                      L".lgrmi3_domainandname = \"%ws\"\n", i, name));
                }
            }
            else {
                w16printfw(L"\tERROR: LOCALGROUP_MEMBERS_INFO_3[%2d]"
                          L".lgrmi3_domainandname = NULL\n", i);
                return -1;
            }
        }

    } else if (info == NULL && (entries != 0 || total != 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is non-zero while buffer pointer is null\n");
        return -1;
    }

    return err;
}


static
int AddUser(const wchar16_t *hostname, const wchar16_t *username)
{
    const char *comment = "sample comment";
    const char *home_directory = "c:\\";
    const char *script_path = "\\\\server\\share\\dir\\script.cmd";
    const char *password = "TestPassword06-?";
    const UINT32 flags = UF_NORMAL_ACCOUNT;

    NET_API_STATUS err = ERROR_SUCCESS;
    UINT32 level, parm_err;
    size_t comment_len, home_directory_len, script_path_len, password_len;
    USER_INFO_1 *info1;

    level = 1;
    info1 = (USER_INFO_1*) malloc(sizeof(USER_INFO_1));

    memset(info1, 0, sizeof(USER_INFO_1));
    info1->usri1_name = wc16sdup(username);

    comment_len = strlen(comment);
    info1->usri1_comment = (wchar16_t*) malloc((comment_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_comment, comment, comment_len + 1);

    home_directory_len = strlen(home_directory);
    info1->usri1_home_dir = (wchar16_t*) malloc((home_directory_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_home_dir, home_directory, home_directory_len);

    script_path_len = strlen(script_path);
    info1->usri1_script_path = (wchar16_t*) malloc((script_path_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_script_path, script_path, script_path_len + 1);

    password_len = strlen(password);
    info1->usri1_password = (wchar16_t*) malloc((password_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_password, password, password_len + 1);

    info1->usri1_flags = flags;
    info1->usri1_priv = USER_PRIV_USER;

    CALL_NETAPI(err, NetUserAdd(hostname, level, (void*)info1, &parm_err));

    LW_SAFE_FREE_MEMORY(info1->usri1_comment);
    LW_SAFE_FREE_MEMORY(info1->usri1_home_dir);
    LW_SAFE_FREE_MEMORY(info1->usri1_script_path);
    LW_SAFE_FREE_MEMORY(info1->usri1_password);
    LW_SAFE_FREE_MEMORY(info1->usri1_name);
    LW_SAFE_FREE_MEMORY(info1);

    return err;
}


static
int DelUser(const wchar16_t *hostname, const wchar16_t *username)
{
    return NetUserDel(hostname, username);
}


static
int AddLocalGroup(const wchar16_t *hostname, const wchar16_t *aliasname)
{
    const char *testcomment = "Sample comment";
    const UINT32 level = 1;

    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_INFO_1 info;
    UINT32 parm_err;
    size_t comment_size;
    wchar16_t *comment = NULL;

    comment_size = (strlen(testcomment) + 1) * sizeof(wchar16_t);
    comment = (wchar16_t*) malloc(comment_size);
    mbstowc16s(comment, testcomment, comment_size);

    info.lgrpi1_name    = wc16sdup(aliasname);
    info.lgrpi1_comment = comment;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_WSTR(info.lgrpi1_name);
    INPUT_ARG_WSTR(info.lgrpi1_comment);
    INPUT_ARG_UINT(parm_err);

    CALL_NETAPI(err, NetLocalGroupAdd(hostname, level, &info, &parm_err));
    
    OUTPUT_ARG_UINT(parm_err);

    LW_SAFE_FREE_MEMORY(info.lgrpi1_name);
    LW_SAFE_FREE_MEMORY(info.lgrpi1_comment);

    return err;
}
#endif // UNUSED


static
int DelLocalGroup(const wchar16_t *hostname, const wchar16_t *aliasname)
{
    return NetLocalGroupDel(hostname, aliasname);
}


#ifdef UNUSED
static
void DoCleanup(const wchar16_t *hostname, const wchar16_t *aliasname,
               const wchar16_t *username)
{
    DelUser(hostname, username);
    DelLocalGroup(hostname, aliasname);
}


static
int AddLocalGroupMember(const wchar16_t *hostname, const wchar16_t *aliasname,
                        const wchar16_t *domname, const wchar16_t *member)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};

    wchar16_t domain_member[512];

    sw16printfw(
            domain_member,
            sizeof(domain_member)/sizeof(domain_member[0]),
            L"%ws\\%ws",
            domname,
            member);
    memberinfo.lgrmi3_domainandname = (wchar16_t*)domain_member;

    CALL_NETAPI(err, NetLocalGroupAddMembers(hostname, aliasname, 3,
                                             &memberinfo, 1));

    return err;
}


static
int DelLocalGroupMember(const wchar16_t *hostname,
                        const wchar16_t *domname,
                        const wchar16_t *aliasname,
                        const wchar16_t *member)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};

    wchar16_t host_member[512];

    sw16printfw(
            host_member,
            sizeof(host_member)/sizeof(host_member[0]),
            L"%ws\\%ws",
            domname,
            member);
    memberinfo.lgrmi3_domainandname = host_member;

    CALL_NETAPI(err, NetLocalGroupDelMembers(hostname, aliasname, 3,
                                             &memberinfo, 1));

    return err;
}


static
void DumpNetUserInfo1(const char *prefix, USER_INFO_1 *info)
{
    wchar16_t *usri1_name = info->usri1_name;
    wchar16_t *usri1_password = info->usri1_password;

    DUMP_WSTR(prefix, usri1_name);
    DUMP_WSTR(prefix, usri1_password);
}
#endif // UNUSED


static
BOOLEAN
CallNetUserEnum(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    DWORD  dwFilter
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwLastTotal = 0;
    DWORD dwCalculatedTotal = 0;
    DWORD dwTotal = 0;
    DWORD dwResume = 0;

    /* max buffer size below 10 bytes doesn't make much sense */
    while (dwMaxLen > 10)
    {
        dwCalculatedTotal = 0;
        dwLastTotal       = (DWORD)-1;

        do
        {
            err = NetUserEnum(pwszHostname,
                              dwLevel,
                              dwFilter,
                              &pBuffer,
                              dwMaxLen,
                              &dwNumEntries,
                              &dwTotal,
                              &dwResume);
            if (err != ERROR_SUCCESS &&
                err != ERROR_MORE_DATA &&
                err != ERROR_NOT_ENOUGH_MEMORY)
            {
                bRet = FALSE;
                goto done;
            }

            if (dwLastTotal != (DWORD)-1)
            {
                ASSERT_TEST(dwLastTotal == dwTotal);
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }

            dwLastTotal        = dwTotal;
            dwCalculatedTotal += dwNumEntries;
        }
        while (err == ERROR_MORE_DATA);

        if (dwMaxLen > 65536)
        {
            dwMaxLen /= 256;
        }
        else if (dwMaxLen <= 65536 && dwMaxLen > 512)
        {
            dwMaxLen /= 4;
        }
        else if (dwMaxLen <= 512)
        {
            dwMaxLen /= 2;
        }
        else if (dwMaxLen < 32)
        {
            dwMaxLen = 0;
        }

        ASSERT_TEST(dwCalculatedTotal == dwTotal);

        dwNumEntries = 0;
        dwTotal      = 0;
        dwResume     = 0;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
CallNetUserAdd(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszUsername,
    PWSTR  pwszDescription,
    PWSTR  pwszComment,
    PWSTR  pwszHomedir,
    PWSTR  pwszScriptPath,
    PWSTR  pwszPassword,
    DWORD  dwFlags,
    DWORD  dwPrivilege,
    DWORD  dwExpectedFlags,
    DWORD  dwExpectedError,
    DWORD  dwExpectedParmError
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    USER_INFO_1 Info1 = {0};
    USER_INFO_2 Info2 = {0};
    USER_INFO_3 Info3 = {0};
    USER_INFO_4 Info4 = {0};
    DWORD dwParmError = 0;

    switch (dwLevel)
    {
    case 1:
        Info1.usri1_name        = pwszUsername;
        Info1.usri1_password    = pwszPassword;
        Info1.usri1_priv        = dwPrivilege;
        Info1.usri1_home_dir    = pwszHomedir;
        Info1.usri1_comment     = pwszDescription;
        Info1.usri1_flags       = dwFlags;
        Info1.usri1_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info1;
        break;

    case 2:
        Info2.usri2_name        = pwszUsername;
        Info2.usri2_password    = pwszPassword;
        Info2.usri2_priv        = dwPrivilege;
        Info2.usri2_home_dir    = pwszHomedir;
        Info2.usri2_comment     = pwszDescription;
        Info2.usri2_flags       = dwFlags;
        Info2.usri2_script_path = pwszScriptPath;
        Info2.usri2_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info2;
        break;

    case 3:
        Info3.usri3_name        = pwszUsername;
        Info3.usri3_password    = pwszPassword;
        Info3.usri3_priv        = dwPrivilege;
        Info3.usri3_home_dir    = pwszHomedir;
        Info3.usri3_comment     = pwszDescription;
        Info3.usri3_flags       = dwFlags;
        Info3.usri3_script_path = pwszScriptPath;
        Info3.usri3_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info3;
        break;

    case 4:
        Info4.usri4_name        = pwszUsername;
        Info4.usri4_password    = pwszPassword;
        Info4.usri4_priv        = dwPrivilege;
        Info4.usri4_home_dir    = pwszHomedir;
        Info4.usri4_comment     = pwszDescription;
        Info4.usri4_flags       = dwFlags;
        Info4.usri4_script_path = pwszScriptPath;
        Info4.usri4_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info4;
        break;
    }

    err = NetUserAdd(pwszHostname,
                     dwLevel,
                     pBuffer,
                     &dwParmError);

    ret = (err == dwExpectedError &&
           dwParmError == dwExpectedParmError);

    return ret;
}


static
BOOLEAN
CallNetUserSetInfo(
    PCWSTR   pwszHostname,
    DWORD    dwLevel,
    PWSTR    pwszUsername,
    PWSTR    pwszFullName,
    PWSTR    pwszDescription,
    PWSTR    pwszComment,
    PWSTR    pwszHomedir,
    PWSTR    pwszScriptPath,
    PWSTR    pwszPassword,
    DWORD    dwFlags,
    DWORD    dwPrivilege,
    DWORD    dwExpectedError,
    DWORD    dwExpectedParmError,
    PBOOLEAN pbRenamed
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    USER_INFO_0 Info0 = {0};
    USER_INFO_1 Info1 = {0};
    USER_INFO_2 Info2 = {0};
    USER_INFO_3 Info3 = {0};
    USER_INFO_4 Info4 = {0};
    USER_INFO_1003 Info1003 = {0};
    USER_INFO_1007 Info1007 = {0};
    USER_INFO_1008 Info1008 = {0};
    USER_INFO_1011 Info1011 = {0};
    USER_INFO_1012 Info1012 = {0};
    DWORD dwParmError = 0;

    switch (dwLevel)
    {
    case 0:
        Info0.usri0_name        = pwszUsername;

        pBuffer = (PVOID)&Info0;
        break;

    case 1:
        Info1.usri1_name        = pwszUsername;
        Info1.usri1_password    = pwszPassword;
        Info1.usri1_priv        = dwPrivilege;
        Info1.usri1_home_dir    = pwszHomedir;
        Info1.usri1_comment     = pwszDescription;
        Info1.usri1_flags       = dwFlags;
        Info1.usri1_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info1;
        break;

    case 2:
        Info2.usri2_name        = pwszUsername;
        Info2.usri2_password    = pwszPassword;
        Info2.usri2_priv        = dwPrivilege;
        Info2.usri2_home_dir    = pwszHomedir;
        Info2.usri2_comment     = pwszDescription;
        Info2.usri2_flags       = dwFlags;
        Info2.usri2_script_path = pwszScriptPath;
        Info2.usri2_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info2;
        break;

    case 3:
        Info3.usri3_name        = pwszUsername;
        Info3.usri3_password    = pwszPassword;
        Info3.usri3_priv        = dwPrivilege;
        Info3.usri3_home_dir    = pwszHomedir;
        Info3.usri3_comment     = pwszDescription;
        Info3.usri3_flags       = dwFlags;
        Info3.usri3_script_path = pwszScriptPath;
        Info3.usri3_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info3;
        break;

    case 4:
        Info4.usri4_name        = pwszUsername;
        Info4.usri4_password    = pwszPassword;
        Info4.usri4_priv        = dwPrivilege;
        Info4.usri4_home_dir    = pwszHomedir;
        Info4.usri4_comment     = pwszDescription;
        Info4.usri4_flags       = dwFlags;
        Info4.usri4_script_path = pwszScriptPath;
        Info4.usri4_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info4;
        break;

    case 1003:
        Info1003.usri1003_password = pwszPassword;

        pBuffer = (PVOID)&Info1003;
        break;

    case 1007:
        Info1007.usri1007_comment = pwszDescription;

        pBuffer = (PVOID)&Info1007;
        break;

    case 1008:
        Info1008.usri1008_flags = dwFlags;

        pBuffer = (PVOID)&Info1008;
        break;

    case 1011:
        Info1011.usri1011_full_name = pwszFullName;

        pBuffer = (PVOID)&Info1011;
        break;

    case 1012:
        Info1012.usri1012_usr_comment = pwszComment;

        pBuffer = (PVOID)&Info1012;
        break;
    }

    err = NetUserSetInfo(pwszHostname,
                         pwszUsername,
                         dwLevel,
                         pBuffer,
                         &dwParmError);

    ret = (err == dwExpectedError &&
           dwParmError == dwExpectedParmError);

    if (dwLevel == 0 && pbRenamed)
    {
        *pbRenamed = TRUE;
    }

    return ret;
}


static
BOOL
CallNetUserGetInfo(
    PCWSTR   pwszHostname,
    PWSTR    pwszUsername,
    PVOID  **pppUserInfo,
    PDWORD   pdwNumUserInfos
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    DWORD dwLevels[] = { 0, 1, 2, 3, 4, 10, 11, 20, 23 };
    DWORD i = 0;
    PVOID *pBuffer = NULL;
    DWORD dwNumBuffers = sizeof(pBuffer)/sizeof(pBuffer[0]);
    DWORD dwLevel = 0;

    err = LwAllocateMemory(sizeof(pBuffer[0]) * 25,
                           OUT_PPVOID(&pBuffer));
    BAIL_ON_WIN_ERROR(err);

    for (i = 0; i < sizeof(dwLevels)/sizeof(dwLevels[0]); i++)
    {
        dwLevel = dwLevels[i];

        err = NetUserGetInfo(pwszHostname,
                             pwszUsername,
                             dwLevel,
                             &pBuffer[dwLevel]);
        if (err != ERROR_SUCCESS)
        {
            ret = FALSE;
        }
    }

    *pppUserInfo     = pBuffer;
    *pdwNumUserInfos = dwNumBuffers;

cleanup:
    return ret;

error:
    for (i = 0; i < 25; i++)
    {
        if (pBuffer[i])
        {
            NetApiBufferFree(pBuffer[i]);
            pBuffer[i] = NULL;
        }
    }

    LW_SAFE_FREE_MEMORY(pBuffer);
    pBuffer = NULL;

    goto cleanup;
}


static
BOOLEAN
TestVerifyUserInfo(
    PWSTR    pwszUsername,
    PWSTR    pwszFullName,
    PWSTR    pwszDescription,
    PWSTR    pwszComment,
    PWSTR    pwszHomedir,
    PWSTR    pwszScriptPath,
    PWSTR    pwszPassword,
    DWORD    dwFlags,
    DWORD    dwPrivilege,
    PVOID   *ppUserInfo
    )
{
    BOOLEAN bRet = TRUE;

    if (pwszUsername)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                                pwszUsername);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_1)ppUserInfo[1])->usri1_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_2)ppUserInfo[2])->usri2_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_10)ppUserInfo[10])->usri10_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_11)ppUserInfo[11])->usri11_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_20)ppUserInfo[20])->usri20_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_0)ppUserInfo[0])->usri0_name,
                            ((PUSER_INFO_23)ppUserInfo[23])->usri23_name);

    if (pwszFullName)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                                pwszFullName);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_full_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_full_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_10)ppUserInfo[10])->usri10_full_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_11)ppUserInfo[11])->usri11_full_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_20)ppUserInfo[20])->usri20_full_name);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_full_name,
                            ((PUSER_INFO_23)ppUserInfo[23])->usri23_full_name);

    if (pwszDescription)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                                pwszDescription);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_10)ppUserInfo[10])->usri10_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_11)ppUserInfo[11])->usri11_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_20)ppUserInfo[20])->usri20_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_comment,
                            ((PUSER_INFO_23)ppUserInfo[23])->usri23_comment);

    if (pwszComment)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_usr_comment,
                                pwszComment);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_usr_comment,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_usr_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_usr_comment,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_usr_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_usr_comment,
                            ((PUSER_INFO_10)ppUserInfo[10])->usri10_usr_comment);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_2)ppUserInfo[2])->usri2_usr_comment,
                            ((PUSER_INFO_11)ppUserInfo[11])->usri11_usr_comment);

    if (pwszHomedir)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                                pwszHomedir);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                            ((PUSER_INFO_2)ppUserInfo[2])->usri2_home_dir);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_home_dir);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_home_dir);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_home_dir);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_home_dir,
                            ((PUSER_INFO_11)ppUserInfo[11])->usri11_home_dir);

    if (pwszScriptPath)
    {
        ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_script_path,
                                pwszScriptPath);
    }

    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_script_path,
                            ((PUSER_INFO_2)ppUserInfo[2])->usri2_script_path);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_script_path,
                            ((PUSER_INFO_3)ppUserInfo[3])->usri3_script_path);
    ASSERT_WC16STRING_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_script_path,
                            ((PUSER_INFO_4)ppUserInfo[4])->usri4_script_path);

    if (dwFlags)
    {
        ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_flags,
                           ((PUSER_INFO_1)ppUserInfo[1])->usri1_flags | dwFlags);
    }

    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_flags,
                       ((PUSER_INFO_2)ppUserInfo[2])->usri2_flags);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_flags,
                       ((PUSER_INFO_3)ppUserInfo[3])->usri3_flags);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_flags,
                       ((PUSER_INFO_4)ppUserInfo[4])->usri4_flags);

    if (dwPrivilege)
    {
        ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                           dwPrivilege);
    }

    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                       ((PUSER_INFO_2)ppUserInfo[2])->usri2_priv);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                       ((PUSER_INFO_3)ppUserInfo[3])->usri3_priv);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                       ((PUSER_INFO_4)ppUserInfo[4])->usri4_priv);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                       ((PUSER_INFO_3)ppUserInfo[3])->usri3_priv);
    ASSERT_DWORD_EQUAL(((PUSER_INFO_1)ppUserInfo[1])->usri1_priv,
                       ((PUSER_INFO_11)ppUserInfo[11])->usri11_priv);

    return bRet;
}


static
BOOLEAN
CallNetUserGetLocalGroups(
    PCWSTR pwszHostname,
    PCWSTR pwszUserName,
    DWORD  dwLevel,
    DWORD  dwFlags
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwLastTotal = 0;
    DWORD dwCalculatedTotal = 0;
    DWORD dwTotal = 0;
    DWORD dwResume = 0;

    /* max buffer size below 10 bytes doesn't make much sense */
    while (dwMaxLen > 10)
    {
        dwCalculatedTotal = 0;
        dwLastTotal       = (DWORD)-1;

        err = NetUserGetLocalGroups(pwszHostname,
                                    pwszUserName,
                                    dwLevel,
                                    dwFlags,
                                    &pBuffer,
                                    dwMaxLen,
                                    &dwNumEntries,
                                    &dwTotal);
        if (err != ERROR_SUCCESS &&
            err != ERROR_MORE_DATA &&
            err != ERROR_NOT_ENOUGH_MEMORY)
        {
            bRet = FALSE;
            goto done;
        }

        if (dwLastTotal != (DWORD)-1)
        {
            ASSERT_TEST(dwLastTotal == dwTotal);
        }

        if (pBuffer)
        {
            NetApiBufferFree(pBuffer);
            pBuffer = NULL;
        }

        dwLastTotal        = dwTotal;
        dwCalculatedTotal += dwNumEntries;

        if (dwMaxLen > 65536)
        {
            dwMaxLen /= 256;
        }
        else if (dwMaxLen <= 65536 && dwMaxLen > 512)
        {
            dwMaxLen /= 4;
        }
        else if (dwMaxLen <= 512)
        {
            dwMaxLen /= 2;
        }
        else if (dwMaxLen < 32)
        {
            dwMaxLen = 0;
        }

        ASSERT_TEST(dwCalculatedTotal == dwTotal);

        dwNumEntries = 0;
        dwTotal      = 0;
        dwResume     = 0;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
CallNetLocalGroupEnum(
    PCWSTR pwszHostname,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwLastTotal = 0;
    DWORD dwCalculatedTotal = 0;
    DWORD dwTotal = 0;
    DWORD dwResume = 0;

    while (dwMaxLen)
    {
        dwCalculatedTotal = 0;
        dwLastTotal       = (DWORD)-1;

        do
        {
            err = NetLocalGroupEnum(pwszHostname,
                                    dwLevel,
                                    &pBuffer,
                                    dwMaxLen,
                                    &dwNumEntries,
                                    &dwTotal,
                                    &dwResume);
            if (err != ERROR_SUCCESS &&
                err != ERROR_MORE_DATA &&
                err != ERROR_NOT_ENOUGH_MEMORY)
            {
                bRet = FALSE;
                goto done;
            }

            if (dwLastTotal != (DWORD)-1)
            {
                ASSERT_TEST(dwLastTotal == dwTotal);
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }

            dwLastTotal        = dwTotal;
            dwCalculatedTotal += dwNumEntries;
        }
        while (err == ERROR_MORE_DATA);

        if (dwMaxLen > 65536)
        {
            dwMaxLen /= 256;
        }
        else if (dwMaxLen <= 65536 && dwMaxLen > 512)
        {
            dwMaxLen /= 4;
        }
        else if (dwMaxLen <= 512)
        {
            dwMaxLen /= 2;
        }
        else if (dwMaxLen < 32)
        {
            dwMaxLen = 0;
        }

        ASSERT_TEST(dwCalculatedTotal == dwTotal);

        dwNumEntries = 0;
        dwTotal      = 0;
        dwResume     = 0;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
CallNetLocalGroupAdd(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszAliasname,
    PWSTR  pwszComment
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    LOCALGROUP_INFO_0 Info0 = {0};
    LOCALGROUP_INFO_1 Info1 = {0};
    DWORD dwParmErr = 0;

    switch (dwLevel)
    {
    case 0:
        Info0.lgrpi0_name    = pwszAliasname;

        pBuffer = &Info0;
        break;

    case 1:
        Info1.lgrpi1_name    = pwszAliasname;
        Info1.lgrpi1_comment = pwszComment;

        pBuffer = &Info1;
        break;
    }

    err = NetLocalGroupAdd(pwszHostname,
                           dwLevel,
                           pBuffer,
                           &dwParmErr);
    ret = (err == ERROR_SUCCESS);

    return ret;
}


static
BOOLEAN
CallNetLocalGroupSetInfo(
    PCWSTR    pwszHostname,
    DWORD     dwLevel,
    PWSTR     pwszAliasname,
    PWSTR     pwszChangedAliasname,
    PWSTR     pwszComment,
    PBOOLEAN  pbRenamed
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    LOCALGROUP_INFO_0 Info0 = {0};
    LOCALGROUP_INFO_1 Info1 = {0};
    LOCALGROUP_INFO_1002 Info1002 = {0};
    DWORD dwParmErr = 0;

    switch (dwLevel)
    {
    case 0:
        Info0.lgrpi0_name       = pwszChangedAliasname;

        pBuffer = (PVOID)&Info0;
        break;

    case 1:
        /* Check if lgrpi1_name really is ignored */
        Info1.lgrpi1_name       = pwszChangedAliasname;
        Info1.lgrpi1_comment    = pwszComment;

        pBuffer = (PVOID)&Info1;
        break;

    case 1002:
        Info1002.lgrpi1002_comment = pwszComment;

        pBuffer = (PVOID)&Info1002;
        break;
    }

    err = NetLocalGroupSetInfo(pwszHostname,
                               pwszAliasname,
                               dwLevel,
                               pBuffer,
                               &dwParmErr);

    ret = (err == ERROR_SUCCESS);

    if (dwLevel == 0 && pbRenamed)
    {
        *pbRenamed = TRUE;
    }

    return ret;
}


static
BOOLEAN
CallNetLocalGroupGetInfo(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel
    )
{
    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;

    err = NetLocalGroupGetInfo(pwszHostname,
                               pwszAliasname,
                               dwLevel,
                               &pBuffer);
    if (err != ERROR_SUCCESS)
    {
        ret = FALSE;
    }

    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return ret;
}


static
BOOLEAN
CallNetLocalGroupGetMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszLocalGroupName,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwLastTotal = 0;
    DWORD dwCalculatedTotal = 0;
    DWORD dwTotal = 0;
    DWORD dwResume = 0;

    while (dwMaxLen)
    {
        dwCalculatedTotal = 0;
        dwLastTotal       = (DWORD)-1;

        do
        {
            err = NetLocalGroupGetMembers(pwszHostname,
                                          pwszLocalGroupName,
                                          dwLevel,
                                          &pBuffer,
                                          dwMaxLen,
                                          &dwNumEntries,
                                          &dwTotal,
                                          &dwResume);
            if (err != ERROR_SUCCESS &&
                err != ERROR_MORE_DATA &&
                err != ERROR_NOT_ENOUGH_MEMORY)
            {
                bRet = FALSE;
                goto done;
            }

            if (dwLastTotal != (DWORD)-1)
            {
                ASSERT_TEST(dwLastTotal == dwTotal);
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }

            dwLastTotal        = dwTotal;
            dwCalculatedTotal += dwNumEntries;
        }
        while (err == ERROR_MORE_DATA);

        if (dwMaxLen > 65536)
        {
            dwMaxLen /= 256;
        }
        else if (dwMaxLen <= 65536 && dwMaxLen > 512)
        {
            dwMaxLen /= 4;
        }
        else if (dwMaxLen <= 512)
        {
            dwMaxLen /= 2;
        }
        else if (dwMaxLen < 32)
        {
            dwMaxLen = 0;
        }

        ASSERT_TEST(dwCalculatedTotal == dwTotal);

        dwNumEntries = 0;
        dwTotal      = 0;
        dwResume     = 0;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
TestValidateDisplayUserInfo(
    PNET_DISPLAY_USER  pUser
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    size_t sUsernameLen = 0;

    ASSERT_TEST(pUser->usri1_name != NULL);

    if (pUser->usri1_name)
    {
        dwError = LwWc16sLen(pUser->usri1_name, &sUsernameLen);
        if (dwError)
        {
            bRet = FALSE;
            return bRet;
        }
    }

    ASSERT_TEST((pUser->usri1_flags & UF_NORMAL_ACCOUNT) ||
                (pUser->usri1_flags & UF_TEMP_DUPLICATE_ACCOUNT));
    ASSERT_TEST(pUser->usri1_user_id > 0);

    return bRet;
}


static
BOOLEAN
TestValidateDisplayMachineInfo(
    PNET_DISPLAY_MACHINE  pMachine
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    size_t sMachnameLen = 0;

    ASSERT_TEST(pMachine->usri2_name != NULL);

    if (pMachine->usri2_name)
    {
        dwError = LwWc16sLen(pMachine->usri2_name, &sMachnameLen);
        if (dwError)
        {
            bRet = FALSE;
            return bRet;
        }
    }

    ASSERT_TEST((pMachine->usri2_flags & UF_WORKSTATION_TRUST_ACCOUNT) ||
                (pMachine->usri2_flags & UF_SERVER_TRUST_ACCOUNT));
    ASSERT_TEST(pMachine->usri2_user_id > 0);

    return bRet;
}


static
BOOLEAN
TestValidateDisplayGroupInfo(
    PNET_DISPLAY_GROUP  pGroup
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    size_t sGroupnameLen = 0;

    ASSERT_TEST(pGroup->grpi3_name != NULL);

    if (pGroup->grpi3_name)
    {
        dwError = LwWc16sLen(pGroup->grpi3_name, &sGroupnameLen);
        if (dwError)
        {
            bRet = FALSE;
            return bRet;
        }
    }

    ASSERT_TEST(pGroup->grpi3_group_id > 0);

    return bRet;
}


static
BOOLEAN
TestValidateWkstaUserInfo(
    PVOID  pInfo,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    PWKSTA_USER_INFO_0 pInfo0 = (PWKSTA_USER_INFO_0)pInfo;
    PWKSTA_USER_INFO_1 pInfo1 = (PWKSTA_USER_INFO_1)pInfo;
    size_t sUsernameLen = 0;
    size_t sLogonDomainLen = 0;
    size_t sOthDomainsLen = 0;
    size_t sLogonServerLen = 0;

    if (dwLevel == 0 ||
        dwLevel == 1)
    {
        ASSERT_TEST(pInfo0->wkui0_username != NULL);

        if (pInfo0->wkui0_username)
        {
            /*
             * Assuming the account name can be up to 63-chars long
             */
            dwError = LwWc16sLen(pInfo0->wkui0_username, &sUsernameLen);
            if (dwError ||
                !(sUsernameLen > 0 && sUsernameLen < 64))
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    if (dwLevel == 1)
    {
        ASSERT_TEST(pInfo1->wkui1_logon_domain != NULL);
        if (pInfo1->wkui1_logon_domain)
        {
            /*
             * Assuming the domain name can be up to 15-chars long
             */
            dwError = LwWc16sLen(pInfo1->wkui1_logon_domain, &sLogonDomainLen);
            if (dwError ||
                !(sLogonDomainLen > 0 && sLogonDomainLen < 16))
            {
                bRet = FALSE;
                return bRet;
            }
        }

        ASSERT_TEST(pInfo1->wkui1_oth_domains != NULL);
        if (pInfo1->wkui1_oth_domains)
        {
            dwError = LwWc16sLen(pInfo1->wkui1_oth_domains, &sOthDomainsLen);
            if (dwError)
            {
                bRet = FALSE;
                return bRet;
            }
        }

        ASSERT_TEST(pInfo1->wkui1_logon_server != NULL);
        if (pInfo1->wkui1_logon_server)
        {
            dwError = LwWc16sLen(pInfo1->wkui1_logon_server, &sLogonServerLen);
            if (dwError)
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    return bRet;
}




static
BOOLEAN
TestValidateSessionInfo(
    PVOID  pInfo,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    PSESSION_INFO_0 pInfo0 = (PSESSION_INFO_0)pInfo;
    PSESSION_INFO_1 pInfo1 = (PSESSION_INFO_1)pInfo;
    PSESSION_INFO_2 pInfo2 = (PSESSION_INFO_2)pInfo;
    PSESSION_INFO_10 pInfo10 = (PSESSION_INFO_10)pInfo;
    PSESSION_INFO_502 pInfo502 = (PSESSION_INFO_502)pInfo;
    size_t sClientNameLen = 0;
    size_t sUserNameLen = 0;
    size_t sClientTypeNameLen = 0;
    size_t sTransportNameLen = 0;

    if (dwLevel == 0 ||
        dwLevel == 1 ||
        dwLevel == 2)
    {
        ASSERT_TEST(pInfo0->sesi0_cname != NULL);
        if (pInfo0->sesi0_cname)
        {
            dwError = LwWc16sLen(pInfo0->sesi0_cname, &sClientNameLen);
            if (dwError ||
                !(sClientNameLen > 0))
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    if (dwLevel == 1 ||
        dwLevel == 2 ||
        dwLevel == 10 ||
        dwLevel == 502)
    {
        ASSERT_TEST(pInfo1->sesi1_username != NULL);
        if (pInfo1->sesi1_username)
        {
            /*
             * Assuming the account name can be up to 63-chars long
             */
            dwError = LwWc16sLen(pInfo1->sesi1_username, &sUserNameLen);
            if (dwError ||
                !(sUserNameLen > 0))
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    if (dwLevel == 2 ||
        dwLevel == 502)
    {
        ASSERT_TEST(pInfo2->sesi2_time > 0);
    }

    if (dwLevel == 10)
    {
        ASSERT_TEST(pInfo10->sesi10_time > 0);
    }

    if (dwLevel == 2 ||
        dwLevel == 502)
    {
        ASSERT_TEST(pInfo2->sesi2_cltype_name != NULL);
        if (pInfo2->sesi2_cltype_name)
        {
            dwError = LwWc16sLen(pInfo2->sesi2_cltype_name,
                                 &sClientTypeNameLen);
            if (dwError ||
                !(sClientTypeNameLen > 0))
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    if (dwLevel == 502)
    {
        ASSERT_TEST(pInfo502->sesi502_transport != NULL);
        if (pInfo502->sesi502_transport)
        {
            dwError = LwWc16sLen(pInfo502->sesi502_transport,
                                 &sTransportNameLen);
            if (dwError ||
                !(sTransportNameLen > 0))
            {
                bRet = FALSE;
                return bRet;
            }
        }
    }

    return bRet;
}


static
BOOLEAN
TestValidateServerInfo(
    PVOID  pInfo,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    PSERVER_INFO_100 pInfo100 = (PSERVER_INFO_100)pInfo;
    size_t sServerNameLen = 0;

    if (dwLevel == 100 ||
        dwLevel == 101)
    {
        ASSERT_TEST(pInfo100->sv100_name != NULL);
        if (pInfo100->sv100_name)
        {
            dwError = LwWc16sLen(pInfo100->sv100_name, &sServerNameLen);
            if (dwError)
            {
                bRet = FALSE;
                return bRet;
            }

            ASSERT_TEST(sServerNameLen > 0);
        }
    }

    return bRet;
}


static
BOOLEAN
CallNetQueryDisplayInfo(
    PCWSTR pwszHostname,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwIndex = 0;
    DWORD dwRequested = 50;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwPrevTotalNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD iEntry = 0;
    PNET_DISPLAY_USER pDispUser = NULL;
    PNET_DISPLAY_MACHINE pDispMachine = NULL;
    PNET_DISPLAY_GROUP pDispGroup = NULL;

    /* max buffer size below 10 bytes doesn't make much sense */
    while (dwPrefMaxLen > 10)
    {
        do
        {
            err = NetQueryDisplayInformation(pwszHostname,
                                             dwLevel,
                                             dwIndex,
                                             dwRequested,
                                             dwPrefMaxLen,
                                             &dwNumEntries,
                                             &pBuffer);
            if (err != ERROR_SUCCESS &&
                err != ERROR_MORE_DATA)
            {
                bRet = FALSE;
                goto done;
            }

            ASSERT_TEST(dwNumEntries <= dwRequested);

            dwTotalNumEntries += dwNumEntries;

            for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
            {
                switch (dwLevel)
                {
                case 1:
                    pDispUser = &(((PNET_DISPLAY_USER)(pBuffer))[iEntry]);

                    bRet   &= TestValidateDisplayUserInfo(pDispUser);
                    dwIndex = pDispUser->usri1_next_index;
                    break;

                case 2:
                    pDispMachine = &(((PNET_DISPLAY_MACHINE)(pBuffer))[iEntry]);

                    bRet   &= TestValidateDisplayMachineInfo(pDispMachine);
                    dwIndex = pDispMachine->usri2_next_index;
                    break;

                case 3:
                    pDispGroup = &(((PNET_DISPLAY_GROUP)(pBuffer))[iEntry]);

                    bRet   &= TestValidateDisplayGroupInfo(pDispGroup);
                    dwIndex = pDispGroup->grpi3_next_index;
                    break;

                default:
                    bRet = FALSE;
                    goto done;
                }
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }
        }
        while (err == ERROR_MORE_DATA);

        if (dwPrefMaxLen > 65536)
        {
            dwPrefMaxLen /= 256;
        }
        else if (dwPrefMaxLen <= 65536 && dwPrefMaxLen > 512)
        {
            dwPrefMaxLen /= 4;
        }
        else if (dwPrefMaxLen <= 512)
        {
            dwPrefMaxLen /= 2;
        }
        else if (dwPrefMaxLen < 32)
        {
            dwPrefMaxLen = 0;
        }

        if (dwPrevTotalNumEntries)
        {
            ASSERT_TEST(dwPrevTotalNumEntries == dwTotalNumEntries);
        }

        dwPrevTotalNumEntries = dwTotalNumEntries;
        dwTotalNumEntries     = 0;
        dwIndex               = 0;
    }

    dwLevel      = 0;
    dwNumEntries = 0;
    err = NetQueryDisplayInformation(pwszHostname,
                                     dwLevel,
                                     dwIndex,
                                     dwRequested,
                                     dwPrefMaxLen,
                                     &dwNumEntries,
                                     &pBuffer);
    if (err != ERROR_INVALID_LEVEL)
    {
        bRet = FALSE;
        goto done;
    }

    if (pBuffer)
    {
        /*
         * There shouldn't be any buffer returned
         */
        bRet = FALSE;
        goto done;
    }

    dwLevel      = 4;
    dwNumEntries = 0;
    err = NetQueryDisplayInformation(pwszHostname,
                                     dwLevel,
                                     dwIndex,
                                     dwRequested,
                                     dwPrefMaxLen,
                                     &dwNumEntries,
                                     &pBuffer);
    if (err != ERROR_INVALID_LEVEL)
    {
        bRet = FALSE;
        goto done;
    }

    if (pBuffer)
    {
        /*
         * There shouldn't be any buffer returned
         */
        bRet = FALSE;
        goto done;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
CallNetWkstaUserEnum(
    PCWSTR pwszHostname,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PWSTR pwszHost = NULL;
    PVOID pBuffer = NULL;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwTotalCounted = 0;
    DWORD dwResume = 0;
    DWORD dwPrevTotalCounted = 0;
    DWORD iEntry = 0;
    PWKSTA_USER_INFO_0 pWkstaUserInfo0 = NULL;
    PWKSTA_USER_INFO_1 pWkstaUserInfo1 = NULL;

    err = LwAllocateWc16String(&pwszHost, pwszHostname);
    if (err)
    {
        bRet = FALSE;
        goto done;
    }

    /* max buffer size below 32 bytes doesn't make much sense */
    while (dwPrefMaxLen > 32)
    {
        do
        {
            err = NetWkstaUserEnum(pwszHost,
                                   dwLevel,
                                   &pBuffer,
                                   dwPrefMaxLen,
                                   &dwNumEntries,
                                   &dwTotalNumEntries,
                                   &dwResume);
            if (err != ERROR_SUCCESS &&
                err != ERROR_MORE_DATA)
            {
                bRet = FALSE;
                goto done;
            }

            dwTotalCounted    += dwNumEntries;
            dwTotalNumEntries  = 0;

            for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
            {
                switch (dwLevel)
                {
                case 0:
                    pWkstaUserInfo0 = &(((PWKSTA_USER_INFO_0)(pBuffer))[iEntry]);

                    bRet   &= TestValidateWkstaUserInfo(pWkstaUserInfo0, dwLevel);
                    break;

                case 1:
                    pWkstaUserInfo1 = &(((PWKSTA_USER_INFO_1)(pBuffer))[iEntry]);

                    bRet   &= TestValidateWkstaUserInfo(pWkstaUserInfo1, dwLevel);
                    break;

                default:
                    bRet = FALSE;
                    goto done;
                }
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }
        }
        while (err == ERROR_MORE_DATA);

        if (dwPrefMaxLen > 65536)
        {
            dwPrefMaxLen /= 256;
        }
        else if (dwPrefMaxLen <= 65536 && dwPrefMaxLen > 512)
        {
            dwPrefMaxLen /= 4;
        }
        else if (dwPrefMaxLen <= 512)
        {
            dwPrefMaxLen /= 2;
        }
        else if (dwPrefMaxLen < 32)
        {
            dwPrefMaxLen = 0;
        }

        if (dwPrevTotalCounted)
        {
            ASSERT_TEST(dwPrevTotalCounted == dwTotalCounted);
        }

        dwPrevTotalCounted  = dwTotalCounted;
        dwTotalCounted      = 0;
        dwResume            = 0;
    }

    dwLevel      = 2;
    dwNumEntries = 0;
    err = NetWkstaUserEnum(pwszHost,
                           dwLevel,
                           &pBuffer,
                           dwPrefMaxLen,
                           &dwNumEntries,
                           &dwTotalNumEntries,
                           &dwResume);
    if (err != ERROR_INVALID_LEVEL)
    {
        bRet = FALSE;
        goto done;
    }

    if (pBuffer)
    {
        /*
         * There shouldn't be any buffer returned
         */
        bRet = FALSE;
        goto done;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
BOOLEAN
CallNetSessionEnum(
    PCWSTR pwszHostname,
    PWSTR  pwszClientName,
    PWSTR  pwszUserName,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    PWSTR pwszHost = NULL;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwTotalCounted = 0;
    DWORD dwResume = 0;
    DWORD dwPrevTotalCounted = 0;
    DWORD iEntry = 0;
    PSESSION_INFO_0 pSessionInfo0 = NULL;
    PSESSION_INFO_1 pSessionInfo1 = NULL;
    PSESSION_INFO_2 pSessionInfo2 = NULL;
    PSESSION_INFO_10 pSessionInfo10 = NULL;
    PSESSION_INFO_502 pSessionInfo502 = NULL;

    err = LwAllocateWc16String(&pwszHost, pwszHostname);
    if (err)
    {
        bRet = FALSE;
        goto done;
    }

    /* max buffer size below 32 bytes doesn't make much sense */
    while (dwPrefMaxLen > 32)
    {
        do
        {
            err = NetSessionEnum(pwszHost,
                                 pwszClientName,
                                 pwszUserName,
                                 dwLevel,
                                 (PBYTE*)&pBuffer,
                                 dwPrefMaxLen,
                                 &dwNumEntries,
                                 &dwTotalNumEntries,
                                 &dwResume);
            if (err == NERR_BufTooSmall)
            {
                goto done;
            }
            else if (err != ERROR_SUCCESS &&
                     err != ERROR_MORE_DATA)
            {
                bRet = FALSE;
                goto done;
            }

            dwTotalCounted    += dwNumEntries;
            dwTotalNumEntries  = 0;

            for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
            {
                switch (dwLevel)
                {
                case 0:
                    pSessionInfo0 = &(((PSESSION_INFO_0)(pBuffer))[iEntry]);

                    bRet   &= TestValidateSessionInfo(pSessionInfo0, dwLevel);
                    break;

                case 1:
                    pSessionInfo1 = &(((PSESSION_INFO_1)(pBuffer))[iEntry]);

                    bRet   &= TestValidateSessionInfo(pSessionInfo1, dwLevel);
                    break;

                case 2:
                    pSessionInfo2 = &(((PSESSION_INFO_2)(pBuffer))[iEntry]);

                    bRet   &= TestValidateSessionInfo(pSessionInfo2, dwLevel);
                    break;

                case 10:
                    pSessionInfo10 = &(((PSESSION_INFO_10)(pBuffer))[iEntry]);

                    bRet   &= TestValidateSessionInfo(pSessionInfo10, dwLevel);
                    break;

                case 502:
                    pSessionInfo502 = &(((PSESSION_INFO_502)(pBuffer))[iEntry]);

                    bRet   &= TestValidateSessionInfo(pSessionInfo502, dwLevel);
                    break;

                default:
                    bRet = FALSE;
                    goto done;
                }
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }
        }
        while (err == ERROR_MORE_DATA);

        if (dwPrefMaxLen > 65536)
        {
            dwPrefMaxLen /= 256;
        }
        else if (dwPrefMaxLen <= 65536 && dwPrefMaxLen > 512)
        {
            dwPrefMaxLen /= 4;
        }
        else if (dwPrefMaxLen <= 512)
        {
            dwPrefMaxLen /= 2;
        }
        else if (dwPrefMaxLen < 32)
        {
            dwPrefMaxLen = 0;
        }

        if (dwPrevTotalCounted)
        {
            ASSERT_TEST(dwPrevTotalCounted == dwTotalCounted);
        }

        dwPrevTotalCounted  = dwTotalCounted;
        dwTotalCounted      = 0;
        dwResume            = 0;
    }

    dwLevel      = 20;
    dwNumEntries = 0;
    err = NetSessionEnum(pwszHost,
                         pwszClientName,
                         pwszUserName,
                         dwLevel,
                         (PBYTE*)&pBuffer,
                         dwPrefMaxLen,
                         &dwNumEntries,
                         &dwTotalNumEntries,
                         &dwResume);
    if (err != ERROR_INVALID_LEVEL)
    {
        bRet = FALSE;
        goto done;
    }

    if (pBuffer)
    {
        /*
         * There shouldn't be any buffer returned
         */
        bRet = FALSE;
        goto done;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    LW_SAFE_FREE_MEMORY(pwszHost);

    return bRet;
}


static
BOOLEAN
CallNetServerEnum(
    PCWSTR pwszHostname,
    PWSTR  pwszDomain,
    DWORD  dwLevel
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwNumEntries = 0;
    DWORD dwTotalNumEntries = 0;
    DWORD dwTotalCounted = 0;
    DWORD dwResume = 0;
    DWORD dwPrevTotalCounted = 0;
    DWORD iEntry = 0;
    DWORD dwServerType = SV_TYPE_DOMAIN_CTRL;
    PSERVER_INFO_100 pServerInfo100 = NULL;
    PSERVER_INFO_101 pServerInfo101 = NULL;

    /* max buffer size below 10 bytes doesn't make much sense */
    while (dwPrefMaxLen > 10)
    {
        do
        {
            err = NetServerEnum(pwszHostname,
                                dwLevel,
                                &pBuffer,
                                dwPrefMaxLen,
                                &dwNumEntries,
                                &dwTotalNumEntries,
                                dwServerType,
                                pwszDomain,
                                &dwResume
                                );
            if (err == NERR_BufTooSmall)
            {
                goto done;
            }
            else if (err != ERROR_SUCCESS &&
                     err != ERROR_MORE_DATA)
            {
                bRet = FALSE;
                goto done;
            }

            ASSERT_TEST(pBuffer != NULL);
            ASSERT_TEST(dwNumEntries > 0);
            ASSERT_TEST(dwTotalNumEntries >= dwNumEntries);

            dwTotalCounted    += dwNumEntries;
            dwTotalNumEntries  = 0;

            for (iEntry = 0; iEntry < dwNumEntries; iEntry++)
            {
                switch (dwLevel)
                {
                case 100:
                    pServerInfo100 = &(((PSERVER_INFO_100)(pBuffer))[iEntry]);

                    bRet  &= TestValidateServerInfo(pServerInfo100, dwLevel);
                    break;

                case 101:
                    pServerInfo101 = &(((PSERVER_INFO_101)(pBuffer))[iEntry]);

                    bRet  &= TestValidateServerInfo(pServerInfo101, dwLevel);
                    break;

                default:
                    bRet = FALSE;
                    goto done;
                }
            }

            if (pBuffer)
            {
                NetApiBufferFree(pBuffer);
                pBuffer = NULL;
            }
        }
        while (err == ERROR_MORE_DATA);

        if (dwPrefMaxLen > 65536)
        {
            dwPrefMaxLen /= 256;
        }
        else if (dwPrefMaxLen <= 65536 && dwPrefMaxLen > 512)
        {
            dwPrefMaxLen /= 4;
        }
        else if (dwPrefMaxLen <= 512)
        {
            dwPrefMaxLen /= 2;
        }
        else if (dwPrefMaxLen < 32)
        {
            dwPrefMaxLen = 0;
        }

        if (dwPrevTotalCounted)
        {
            ASSERT_TEST(dwPrevTotalCounted == dwTotalCounted);
        }

        dwPrevTotalCounted  = dwTotalCounted;
        dwTotalCounted      = 0;
        dwResume            = 0;
    }

    dwLevel      = 50;
    dwNumEntries = 0;
    err = NetServerEnum(pwszHostname,
                        dwLevel,
                        &pBuffer,
                        dwPrefMaxLen,
                        &dwNumEntries,
                        &dwTotalNumEntries,
                        dwServerType,
                        pwszDomain,
                        &dwResume
                        );
    if (err != ERROR_INVALID_LEVEL)
    {
        bRet = FALSE;
        goto done;
    }

    if (pBuffer)
    {
        /*
         * There shouldn't be any buffer returned
         */
        bRet = FALSE;
        goto done;
    }

done:
    if (pBuffer)
    {
        NetApiBufferFree(pBuffer);
    }

    return bRet;
}


static
DWORD
TestNetUserEnum(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultFilter = 0;
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 20 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD dwFilter = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "filter", pt_uint32,
                       (UINT32*)&dwFilter, (UINT32*)&dwDefaultFilter);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("filter", pt_uint32, &dwFilter);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        if (!dwFilter)
        {
            ret &= CallNetUserEnum(pwszHostname,
                                   dwLevel,
                                   FILTER_NORMAL_ACCOUNT);

            ret &= CallNetUserEnum(pwszHostname,
                                   dwLevel,
                                   FILTER_INTERDOMAIN_TRUST_ACCOUNT);

            ret &= CallNetUserEnum(pwszHostname,
                                   dwLevel,
                                   FILTER_WORKSTATION_TRUST_ACCOUNT);

            ret &= CallNetUserEnum(pwszHostname,
                                   dwLevel,
                                   FILTER_SERVER_TRUST_ACCOUNT);
        }
        else
        {
            ret &= CallNetUserEnum(pwszHostname,
                                   dwLevel,
                                   dwFilter);
        }
    }
    return ret;
}


static
BOOLEAN
CallNetUserDel(
    PCWSTR  pwszHostname,
    PWSTR   pwszUsername,
    DWORD   dwExpectedError
    )
{
    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;

    err = NetUserDel(pwszHostname,
                     pwszUsername);

    bRet = (err == dwExpectedError);

    return bRet;
}


static
DWORD
TestNetUserAdd(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultTestSetName = "validation";

    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    DWORD dwLevel = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszScriptPath = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwFlags = 0;
    DWORD dwPrivilege = 0;
    DWORD dwExpectedFlags = 0;
    DWORD dwExpectedError = 0;
    DWORD dwExpectedParmError = 0;
    PSTR pszTestSetName = NULL;
    PTEST_NET_USER_ADD pTestSet = NULL;
    DWORD dwNumTests = 0;
    DWORD i = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       &pszTestSetName, &pszDefaultTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("testset", pt_string, &pszTestSetName);

    err = TestGetNetUserAddTestSet(pszTestSetName,
                                   &pTestSet,
                                   &dwNumTests);
    if (err != 0) netapi_fail(err);

    for (i = 0; i < dwNumTests; i++)
    {
        dwLevel = pTestSet[i].dwLevel;

        if (pTestSet[i].pszUserName)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszUserName,
                               &pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszComment)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszComment,
                               &pwszComment);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszDescription)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszDescription,
                               &pwszDescription);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszHomeDir)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszHomeDir,
                               &pwszHomedir);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszScriptPath)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszScriptPath,
                               &pwszScriptPath);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszPassword)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszPassword,
                               &pwszPassword);
            if (err != 0) netapi_fail(err);
        }

        dwFlags             = pTestSet[i].dwFlags;
        dwPrivilege         = pTestSet[i].dwPrivilege;
        dwExpectedFlags     = pTestSet[i].dwExpectedFlags;
        dwExpectedError     = pTestSet[i].dwError;
        dwExpectedParmError = pTestSet[i].dwParmError;

        if (pTestSet[i].bPreCleanup)
        {
            err = CleanupAccount(pwszHostname,
                                 pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        ret &= CallNetUserAdd(pwszHostname,
                              dwLevel,
                              pwszUsername,
                              pwszDescription,
                              pwszComment,
                              pwszHomedir,
                              pwszScriptPath,
                              pwszPassword,
                              dwFlags,
                              dwPrivilege,
                              dwExpectedFlags,
                              dwExpectedError,
                              dwExpectedParmError);

        if (pTestSet[i].bPostCleanup)
        {
            err = CleanupAccount(pwszHostname,
                                 pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        LW_SAFE_FREE_MEMORY(pwszUsername);
        LW_SAFE_FREE_MEMORY(pwszDescription);
        LW_SAFE_FREE_MEMORY(pwszComment);
        LW_SAFE_FREE_MEMORY(pwszHomedir);
        LW_SAFE_FREE_MEMORY(pwszScriptPath);
        LW_SAFE_FREE_MEMORY(pwszPassword);

        pwszUsername    = NULL;
        pwszDescription = NULL;
        pwszComment     = NULL;
        pwszHomedir     = NULL;
        pwszScriptPath  = NULL;
        pwszPassword    = NULL;
    }

done:
    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszDescription);
    LW_SAFE_FREE_MEMORY(pwszComment);
    LW_SAFE_FREE_MEMORY(pwszHomedir);
    LW_SAFE_FREE_MEMORY(pwszScriptPath);
    LW_SAFE_FREE_MEMORY(pwszPassword);

    return (ret &&
            err == ERROR_SUCCESS);
}


static
DWORD
TestNetUserDel(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultTestSetName = "validation";

    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    PTEST_NET_USER_DEL pTestSet = NULL;
    DWORD dwNumTests = 0;
    DWORD i = 0;
    PWSTR pwszUsername = NULL;
    BOOLEAN bCreated = FALSE;
    DWORD dwExpectedError = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       &pszTestSetName, &pszDefaultTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("testset", pt_string, &pszTestSetName);

    err = TestGetNetUserDelTestSet(pszTestSetName,
                                   &pTestSet,
                                   &dwNumTests);
    if (err != 0) netapi_fail(err);

    for (i = 0; i < dwNumTests; i++)
    {
        if (pTestSet[i].pszUserName)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszUserName,
                               &pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        dwExpectedError = pTestSet[i].dwError;

        if (pTestSet[i].bPrepare)
        {
            err = EnsureUserAccount(pwszHostname,
                                    pwszUsername,
                                    &bCreated);
            if (err != 0) netapi_fail(err);
        }

        ret &= CallNetUserDel(pwszHostname,
                              pwszUsername,
                              dwExpectedError);

        if (pTestSet[i].bCleanup && bCreated)
        {
            err = EnsureUserAccount(pwszHostname,
                                    pwszUsername,
                                    &bCreated);
            if (err != 0) netapi_fail(err);
        }

        LW_SAFE_FREE_MEMORY(pwszUsername);

        pwszUsername = NULL;
    }

done:
    LW_SAFE_FREE_MEMORY(pwszUsername);

    return (ret &&
            err == ERROR_SUCCESS);
}


static
DWORD
TestNetUserGetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const PSTR pszDefaultUsername = "TestUser";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszUsername = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 20 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOLEAN bCreated = FALSE;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, pwszUsername);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        err = EnsureUserAccount(pwszHostname,
                                pwszUsername,
                                &bCreated);
        if (err != 0) netapi_fail(err);

        if (bCreated)
        {
            err = NetUserDel(pwszHostname,
                             pwszUsername);
            if (err == ERROR_NO_SUCH_USER)
            {
                err = ERROR_SUCCESS;
            }
            else
            {
                netapi_fail(status);
            }
        }
    }

done:
    LW_SAFE_FREE_MEMORY(pwszUsername);

    return ret;
}


static
DWORD
TestNetUserSetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultTestSetName = "validation";

    BOOLEAN bRet = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszTestSetName = NULL;
    PTEST_NET_USER_SET_INFO pTestSet = NULL;
    DWORD dwNumTests = 0;
    DWORD dwLevel = 0;
    DWORD i = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    PWSTR pwszChangedUsername = NULL;
    PWSTR pwszDescription = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszScriptPath = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwFlags = 0;
    DWORD dwPrivilege = 0;
    DWORD dwExpectedError = 0;
    DWORD dwExpectedParmError = 0;
    BOOLEAN bCreated = FALSE;
    BOOLEAN bRenamed = FALSE;
    PVOID *ppUserInfo = NULL;
    DWORD dwNumUserInfos = 0;
    DWORD iUserInfo = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "testset", pt_string,
                       &pszTestSetName, &pszDefaultTestSetName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("testset", pt_string, &pszTestSetName);

    err = TestGetNetUserSetInfoTestSet(pszTestSetName,
                                       &pTestSet,
                                       &dwNumTests);
    if (err != 0) netapi_fail(err);
    
    for (i = 0; i < dwNumTests; i++)
    {
        dwLevel = pTestSet[i].dwLevel;

        if (pTestSet[i].pszUserName)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszUserName,
                               &pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].bPrepare)
        {
            err = EnsureUserAccount(pwszHostname,
                                    pwszUsername,
                                    &bCreated);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszFullName)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszFullName,
                               &pwszFullName);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszComment)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszComment,
                               &pwszComment);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszHomeDir)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszHomeDir,
                               &pwszHomedir);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszScriptPath)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszScriptPath,
                               &pwszScriptPath);
            if (err != 0) netapi_fail(err);
        }

        if (pTestSet[i].pszPassword)
        {           
            err = LwMbsToWc16s(pTestSet[i].pszPassword,
                               &pwszPassword);
            if (err != 0) netapi_fail(err);
        }

        dwFlags             = pTestSet[i].dwFlags;
        dwPrivilege         = pTestSet[i].dwPrivilege;
        dwExpectedError     = pTestSet[i].dwError;
        dwExpectedParmError = pTestSet[i].dwParmError;

        bRet &= CallNetUserSetInfo(pwszHostname,
                                   dwLevel,
                                   pwszUsername,
                                   pwszFullName,
                                   pwszDescription,
                                   pwszComment,
                                   pwszHomedir,
                                   pwszScriptPath,
                                   pwszPassword,
                                   dwFlags,
                                   dwPrivilege,
                                   dwExpectedError,
                                   dwExpectedParmError,
                                   &bRenamed);

        bRet &= CallNetUserGetInfo(pwszHostname,
                                   pwszUsername,
                                   &ppUserInfo,
                                   &dwNumUserInfos);

        bRet &= TestVerifyUserInfo(pwszUsername,
                                   pwszFullName,
                                   pwszDescription,
                                   pwszComment,
                                   pwszHomedir,
                                   pwszScriptPath,
                                   pwszPassword,
                                   dwFlags,
                                   dwPrivilege,
                                   ppUserInfo);

        if (pTestSet[i].bCleanup)
        {
            err = CleanupAccount(pwszHostname,
                                 pwszUsername);
            if (err != 0) netapi_fail(err);
        }

        for (iUserInfo = 0;
             iUserInfo < dwNumUserInfos;
             iUserInfo++)
        {
            NetApiBufferFree(ppUserInfo[iUserInfo]);
            ppUserInfo[iUserInfo] = NULL;
        }

        LW_SAFE_FREE_MEMORY(pwszUsername);
        LW_SAFE_FREE_MEMORY(pwszFullName);
        LW_SAFE_FREE_MEMORY(pwszDescription);
        LW_SAFE_FREE_MEMORY(pwszComment);
        LW_SAFE_FREE_MEMORY(pwszHomedir);
        LW_SAFE_FREE_MEMORY(pwszScriptPath);
        LW_SAFE_FREE_MEMORY(pwszPassword);

        pwszUsername    = NULL;
        pwszFullName    = NULL;
        pwszDescription = NULL;
        pwszComment     = NULL;
        pwszHomedir     = NULL;
        pwszScriptPath  = NULL;
        pwszPassword    = NULL;
    }

done:
    for (iUserInfo = 0;
         iUserInfo < dwNumUserInfos;
         iUserInfo++)
    {
        NetApiBufferFree(ppUserInfo[iUserInfo]);
    }

    LW_SAFE_FREE_MEMORY(pwszUsername);
    LW_SAFE_FREE_MEMORY(pwszFullName);
    LW_SAFE_FREE_MEMORY(pwszDescription);
    LW_SAFE_FREE_MEMORY(pwszComment);
    LW_SAFE_FREE_MEMORY(pwszHomedir);
    LW_SAFE_FREE_MEMORY(pwszScriptPath);
    LW_SAFE_FREE_MEMORY(pwszPassword);
    LW_SAFE_FREE_MEMORY(pwszChangedUsername);

    return (bRet &&
            err == ERROR_SUCCESS);
}


static
DWORD
TestNetUserGetLocalGroups(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const PSTR pszDefaultUsername = "TestUser";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD err = ERROR_SUCCESS;
    PWSTR pwszUsername = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOLEAN bCreated = FALSE;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, pwszUsername);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        err = EnsureUserAccount(pwszHostname,
                                pwszUsername,
                                &bCreated);
        if (err != 0) netapi_fail(err);

        ret &= CallNetUserGetLocalGroups(pwszHostname,
                                         pwszUsername,
                                         dwLevel,
                                         0);
        if (bCreated)
        {
            err = CleanupAccount(pwszHostname,
                                 pwszUsername);
            if (err != 0) netapi_fail(err);
        }
    }

done:
    LW_SAFE_FREE_MEMORY(pwszUsername);

    return ret;
}


static
DWORD
TestNetJoinDomain(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const char *def_accountou = NULL;
    const int def_rejoin = 0;
    const int def_create = 1;
    const int def_deferspn = 0;

    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    PWSTR username = NULL;
    PWSTR password = NULL;
    PWSTR domain_name = NULL;
    PWSTR dnsDomainName = NULL;
    PWSTR accountou = NULL;
    PWSTR joined_domain_name = NULL;
    PWSTR machine_account = NULL;
    PWSTR machine_password = NULL;
    int rejoin, create, deferspn;
    int opts;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string,
                       &domain_name, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "accountou", pt_w16string,
                       &accountou, &def_accountou);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "rejoin", pt_int32,
                       &rejoin, &def_rejoin);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "create", pt_int32,
                       &create, &def_create);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "deferspn", pt_int32,
                       &deferspn, &def_deferspn);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("domainname", pt_w16string, domain_name);
    PARAM_INFO("accountou", pt_w16string, accountou);
    PARAM_INFO("rejoin", pt_int32, &rejoin);
    PARAM_INFO("create", pt_int32, &create);
    PARAM_INFO("deferspn", pt_int32, &deferspn);

    opts = NETSETUP_JOIN_DOMAIN;
    if (create) opts |= NETSETUP_ACCT_CREATE;
    if (rejoin) opts |= NETSETUP_DOMAIN_JOIN_IF_JOINED;
    if (deferspn) opts |= NETSETUP_DEFER_SPN_SET;

    CALL_NETAPI(err, NetJoinDomain(pwszHostname, domain_name, accountou,
                                   username, password, opts));
    if (err != 0) netapi_fail(err);

    err = GetMachinePassword(
                    &dnsDomainName,
                    &joined_domain_name,
                    &machine_account,
                    &machine_password,
                    NULL);
    if (err != 0) netapi_fail(err);

    RESULT_WSTR(joined_domain_name);
    RESULT_WSTR(machine_account);
    RESULT_WSTR(machine_password);

done:
    LW_SAFE_FREE_MEMORY(username);
    LW_SECURE_FREE_WSTRING(password);
    LW_SAFE_FREE_MEMORY(domain_name);
    LW_SAFE_FREE_MEMORY(accountou);
    LW_SAFE_FREE_MEMORY(joined_domain_name);
    LW_SAFE_FREE_MEMORY(machine_account);
    LW_SECURE_FREE_WSTRING(machine_password);

    return (err == ERROR_SUCCESS);
}


static
DWORD
TestNetUnjoinDomain(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const int def_disable = 1;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *password;
    int disable;
    int opts;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "disable", pt_int32,
                       &disable, &def_disable);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("disable", pt_int32, &disable);

    opts = (disable) ? NETSETUP_ACCT_DELETE : 0;

    CALL_NETAPI(err, NetUnjoinDomain(NULL, username, password, opts));
    if (err != 0) netapi_fail(err);

done:
    LW_SAFE_FREE_MEMORY(username);
    LW_SAFE_FREE_MEMORY(password);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


static
DWORD
TestNetUserChangePassword(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const char *defusername = "TestUser";
    const char *defoldpass = "";
    const char *defnewpass = "newpassword";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *oldpassword, *newpassword;
    BOOLEAN bCreated = FALSE;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "oldpassword", pt_w16string,
                       &oldpassword, &defoldpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "newpassword", pt_w16string,
                       &newpassword, &defnewpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("oldpassword", pt_w16string, oldpassword);
    PARAM_INFO("newpassword", pt_w16string, newpassword);

    err = EnsureUserAccount(pwszHostname,
                            username,
                            &bCreated);
    if (err != 0) netapi_fail(err);

    CALL_NETAPI(err, NetUserChangePassword(pwszHostname,
                                           username,
                                           oldpassword,
                                           newpassword));
    if (err != 0) netapi_fail(err);

done:
    LW_SAFE_FREE_MEMORY(username);
    LW_SAFE_FREE_MEMORY(oldpassword);
    LW_SAFE_FREE_MEMORY(newpassword);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


static
DWORD
TestNetLocalGroupEnum(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        ret &= CallNetLocalGroupEnum(pwszHostname,
                                     dwLevel);
    }

    return ret;
}


static
DWORD
TestNetLocalGroupAdd(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultAliasname = "TestLocalGroup";
    PCSTR pszDefaultComment = "Test comment for new local group";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    DWORD err = ERROR_SUCCESS;
    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1 };
    DWORD dwLevel = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwNumLevels = 0;
    PWSTR pwszAliasname = NULL;
    PWSTR pwszComment = NULL;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, pwszAliasname);
    PARAM_INFO("comment", pt_w16string, pwszComment);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        err = CleanupLocalGroup(pwszHostname,
                                pwszAliasname);
        if (err != 0) netapi_fail(err);

        ret &= CallNetLocalGroupAdd(pwszHostname,
                                    dwLevel,
                                    pwszAliasname,
                                    pwszComment);
    }

    err = CleanupLocalGroup(pwszHostname,
                            pwszAliasname);
    if (err != 0) netapi_fail(err);

done:
    LW_SAFE_FREE_MEMORY(pwszAliasname);
    LW_SAFE_FREE_MEMORY(pwszComment);

    return ret;
}


static
DWORD
TestNetLocalGroupDel(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const char *def_aliasname = "TestAlias";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *aliasname;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);

    err = DelLocalGroup(pwszHostname, aliasname);
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:
    LW_SAFE_FREE_MEMORY(aliasname);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


static
DWORD
TestNetLocalGroupGetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const PSTR pszDefaultAliasname = "TestLocalGroup";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD err = ERROR_SUCCESS;
    PWSTR pwszAliasname = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 1 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOLEAN bCreated = FALSE;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, pwszAliasname);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        err = EnsureLocalGroup(pwszHostname,
                               pwszAliasname,
                               &bCreated);
        if (err != 0) netapi_fail(err);

        ret &= CallNetLocalGroupGetInfo(pwszHostname,
                                        pwszAliasname,
                                        dwLevel);

        if (bCreated)
        {
            err = CleanupLocalGroup(pwszHostname,
                                    pwszAliasname);
            if (err != 0) netapi_fail(err);
        }
    }

done:
    LW_SAFE_FREE_MEMORY(pwszAliasname);

    return ret;
}


static
DWORD
TestNetLocalGroupSetInfo(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultAliasname = "TestLocalGroup";
    PCSTR pszChangedAliasname = "TestLocalGroupRename";
    PCSTR pszDefaultComment = "Test comment for local group";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 1002 };
    DWORD dwLevel = 0;
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    PWSTR pwszAliasname = NULL;
    PWSTR pwszChangedAliasname = NULL;
    PWSTR pwszComment = NULL;
    DWORD i = 0;
    BOOLEAN bCreated = FALSE;
    BOOLEAN bRenamed = FALSE;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    err = LwMbsToWc16s(pszChangedAliasname,
                       &pwszChangedAliasname);
    if (err != 0) netapi_fail(err);

    PARAM_INFO("aliasname", pt_w16string, &pwszAliasname);
    PARAM_INFO("aliasnamechange", pt_w16string, &pwszChangedAliasname);
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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        err = EnsureLocalGroup(pwszHostname,
                               pwszAliasname,
                               &bCreated);
        if (err != 0) netapi_fail(err);

        err = CleanupLocalGroup(pwszHostname,
                                pwszChangedAliasname);
        if (err != 0) netapi_fail(err);

        ret &= CallNetLocalGroupSetInfo(pwszHostname,
                                        dwLevel,
                                        pwszAliasname,
                                        pwszChangedAliasname,
                                        pwszComment,
                                        &bRenamed);

        if (bRenamed)
        {
            ret &= CallNetLocalGroupSetInfo(pwszHostname,
                                            0,
                                            pwszChangedAliasname,
                                            pwszAliasname,
                                            NULL,
                                            &bRenamed);

            /* account has been renamed back to original name */
            bRenamed = FALSE;
        }
    }

    if (bCreated)
    {
        err = CleanupLocalGroup(pwszHostname,
                                pwszAliasname);
        if (err != 0) netapi_fail(err);
    }

done:
    LW_SAFE_FREE_MEMORY(pwszAliasname);
    LW_SAFE_FREE_MEMORY(pwszComment);
    LW_SAFE_FREE_MEMORY(pwszChangedAliasname);

    return ret;
}


static
DWORD
TestNetLocalGroupGetMembers(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    PCSTR pszDefaultLocalGroupName = "Administrators";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 3 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    PWSTR pwszLocalGroupName = NULL;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "aliasname", pt_w16string,
                       &pwszLocalGroupName, &pszDefaultLocalGroupName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("level", pt_uint32, &dwLevel);
    PARAM_INFO("aliasname", pt_w16string, &pwszLocalGroupName);

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        ret &= CallNetLocalGroupGetMembers(pwszHostname,
                                           pwszLocalGroupName,
                                           dwLevel);
    }

    LW_SAFE_FREE_MEMORY(pwszLocalGroupName);

    return ret;
}


static
DWORD
TestNetGetDomainName(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    wchar16_t *domain_name;

    TESTINFO(pTest, pwszHostname);

    CALL_NETAPI(err, NetGetDomainName(pwszHostname, &domain_name));
    if (err != 0) netapi_fail(err);

    OUTPUT_ARG_WSTR(domain_name);

done:
    LW_SAFE_FREE_MEMORY(domain_name);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


static
DWORD
TestNetQueryDisplayInformation(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 1, 2, 3 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        ret &= CallNetQueryDisplayInfo(pwszHostname,
                                       dwLevel);
    }

    return ret;
}


static
DWORD
TestNetWkstaUserEnum(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOLEAN ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        ret &= CallNetWkstaUserEnum(pwszHostname,
                                    dwLevel);
    }

    return ret;
}


static
DWORD
TestNetSessionEnum(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);
    PCSTR pszDefaultName = "(unknown)";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 10, 502 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    PWSTR pwszClientName = NULL;
    PWSTR pwszUserName = NULL;
    PWSTR pwszDefaultName = NULL;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "clientname", pt_w16string,
                       &pwszClientName, &pszDefaultName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "username", pt_w16string,
                       &pwszUserName, &pszDefaultName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("level", pt_uint32, &dwLevel);
    PARAM_INFO("clientname", pt_w16string, &pwszClientName);
    PARAM_INFO("username", pt_w16string, &pwszUserName);

    dwError = LwMbsToWc16s(pszDefaultName, &pwszDefaultName);
    BAIL_ON_WIN_ERROR(dwError);

    if (wc16scmp(pwszClientName, pwszDefaultName) == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszClientName);
        pwszClientName = NULL;
    }

    if (wc16scmp(pwszUserName, pwszDefaultName) == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszUserName);
        pwszUserName = NULL;
    }

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        bRet &= CallNetSessionEnum(pwszHostname,
                                   pwszClientName,
                                   pwszUserName,
                                   dwLevel);
    }

error:
    LW_SAFE_FREE_MEMORY(pwszDefaultName);
    LW_SAFE_FREE_MEMORY(pwszClientName);
    LW_SAFE_FREE_MEMORY(pwszUserName);

    return bRet;
}


static
DWORD
TestNetServerEnum(
    PTEST         pTest,
    PCWSTR        pwszHostname,
    PCWSTR        pwszBindingString,
    PCREDENTIALS  pCreds,
    PPARAMETER    pOptions,
    DWORD         dwOptcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);
    PCSTR pszDefaultName = "(unknown)";

    BOOLEAN bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 100, 101 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    PWSTR pwszDefaultName = NULL;
    PWSTR pwszDomainName = NULL;

    TESTINFO(pTest, pwszHostname);

    perr = fetch_value(pOptions, dwOptcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(pOptions, dwOptcount, "domainname", pt_w16string,
                       &pwszDomainName, &pszDefaultName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("level", pt_uint32, &dwLevel);
    PARAM_INFO("domainname", pt_w16string, &pwszDomainName);

    dwError = LwMbsToWc16s(pszDefaultName, &pwszDefaultName);
    BAIL_ON_WIN_ERROR(dwError);

    if (wc16scmp(pwszDomainName, pwszDefaultName) == 0)
    {
        LW_SAFE_FREE_MEMORY(pwszDomainName);
        pwszDomainName = NULL;
    }

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

    for (i = 0; i < dwNumLevels; i++)
    {
        dwLevel = pdwLevels[i];

        bRet &= CallNetServerEnum(pwszHostname,
                                  pwszDomainName,
                                  dwLevel);
    }

error:
    LW_SAFE_FREE_MEMORY(pwszDefaultName);
    LW_SAFE_FREE_MEMORY(pwszDomainName);

    return bRet;
}


VOID
SetupNetApiTests(PTEST t)
{
    DWORD dwError = ERROR_SUCCESS;

    dwError = SrvSvcInitMemory();
    if (dwError) return;

    AddTest(t, "NETAPI-USER-ENUM", TestNetUserEnum);
    AddTest(t, "NETAPI-USER-ADD", TestNetUserAdd);
    AddTest(t, "NETAPI-USER-DEL", TestNetUserDel);
    AddTest(t, "NETAPI-USER-GETINFO", TestNetUserGetInfo);
    AddTest(t, "NETAPI-USER-SETINFO", TestNetUserSetInfo);
    AddTest(t, "NETAPI-USER-GET-LOCAL-GROUPS", TestNetUserGetLocalGroups);
    AddTest(t, "NETAPI-JOIN-DOMAIN", TestNetJoinDomain);
    AddTest(t, "NETAPI-UNJOIN-DOMAIN", TestNetUnjoinDomain);
    AddTest(t, "NETAPI-USER-CHANGE-PASSWORD", TestNetUserChangePassword);
    AddTest(t, "NETAPI-GET-DOMAIN-NAME", TestNetGetDomainName);
    AddTest(t, "NETAPI-LOCAL-GROUP-ENUM", TestNetLocalGroupEnum);
    AddTest(t, "NETAPI-LOCAL-GROUP-ADD", TestNetLocalGroupAdd);
    AddTest(t, "NETAPI-LOCAL-GROUP-DEL", TestNetLocalGroupDel);
    AddTest(t, "NETAPI-LOCAL-GROUP-GETINFO", TestNetLocalGroupGetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-SETINFO", TestNetLocalGroupSetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-MEMBERS", TestNetLocalGroupGetMembers);
    AddTest(t, "NETAPI-QUERY-DISP-INFO", TestNetQueryDisplayInformation);
    AddTest(t, "NETAPI-WKSTA-USER-ENUM", TestNetWkstaUserEnum);
    AddTest(t, "NETAPI-SESSION-ENUM", TestNetSessionEnum);
    AddTest(t, "NETAPI-SERVER-ENUM", TestNetServerEnum);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
