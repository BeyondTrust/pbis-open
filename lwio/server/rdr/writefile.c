/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        writefile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        WriteFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "rdr.h"

#define WRITE_DATA_OFFSET     133

NTSTATUS
RdrWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwNumBytesWritten = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;
    BOOLEAN bFileIsLocked = FALSE;
    ULONG ulWriteMax = 0;

    LWIO_LOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    ulWriteMax = pFile->pTree->pSession->pSocket->maxBufferSize - WRITE_DATA_OFFSET;
    if (ulWriteMax > UINT16_MAX)
    {
        ulWriteMax = UINT16_MAX;
    }

    do
    {
        uint16_t wBytesToWrite = (uint16_t) ulWriteMax;
        uint16_t wBytesWritten = 0;
        USHORT usWriteMode = 0;

        if (dwNumBytesToWrite < ulWriteMax)
        {
            wBytesToWrite = (uint16_t) dwNumBytesToWrite;
        }

        /* If file is a named pipe in message mode and this is the
           first WriteAndX, set the "start of message" bit in the write
           mode */
        if (pFile->usFileType == 0x2 && dwNumBytesWritten == 0)
        {
            usWriteMode |= 0x8;
        }

        ntStatus = WireWriteFile(
                    pFile->pTree,
                    pFile->fid,
                    pFile->llOffset,
                    usWriteMode,
                    pBuffer + dwNumBytesWritten,
                    wBytesToWrite,
                    &wBytesWritten,
                    NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        pFile->llOffset += wBytesWritten;
        dwNumBytesWritten += wBytesWritten;
        dwNumBytesToWrite -= wBytesWritten;

    } while (dwNumBytesToWrite);

error:

    LWIO_UNLOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    *pdwNumBytesWritten = dwNumBytesWritten;

    return ntStatus;
}
