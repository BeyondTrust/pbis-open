/*
 * Copyright Likewise Software
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
 * Module Name:
 *
 *        dfs.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common DFS Code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

#define NEGATIVE_CACHE_TIME 600

static LW_LIST_LINKS gDfsNamespaces = {&gDfsNamespaces, &gDfsNamespaces};
static pthread_mutex_t gDfsLock = PTHREAD_MUTEX_INITIALIZER;

static
VOID
RdrDfsFreeNamespace(
    PRDR_DFS_NAMESPACE pNamespace
    )
{
    USHORT usIndex = 0;

    if (pNamespace)
    {
        if (pNamespace->pReferrals)
        {
            for (usIndex = 0; usIndex < pNamespace->usReferralCount; usIndex++)
            {
                RTL_FREE(&pNamespace->pReferrals[usIndex].pwszReferral);
            }

            RTL_FREE(&pNamespace->pReferrals);
        }

        RTL_FREE(&pNamespace);
    }
}

static
VOID
RdrDfsExpireNamespaces(
    ULONG ulTime
    )
{
    PLW_LIST_LINKS pLink = NULL;
    PLW_LIST_LINKS pNext = NULL;

    for (pLink = gDfsNamespaces.Next; pLink != &gDfsNamespaces; pLink = pNext)
    {
        PRDR_DFS_NAMESPACE pNamespace = LW_STRUCT_FROM_FIELD(pLink, RDR_DFS_NAMESPACE, Link);
        pNext = pLink->Next;

        if (pNamespace->ulExpirationTime <= ulTime)
        {
            LwListRemove(pLink);
            RdrDfsFreeNamespace(pNamespace);
        }
    }
}

/* Call with lock held */
static
PRDR_DFS_NAMESPACE
RdrDfsFindPreciseNamespace(
    PCWSTR pwszNamespace
    )
{
    PLW_LIST_LINKS pLink = NULL;

    while ((pLink = LwListTraverse(&gDfsNamespaces, pLink)))
    {
        PRDR_DFS_NAMESPACE pNamespace = LW_STRUCT_FROM_FIELD(pLink, RDR_DFS_NAMESPACE, Link);

        if (LwRtlWC16StringIsEqual(pwszNamespace, pNamespace->pwszNamespace, FALSE))
        {
            return pNamespace;
        }
    }

    return NULL;
}

/* Call with lock held
 * FIXME: expire stale entries
 */
static
PRDR_DFS_NAMESPACE
RdrDfsFindMatchingNamespace(
    PCWSTR pwszPath
    )
{
    PLW_LIST_LINKS pLink = NULL;
    UNICODE_STRING path = {0};
    UNICODE_STRING namespace = {0};

    LwRtlUnicodeStringInit(&path, pwszPath);

    while ((pLink = LwListTraverse(&gDfsNamespaces, pLink)))
    {
        PRDR_DFS_NAMESPACE pNamespace = LW_STRUCT_FROM_FIELD(pLink, RDR_DFS_NAMESPACE, Link);

        LwRtlUnicodeStringInit(&namespace, pNamespace->pwszNamespace);

        if (LwRtlUnicodeStringIsPrefix(&namespace, &path, FALSE))
        {
            return pNamespace;
        }
    }

    return NULL;
}

/* Inserts namespace into list, making sure longer entries come first.
 * Call with lock held
 */
static
VOID
RdrDfsInsertNamespace(
    PRDR_DFS_NAMESPACE pNamespace
    )
{
    PLW_LIST_LINKS pLink = NULL;
    ULONG ulNamespaceLen = LwRtlWC16StringNumChars(pNamespace->pwszNamespace);

    while ((pLink = LwListTraverse(&gDfsNamespaces, pLink)))
    {
        PRDR_DFS_NAMESPACE pInsertPoint = LW_STRUCT_FROM_FIELD(pLink, RDR_DFS_NAMESPACE, Link);

        if (ulNamespaceLen > LwRtlWC16StringNumChars(pInsertPoint->pwszNamespace))
        {
            /* Insert here */
            LwListInsertBefore(pLink, &pNamespace->Link);
            return;
        }
    }

    /* Insert at end of list */
    LwListInsertTail(&gDfsNamespaces, &pNamespace->Link);
}

NTSTATUS
RdrDfsResolvePath(
    PCWSTR pwszPath,
    USHORT usTry,
    PWSTR* ppwszResolved,
    PBOOLEAN pbIsRoot
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PRDR_DFS_NAMESPACE pNamespace = FALSE;
    time_t now = 0;

    if (time(&now) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    LWIO_LOCK_MUTEX(bLocked, &gDfsLock);

    /* Clear expired cache entries before we search it */
    RdrDfsExpireNamespaces((ULONG) now);

    pNamespace = RdrDfsFindMatchingNamespace(pwszPath);

    if (!pNamespace)
    {
        status = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    if (pNamespace->usReferralCount == 0)
    {
        /* Negative cache entry -- path resolves to itself */
        status = LwRtlWC16StringDuplicate(ppwszResolved, pwszPath);
        BAIL_ON_NT_STATUS(status);

        *pbIsRoot = FALSE;
        goto cleanup;
    }

    if (usTry >= pNamespace->usReferralCount)
    {
        /* We've run out of referrals to try */
        status = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    status = LwRtlWC16StringAllocatePrintfW(
        ppwszResolved,
        L"%ws%ws",
        pNamespace->pReferrals[usTry].pwszReferral,
        pwszPath + LwRtlWC16StringNumChars(pNamespace->pwszNamespace));
    BAIL_ON_NT_STATUS(status);

    *pbIsRoot = pNamespace->pReferrals[usTry].bIsRoot;

cleanup:

    LWIO_UNLOCK_MUTEX(bLocked, &gDfsLock);

    return status;

error:

    *ppwszResolved = NULL;
    *pbIsRoot = FALSE;

    goto cleanup;
}

/* NULL pResponse indicates desire for negative cache entry */
NTSTATUS
RdrDfsRegisterNamespace(
    PCWSTR pwszPath,
    PDFS_RESPONSE_HEADER pResponse,
    ULONG ulResponseSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PRDR_DFS_NAMESPACE pNamespace = NULL;
    PDFS_REFERRAL_V4_NORMAL pReferral4 = NULL;
    PBYTE pHeader = (PBYTE) pResponse;
    PBYTE pCursor = NULL;
    PWSTR pwszReferral = NULL;
    PWSTR pwszEnd = NULL;
    USHORT usIndex = 0;
    USHORT usSize = 0;
    time_t now = 0;

    if (time(&now) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    status = LW_RTL_ALLOCATE_AUTO(&pNamespace);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pNamespace->pwszNamespace, pwszPath);
    BAIL_ON_NT_STATUS(status);

    if (pResponse)
    {
        /* Parse positive entry */

        /* Check for overflow */
        if (ulResponseSize < sizeof(*pResponse))
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        SMB_HTOL16_INPLACE(pResponse->usNumReferrals);
        SMB_HTOL16_INPLACE(pResponse->usPathConsumed);
        SMB_HTOL32_INPLACE(pResponse->ulFlags);

        /* Check for consumed bytes sanity/overflow */
        if (pResponse->usPathConsumed % 2 ||
            pResponse->usPathConsumed / 2 > LwRtlWC16StringNumChars(pwszPath))
        {
            status = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(status);
        }

        pNamespace->pwszNamespace[pResponse->usPathConsumed / 2] = '\0';
        pNamespace->usReferralCount = pResponse->usNumReferrals;

        status = LW_RTL_ALLOCATE_ARRAY_AUTO(&pNamespace->pReferrals, pNamespace->usReferralCount);
        BAIL_ON_NT_STATUS(status);

        for (pCursor = pHeader + sizeof(*pResponse), usIndex = 0;
            usIndex < pNamespace->usReferralCount;
            usIndex++, pCursor = pCursor + usSize)
        {
            if (pCursor + sizeof(*pReferral4) >= pHeader + ulResponseSize)
            {
                status = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(status);
            }

            pReferral4 = (PDFS_REFERRAL_V4_NORMAL) pCursor;

            SMB_HTOL16_INPLACE(pReferral4->Base.usFlags);
            SMB_HTOL32_INPLACE(pReferral4->Base.ulTimeToLive);
            SMB_HTOL16_INPLACE(pReferral4->Base.usServerType);
            SMB_HTOL16_INPLACE(pReferral4->Base.usSize);
            SMB_HTOL16_INPLACE(pReferral4->Base.usVersionNumber);

            SMB_HTOL16_INPLACE(pReferral4->usDfsAlternatePathOffset);
            SMB_HTOL16_INPLACE(pReferral4->usDfsPathOffset);
            SMB_HTOL16_INPLACE(pReferral4->usNetworkAddressOffset);

            pwszReferral = (PWSTR) ((PBYTE) pReferral4 + pReferral4->usNetworkAddressOffset);

            if ((PBYTE) (pwszReferral + 1) >= pHeader + ulResponseSize)
            {
                status = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(status);
            }

            for (pwszEnd = pwszReferral; *pwszEnd; pwszEnd++)
            {
                /* Byte swap string while looking for the end */
                SMB_HTOL16_INPLACE(*pwszEnd);
                if ((PBYTE) (pwszEnd + 1) >= pHeader + ulResponseSize)
                {
                    status = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(status);
                }
            }

            status = LwRtlWC16StringDuplicate(&pNamespace->pReferrals[usIndex].pwszReferral, pwszReferral);
            BAIL_ON_NT_STATUS(status);

            pNamespace->pReferrals[usIndex].bIsRoot = pReferral4->Base.usServerType == DFS_TARGET_ROOT;

            if (pNamespace->ulExpirationTime == 0)
            {
                pNamespace->ulExpirationTime = (ULONG) now + pReferral4->Base.ulTimeToLive;
            }
            else if (pNamespace->ulExpirationTime != (ULONG) now + pReferral4->Base.ulTimeToLive)
            {
                /* All entries should have the same expiration time */
                status = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(status);
            }

        }
    }
    else
    {
        /* Negative cache entry */
        pNamespace->ulExpirationTime = (ULONG) now + NEGATIVE_CACHE_TIME;
    }

    LWIO_LOCK_MUTEX(bLocked, &gDfsLock);

    if (RdrDfsFindPreciseNamespace(pNamespace->pwszNamespace))
    {
        /* Two threads raced to create the same entry. */
        RdrDfsFreeNamespace(pNamespace);
        pNamespace = NULL;
    }
    else
    {
        RdrDfsInsertNamespace(pNamespace);
        pNamespace = NULL;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bLocked, &gDfsLock);

    return status;

error:

    if (pNamespace)
    {
        RdrDfsFreeNamespace(pNamespace);
    }

    goto cleanup;
}
