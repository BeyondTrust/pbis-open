/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nfssvc_netshareenum.c
 *
 * Abstract:
 *
 *        Likewise Server Service (nfssvc) RPC client and server
 *
 *        NfsSvcNetShareEnum server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NfsSvcNetShareEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out, ref] */ UINT32 *level,
    /* [in, out, ref] */ nfssvc_NetShareCtr *ctr,
    /* [in] */ UINT32 preferred_maximum_length,
    /* [out] */ UINT32 *total_entries,
    /* [in, out] */ UINT32 *resume_handle
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = NFS_DEVCTL_ENUM_SHARE;
    wchar16_t wszDriverName[] = NFS_DRIVER_NAME_W;
    IO_FILE_NAME filename =
                        {
                              .RootFileHandle = NULL,
                              .FileName = &wszDriverName[0],
                              .IoNameOptions = 0
                        };
    SHARE_INFO_ENUM_PARAMS EnumParamsIn = { 0 };
    PSHARE_INFO_ENUM_PARAMS pEnumParamsOut = NULL;
    nfssvc_NetShareCtr0 *ctr0 = NULL;
    nfssvc_NetShareCtr1 *ctr1 = NULL;
    nfssvc_NetShareCtr2 *ctr2 = NULL;
    nfssvc_NetShareCtr501 *ctr501 = NULL;
    nfssvc_NetShareCtr502 *ctr502 = NULL;

    EnumParamsIn.dwInfoLevel = *level;

    ntStatus = LwShareInfoMarshalEnumParameters(
                        &EnumParamsIn,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &hFile,
                        NULL,
                        &IoStatusBlock,
                        &filename,
                        NULL,
                        NULL,
                        DesiredAccess,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(dwOutLength, (void**)&pOutBuffer);
    BAIL_ON_NFSSVC_ERROR(dwError);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &IoStatusBlock,
                    IoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength
                    );    

    while (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        /* We need more space in output buffer to make this call */

        LW_SAFE_FREE_MEMORY(pOutBuffer);
        dwOutLength *= 2;

        dwError = LwAllocateMemory(dwOutLength, (void**)&pOutBuffer);
        BAIL_ON_NFSSVC_ERROR(dwError);

        ntStatus = NtDeviceIoControlFile(
                        hFile,
                        NULL,
                        &IoStatusBlock,
                        IoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength
                        );
    }

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalEnumParameters(
                        pOutBuffer,
                        dwOutLength,
                        &pEnumParamsOut
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pEnumParamsOut->dwInfoLevel) {

    case 0:
        ctr0 = ctr->ctr0;
        ctr0->count = pEnumParamsOut->dwNumEntries;

        dwError = NfsSvcNfsAllocateMemory(
                            sizeof(*ctr0->array) * ctr0->count,
                            (void**)&ctr0->array
                            );
        BAIL_ON_NFSSVC_ERROR(dwError);
        memcpy((void*)ctr0->array, (void*)pEnumParamsOut->info.p0,
               sizeof(*ctr0->array) * ctr0->count);
        break;

    case 1:
        ctr1 = ctr->ctr1;
        ctr1->count = pEnumParamsOut->dwNumEntries;

        dwError = NfsSvcNfsAllocateMemory(
                            sizeof(*ctr1->array) * ctr1->count,
                            (void**)&ctr1->array
                            );
        BAIL_ON_NFSSVC_ERROR(dwError);
        memcpy((void*)ctr1->array, (void*)pEnumParamsOut->info.p1,
               sizeof(*ctr1->array) * ctr1->count);
        break;

    case 2:
        ctr2 = ctr->ctr2;
        ctr2->count = pEnumParamsOut->dwNumEntries;

        dwError = NfsSvcNfsAllocateMemory(
                            sizeof(*ctr2->array) * ctr2->count,
                            (void**)&ctr2->array
                            );
        BAIL_ON_NFSSVC_ERROR(dwError);
        memcpy((void*)ctr2->array, (void*)pEnumParamsOut->info.p2,
               sizeof(*ctr2->array) * ctr2->count);
        break;

    case 501:
        ctr501 = ctr->ctr501;
        ctr501->count = pEnumParamsOut->dwNumEntries;

        dwError = NfsSvcNfsAllocateMemory(
                            sizeof(*ctr501->array) * ctr501->count,
                            (void**)&ctr501->array
                            );
        BAIL_ON_NFSSVC_ERROR(dwError);
        memcpy((void*)ctr501->array, (void*)pEnumParamsOut->info.p501,
               sizeof(*ctr501->array) * ctr501->count);
        break;

    case 502:
        ctr502 = ctr->ctr502;
        ctr502->count = pEnumParamsOut->dwNumEntries;

        dwError = NfsSvcNfsAllocateMemory(
                            sizeof(*ctr502->array) * ctr502->count,
                            (void**)&ctr502->array
                            );
        BAIL_ON_NFSSVC_ERROR(dwError);
        memcpy((void*)ctr502->array, (void*)pEnumParamsOut->info.p502,
               sizeof(*ctr502->array) * ctr502->count);
        break;
    
    default:

        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *level         = pEnumParamsOut->dwInfoLevel;
    *total_entries = pEnumParamsOut->dwNumEntries;

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);
    LW_SAFE_FREE_MEMORY(pEnumParamsOut);

    return dwError;

error:
    if (pEnumParamsOut)
    {        
        switch (pEnumParamsOut->dwInfoLevel) {
        case 0:
            NfsSvcNfsFreeMemory(ctr0->array);
            break;

        case 1:
            NfsSvcNfsFreeMemory(ctr1->array);
            break;

        case 2:
            NfsSvcNfsFreeMemory(ctr2->array);
            break;

        case 501:
            NfsSvcNfsFreeMemory(ctr501->array);
            break;

        case 502:
            NfsSvcNfsFreeMemory(ctr502->array);
            break;
        default:

            NFSSVC_LOG_ERROR("Unsupported info level [%u]",
                             pEnumParamsOut->dwInfoLevel);
            break;
        }
    }

    *level         = 0;
    *total_entries = 0;

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
