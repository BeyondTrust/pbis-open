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
 *        defs.h
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *        GSSAPI Wrappers
 *
 *        Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define LSA_GSS_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LSA_GSS_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           abort(); \
       } \
       bInLock = FALSE; \
    }

#define BAIL_ON_SEC_ERROR(pGssErrorInfo)                             \
    if (((pGssErrorInfo)->dwMajorStatus != GSS_S_COMPLETE) &&        \
        ((pGssErrorInfo)->dwMajorStatus != GSS_S_CONTINUE_NEEDED)) { \
        dwError = LW_ERROR_GSS_CALL_FAILED;                          \
        goto error;                                                  \
    }

#define BAIL_ON_LSA_GSS_ERROR(dwError) \
    if (dwError) \
    { \
        goto error; \
    }

#define BAIL_ON_INVALID_POINTER(p) \
    if (!p) \
    { \
        dwError = LW_ERROR_INVALID_PARAMETER; \
        goto error; \
    }

#define LSA_GSS_LOG_ERROR(fmt,...)
#define LSA_GSS_LOG_VERBOSE(fmt,...)
