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
 *        lwnet-futils.c
 *
 * Abstract:
 *
 *        BeyondTrust Site Manager
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
