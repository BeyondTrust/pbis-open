/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) BeyondTrust Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsadefs.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Lsa rpc server definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _LSASRVDEFS_H_
#define _LSASRVDEFS_H_

#define LSA_RPC_DIR                            CACHEDIR "/rpc"
#define LSA_DEFAULT_LPC_SOCKET_PATH            LSA_RPC_DIR "/lsass"

#define LSASS_KRB5_CACHE_PATH                  "FILE:" CACHEDIR "/krb5cc_lsass"


#define LSA_BUILTIN_DOMAIN_NAME \
    {'B','U','I','L','T','I','N',0}


/*
 * Indices of an array returned from LsaSrvSelectAccountsByDomainName
 * and LsaSrvSelectAccountsByDomainSid
 */

typedef enum _LSA_ACCOUNT_TYPE
{
    LSA_DOMAIN_ACCOUNTS              = 0,
    LSA_FOREIGN_DOMAIN_ACCOUNTS,
    LSA_LOCAL_DOMAIN_ACCOUNTS,
    LSA_BUILTIN_DOMAIN_ACCOUNTS,
    LSA_OTHER_ACCOUNTS,
    LSA_ACCOUNT_TYPE_SENTINEL

} LSA_ACCOUNT_TYPE;


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


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gLsaSrvDataMutex);     \
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
        ret = pthread_mutex_unlock(&gLsaSrvDataMutex);   \
        if (ret && dwError == STATUS_SUCCESS) {          \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)


#endif /* _LSASRVDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
