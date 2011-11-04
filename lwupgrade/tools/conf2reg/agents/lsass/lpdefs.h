/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lpdefs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Defines)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LPDEFS_H__
#define __LPDEFS_H__

#define LOCAL_CFG_TAG_LOCAL_PROVIDER "lsa-local-provider"
#define LOCAL_CFG_TAG_AUTH_PROVIDER  "auth provider"

#define LOCAL_CFG_DEFAULT_LOGIN_SHELL             "/bin/sh"
#define LOCAL_CFG_DEFAULT_HOMEDIR_PREFIX          "/home"
#define LOCAL_CFG_DEFAULT_HOMEDIR_TEMPLATE        "%H/%U"
#define LOCAL_CFG_DEFAULT_SKELETON_DIRS           "/etc/skel"
#define LOCAL_CFG_DEFAULT_CREATE_HOMEDIR          TRUE
#define LOCAL_CFG_DEFAULT_HOMEDIR_UMASK           "022"
#define LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT (5)

#define LOCAL_LOCK_MUTEX(bInLock, pMutex)  \
        if (!bInLock) {                    \
           pthread_mutex_lock(pMutex);     \
           bInLock = TRUE;                 \
        }

#define LOCAL_UNLOCK_MUTEX(bInLock, pMutex) \
        if (bInLock) {                      \
           pthread_mutex_unlock(pMutex);    \
           bInLock = FALSE;                 \
        }

#define LOCAL_OBJECT_CLASS_UNKNOWN         0
#define LOCAL_OBJECT_CLASS_DOMAIN          1
#define LOCAL_OBJECT_CLASS_GROUP           4
#define LOCAL_OBJECT_CLASS_USER            5
#define LOCAL_OBJECT_CLASS_GROUP_MEMBER    6

#define LOCAL_DB_DIR_ATTR_OBJECT_CLASS         "ObjectClass"
#define LOCAL_DB_DIR_ATTR_OBJECT_SID           "ObjectSID"
#define LOCAL_DB_DIR_ATTR_DISTINGUISHED_NAME   "DistinguishedName"
#define LOCAL_DB_DIR_ATTR_DOMAIN               "Domain"
#define LOCAL_DB_DIR_ATTR_NETBIOS_NAME         "NetBIOSName"
#define LOCAL_DB_DIR_ATTR_COMMON_NAME          "CommonName"
#define LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME     "SamAccountName"
#define LOCAL_DB_DIR_ATTR_USER_PRINCIPAL_NAME  "UserPrincipalName"
#define LOCAL_DB_DIR_ATTR_UID                  "UID"
#define LOCAL_DB_DIR_ATTR_GID                  "GID"
#define LOCAL_DB_DIR_ATTR_PASSWORD             "Password"
#define LOCAL_DB_DIR_ATTR_LM_HASH              "LMHash"
#define LOCAL_DB_DIR_ATTR_NT_HASH              "NTHash"
#define LOCAL_DB_DIR_ATTR_ACCOUNT_FLAGS        "AccountFlags"
#define LOCAL_DB_DIR_ATTR_PASSWORD_LAST_SET    "PasswordLastSet"
#define LOCAL_DB_DIR_ATTR_FULL_NAME            "FullName"
#define LOCAL_DB_DIR_ATTR_ACCOUNT_EXPIRY       "AccountExpiry"
#define LOCAL_DB_DIR_ATTR_GECOS                "Gecos"
#define LOCAL_DB_DIR_ATTR_HOME_DIR             "Homedir"
#define LOCAL_DB_DIR_ATTR_SHELL                "LoginShell"
#define LOCAL_DB_DIR_ATTR_MAX_PWD_AGE          "MaxPwdAge"
#define LOCAL_DB_DIR_ATTR_PWD_PROMPT_TIME      "PwdPromptTime"

#define LOCAL_DIR_ATTR_OBJECT_CLASS  \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define LOCAL_DIR_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define LOCAL_DIR_ATTR_DISTINGUISHED_NAME  \
    {'D','i','s','t','i','n','g','u','i','s','h','e','d','N','a','m','e',0}
#define LOCAL_DIR_ATTR_DOMAIN \
    {'D','o','m','a','i','n',0}
#define LOCAL_DIR_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define LOCAL_DIR_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME \
    {'U','s','e','r','P','r','i','n','c','i','p','a','l','N','a','m','e',0}
#define LOCAL_DIR_ATTR_UID \
    {'U','I','D',0}
#define LOCAL_DIR_ATTR_GID \
    {'G','I','D',0}
#define LOCAL_DIR_ATTR_PASSWORD \
    {'P','a','s','s','w','o','r','d',0}
#define LOCAL_DIR_ATTR_LM_HASH \
    {'L','M','H','a','s','h',0}
#define LOCAL_DIR_ATTR_NT_HASH \
    {'N','T','H','a','s','h',0}
#define LOCAL_DIR_ATTR_ACCOUNT_FLAGS \
    {'A','c','c','o','u','n','t','F','l','a','g','s',0}
#define LOCAL_DIR_ATTR_PASSWORD_LAST_SET \
    {'P','a','s','s','w','o','r','d','L','a','s','t','S','e','t',0}
#define LOCAL_DIR_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define LOCAL_DIR_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define LOCAL_DIR_ATTR_GECOS \
    {'G','e','c','o','s',0}
#define LOCAL_DIR_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define LOCAL_DIR_ATTR_SHELL \
    {'L','o','g','i','n','S','h','e','l','l',0}
#define LOCAL_DIR_ATTR_MAX_PWD_AGE \
    {'M','a','x','P','w','d','A','g','e',0}
#define LOCAL_DIR_ATTR_PWD_PROMPT_TIME \
    {'P','w','d','P','r','o','m','p','t','T','i','m','e',0}
#define LOCAL_DIR_ATTR_PRIMARY_GROUP \
    {'P','r','i','m','a','r','y','G','r','o','u','p',0}
#define LOCAL_DIR_ATTR_LAST_LOGON \
    {'L','a','s','t','L','o','g','o','n',0}
#define LOCAL_DIR_ATTR_LAST_LOGOFF \
    {'L','a','s','t','L','o','g','o','f','f',0}

#define LOCAL_DIR_CN_PREFIX_ANSI  "CN="
#define LOCAL_DIR_OU_PREFIX_ANSI  "OU="
#define LOCAL_DIR_DC_PREFIX_ANSI  "DC="
#define LOCAL_DIR_DELIMITER_COMMA ","
#define LOCAL_DIR_DELIMITER_DOT   "."

#define LOCAL_DIR_CN_PREFIX \
    {'C','N','=',0}
#define LOCAL_DIR_OU_PREFIX \
    {'O','U','=',0}
#define LOCAL_DIR_DC_PREFIX \
    {'D','C','=',0}

typedef DWORD LOCAL_ACB, *PLOCAL_ACB;

#define LOCAL_ACB_DISABLED                 (0x00000001)
#define LOCAL_ACB_HOMDIRREQ                (0x00000002)
#define LOCAL_ACB_PWNOTREQ                 (0x00000004)
#define LOCAL_ACB_TEMPDUP                  (0x00000008)
#define LOCAL_ACB_NORMAL                   (0x00000010)
#define LOCAL_ACB_MNS                      (0x00000020)
#define LOCAL_ACB_DOMTRUST                 (0x00000040)
#define LOCAL_ACB_WSTRUST                  (0x00000080)
#define LOCAL_ACB_SVRTRUST                 (0x00000100)
#define LOCAL_ACB_PWNOEXP                  (0x00000200)
#define LOCAL_ACB_AUTOLOCK                 (0x00000400)
#define LOCAL_ACB_ENC_TXT_PWD_ALLOWED      (0x00000800)
#define LOCAL_ACB_SMARTCARD_REQUIRED       (0x00001000)
#define LOCAL_ACB_TRUSTED_FOR_DELEGATION   (0x00002000)
#define LOCAL_ACB_NOT_DELEGATED            (0x00004000)
#define LOCAL_ACB_USE_DES_KEY_ONLY         (0x00008000)
#define LOCAL_ACB_DONT_REQUIRE_PREAUTH     (0x00010000)
#define LOCAL_ACB_PW_EXPIRED               (0x00020000)
#define LOCAL_ACB_NO_AUTH_DATA_REQD        (0x00080000)

#define LOWEST_UID   (1000)
#define LOWEST_GID   (1000)


#endif /* __LPDEFS_H__ */

