/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samrdefs.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMRSRVDEFS_H_
#define _SAMRSRVDEFS_H_

#define LSA_RPC_DIR                              CACHEDIR "/rpc"

#define SAMR_RPC_CFG_DEFAULT_LPC_SOCKET_PATH     LSA_RPC_DIR "/lsass"
#define SAMR_RPC_CFG_DEFAULT_LOGIN_SHELL         "/bin/sh"
#define SAMR_RPC_CFG_DEFAULT_HOMEDIR_PREFIX      "/home"
#define SAMR_RPC_CFG_DEFAULT_HOMEDIR_TEMPLATE    "%H/%U"

#define LSASS_KRB5_CACHE_PATH                    "FILE:" CACHEDIR "/krb5cc_lsass"



#define DS_ATTR_RECORD_ID \
    {'O','b','j','e','c','t','R','e','c','o','r','d','I','d',0}
#define DS_ATTR_DISTINGUISHED_NAME  \
    {'D','i','s','t','i','n','g','u','i','s','h','e','d','N','a','m','e',0}
#define DS_ATTR_OBJECT_CLASS \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define DS_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define DS_ATTR_SECURITY_DESCRIPTOR \
    {'S','e','c','u','r','i','t','y','D','e','s','c','r','i','p','t','o','r',0}
#define DS_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define DS_ATTR_DOMAIN \
    {'D','o','m','a','i','n',0}
#define DS_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define DS_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define DS_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define DS_ATTR_UID \
    {'U','I','D',0}
#define DS_ATTR_GID \
    {'G','I','D',0}
#define DS_ATTR_PRIMARY_GROUP \
    {'P','r','i','m','a','r','y','G','r','o','u','p',0}
#define DS_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define DS_ATTR_DESCRIPTION \
    {'D','e','s','c','r','i','p','t','i','o','n',0}
#define DS_ATTR_COMMENT \
    {'C','o','m','m','e','n','t',0}
#define DS_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define DS_ATTR_HOME_DRIVE \
    {'H','o','m','e','d','r','i','v','e',0}
#define DS_ATTR_SHELL \
    {'L','o','g','i','n','S','h','e','l','l',0}
#define DS_ATTR_LOGON_SCRIPT \
    {'L','o','g','o','n','S','c','r','i','p','t',0}
#define DS_ATTR_PROFILE_PATH \
    {'P','r','o','f','i','l','e','P','a','t','h',0}
#define DS_ATTR_WORKSTATIONS \
    {'W','o','r','k','s','t','a','t','i','o','n','s',0}
#define DS_ATTR_PARAMETERS \
    {'P','a','r','a','m','e','t','e','r','s',0}
#define DS_ATTR_PASSWORD_LAST_SET \
    {'P','a','s','s','w','o','r','d','L','a','s','t','S','e','t',0}
#define DS_ATTR_ALLOW_PASSWORD_CHANGE \
    {'A','l','l','o','w','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DS_ATTR_FORCE_PASSWORD_CHANGE \
    {'F','o','r','c','e','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define DS_ATTR_PASSWORD \
    {'P','a','s','s','w','o','r','d',0}
#define DS_ATTR_ACCOUNT_FLAGS \
    {'A','c','c','o','u','n','t','F','l','a','g','s',0}
#define DS_ATTR_CREATED_TIME \
    {'C','r','e','a','t','e','d','T','i','m','e',0}
#define DS_ATTR_MIN_PWD_LENGTH \
    {'M','i','n','P','w','d','L','e','n','g','t','h',0}
#define DS_ATTR_PWD_PROMPT_TIME \
    {'P','w','d','P','r','o','m','p','t','T','i','m','e',0}
#define DS_ATTR_PWD_HISTORY_LENGTH \
    {'P','w','d','H','i','s','t','o','r','y','L','e','n','g','t','h',0}
#define DS_ATTR_PWD_PROPERTIES \
    {'P','w','d','P','r','o','p','e','r','t','i','e','s',0}
#define DS_ATTR_MAX_PWD_AGE \
    {'M','a','x','P','w','d','A','g','e',0}
#define DS_ATTR_MIN_PWD_AGE \
    {'M','i','n','P','w','d','A','g','e',0}
#define DS_ATTR_LAST_LOGON \
    {'L','a','s','t','L','o','g','o','n',0}
#define DS_ATTR_LAST_LOGOFF \
    {'L','a','s','t','L','o','g','o','f','f',0}
#define DS_ATTR_FORCE_LOGOFF_TIME \
    {'F','o','r','c','e','L','o','g','o','f','f','T','i','m','e',0}
#define DS_ATTR_SEQUENCE_NUMBER \
    {'S','e','q','u','e','n','c','e','N','u','m','b','e','r',0}
#define DS_ATTR_LOCKOUT_DURATION \
    {'L','o','c','k','o','u','t','D','u','r','a','t','i','o','n',0}
#define DS_ATTR_LOCKOUT_WINDOW \
    {'L','o','c','k','o','u','t','W','i','n','d','o','w',0}
#define DS_ATTR_LOCKOUT_THRESHOLD \
    {'L','o','c','k','o','u','t','T','h','r','e','s','h','o','l','d',0}
#define DS_ATTR_LOGON_COUNT \
    {'L','o','g','o','n','C','o','u','n','t',0}
#define DS_ATTR_BAD_PASSWORD_COUNT \
    {'B','a','d','P','w','d','C','o','u','n','t',0}
#define DS_ATTR_LOGON_HOURS \
    {'L','o','g','o','n','H','o','u','r','s',0}
#define DS_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define DS_ATTR_ROLE \
    {'R','o','l','e',0}
#define DS_ATTR_COUNTRY_CODE \
    {'C','o','u','n','t','r','y','C','o','d','e',0}
#define DS_ATTR_CODE_PAGE \
    {'C','o','d','e','P','a','g','e',0}
#define DS_ATTR_LM_HASH \
    {'L','M','H','a','s','h',0}
#define DS_ATTR_NT_HASH \
    {'N','T','H','a','s','h',0}


#define DS_OBJECT_CLASS_UNKNOWN          (0)
#define DS_OBJECT_CLASS_DOMAIN           (1)
#define DS_OBJECT_CLASS_BUILTIN_DOMAIN   (2)
#define DS_OBJECT_CLASS_CONTAINER        (3)
#define DS_OBJECT_CLASS_LOCAL_GROUP      (4)
#define DS_OBJECT_CLASS_USER             (5)
#define DS_OBJECT_CLASS_LOCALGRP_MEMBER  (6)
#define DS_OBJECT_CLASS_SENTINEL         (7)


#define SAMR_BUILTIN_DOMAIN_NAME \
    {'B','U','I','L','T','I','N',0}


#define BAIL_ON_NTSTATUS_ERROR(status)                   \
    do {                                                 \
        if ((status) != STATUS_SUCCESS) {                \
            LSA_LOG_DEBUG("Error at %s:%d code: %s "     \
                          "(0x%08x)",                    \
                          __FILE__, __LINE__,            \
                          LwNtStatusToName((status)),    \
                          (status));                     \
            goto error;                                  \
        }                                                \
    } while (0)



#define BAIL_ON_NO_MEMORY(ptr)                           \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            ntStatus = STATUS_NO_MEMORY;                 \
            LSA_LOG_ERROR("Error: out of memory");       \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PTR(ptr)                         \
    do {                                                 \
        if (ptr == NULL) {                               \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid pointer");     \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PARAMETER(cond)                  \
    do {                                                 \
        if (!(cond)) {                                   \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid parameter");   \
            goto error;                                  \
        }                                                \
    } while (0)


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gSamrSrvDataMutex);    \
        if (ret) {                                       \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 1;                                \
        }                                                \
    } while (0)


#define GLOBAL_DATA_UNLOCK(locked)                       \
    do {                                                 \
        int ret = 0;                                     \
        if (!locked) break;                              \
        ret = pthread_mutex_unlock(&gSamrSrvDataMutex);  \
        if (ret && dwError == STATUS_SUCCESS) {          \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)



#endif /* _SAMRSRVDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
