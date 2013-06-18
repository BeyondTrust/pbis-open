/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbuser.h
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM User Specific Management Methods
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __SAM_DB_USER_H__
#define __SAM_DB_USER_H__

DWORD
SamDbSetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
SamDbChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

DWORD
SamDbVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
SamDbGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    );

#endif /* __SAM_DB_USER_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
