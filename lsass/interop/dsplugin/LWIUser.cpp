/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "LWIUser.h"


long
CreateLWIUser(
    PCSTR pszName,
    PCSTR pszDisplayName,
    PCSTR pszNameAsQueried,
    PCSTR pszPassword,
    PCSTR pszClass,
    PCSTR pszGecos,
    PCSTR pszNFSHomeDirectory,
    PCSTR pszHomeDirectory,
    PCSTR pszOrigNFSHomeDirectory,
    PCSTR pszOrigHomeDirectory,
    PCSTR pszShell,
    uid_t uid,
    gid_t gid,
    PAD_USER_ATTRIBUTES padUserADInfo,
    PLWIUSER* ppLWIUser
    )
{
    long macError = eDSNoErr;
    PLWIUSER pUser = NULL;

    macError = LwAllocateMemory(sizeof(LWIUSER), (PVOID*)&pUser);
    GOTO_CLEANUP_ON_MACERROR(macError);

    if (pszName)
    {
        macError = LwAllocateString(pszName, &pUser->pw_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszDisplayName)
    {
        macError = LwAllocateString(pszDisplayName, &pUser->pw_display_name);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszNameAsQueried)
    {
        macError = LwAllocateString(pszNameAsQueried, &pUser->pw_name_as_queried);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszPassword)
    {
        macError = LwAllocateString(pszPassword, &pUser->pw_passwd);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszClass)
    {
        macError = LwAllocateString(pszClass, &pUser->pw_class);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszGecos)
    {
        macError = LwAllocateString(pszGecos, &pUser->pw_gecos);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (!pszNFSHomeDirectory && pszHomeDirectory && *pszHomeDirectory)
    {
        macError = LwAllocateString(pszHomeDirectory, &pUser->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }
    else if (pszNFSHomeDirectory)
    {
        macError = LwAllocateString(pszNFSHomeDirectory, &pUser->pw_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszHomeDirectory)
    {
        macError = LwAllocateString(pszHomeDirectory, &pUser->pw_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszOrigNFSHomeDirectory)
    {
        macError = LwAllocateString(pszOrigNFSHomeDirectory, &pUser->pw_orig_nfs_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszOrigHomeDirectory)
    {
        macError = LwAllocateString(pszOrigHomeDirectory, &pUser->pw_orig_home_dir);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (pszShell)
    {
        macError = LwAllocateString(pszShell, &pUser->pw_shell);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    if (padUserADInfo)
    {
        macError = CopyADUserInfo(padUserADInfo, &pUser->padUserInfo);
        GOTO_CLEANUP_ON_MACERROR(macError);
    }

    pUser->pw_uid = uid;
    pUser->pw_gid = gid;

    *ppLWIUser = pUser;
    pUser = NULL;

cleanup:

    FreeLWIUser(pUser);

    return macError;
}

void
FreeLWIUser(PLWIUSER pLWIUser)
{
    if (pLWIUser)
    {
        if (pLWIUser->pw_name)
            LW_SAFE_FREE_STRING(pLWIUser->pw_name);
        if (pLWIUser->pw_display_name)
            LW_SAFE_FREE_STRING(pLWIUser->pw_display_name);
        if (pLWIUser->pw_name_as_queried)
            LW_SAFE_FREE_STRING(pLWIUser->pw_name_as_queried);
        if (pLWIUser->pw_passwd)
            LW_SAFE_FREE_STRING(pLWIUser->pw_passwd);
        if (pLWIUser->pw_class)
            LW_SAFE_FREE_STRING(pLWIUser->pw_class);
        if (pLWIUser->pw_gecos)
            LW_SAFE_FREE_STRING(pLWIUser->pw_gecos);
        if (pLWIUser->pw_nfs_home_dir)
            LW_SAFE_FREE_STRING(pLWIUser->pw_nfs_home_dir);
        if (pLWIUser->pw_home_dir)
            LW_SAFE_FREE_STRING(pLWIUser->pw_home_dir);
        if (pLWIUser->pw_orig_home_dir)
            LW_SAFE_FREE_STRING(pLWIUser->pw_orig_home_dir);
        if (pLWIUser->pw_orig_nfs_home_dir)
            LW_SAFE_FREE_STRING(pLWIUser->pw_orig_nfs_home_dir);
        if (pLWIUser->pw_shell)
            LW_SAFE_FREE_STRING(pLWIUser->pw_shell);
        if (pLWIUser->padUserInfo)
            FreeADUserInfo(pLWIUser->padUserInfo);

        LwFreeMemory(pLWIUser);
    }
}
