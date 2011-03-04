/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
    PMCXVALUE pMCXValues,
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

    if (pMCXValues)
    {
        macError = CopyMCXValueList(pMCXValues, &pUser->pMCXValues);
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
	    if (pLWIUser->pMCXValues)
	        FreeMCXValueList(pLWIUser->pMCXValues);
        if (pLWIUser->padUserInfo)
            FreeADUserInfo(pLWIUser->padUserInfo);

        LwFreeMemory(pLWIUser);
    }
}
