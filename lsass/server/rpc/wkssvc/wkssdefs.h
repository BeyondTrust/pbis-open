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
 *        wkssvcdefs.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        WksSvc rpc server definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _WKSSDEFS_H_
#define _WKSSDEFS_H_

#define LSA_RPC_DIR                            CACHEDIR "/rpc"
#define WKSS_RPC_CFG_DEFAULT_LPC_SOCKET_PATH   LSA_RPC_DIR "/lsass"

#define LSASS_KRB5_CACHE_PATH                  "FILE:" CACHEDIR "/krb5cc_lsass"


/*
 * Internal access flags for doing security checks
 */
#define WKSSVC_ACCESS_GET_INFO_1               (0x00000001)
#define WKSSVC_ACCESS_GET_INFO_2               (0x00000002)
#define WKSSVC_ACCESS_SET_INFO_1               (0x00000004)
#define WKSSVC_ACCESS_SET_INFO_2               (0x00000008)
#define WKSSVC_ACCESS_JOIN_DOMAIN              (0x00000010)
#define WKSSVC_ACCESS_RENAME_MACHINE           (0x00000020)


#define BAIL_ON_NO_MEMORY(ptr, err)                      \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            err = ERROR_OUTOFMEMORY;                     \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PTR(ptr, err)                    \
    do {                                                 \
        if (ptr == NULL) {                               \
            err = ERROR_INVALID_PARAMETER;               \
            LSA_LOG_ERROR("Error: invalid pointer");     \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PARAMETER(cond, err)             \
    do {                                                 \
        if (!(cond)) {                                   \
            err = ERROR_INVALID_PARAMETER;               \
            LSA_LOG_ERROR("Error: invalid parameter");   \
            goto error;                                  \
        }                                                \
    } while (0)


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gWkssSrvDataMutex);    \
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
        ret = pthread_mutex_unlock(&gWkssSrvDataMutex);  \
        if (ret && dwError == STATUS_SUCCESS) {          \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)


#endif /* _WKSSDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
