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
