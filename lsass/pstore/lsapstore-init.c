/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *     lsapstore-init.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Initialization code
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"
#include "lsapstore-backend.h"
#include <lw/errno.h>

typedef struct _LSA_PSTORE_STATE {
    struct {
        pthread_once_t OnceControl;
        DWORD Error;
        BOOLEAN Done;
    } Init;
    pthread_mutex_t Mutex;
    pthread_mutex_t* pMutex;
    LONG RefCount;
    PLSA_PSTORE_BACKEND_STATE BackendState;
} LSA_PSTORE_STATE, *PLSA_PSTORE_STATE;

static LSA_PSTORE_STATE LsaPstoreState = { { ONCE_INIT } };

//
// Prototypes
//

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    );

//
// Functions
//

__attribute__((constructor))
VOID
LsaPstoreInitializeLibrary(
    VOID
    )
{
    LwInterlockedIncrement(&LsaPstoreState.RefCount);
}

__attribute__((destructor))
VOID
LsaPstoreCleanupLibrary(
    VOID
    )
{
    if (0 == LwInterlockedDecrement(&LsaPstoreState.RefCount))
    {
        LsaPstorepCleanupLibraryInternal();
    }
}

static
DWORD
LsaPstorepInitializeLibraryInternal(
    VOID
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    int error = 0;
    pthread_mutexattr_t mutexAttributes;
    pthread_mutexattr_t* pMutexAttributes = NULL;

    if (pState->Init.Done)
    {
        LwInterlockedIncrement(&pState->RefCount);

        dwError = pState->Init.Error;
        GOTO_CLEANUP_EE(EE);
    }

    error = pthread_mutexattr_init(&mutexAttributes);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pMutexAttributes = &mutexAttributes;

    error = pthread_mutexattr_settype(pMutexAttributes, PTHREAD_MUTEX_RECURSIVE);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    error = pthread_mutex_init(&pState->Mutex, pMutexAttributes);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pState->pMutex = &pState->Mutex;

    dwError = LsaPstorepBackendInitialize(&pState->BackendState);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

cleanup:
    if (dwError)
    {
        LsaPstorepCleanupLibraryInternal();
    }

    if (pMutexAttributes)
    {
        pthread_mutexattr_destroy(pMutexAttributes);
    }

    pState->Init.Error = dwError;
    pState->Init.Done = TRUE;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    )
{
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    pthread_once_t onceControl = ONCE_INIT;

    if (pState->BackendState)
    {
        LsaPstorepBackendCleanup(pState->BackendState);
        pState->BackendState = NULL;
    }

    if (pState->pMutex)
    {
        pthread_mutex_destroy(pState->pMutex);
        pState->pMutex = NULL;
    }

    pState->Init.OnceControl = onceControl;
    pState->Init.Error = 0;
    pState->Init.Done = FALSE;
}

static
VOID
LsaPstoreInitializeLibraryOnce(
    VOID
    )
{
    LsaPstorepInitializeLibraryInternal();
}

DWORD
LsaPstorepEnsureInitialized(
    OUT PLSA_PSTORE_BACKEND_STATE* BackendState
    )
{
    DWORD dwError = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    PLSA_PSTORE_BACKEND_STATE backendState = NULL;

    if (pState->RefCount)
    {
        pthread_once(&pState->Init.OnceControl, LsaPstoreInitializeLibraryOnce);
        dwError = pState->Init.Error;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }
    else
    {
        dwError = ERROR_DLL_INIT_FAILED;
        GOTO_CLEANUP_ON_WINERROR(dwError);
    }

    backendState = pState->BackendState;

cleanup:
    if (dwError)
    {
        backendState = NULL;
    }

    *BackendState = backendState;

    LSA_PSTORE_LOG_LEAVE_ERROR(dwError);
    return dwError;
}

VOID
LsaPstorepLock(
    VOID
    )
{
    int error = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    error = pthread_mutex_lock(pState->pMutex);
    if (error)
    {
        LW_RTL_LOG_ERROR("Failed to lock mutex with errno = %d (%s)",
                         error, LwErrnoToName(error));
        assert(!error);
    }
}

VOID
LsaPstorepUnlock(
    VOID
    )
{
    int error = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    error = pthread_mutex_unlock(pState->pMutex);
    if (error)
    {
        LW_RTL_LOG_ERROR("Failed to unlock mutex with errno = %d (%s)",
                         error, LwErrnoToName(error));
        assert(!error);
    }
}
