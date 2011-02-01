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

static
NTSTATUS
RdrDfsChaseReferral(
    PRDR_SOCKET pSocket,
    PRDR_OP_CONTEXT pContext
    );

static
BOOLEAN
RdrDfsTreeConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
BOOLEAN
RdrDfsChaseReferralTreeConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

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

/* Call with lock held */
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
    RDR_DFS_REFERRAL temp = {0};

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
        status = usTry > 0 ? STATUS_DFS_UNAVAILABLE : STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

    if (pNamespace->usReferralCount == 0)
    {
        /*
         * Negative cache entry -- path resolves to itself
         * If usTry > 0, we are out of luck
         */
        if (usTry > 0)
        {
            status = STATUS_DFS_UNAVAILABLE;
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlWC16StringDuplicate(ppwszResolved, pwszPath);
        BAIL_ON_NT_STATUS(status);

        *pbIsRoot = FALSE;
        goto cleanup;
    }

    if (usTry >= pNamespace->usReferralCount)
    {
        /* We've run out of referrals to try */
        status = STATUS_DFS_UNAVAILABLE;
        BAIL_ON_NT_STATUS(status);
    }

    status = LwRtlWC16StringAllocatePrintfW(
        ppwszResolved,
        L"%ws%ws",
        pNamespace->pReferrals[usTry].pwszReferral,
        pwszPath + LwRtlWC16StringNumChars(pNamespace->pwszNamespace));
    BAIL_ON_NT_STATUS(status);

    *pbIsRoot = pNamespace->pReferrals[usTry].bIsRoot;

    /*
     * Swap the entry into index 0.  This has the effect of making the first
     * working entry we try the default for any future lookups.
     */
    temp = pNamespace->pReferrals[0];
    pNamespace->pReferrals[0] = pNamespace->pReferrals[usTry];
    pNamespace->pReferrals[usTry] = temp;

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

            usSize = pReferral4->Base.usSize;
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

static
BOOLEAN
RdrDfsStatusIsRetriable(
    NTSTATUS status
    )
{
    switch (status)
    {
    case STATUS_SUCCESS:
    case STATUS_PENDING:
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return FALSE;
    default:
        return TRUE;
    }
}

NTSTATUS
RdrDfsConnectAttempt(
    PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare = NULL;
    PWSTR pwszResolved = NULL;
    BOOLEAN bIsRoot = FALSE;
    BOOLEAN bStopOnDfs = FALSE;

    do
    {
        RTL_FREE(&pwszServer);
        RTL_FREE(&pwszShare);
        RTL_FREE(&pwszResolved);
        RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);
        RTL_FREE(pContext->State.DfsConnect.ppwszCanonicalPath);

        status = RdrConvertUnicodeStringPath(
            pContext->State.DfsConnect.pPath,
            &pwszServer,
            &pwszShare,
            pContext->State.DfsConnect.ppwszFilePath);
        BAIL_ON_NT_STATUS(status);

        status = RdrConstructCanonicalPath(
            pwszShare,
            *pContext->State.DfsConnect.ppwszFilePath,
            pContext->State.DfsConnect.ppwszCanonicalPath);
        BAIL_ON_NT_STATUS(status);

        if (!RdrShareIsIpc(pwszShare))
        {
            /* Resolve against DFS cache */
            status = RdrDfsResolvePath(
                *pContext->State.DfsConnect.ppwszCanonicalPath,
                *pContext->State.DfsConnect.pusTry,
                &pwszResolved,
                &bIsRoot);
            switch (status)
            {
            case STATUS_DFS_UNAVAILABLE:
                status = pContext->State.DfsConnect.OrigStatus;
                BAIL_ON_NT_STATUS(status);
            case STATUS_NOT_FOUND:
                /* Proceed with current path verbatim but back off if the server supports DFS */
                bStopOnDfs = TRUE;
                status = STATUS_SUCCESS;
                break;
            default:
                BAIL_ON_NT_STATUS(status);

                /*
                 * Since we got a hit in our referral cache,
                 * we don't need to chase referrals when tree connecting
                 */
                bStopOnDfs = FALSE;

                RTL_FREE(&pwszServer);
                RTL_FREE(&pwszShare);
                RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);

                status = RdrConvertPath(
                    pwszResolved,
                    &pwszServer,
                    &pwszShare,
                    pContext->State.DfsConnect.ppwszFilePath);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else if (*pContext->State.DfsConnect.pusTry > 0)
        {
            /* No point in retrying connections to IPC$ */
            status = pContext->State.DfsConnect.OrigStatus;
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            bStopOnDfs = FALSE;
        }

        pContext->Continue = RdrDfsTreeConnectComplete;

        (*pContext->State.DfsConnect.pusTry)++;
        status = RdrTreeConnect(
            pwszServer,
            pwszShare,
            pContext->State.DfsConnect.pCreds,
            pContext->State.DfsConnect.Uid,
            bStopOnDfs,
            pContext);
        if (!NT_SUCCESS(status) && status != STATUS_PENDING
            && !pContext->State.DfsConnect.OrigStatus)
        {
            pContext->State.DfsConnect.OrigStatus = status;
        }
    } while (RdrDfsStatusIsRetriable(status));
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszServer);
    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszResolved);

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrDfsTreeConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    switch (status)
    {
    case STATUS_DFS_EXIT_PATH_FOUND:
        *pContext->State.DfsConnect.pusTry = 0;
        pContext->State.DfsConnect.OrigStatus = STATUS_SUCCESS;
        status = RdrDfsChaseReferral(NULL, pContext);
        break;
    default:
        if (RdrDfsStatusIsRetriable(status))
        {
            if (!pContext->State.DfsConnect.OrigStatus)
            {
                pContext->State.DfsConnect.OrigStatus = status;
            }
            status = RdrDfsConnectAttempt(pContext);
        }
    }
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        if (status != STATUS_SUCCESS)
        {
            RTL_FREE(pContext->State.DfsConnect.ppwszCanonicalPath);
            RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);
        }
        RdrContinueContext(pContext->State.DfsConnect.pContinue, status, pParam);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}

static
NTSTATUS
RdrDfsChaseReferral(
    PRDR_SOCKET pSocket,
    PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwszServer = NULL;
    PWSTR pwszShare = NULL;

    if (pSocket)
    {
        /*
         * If given an existing socket, send the DFS referral request to that server.
         * This is necessary for resolving DFS links after getting STATUS_PATH_NOT_COVERED,
         */
        status = LwRtlWC16StringDuplicate(&pwszServer, pSocket->pwszCanonicalName);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = RdrConvertUnicodeStringPath(
            pContext->State.DfsConnect.pPath,
            &pwszServer,
            NULL,
            NULL);
        BAIL_ON_NT_STATUS(status);


    }

    status = LwRtlWC16StringAllocatePrintfW(&pwszShare, L"\\\\%ws\\IPC$", pwszServer);
    BAIL_ON_NT_STATUS(status);

    pContext->Continue = RdrDfsChaseReferralTreeConnectComplete;

    status = RdrTreeConnect(
        pwszServer,
        pwszShare,
        pContext->State.DfsConnect.pCreds,
        pContext->State.DfsConnect.Uid,
        FALSE,
        pContext);
    BAIL_ON_NT_STATUS(status);

 cleanup:

     RTL_FREE(&pwszServer);
     RTL_FREE(&pwszShare);

     return status;

 error:

     goto cleanup;
}

static
BOOLEAN
RdrDfsChaseReferralTreeConnectComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    BAIL_ON_NT_STATUS(status);

    switch (RDR_OBJECT_PROTOCOL(pParam))
    {
    case SMB_PROTOCOL_VERSION_1:
        status = RdrDfsChaseReferral1(pContext, (PRDR_TREE) pParam);
        break;
    case SMB_PROTOCOL_VERSION_2:
        status = RdrDfsChaseReferral2(pContext, (PRDR_TREE2) pParam);
        break;
    default:
        status = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        RTL_FREE(pContext->State.DfsConnect.ppwszCanonicalPath);
        RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);
        RdrContinueContext(pContext->State.DfsConnect.pContinue, status, NULL);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}

NTSTATUS
RdrDfsConnect(
    IN OPTIONAL PRDR_SOCKET pSocket,
    IN PUNICODE_STRING pPath,
    IN PIO_CREDS pCreds,
    IN uid_t Uid,
    IN NTSTATUS lastError,
    IN OUT PUSHORT pusTry,
    OUT PWSTR* ppwszFilePath,
    OUT PWSTR* ppwszCanonicalPath,
    IN PRDR_OP_CONTEXT pContinue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(NULL, &pContext);
    BAIL_ON_NT_STATUS(status);

    pContext->State.DfsConnect.pCreds = pCreds;
    pContext->State.DfsConnect.Uid = Uid;
    pContext->State.DfsConnect.pPath = pPath;
    pContext->State.DfsConnect.ppwszFilePath = ppwszFilePath;
    pContext->State.DfsConnect.ppwszCanonicalPath = ppwszCanonicalPath;
    pContext->State.DfsConnect.pusTry = pusTry;
    pContext->State.DfsConnect.pContinue = pContinue;

    switch (lastError)
    {
    case STATUS_SUCCESS:
        status = RdrDfsConnectAttempt(pContext);
        BAIL_ON_NT_STATUS(status);
        break;
    case STATUS_PATH_NOT_COVERED:
        *pContext->State.DfsConnect.pusTry = 0;
        pContext->State.DfsConnect.OrigStatus = lastError;
        status = RdrDfsChaseReferral(pSocket, pContext);
        BAIL_ON_NT_STATUS(status);
        break;
    default:
        if (RdrDfsStatusIsRetriable(lastError))
        {
            pContext->State.DfsConnect.OrigStatus = lastError;
            status = RdrDfsConnectAttempt(pContext);
        }
        else
        {
            status = lastError;
        }
        BAIL_ON_NT_STATUS(status);
    }

cleanup:

    if (status != STATUS_PENDING)
    {
        if (pContext)
        {
            RTL_FREE(pContext->State.DfsConnect.ppwszCanonicalPath);
            RTL_FREE(pContext->State.DfsConnect.ppwszFilePath);
            RdrFreeContext(pContext);
        }
    }

    return status;

error:

    goto cleanup;
}
