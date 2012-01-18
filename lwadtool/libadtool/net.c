/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        net.c
 *
 * Abstract:
 *
 *        This file contains netapi wrappers.
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 24, 2010
 *
 */

#include "includes.h"

/**
 * Normalize user name by adding domain prefix if the name is not UPN.
 *
 * @param userNameC User name.
 * @param domainC Domain name.
 * @param userNameN Normalized user name to fill out.
 * @return 0 on success; error code on failure.
 */
static DWORD
NormalizeUserName(
    IN PSTR userNameC,
    IN PSTR domainC,
    OUT PSTR *userNameN
)
{
    DWORD dwError = ERROR_SUCCESS;
    PSTR userName = NULL;
    PSTR domain = NULL;

    if(!IsFullOrUPN(userNameC)) {
        dwError = GetDomainComp(domainC, &domain);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

        dwError = LwAllocateStringPrintf(&userName, "%s\\%s", domain, userNameC);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }
    else {
        dwError = LwStrDupOrNull((PCSTR) userNameC, &userName);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    *userNameN = userName;

    cleanup:
        LW_SAFE_FREE_MEMORY(domain);
        return dwError;

    error:
        LW_SAFE_FREE_MEMORY(userName);
        goto cleanup;
}

/**********************************************************/
/*                 AD user operations                     */
/**********************************************************/

/**
 * Delete AD user.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserDelete(
    IN AppContextTP appContext,
    IN PSTR userNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR hostName = NULL;
    PWSTR userName = NULL;
    PSTR  userNameN = NULL;

    dwError = NormalizeUserName(userNameC, appContext->workConn->domainName, &userNameN);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Deleting user %s ...\n",
                appContext->actionName, userNameN);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetUserDel((PCWSTR) hostName, (PCWSTR) userName);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done deleting user %s\n",
                appContext->actionName, userNameN);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(userName);
        LW_SAFE_FREE_MEMORY(userNameN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Add AD user with default properties.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserAdd(
    IN AppContextTP appContext,
    IN PSTR userNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    USER_INFO_0 Info = { 0 };
    DWORD parmError = 0;
    PWSTR hostName = NULL;
    PWSTR userName = NULL;
    PSTR  userNameN = NULL;

    dwError = NormalizeUserName(userNameC, appContext->workConn->domainName, &userNameN);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    Info.usri0_name = userName;

    PrintStderr(appContext, LogLevelTrace, "%s: Adding user %s ...\n",
                appContext->actionName, userNameN);

    /* Perform the add operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetUserAdd((PCWSTR) hostName, 0, (PVOID) &Info, &parmError);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done adding user %s\n",
                appContext->actionName, userNameN);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(userName);
        LW_SAFE_FREE_MEMORY(userNameN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Modify AD user account.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param passwordC Password
 * @param flags Account controls
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserSetInfo(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR  passwordC,
    IN DWORD flags
)
{
    BOOL isRenamed = FALSE;

    return AdtNetUserSetInfoFromParams(
            appContext,
            2,
            userNameC,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            passwordC,
            flags,
            &isRenamed
        );
}

/**
 * Set user account controls.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param flags Account controls.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserSetInfoFlags(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN DWORD flags
)
{
   DWORD dwError = ERROR_SUCCESS;
   DWORD parmErr = 0;
   PWSTR hostName = NULL;
   PWSTR userName = NULL;
   PSTR  userNameN = NULL;
   USER_INFO_1008 info1008;

   userNameN = GetNameComp(userNameC);

   dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   PrintStderr(appContext, LogLevelTrace, "%s: Changing control flags of user %s ...\n",
               appContext->actionName, userNameN);

   /* Perform the modify operation. */
   if (!appContext->gopts.isReadOnly) {
        info1008.usri1008_flags = flags;

        dwError = NetUserSetInfo(hostName,
                                 userName,
                                 1008,
                                 (PVOID) &info1008,
                                 &parmErr);

        if (dwError) {
            dwError += ADT_WIN_ERR_BASE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done changing control flags of user %s\n",
               appContext->actionName, userNameN);

   cleanup:
       LW_SAFE_FREE_MEMORY(hostName);
       LW_SAFE_FREE_MEMORY(userName);
       LW_SAFE_FREE_MEMORY(userNameN);

       return dwError;

   error:
       goto cleanup;
}

/**
 * Reset user's password and, optionally, set account control flags.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param password Password; must be NULL if we do not want to change it.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserSetPassword(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR passwordC
)
{
   DWORD dwError = ERROR_SUCCESS;
   DWORD parmErr = 0;
   PWSTR hostName = NULL;
   PWSTR userName = NULL;
   PWSTR password = NULL;
   PSTR  userNameN = NULL;
   USER_INFO_1003 info1003;

   userNameN = GetNameComp(userNameC);

   dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   if (passwordC) {
       dwError = LwMbsToWc16s((PCSTR) passwordC, &password);
       ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

       info1003.usri1003_password = password;
       password = NULL;
   }
   else {
       info1003.usri1003_password = NULL;
   }

   PrintStderr(appContext, LogLevelTrace, "%s: Setting password of user %s ...\n",
               appContext->actionName, userNameN);

   /* Perform the modify operation. */
   if (!appContext->gopts.isReadOnly) {
        dwError = NetUserSetInfo(hostName,
                                 userName,
                                 1003,
                                 (PVOID) &info1003,
                                 &parmErr);

        if (dwError) {
            dwError += ADT_WIN_ERR_BASE;
            ADT_BAIL_ON_ERROR_NP(dwError);
        }
    }

   PrintStderr(appContext, LogLevelTrace, "%s: Done setting password of user %s\n",
               appContext->actionName, userNameN);

   cleanup:
       LW_SAFE_FREE_MEMORY(hostName);
       LW_SAFE_FREE_MEMORY(userName);
       LW_SAFE_FREE_MEMORY(password);
       LW_SAFE_FREE_MEMORY(userNameN);

       return dwError;

   error:
       goto cleanup;
}

/**
 * Modify AD user account.
 *
 * @param appContext Application context reference.
 * @param info User information.
 * @param userNameC User name.
 * @param password Password; must be NULL if we do not want to change it.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserSetInfo4(
    IN AppContextTP appContext,
    IN PUSER_INFO_4 info,
    IN PSTR  userNameC,
    IN PSTR passwordC
)
{
   DWORD dwError = ERROR_SUCCESS;
   DWORD parmErr = 0;
   PWSTR hostName = NULL;
   PWSTR userName = NULL;
   PWSTR password = NULL;
   PSTR  userNameN = NULL;

   userNameN = GetNameComp(userNameC);

   dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
   ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

   if (passwordC) {
       dwError = LwMbsToWc16s((PCSTR) passwordC, &password);
       ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

       LW_SAFE_FREE_MEMORY(info->usri4_password);
       info->usri4_password = password;
       password = NULL;
   }
   else {
       info->usri4_password = NULL;
   }

   PrintStderr(appContext, LogLevelTrace, "%s: Changing properties of user %s ...\n",
               appContext->actionName, userNameN);

   /* Perform the modify operation. */
   if(!appContext->gopts.isReadOnly) {
       dwError = NetUserSetInfo(hostName, userName, 4, (PVOID) &info, &parmErr);
   }

   PrintStderr(appContext, LogLevelTrace, "%s: Done changing properties of user %s\n",
               appContext->actionName, userNameN);

   if (dwError) {
       dwError += ADT_WIN_ERR_BASE;
       ADT_BAIL_ON_ERROR_NP(dwError);
   }

   cleanup:
       LW_SAFE_FREE_MEMORY(hostName);
       LW_SAFE_FREE_MEMORY(userName);
       LW_SAFE_FREE_MEMORY(password);
       LW_SAFE_FREE_MEMORY(userNameN);

       return dwError;

   error:
       goto cleanup;
}

/**
 * Modify AD user account.
 *
 * @param appContext Application context reference.
 * @param level Info level.
 * @param userNameC User name.
 * @param fullNameC Full user name.
 * @param commentC Comments.
 * @param homeDirC User's home directory
 * @param scriptPathC Full path to executable logon script
 * @param passwordC Password
 * @param flags Account controls
 * @param isRenamed Will be set to true is the accont has been renamed.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserSetInfoFromParams(
    IN AppContextTP appContext,
    IN DWORD level,
    IN PSTR  userNameC,
    IN PSTR  changedUserNameC,
    IN PSTR  fullNameC,
    IN PSTR  commentC,
    IN PSTR  homeDirC,
    IN PSTR  scriptPathC,
    IN PSTR  passwordC,
    IN DWORD flags,
    IN PBOOL isRenamed
)
{
    DWORD dwError = ERROR_SUCCESS;
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
    DWORD parmErr = 0;

    PWSTR hostName = NULL;
    PWSTR userName = NULL;
    PWSTR changedUserName = NULL;
    PWSTR fullName = NULL;
    PWSTR comment = NULL;
    PWSTR homeDir = NULL;
    PWSTR scriptPath = NULL;
    PWSTR password = NULL;
    PSTR  userNameN = NULL;

    userNameN = GetNameComp(userNameC);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverAddress), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    if(changedUserNameC) {
        dwError = LwMbsToWc16s((PCSTR) changedUserNameC, &changedUserName);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (fullNameC) {
        dwError = LwMbsToWc16s((PCSTR) fullNameC, &fullName);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (commentC) {
        dwError = LwMbsToWc16s((PCSTR) commentC, &comment);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (homeDirC) {
        dwError = LwMbsToWc16s((PCSTR) homeDirC, &homeDir);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (scriptPathC) {
        dwError = LwMbsToWc16s((PCSTR) scriptPathC, &scriptPath);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    if (passwordC) {
        dwError = LwMbsToWc16s((PCSTR) passwordC, &password);
        ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);
    }

    switch (level)
    {
    case 0:
        Info0.usri0_name        = changedUserName;

        pBuffer = (PVOID)&Info0;
        break;

    case 1:
        Info1.usri1_name        = userName;
        Info1.usri1_password    = password;
        Info1.usri1_priv        = USER_PRIV_USER;
        Info1.usri1_home_dir    = homeDir;
        Info1.usri1_comment     = comment;
        Info1.usri1_flags       = flags;
        Info1.usri1_script_path = scriptPath;

        pBuffer = (PVOID)&Info1;
        break;

    case 2:
        Info2.usri2_name        = userName;
        Info2.usri2_password    = password;
        Info2.usri2_priv        = USER_PRIV_USER;
        Info2.usri2_home_dir    = homeDir;
        Info2.usri2_comment     = comment;
        Info2.usri2_flags       = flags;
        Info2.usri2_script_path = scriptPath;

        pBuffer = (PVOID)&Info2;
        break;

    case 3:
        Info3.usri3_name        = userName;
        Info3.usri3_password    = password;
        Info3.usri3_priv        = USER_PRIV_USER;
        Info3.usri3_home_dir    = homeDir;
        Info3.usri3_comment     = comment;
        Info3.usri3_flags       = flags;
        Info3.usri3_script_path = scriptPath;

        pBuffer = (PVOID)&Info3;
        break;

    case 4:
        Info4.usri4_name        = userName;
        Info4.usri4_password    = password;
        Info4.usri4_priv        = USER_PRIV_USER;
        Info4.usri4_home_dir    = homeDir;
        Info4.usri4_comment     = comment;
        Info4.usri4_flags       = flags;
        Info4.usri4_script_path = scriptPath;

        pBuffer = (PVOID)&Info4;
        break;

    case 1003:
        Info1003.usri1003_password = password;

        pBuffer = (PVOID)&Info1003;
        break;

    case 1007:
        Info1007.usri1007_comment = comment;

        pBuffer = (PVOID)&Info1007;
        break;

    case 1008:
        Info1008.usri1008_flags = flags;

        pBuffer = (PVOID)&Info1008;
        break;

    case 1011:
        Info1011.usri1011_full_name = fullName;

        pBuffer = (PVOID)&Info1011;
        break;
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Changing properties of user %s ...\n",
                appContext->actionName, userNameN);

    /* Perform the modify operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetUserSetInfo(hostName, userName, level, pBuffer, &parmErr);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done changing properties of user %s\n",
                appContext->actionName, userNameN);

    if (level == 0 && isRenamed) {
        *isRenamed = TRUE;
    }

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(userName);
        LW_SAFE_FREE_MEMORY(changedUserName);
        LW_SAFE_FREE_MEMORY(fullName);
        LW_SAFE_FREE_MEMORY(comment);
        LW_SAFE_FREE_MEMORY(homeDir);
        LW_SAFE_FREE_MEMORY(scriptPath);
        LW_SAFE_FREE_MEMORY(password);
        LW_SAFE_FREE_MEMORY(userNameN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Get AD user account properties.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param level Info level.
 * @param info Account information returned.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserGetInfo4(
    IN AppContextTP appContext,
    IN  PSTR  userNameC,
    OUT PUSER_INFO_4 *info
)
{
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = NULL;
    PWSTR hostName = NULL;
    PWSTR userName = NULL;
    PSTR  userNameN = NULL;

    userNameN = GetNameComp(userNameC);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) userNameN, &userName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Reading properties of user %s ...\n",
                appContext->actionName, userNameN);

    PrintStderr(appContext, LogLevelTrace, "%s: Calling NetUserGetInfo(%s, %s, %d, %s)\n",
                appContext->actionName, appContext->workConn->serverName, userNameN, 4, "&pBuffer");

    dwError = NetUserGetInfo(hostName, userName, 4, &pBuffer);

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done reading properties of user %s\n",
                appContext->actionName, userNameN);

    *info = (PUSER_INFO_4) pBuffer;

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(userName);
        LW_SAFE_FREE_MEMORY(userNameN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Change user's password.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param password User's password to set.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetUserChangePassword(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR  password
)
{
    BOOL isRenamed = FALSE;

    return AdtNetUserSetInfoFromParams(
        appContext,
        1,
        userNameC,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        password,
        0,
        &isRenamed
    );
}

/**********************************************************/
/*                 AD group operations                    */
/**********************************************************/

/**
 * Delete AD local group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetGroupDelete(
    IN AppContextTP appContext,
    IN PSTR aliasNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR hostName = NULL;
    PWSTR aliasName = NULL;

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) aliasNameC, &aliasName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    PrintStderr(appContext, LogLevelTrace, "%s: Deleting group %s ...\n",
                appContext->actionName, aliasNameC);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetLocalGroupDel((PCWSTR) hostName, (PCWSTR) aliasName);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done deleting group %s\n",
                appContext->actionName, aliasNameC);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(aliasName);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Add AD local group with default properties.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @return 0 on success; error code on failure.
 */
DWORD
AdtNetGroupAdd(
    IN AppContextTP appContext,
    IN PSTR aliasNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    LOCALGROUP_INFO_0 Info = { 0 };
    DWORD parmError = 0;
    PWSTR hostName = NULL;
    PWSTR aliasName = NULL;

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) aliasNameC, &aliasName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    Info.lgrpi0_name = aliasName;

    PrintStderr(appContext, LogLevelTrace, "%s: Adding group %s ...\n",
                appContext->actionName, aliasNameC);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetLocalGroupAdd((PCWSTR) hostName, 0, (PVOID) &Info, &parmError);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done adding group %s\n",
                appContext->actionName, aliasNameC);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(aliasName);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Add a member to a local domain group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @param memberNameC Memeber name.
 * @return 0 on success; error code on failure.
 */
DWORD AdtNetLocalGroupAddMember(
    IN AppContextTP appContext,
    IN PSTR aliasNameC,
    IN PSTR memberNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};
    PWSTR hostName = NULL;
    PWSTR aliasName = NULL;
    PWSTR memberName = NULL;
    PSTR  memberNameN = NULL;

    dwError = NormalizeUserName(memberNameC, appContext->workConn->domainName, &memberNameN);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) aliasNameC, &aliasName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) memberNameN, &memberName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    memberinfo.lgrmi3_domainandname = memberName;

    PrintStderr(appContext, LogLevelTrace, "%s: Adding member %s to group %s ...\n",
                appContext->actionName, memberNameN, aliasNameC);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetLocalGroupAddMembers(hostName, aliasName, 3, &memberinfo, 1);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done adding member %s to group %s ...\n",
                appContext->actionName, memberNameN, aliasNameC);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(aliasName);
        LW_SAFE_FREE_MEMORY(memberName);
        LW_SAFE_FREE_MEMORY(memberNameN);

        return dwError;

    error:
        goto cleanup;
}

/**
 * Delete a member from a local domain group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @param memberNameC Member name.
 * @return 0 on success; error code on failure.
 */
DWORD AdtNetLocalGroupDeleteMember(
    IN AppContextTP appContext,
    IN PSTR aliasNameC,
    IN PSTR memberNameC
)
{
    DWORD dwError = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};
    PWSTR hostName = NULL;
    PWSTR aliasName = NULL;
    PWSTR memberName = NULL;
    PSTR  memberNameN = NULL;

    dwError = NormalizeUserName(memberNameC, appContext->workConn->domainName, &memberNameN);
    ADT_BAIL_ON_ERROR_NP(dwError);

    dwError = LwMbsToWc16s((PCSTR) (appContext->workConn->serverName), &hostName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) aliasNameC, &aliasName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    dwError = LwMbsToWc16s((PCSTR) memberNameN, &memberName);
    ADT_BAIL_ON_ALLOC_FAILURE_NP(!dwError);

    memberinfo.lgrmi3_domainandname = memberName;

    PrintStderr(appContext, LogLevelTrace, "%s: Deleting member %s from group %s ...\n",
                appContext->actionName, memberNameN, aliasNameC);

    /* Perform the delete operation. */
    if(!appContext->gopts.isReadOnly) {
        dwError = NetLocalGroupDelMembers(hostName, aliasName, 3, &memberinfo, 1);
    }

    if (dwError) {
        dwError += ADT_WIN_ERR_BASE;
        ADT_BAIL_ON_ERROR_NP(dwError);
    }

    PrintStderr(appContext, LogLevelTrace, "%s: Done deleting member %s from group %s ...\n",
                appContext->actionName, memberNameN, aliasNameC);

    cleanup:
        LW_SAFE_FREE_MEMORY(hostName);
        LW_SAFE_FREE_MEMORY(aliasName);
        LW_SAFE_FREE_MEMORY(memberName);
        LW_SAFE_FREE_MEMORY(memberNameN);

        return dwError;

    error:
        goto cleanup;
}
