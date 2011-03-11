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
 *        net.h
 *
 * Abstract:
 *
 *        
 *
 * Authors: Author: CORP\slavam
 * 
 * Created on: May 24, 2010
 *
 */

#ifndef _ADTOOL_NET_H_
#define _ADTOOL_NET_H_

/**
 * Delete AD user.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserDelete(
    IN AppContextTP appContext,
    IN PSTR userNameC
);

/**
 * Add AD user with default properties.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserAdd(
    IN AppContextTP appContext,
    IN PSTR userNameC
);

/**
 * Modify AD user account.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param passwordC Password
 * @param flags Account controls
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserSetInfo(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR  passwordC,
    IN DWORD flags
);

/**
 * Modify AD user account.
 *
 * @param appContext Application context reference.
 * @param info User information.
 * @param userNameC User name.
 * @param password Password; must be NULL if we do not want to change it.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserSetInfo4(
    IN AppContextTP appContext,
    IN PUSER_INFO_4 info,
    IN PSTR  userNameC,
    IN PSTR passwordC
);

/**
 * Reset user's password and, optionally, set account control flags.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param password Password; must be NULL if we do not want to change it.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserSetPassword(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR passwordC
);

/**
 * Set user account controls.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param flags Account controls.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserSetInfoFlags(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN DWORD flags
);

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
extern DWORD
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
);

/**
 * Get AD user account properties.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param level Info level.
 * @param info Account information returned.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserGetInfo4(
    IN  AppContextTP appContext,
    IN  PSTR  userNameC,
    OUT PUSER_INFO_4 *info
);

/**
 * Change user's password.
 *
 * @param appContext Application context reference.
 * @param userNameC User name.
 * @param password User's password to set.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetUserChangePassword(
    IN AppContextTP appContext,
    IN PSTR  userNameC,
    IN PSTR  password
);

/**
 * Delete AD local group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetGroupDelete(
    IN AppContextTP appContext,
    IN PSTR aliasNameC
);

/**
 * Add AD local group with default properties.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @return 0 on success; error code on failure.
 */
extern DWORD
AdtNetGroupAdd(
    IN AppContextTP appContext,
    IN PSTR aliasNameC
);

/**
 * Add a member to a local domain group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @param memberNameC Memeber name.
 * @return 0 on success; error code on failure.
 */
extern DWORD AdtNetLocalGroupAddMember(
    IN AppContextTP appContext,
    IN PSTR aliasNameC,
    IN PSTR memberNameC
);

/**
 * Delete a member from a local domain group.
 *
 * @param appContext Application context reference.
 * @param aliasNameC Group name.
 * @param memberNameC Member name.
 * @return 0 on success; error code on failure.
 */
extern DWORD AdtNetLocalGroupDeleteMember(
    IN AppContextTP appContext,
    IN PSTR aliasNameC,
    IN PSTR memberNameC
);

#endif /* _ADTOOL_NET_H_ */
