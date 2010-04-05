#include "includes.h"

static
VOID
NfsCCBFree(
    PNFS_CCB pCCB
    );

NTSTATUS
NfsCCBCreate(
    PNFS_IRP_CONTEXT pIrpContext,
    PNFS_CCB*        ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_CCB pCCB = NULL;

    ntStatus = NfsAllocateMemory(sizeof(NFS_CCB), (PVOID*)&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = NFS_CCB_DEVICE;
    pCCB->refCount = 1;

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:

    *ppCCB = NULL;

    goto cleanup;
}

NTSTATUS
NfsCCBGet(
    IO_FILE_HANDLE FileHandle,
    PNFS_CCB*      ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNFS_CCB pCCB = NULL;

    pCCB = (PNFS_CCB)IoFileGetContext(FileHandle);
    if (!pCCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:

    *ppCCB = NULL;

    goto cleanup;
}

NTSTATUS
NfsCCBSet(
    IO_FILE_HANDLE FileHandle,
    PNFS_CCB       pCCB
    )
{
    return IoFileSetContext(FileHandle, pCCB);
}

VOID
NfsCCBRelease(
    PNFS_CCB pCCB
    )
{
    if (InterlockedDecrement(&pCCB->refCount) == 0)
    {
        NfsCCBFree(pCCB);
    }
}

static
VOID
NfsCCBFree(
    PNFS_CCB pCCB
    )
{
    NfsFreeMemory(pCCB);
}

