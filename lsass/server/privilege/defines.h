/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software    2004-2011
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
 *        defines.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Privileges macros
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef __LSASRV_PRIVS_DEFINES_H__
#define __LSASRV_PRIVS_DEFINES_H__


#define LSASS_REG_KEY             HKEY_THIS_MACHINE "\\Services\\lsass"
#define LSA_PRIVILEGES_REG_KEY    LSASS_REG_KEY "\\Privileges"
#define LSA_ACCOUNTS_REG_KEY      LSASS_REG_KEY "\\Accounts"

#define LSA_PRIVILEGE_LUID_LOW_NAME       "ValueLow"
#define LSA_PRIVILEGE_LUID_HIGH_NAME      "ValueHigh"
#define LSA_PRIVILEGE_ENABLED_NAME        "EnabledByDefault"
#define LSA_PRIVILEGE_DESCRIPTION_NAME    "Description"
#define LSA_PRIVILEGE_SECURITY_DESC_NAME  "SecurityDescriptor"

#define LSA_PRIVILEGE_LUID_LOW_NAME_W \
    {'V','a','l','u','e','L','o','w','\0'}
#define LSA_PRIVILEGE_LUID_HIGH_NAME_W \
    {'V','a','l','u','e','H','i','g','h','\0'}
#define LSA_PRIVILEGE_ENABLED_NAME_W \
    {'E','n','a','b','l','e','d','B','y','D','e','f','a','u','l','t','\0'}
#define LSA_PRIVILEGE_DESCRIPTION_NAME_W \
    {'D','e','s','c','r','i','p','t','i','o','n','\0'}
#define LSA_PRIVILEGE_SECURITY_DESC_NAME_W \
    {'S','e','c','u','r','i','t','y','D','e','s','c','r','i','p','t','o','r','\0'}

#define LSA_ACCOUNT_PRIVILEGES_NAME         "Privileges"
#define LSA_ACCOUNT_SYS_ACCESS_RIGHTS_NAME  "SystemAccessRights"

#define LSA_ACCOUNT_PRIVILEGES_NAME_W \
    {'P','r','i','v','i','l','e','g','e','s','\0'}
#define LSA_ACCOUNT_SYS_ACCESS_RIGHTS_NAME_W \
    {'S','y','s','t','e','m','A','c','c','e','s','s','R','i','g','h','t','s','\0'}

#define LSA_PRIVILEGES_DB_SIZE    (16)
#define LSA_ACCOUNTS_DB_SIZE      (16)

#define LSA_PRIVILEGE_VALID_PREFIXES {"Se"}

#define LSA_MAX_PRIVILEGES_COUNT  (100)


#define LSASRV_PRIVS_WRLOCK_RWLOCK(bInLock, pRwLock) \
        if (!bInLock) { \
            pthread_rwlock_wrlock(pRwLock); \
            bInLock = TRUE; \
        }

#define LSASRV_PRIVS_RDLOCK_RWLOCK(bInLock, pRwLock) \
        if (!bInLock) { \
            pthread_rwlock_rdlock(pRwLock); \
            bInLock = TRUE; \
        }

#define LSASRV_PRIVS_UNLOCK_RWLOCK(bInLock, pRwLock) \
        if (bInLock) { \
            pthread_rwlock_unlock(pRwLock); \
            bInLock = FALSE; \
        }


#endif /* __LSASRV_PRIVS_DEFINES_H__ */
