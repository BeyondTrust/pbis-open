
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
 *        credentials.c
 *
 * Abstract:
 *
 *        BeyondTrust Security and Authentication Subsystem (LSASS)
 *
 *
 *
 * Authors:
 */
#include "api.h"
#include <lsasrvcred.h>

#define ENTER_CREDS_LIST(bInLock)                               \
    do                                                          \
    {                                                           \
        if (!bInLock)                                           \
        {                                                       \
            if (pthread_mutex_lock(&gLsaCredState.LsaCredsListLock) < 0) \
            { \
                abort(); \
            } \
            bInLock = TRUE;                                     \
        }                                                       \
    } while (0)

#define LEAVE_CREDS_LIST(bReleaseLock)                          \
    do                                                          \
    {                                                           \
        if (bReleaseLock)                                       \
        {                                                       \
            if (pthread_mutex_unlock(&gLsaCredState.LsaCredsListLock) < 0) \
            { \
                abort(); \
            } \
            bReleaseLock = FALSE;                               \
        }                                                       \
    } while (0)

typedef struct _LSA_CREDENTIALS
{
    PSTR            pUserName;
    PSTR            pPassword;
    uid_t           UserId;
    LONG            nRefCount;
    LSA_LIST_LINKS   ListEntry;
} LSA_CREDENTIALS,  *PLSA_CREDENTIALS;

typedef struct _LSA_CREDENTIALS_STATE
{
    LSA_LIST_LINKS LsaCredsList;
    BOOLEAN bIsInitialized;
    pthread_mutex_t LsaCredsListLock;
} LSA_CREDENTIALS_STATE, *PLSA_CREDENTIALS_STATE;

static LSA_CREDENTIALS_STATE gLsaCredState = {
    .LsaCredsListLock = PTHREAD_MUTEX_INITIALIZER
    };

static
PLSA_CREDENTIALS
LsaFindCredByUidUnsafe(
    IN uid_t Uid
    )
{
    // Note that gLsaCredState.LsaCredsListLock must already be acquired.

    PLSA_CREDENTIALS pCredTrav = NULL;
    PLSA_LIST_LINKS pCredListEntry = NULL;
    PLSA_CREDENTIALS pCred = NULL;

    for (pCredListEntry = gLsaCredState.LsaCredsList.Next;
         pCredListEntry != &gLsaCredState.LsaCredsList;
         pCredListEntry = pCredListEntry->Next)
    {
        pCredTrav = LW_STRUCT_FROM_FIELD(
            pCredListEntry,
            LSA_CREDENTIALS,
            ListEntry);

        if (Uid == pCredTrav->UserId)
        {
            InterlockedIncrement(&pCredTrav->nRefCount);
            pCred = pCredTrav;
            break;
        }
    }

    return pCred;
}

static
VOID
LsaFreeCred(
    IN OUT PLSA_CREDENTIALS pCredential
    )
{
    if (pCredential)
    {
        LW_SAFE_FREE_MEMORY(pCredential->pUserName);
        LW_SECURE_FREE_STRING(pCredential->pPassword);
        LwFreeMemory(pCredential);
    }
}

static
DWORD
LsaAllocateCred(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const uid_t* pUid,
    OUT PLSA_CREDENTIALS* ppCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CREDENTIALS pCred = NULL;

    dwError = LwAllocateMemory(sizeof(*pCred), OUT_PPVOID(&pCred));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszUserName, &pCred->pUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszPassword, &pCred->pPassword);
    BAIL_ON_LSA_ERROR(dwError);

    pCred->nRefCount = 1;

    if (pUid)
    {
        pCred->UserId = *pUid;
    }

cleanup:
    *ppCredential = pCred;

    return dwError;

error:
    LsaFreeCred(pCred);
    pCred = NULL;

    goto cleanup;
}

static
BOOLEAN
LsaCredContains(
    IN PLSA_CREDENTIALS pCred,
    IN PCSTR pszUserName,
    IN PCSTR pszPassword
    )
{
    BOOLEAN bMatches = TRUE;

    if (strcasecmp(pszUserName, pCred->pUserName))
    {
        bMatches = FALSE;
    }

    if (strcmp(pszPassword, pCred->pPassword))
    {
        bMatches = FALSE;
    }

    return bMatches;
}

static
VOID
LsaInitializeCredentialsDatabase(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    if (!gLsaCredState.bIsInitialized)
    {
        ENTER_CREDS_LIST(bInLock);

        if (!gLsaCredState.bIsInitialized)
        {
            LsaListInit(&gLsaCredState.LsaCredsList);
            gLsaCredState.bIsInitialized = TRUE;
        }

        LEAVE_CREDS_LIST(bInLock);
    }
}

DWORD
LsaAddCredential(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const uid_t* pUid,
    OUT PLSA_CRED_HANDLE phCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCredOld = NULL;
    PLSA_CREDENTIALS pCredNew = NULL;
    LSA_CRED_HANDLE CredHandle = NULL;

    LsaInitializeCredentialsDatabase();

    if (!pszUserName  ||
        !pszPassword  ||
        (pUid && !*pUid))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_CREDS_LIST(bInLock);

    if (pUid)
    {
        pCredOld = LsaFindCredByUidUnsafe(*pUid);
    }

    if (!pCredOld || !LsaCredContains(pCredOld, pszUserName, pszPassword))
    {
        dwError = LsaAllocateCred(pszUserName, pszPassword, pUid, &pCredNew);
        BAIL_ON_LSA_ERROR(dwError);

        LsaListInsertHead(&gLsaCredState.LsaCredsList, &pCredNew->ListEntry);

        if (pCredOld)
        {
            LsaListRemove(&pCredOld->ListEntry);

            // This release is NOT intended to destroy the credential (some one
            // may still be using it).  It's intended to remove the reference
            // added by the LsaFindCredByUid called above.
        }

        CredHandle = pCredNew;
        pCredNew = NULL;
    }
    else
    {
        CredHandle = pCredOld;
        pCredOld = NULL;
    }

cleanup:
    LEAVE_CREDS_LIST(bInLock);

    if (dwError)
    {
        LsaReleaseCredential(&CredHandle);
    }

    LsaReleaseCredential(&pCredOld);
    LsaReleaseCredential(&pCredNew);

    *phCredential = CredHandle;

    return dwError;

error:
    goto cleanup;
}

VOID
LsaReferenceCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    InterlockedIncrement(&hCredential->nRefCount);
}

VOID
LsaReleaseCredential(
    IN PLSA_CRED_HANDLE phCredential
    )
{
    BOOLEAN bInLock = FALSE;

    if (*phCredential)
    {
        PLSA_CREDENTIALS pCred = *phCredential;
        LONG count = 0;

        ENTER_CREDS_LIST(bInLock);

        count = InterlockedDecrement(&pCred->nRefCount);
        LW_ASSERT(count >= 0);

        if (0 == count)
        {
            LsaListRemove(&pCred->ListEntry);
        }

        LEAVE_CREDS_LIST(bInLock);

        if (0 == count)
        {
            LsaFreeCred(pCred);
        }

        *phCredential = NULL;
    }
}

LSA_CRED_HANDLE
LsaGetCredential(
    IN uid_t Uid
    )
{
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;

    LsaInitializeCredentialsDatabase();

    ENTER_CREDS_LIST(bInLock);

    pCred = LsaFindCredByUidUnsafe(Uid);

    LEAVE_CREDS_LIST(bInLock);

    return pCred;
}

VOID
LsaGetCredentialInfo(
    IN LSA_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL uid_t* pUid
    )
{
    if (pszUserName)
    {
        *pszUserName = CredHandle->pUserName;
    }

    if (pszPassword)
    {
        *pszPassword = CredHandle->pPassword;
    }

    if (pUid)
    {
        *pUid = CredHandle->UserId;
    }
}
