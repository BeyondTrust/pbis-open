
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        credentials.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
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
