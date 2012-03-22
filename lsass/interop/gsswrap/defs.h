/*
 * Copyright Likewise Software
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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
