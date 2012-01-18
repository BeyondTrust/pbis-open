/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lwnet-futils.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        File Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LWNetReadNextLine(
    FILE* fp,
    PSTR *output,
    PBOOLEAN pbEndOfFile
    )
{
    DWORD dwError = 0;
    PVOID pBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwMaxSize = 100;

    *pbEndOfFile = 0;
    *output = NULL;

    dwError = LWNetAllocateMemory(
                  dwMaxSize,
                  &pBuffer);
    BAIL_ON_LWNET_ERROR(dwError);

    while(1)
    {
        if(fgets(pBuffer + dwSize,
                dwMaxSize - dwSize, fp) ==  NULL)
        {
            if (feof(fp)) {
                *pbEndOfFile = 1;
            } else {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LWNET_ERROR(dwError);
            }
        }
        dwSize += strlen(pBuffer + dwSize);

        if (*pbEndOfFile)
        {
            break;
        }
        if (dwSize == dwMaxSize - 1 &&
                ((char *)pBuffer)[dwSize - 1] != '\n')
        {
            dwMaxSize *= 2;
            dwError = LWNetReallocMemory(
                          pBuffer,
                          &pBuffer,
                          dwMaxSize);
            BAIL_ON_LWNET_ERROR(dwError);
        }
        else
        {
            break;
        }
    }

    ((char *)pBuffer)[dwSize] = 0;

    *output = pBuffer;
    pBuffer = NULL;

cleanup:

    LWNET_SAFE_FREE_MEMORY(pBuffer);

    return dwError;

error:

    goto cleanup;
}
