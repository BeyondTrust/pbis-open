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

    SAFE_FREE(info1->usri1_comment);
    SAFE_FREE(info1->usri1_home_dir);
    SAFE_FREE(info1->usri1_script_path);
    SAFE_FREE(info1->usri1_password);
    SAFE_FREE(info1->usri1_name);
    SAFE_FREE(info1);

    return err;
}


int DelUser(const wchar16_t *hostname, const wchar16_t *username)
{
    return NetUserDel(hostname, username);
}


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

    SAFE_FREE(info.lgrpi1_name);
    SAFE_FREE(info.lgrpi1_comment);

    return err;
}


int DelLocalGroup(const wchar16_t *hostname, const wchar16_t *aliasname)
{
    return NetLocalGroupDel(hostname, aliasname);
}


void DoCleanup(const wchar16_t *hostname, const wchar16_t *aliasname,
               const wchar16_t *username)
{
    DelUser(hostname, username);
    DelLocalGroup(hostname, aliasname);
}


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


void DumpNetUserInfo1(const char *prefix, USER_INFO_1 *info)
{
    wchar16_t *usri1_name = info->usri1_name;
    wchar16_t *usri1_password = info->usri1_password;

    DUMP_WSTR(prefix, usri1_name);
    DUMP_WSTR(prefix, usri1_password);
}


static
BOOL
CallNetUserEnum(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    DWORD  dwFilter
    )
{
    BOOL bRet = TRUE;
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
BOOL
CallNetUserAdd(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszUsername,
    PWSTR  pwszComment,
    PWSTR  pwszHomedir,
    PWSTR  pwszScriptPath,
    PWSTR  pwszPassword,
    DWORD  dwFlags
    )
{
    BOOL ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    USER_INFO_1 Info1 = {0};
    USER_INFO_2 Info2 = {0};
    USER_INFO_3 Info3 = {0};
    USER_INFO_4 Info4 = {0};
    DWORD dwParmErr = 0;

    switch (dwLevel)
    {
    case 1:
        Info1.usri1_name        = pwszUsername;
        Info1.usri1_password    = pwszPassword;
        Info1.usri1_priv        = USER_PRIV_USER;
        Info1.usri1_home_dir    = pwszHomedir;
        Info1.usri1_comment     = pwszComment;
        Info1.usri1_flags       = dwFlags;
        Info1.usri1_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info1;
        break;

    case 2:
        Info2.usri2_name        = pwszUsername;
        Info2.usri2_password    = pwszPassword;
        Info2.usri2_priv        = USER_PRIV_USER;
        Info2.usri2_home_dir    = pwszHomedir;
        Info2.usri2_comment     = pwszComment;
        Info2.usri2_flags       = dwFlags;
        Info2.usri2_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info2;
        break;

    case 3:
        Info3.usri3_name        = pwszUsername;
        Info3.usri3_password    = pwszPassword;
        Info3.usri3_priv        = USER_PRIV_USER;
        Info3.usri3_home_dir    = pwszHomedir;
        Info3.usri3_comment     = pwszComment;
        Info3.usri3_flags       = dwFlags;
        Info3.usri3_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info3;
        break;

    case 4:
        Info4.usri4_name        = pwszUsername;
        Info4.usri4_password    = pwszPassword;
        Info4.usri4_priv        = USER_PRIV_USER;
        Info4.usri4_home_dir    = pwszHomedir;
        Info4.usri4_comment     = pwszComment;
        Info4.usri4_flags       = dwFlags;
        Info4.usri4_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info4;
        break;
    }

    err = NetUserAdd(pwszHostname,
                     dwLevel,
                     pBuffer,
                     &dwParmErr);

    ret = (err == ERROR_SUCCESS);

    return ret;
}


static
BOOL
CallNetUserSetInfo(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszUsername,
    PWSTR  pwszChangedUsername,
    PWSTR  pwszFullName,
    PWSTR  pwszComment,
    PWSTR  pwszHomedir,
    PWSTR  pwszScriptPath,
    PWSTR  pwszPassword,
    DWORD  dwFlags,
    PBOOL  pbRenamed
    )
{
    BOOL ret = TRUE;
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
    DWORD dwParmErr = 0;

    switch (dwLevel)
    {
    case 0:
        Info0.usri0_name        = pwszChangedUsername;

        pBuffer = (PVOID)&Info0;
        break;

    case 1:
        Info1.usri1_name        = pwszUsername;
        Info1.usri1_password    = pwszPassword;
        Info1.usri1_priv        = USER_PRIV_USER;
        Info1.usri1_home_dir    = pwszHomedir;
        Info1.usri1_comment     = pwszComment;
        Info1.usri1_flags       = dwFlags;
        Info1.usri1_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info1;
        break;

    case 2:
        Info2.usri2_name        = pwszUsername;
        Info2.usri2_password    = pwszPassword;
        Info2.usri2_priv        = USER_PRIV_USER;
        Info2.usri2_home_dir    = pwszHomedir;
        Info2.usri2_comment     = pwszComment;
        Info2.usri2_flags       = dwFlags;
        Info2.usri2_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info2;
        break;

    case 3:
        Info3.usri3_name        = pwszUsername;
        Info3.usri3_password    = pwszPassword;
        Info3.usri3_priv        = USER_PRIV_USER;
        Info3.usri3_home_dir    = pwszHomedir;
        Info3.usri3_comment     = pwszComment;
        Info3.usri3_flags       = dwFlags;
        Info3.usri3_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info3;
        break;

    case 4:
        Info4.usri4_name        = pwszUsername;
        Info4.usri4_password    = pwszPassword;
        Info4.usri4_priv        = USER_PRIV_USER;
        Info4.usri4_home_dir    = pwszHomedir;
        Info4.usri4_comment     = pwszComment;
        Info4.usri4_flags       = dwFlags;
        Info4.usri4_script_path = pwszScriptPath;

        pBuffer = (PVOID)&Info4;
        break;

    case 1003:
        Info1003.usri1003_password = pwszPassword;

        pBuffer = (PVOID)&Info1003;
        break;

    case 1007:
        Info1007.usri1007_comment = pwszComment;

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
    }

    err = NetUserSetInfo(pwszHostname,
                         pwszUsername,
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
BOOL
CallNetUserGetInfo(
    PCWSTR pwszHostname,
    PCWSTR pwszUsername,
    DWORD  dwLevel
    )
{
    BOOL ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    PVOID pBuffer = NULL;

    err = NetUserGetInfo(pwszHostname,
                         pwszUsername,
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
BOOL
CallNetUserGetLocalGroups(
    PCWSTR pwszHostname,
    PCWSTR pwszUserName,
    DWORD  dwLevel,
    DWORD  dwFlags
    )
{
    BOOL bRet = TRUE;
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
BOOL
CallNetLocalGroupEnum(
    PCWSTR pwszHostname,
    DWORD  dwLevel
    )
{
    BOOL bRet = TRUE;
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
BOOL
CallNetLocalGroupAdd(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszAliasname,
    PWSTR  pwszComment
    )
{
    BOOL ret = TRUE;
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
BOOL
CallNetLocalGroupSetInfo(
    PCWSTR pwszHostname,
    DWORD  dwLevel,
    PWSTR  pwszAliasname,
    PWSTR  pwszChangedAliasname,
    PWSTR  pwszComment,
    PBOOL  pbRenamed
    )
{
    BOOL ret = TRUE;
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
BOOL
CallNetLocalGroupGetInfo(
    PCWSTR pwszHostname,
    PCWSTR pwszAliasname,
    DWORD  dwLevel
    )
{
    BOOL ret = TRUE;
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
BOOL
CallNetLocalGroupGetMembers(
    PCWSTR pwszHostname,
    PCWSTR pwszLocalGroupName,
    DWORD  dwLevel
    )
{
    BOOL bRet = TRUE;
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


int
TestNetUserEnum(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    const DWORD dwDefaultFilter = 0;
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 20 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    DWORD dwFilter = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "filter", pt_uint32,
                       (UINT32*)&dwFilter, (UINT32*)&dwDefaultFilter);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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
            ret &= CallNetUserEnum(hostname,
                                   dwLevel,
                                   FILTER_NORMAL_ACCOUNT);

            ret &= CallNetUserEnum(hostname,
                                   dwLevel,
                                   FILTER_INTERDOMAIN_TRUST_ACCOUNT);

            ret &= CallNetUserEnum(hostname,
                                   dwLevel,
                                   FILTER_WORKSTATION_TRUST_ACCOUNT);

            ret &= CallNetUserEnum(hostname,
                                   dwLevel,
                                   FILTER_SERVER_TRUST_ACCOUNT);
        }
        else
        {
            ret &= CallNetUserEnum(hostname,
                                   dwLevel,
                                   dwFilter);
        }
    }

done:
    RELEASE_SESSION_CREDS;

    return ret;
}


int
TestNetUserAdd(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefaultUsername = "TestUser";
    PCSTR pszDefaultComment = "sample comment";
    PCSTR pszDefaultHomedir = "c:\\";
    PCSTR pszDefaultScriptPath = "\\\\server\\share\\dir\\script.cmd";
    PCSTR pszDefaultPassword = NULL;
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 1, 2, 3, 4, };
    DWORD dwLevel = 0;
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszScriptPath = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwFlags = 0;
    DWORD i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "homedir", pt_w16string,
                       &pwszHomedir, &pszDefaultHomedir);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "scriptpath", pt_w16string,
                       &pwszScriptPath, &pszDefaultScriptPath);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &pwszPassword, &pszDefaultPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, &pwszUsername);
    PARAM_INFO("comment", pt_w16string, &pwszComment);
    PARAM_INFO("homedir", pt_w16string, &pwszHomedir);
    PARAM_INFO("scriptpath", pt_w16string, &pwszScriptPath);
    PARAM_INFO("password", pt_w16string, &pwszPassword);
    PARAM_INFO("level", pt_uint32, &dwLevel);

    /* We're testing user accounts only */
    dwFlags = UF_NORMAL_ACCOUNT;

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

        status = CleanupAccount(hostname, pwszUsername);
        if (status != 0) rpc_fail(status);


        ret &= CallNetUserAdd(hostname,
                              dwLevel,
                              pwszUsername,
                              pwszComment,
                              pwszHomedir,
                              pwszScriptPath,
                              pwszPassword,
                              dwFlags);
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszUsername);
    SAFE_FREE(pwszComment);
    SAFE_FREE(pwszHomedir);
    SAFE_FREE(pwszScriptPath);
    SAFE_FREE(pwszPassword);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserDel(struct test *t, const wchar16_t *hostname,
                   const wchar16_t *user, const wchar16_t *pass,
                   struct parameter *options, int optcount)
{
    PCSTR pszDefaultUsername = "TestUser";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PWSTR pwszUsername = NULL;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, pwszUsername);

    status = EnsureUserAccount(hostname, pwszUsername, &bCreated);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err, NetUserDel(hostname, pwszUsername));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszUsername);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int
TestNetUserGetInfo(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    const PSTR pszDefaultUsername = "TestUser";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszUsername = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 20 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        status = EnsureUserAccount(hostname,
                                   pwszUsername,
                                   &bCreated);
        if (status != 0) rpc_fail(status);

        ret &= CallNetUserGetInfo(hostname,
                                  pwszUsername,
                                  dwLevel);

        if (bCreated)
        {
            status = CleanupAccount(hostname, pwszUsername);
            if (status != 0) rpc_fail(status);
        }
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszUsername);

    return ret;
}


int
TestNetUserSetInfo(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefaultUsername = "TestUser";
    PCSTR pszChangedUsername = "TestUserRename";
    PCSTR pszDefaultFullName = "Full user name";
    PCSTR pszDefaultComment = "sample comment";
    PCSTR pszDefaultHomedir = "c:\\";
    PCSTR pszDefaultScriptPath = "\\\\server\\share\\dir\\script.cmd";
    PCSTR pszDefaultPassword = NULL;
    const DWORD dwDefaultFlags = UF_NORMAL_ACCOUNT | UF_ACCOUNTDISABLE;
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1, 2, 3, 4, 1003, 1007, 1008, 1011 };
    DWORD dwLevel = 0;
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    PWSTR pwszChangedUsername = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszScriptPath = NULL;
    PWSTR pwszPassword = NULL;
    DWORD dwFlags = 0;
    DWORD i = 0;
    BOOL bCreated = FALSE;
    BOOL bRenamed = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "fullname", pt_w16string,
                       &pwszFullName, &pszDefaultFullName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "homedir", pt_w16string,
                       &pwszHomedir, &pszDefaultHomedir);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "scriptpath", pt_w16string,
                       &pwszScriptPath, &pszDefaultScriptPath);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &pwszPassword, &pszDefaultPassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "flags", pt_uint32,
                       (UINT32*)&dwFlags, (UINT32*)&dwDefaultFlags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    err = LwMbsToWc16s(pszChangedUsername,
                       &pwszChangedUsername);
    if (err != 0) netapi_fail(err);

    PARAM_INFO("username", pt_w16string, &pwszUsername);
    PARAM_INFO("fullname", pt_w16string, &pwszFullName);
    PARAM_INFO("usernamechange", pt_w16string, &pwszChangedUsername);
    PARAM_INFO("comment", pt_w16string, &pwszComment);
    PARAM_INFO("homedir", pt_w16string, &pwszHomedir);
    PARAM_INFO("scriptpath", pt_w16string, &pwszScriptPath);
    PARAM_INFO("password", pt_w16string, &pwszPassword);
    PARAM_INFO("flags", pt_uint32, &dwFlags);
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

        /* Don't test setting new password if there's no password supplied */
        if (dwLevel == 1003 &&
            pwszPassword == NULL) continue;

        status = EnsureUserAccount(hostname,
                                   pwszUsername,
                                   &bCreated);
        if (status != 0) rpc_fail(status);

        status = CleanupAccount(hostname,
                                pwszChangedUsername);
        if (status != 0) rpc_fail(status);

        ret &= CallNetUserSetInfo(hostname,
                                  dwLevel,
                                  pwszUsername,
                                  pwszChangedUsername,
                                  pwszFullName,
                                  pwszComment,
                                  pwszHomedir,
                                  pwszScriptPath,
                                  pwszPassword,
                                  dwFlags,
                                  &bRenamed);

        if (bRenamed)
        {
            ret &= CallNetUserSetInfo(hostname,
                                      0,
                                      pwszChangedUsername,
                                      pwszUsername,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      0,
                                      &bRenamed);

            /* account has been renamed back to original name */
            bRenamed = FALSE;
        }
    }

    if (bCreated)
    {
        status = CleanupAccount(hostname, pwszUsername);
        if (status != 0) rpc_fail(status);
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszUsername);
    SAFE_FREE(pwszFullName);
    SAFE_FREE(pwszComment);
    SAFE_FREE(pwszHomedir);
    SAFE_FREE(pwszScriptPath);
    SAFE_FREE(pwszPassword);
    SAFE_FREE(pwszChangedUsername);

    return ret;
}


int
TestNetUserGetLocalGroups(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    const PSTR pszDefaultUsername = "TestUser";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszUsername = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &pwszUsername, &pszDefaultUsername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        status = EnsureUserAccount(hostname,
                                   pwszUsername,
                                   &bCreated);
        if (status != 0) rpc_fail(status);

        ret &= CallNetUserGetLocalGroups(hostname,
                                         pwszUsername,
                                         dwLevel,
                                         0);
        if (bCreated)
        {
            status = CleanupAccount(hostname, pwszUsername);
            if (status != 0) rpc_fail(status);
        }
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszUsername);

    return ret;
}




int TestNetJoinDomain(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const char *def_accountou = NULL;
    const int def_rejoin = 0;
    const int def_create = 1;
    const int def_deferspn = 0;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *password, *accountou;
    wchar16_t *domain_name, *machine_account, *machine_password;
    int rejoin, create, deferspn;
    int opts;
    HANDLE store = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pi = NULL;
    char host[128] = {0};

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "accountou", pt_w16string,
                       &accountou, &def_accountou);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "rejoin", pt_int32,
                       &rejoin, &def_rejoin);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "create", pt_int32,
                       &create, &def_create);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "deferspn", pt_int32,
                       &deferspn, &def_deferspn);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("accountout", pt_w16string, accountou);
    PARAM_INFO("rejoin", pt_int32, &rejoin);
    PARAM_INFO("create", pt_int32, &create);
    PARAM_INFO("deferspn", pt_int32, &deferspn);

    opts = NETSETUP_JOIN_DOMAIN;
    if (create) opts |= NETSETUP_ACCT_CREATE;
    if (rejoin) opts |= NETSETUP_DOMAIN_JOIN_IF_JOINED;
    if (deferspn) opts |= NETSETUP_DEFER_SPN_SET;

    CALL_NETAPI(err, NetJoinDomain(NULL, hostname, accountou,
                                   username, password, opts));
    if (err != 0) netapi_fail(err);

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &store);
    if (status != STATUS_SUCCESS) return false;

    //    host = awc16stombs(hostname);
    //    if (host == NULL) return false;
    gethostname(host, sizeof(host));

    status = LwpsGetPasswordByHostName(store, host, &pi);
    if (status != STATUS_SUCCESS) return false;

    domain_name       = pi->pwszDomainName;
    machine_account   = pi->pwszMachineAccount;
    machine_password  = pi->pwszMachinePassword;
    RESULT_WSTR(domain_name);
    RESULT_WSTR(machine_account);
    RESULT_WSTR(machine_password);

done:
    status = LwpsClosePasswordStore(store);
    if (status != STATUS_SUCCESS) return false;

    SAFE_FREE(username);
    SAFE_FREE(password);
    SAFE_FREE(accountou);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUnjoinDomain(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const int def_disable = 1;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *password;
    int disable;
    int opts;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "disable", pt_int32,
                       &disable, &def_disable);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("disable", pt_int32, &disable);

    opts = (disable) ? NETSETUP_ACCT_DELETE : 0;

    CALL_NETAPI(err, NetUnjoinDomain(NULL, username, password, opts));
    if (err != 0) netapi_fail(err);

done:
    SAFE_FREE(username);
    SAFE_FREE(password);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetMachineChangePassword(struct test *t, const wchar16_t *hostname,
                                 const wchar16_t *user, const wchar16_t *pass,
                                 struct parameter *options, int optcount)
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;

    TESTINFO(t, hostname, user, pass);

    CALL_NETAPI(err, NetMachineChangePassword());
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:
    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserChangePassword(struct test *t, const wchar16_t *hostname,
                              const wchar16_t *user, const wchar16_t *pass,
                              struct parameter *options, int optcount)
{
    const char *defusername = "TestUser";
    const char *defoldpass = "";
    const char *defnewpass = "newpassword";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *oldpassword, *newpassword;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "oldpassword", pt_w16string,
                       &oldpassword, &defoldpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "newpassword", pt_w16string,
                       &newpassword, &defnewpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("oldpassword", pt_w16string, oldpassword);
    PARAM_INFO("newpassword", pt_w16string, newpassword);

    status = EnsureUserAccount(hostname, username, &bCreated);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err, NetUserChangePassword(hostname, username,
                                           oldpassword, newpassword));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);
    SAFE_FREE(oldpassword);
    SAFE_FREE(newpassword);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserLocalGroups(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const char *defusername = "TestUser";
    const char *defaliasname = "TestAlias";
    const char *def_admin_user = "Administrator";
    const char *def_guest_user = "Guest";
    const char *def_guests_group = "Guests";
    const char *def_admins_group = "Administrators";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    LOCALGROUP_USERS_INFO_0 *grpinfo;
    UINT32 entries;
    wchar16_t *username, *aliasname, *guest_user, *admin_user;
    wchar16_t *guests_group, *admins_group, *domname;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &aliasname, &defaliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestuser", pt_w16string,
                       &guest_user, &def_guest_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminuser", pt_w16string,
                       &admin_user, &def_admin_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestsgroup", pt_w16string,
                       &guests_group, &def_guests_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminsgroup", pt_w16string,
                       &admins_group, &def_admins_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("guestuser", pt_w16string, guest_user);
    PARAM_INFO("adminuser", pt_w16string, admin_user);
    PARAM_INFO("guestsgroup", pt_w16string, guests_group);
    PARAM_INFO("adminsgroup", pt_w16string, admins_group);

    grpinfo = NULL;

    /*
     * Test 1a: Get groups of an existing and known user
     */

    err = GetUserLocalGroups(hostname, admin_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    /*
     * Test 1b: Get groups of an existing and known user
     */

    err = GetUserLocalGroups(hostname, guest_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);


    /*
     * Test 2: Get groups of newly created user has no group memberships yet
     */
    status = EnsureUserAccount(hostname, username, &bCreated);
    if (status != 0) rpc_fail(status);


    err = GetUserLocalGroups(hostname, username, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (grpinfo != NULL && entries > 0) {
        printf("Groups found while there should be none\n");
        return false;
    }

    /*
     * Test 3: Add user to 2 groups and get the local groups list
     */

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    err = AddLocalGroupMember(hostname, admins_group, domname, username);
    if (err != 0) netapi_fail(err);

    err = AddLocalGroupMember(hostname, guests_group, domname, username);
    if (err != 0) netapi_fail(err);

    err = GetUserLocalGroups(hostname, username, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 2) {
        w16printfw(L"User %ws should be member of at 2 groups because"
                  L"they have been added to groups %ws and %ws",
                  username, admins_group, guests_group);
        return false;
    }


    /*
     * Test 4: Add 2 existing users to a newly created group, and get the local groups list
     */
    status = EnsureAlias(hostname, aliasname, &bCreated);

    err = AddLocalGroupMember(hostname, aliasname, domname, admin_user);
    if (err != 0) netapi_fail(err);

    err = AddLocalGroupMember(hostname, aliasname, domname, guest_user);
    if (err != 0) netapi_fail(err);

    err = GetUserLocalGroups(hostname, admin_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 1) {
        w16printfw(L"User %ws should be member of at least 1 alias because"
                  L"they have been added to group %ws",
                  admin_user, aliasname);
        return false;
    }

    err = GetUserLocalGroups(hostname, guest_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 1) {
        w16printfw(L"User %ws should be member of at least 1 alias because"
                  L"they have been added to group %ws",
                  guest_user, aliasname);
        return false;
    }

done:
    DoCleanup(hostname, aliasname, username);

    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);
    SAFE_FREE(aliasname);
    SAFE_FREE(admin_user);
    SAFE_FREE(guest_user);
    SAFE_FREE(admins_group);
    SAFE_FREE(guests_group);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int
TestNetLocalGroupEnum(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        ret &= CallNetLocalGroupEnum(hostname,
                                     dwLevel);
    }

done:
    RELEASE_SESSION_CREDS;

    return ret;
}


int
TestNetLocalGroupAdd(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount)
{
    PCSTR pszDefaultAliasname = "TestLocalGroup";
    PCSTR pszDefaultComment = "Test comment for new local group";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    NTSTATUS status = STATUS_SUCCESS;
    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 1 };
    DWORD dwLevel = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwNumLevels = 0;
    PWSTR pwszAliasname = NULL;
    PWSTR pwszComment = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        status = CleanupAlias(hostname,
                              pwszAliasname);
        if (status != 0) rpc_fail(status);

        ret &= CallNetLocalGroupAdd(hostname,
                                    dwLevel,
                                    pwszAliasname,
                                    pwszComment);
    }

    status = CleanupAlias(hostname,
                          pwszAliasname);
    if (status != 0) rpc_fail(status);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszAliasname);
    SAFE_FREE(pwszComment);

    return ret;
}


int TestDelLocalGroup(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const char *def_aliasname = "TestAlias";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *aliasname;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);

    err = DelLocalGroup(hostname, aliasname);
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(aliasname);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int
TestNetLocalGroupGetInfo(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    const PSTR pszDefaultAliasname = "TestLocalGroup";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszAliasname = NULL;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 1 };
    DWORD dwNumLevels = 0;
    PDWORD pdwLevels = NULL;
    DWORD i = 0;
    DWORD dwLevel = 0;
    BOOL bCreated = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        status = EnsureAlias(hostname,
                             pwszAliasname,
                             &bCreated);
        if (status != 0) rpc_fail(status);

        ret &= CallNetLocalGroupGetInfo(hostname,
                                        pwszAliasname,
                                        dwLevel);

        if (bCreated)
        {
            status = CleanupAlias(hostname, pwszAliasname);
            if (status != 0) rpc_fail(status);
        }
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszAliasname);

    return ret;
}


int
TestNetLocalGroupSetInfo(
    struct test *t,
    PCWSTR hostname,
    PCWSTR user,
    PCWSTR pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefaultAliasname = "TestLocalGroup";
    PCSTR pszChangedAliasname = "TestLocalGroupRename";
    PCSTR pszDefaultComment = "Test comment for local group";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
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
    BOOL bCreated = FALSE;
    BOOL bRenamed = FALSE;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &pwszAliasname, &pszDefaultAliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string,
                       &pwszComment, &pszDefaultComment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_uint32,
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

        status = EnsureAlias(hostname,
                             pwszAliasname,
                             &bCreated);
        if (status != 0) rpc_fail(status);

        status = CleanupAlias(hostname,
                              pwszChangedAliasname);
        if (status != 0) rpc_fail(status);

        ret &= CallNetLocalGroupSetInfo(hostname,
                                        dwLevel,
                                        pwszAliasname,
                                        pwszChangedAliasname,
                                        pwszComment,
                                        &bRenamed);

        if (bRenamed)
        {
            ret &= CallNetLocalGroupSetInfo(hostname,
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
        status = CleanupAlias(hostname, pwszAliasname);
        if (status != 0) rpc_fail(status);
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(pwszAliasname);
    SAFE_FREE(pwszComment);
    SAFE_FREE(pwszChangedAliasname);

    return ret;
}


int
TestNetLocalGroupGetMembers(
    struct test *t,
    const wchar16_t *hostname,
    const wchar16_t *user,
    const wchar16_t *pass,
    struct parameter *options,
    int optcount
    )
{
    PCSTR pszDefaultLocalGroupName = "Administrators";
    const DWORD dwDefaultLevel = (DWORD)(-1);

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    DWORD i = 0;
    DWORD dwSelectedLevels[] = { 0 };
    DWORD dwAvailableLevels[] = { 0, 3 };
    PDWORD pdwLevels = NULL;
    DWORD dwNumLevels = 0;
    DWORD dwLevel = 0;
    PWSTR pwszLocalGroupName = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "level", pt_uint32,
                       (UINT32*)&dwLevel, (UINT32*)&dwDefaultLevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
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

        ret &= CallNetLocalGroupGetMembers(hostname,
                                           pwszLocalGroupName,
                                           dwLevel);
    }

done:
    SAFE_FREE(pwszLocalGroupName);

    RELEASE_SESSION_CREDS;

    return ret;
}



int TestNetGetDomainName(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcounta)
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    wchar16_t *domain_name;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    CALL_NETAPI(err, NetGetDomainName(hostname, &domain_name));
    if (err != 0) netapi_fail(err);

    OUTPUT_ARG_WSTR(domain_name);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(domain_name);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


void SetupNetApiTests(struct test *t)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = NetInitMemory();
    if (status) return;

    AddTest(t, "NETAPI-USER-ENUM", TestNetUserEnum);
    AddTest(t, "NETAPI-USER-ADD", TestNetUserAdd);
    AddTest(t, "NETAPI-USER-DEL", TestNetUserDel);
    AddTest(t, "NETAPI-USER-GETINFO", TestNetUserGetInfo);
    AddTest(t, "NETAPI-USER-SETINFO", TestNetUserSetInfo);
    AddTest(t, "NETAPI-USER-GET-LOCAL-GROUPS", TestNetUserGetLocalGroups);
    AddTest(t, "NETAPI-JOIN-DOMAIN", TestNetJoinDomain);
    AddTest(t, "NETAPI-UNJOIN-DOMAIN", TestNetUnjoinDomain);
    AddTest(t, "NETAPI-MACHINE-CHANGE-PASSWORD", TestNetMachineChangePassword);
    AddTest(t, "NETAPI-USER-CHANGE-PASSWORD", TestNetUserChangePassword);
    AddTest(t, "NETAPI-GET-DOMAIN-NAME", TestNetGetDomainName);
    AddTest(t, "NETAPI-USER-LOCAL-GROUPS", TestNetUserLocalGroups);
    AddTest(t, "NETAPI-LOCAL-GROUP-ENUM", TestNetLocalGroupEnum);
    AddTest(t, "NETAPI-LOCAL-GROUP-ADD", TestNetLocalGroupAdd);
    AddTest(t, "NETAPI-LOCAL-GROUP-GETINFO", TestNetLocalGroupGetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-SETINFO", TestNetLocalGroupSetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-MEMBERS", TestNetLocalGroupGetMembers);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
