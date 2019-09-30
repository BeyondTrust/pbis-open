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
 * Module Name:
 *
 *        path.c
 *
 * Abstract:
 *
 *        LWIO Redirector
 *
 *        Common path handling code
 *
 * Author: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "rdr.h"

#define IS_SEPARATOR(c) ((c) == '\\' || (c) == '/')

NTSTATUS
RdrConvertUnicodeStringPath(
    PUNICODE_STRING pIoPath,
    PWSTR* ppwszHost,
    PWSTR* ppwszShare,
    PWSTR* ppwszFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PCWSTR pwszIoPath = NULL;
    PWSTR pwszFreeIoPath = NULL;
    PWSTR pwszHost = NULL;
    PWSTR pwszShare = NULL;
    PWSTR pwszFile = NULL;

    if (RTL_STRING_IS_NULL_TERMINATED(pIoPath))
    {
        pwszIoPath = pIoPath->Buffer;
    }
    else
    {
        status = LwRtlWC16StringAllocateFromUnicodeString(
                        &pwszFreeIoPath, pIoPath);
        BAIL_ON_NT_STATUS(status);

        pwszIoPath = pwszFreeIoPath;
    }

    status = RdrConvertPath(
                    pwszIoPath,
                    ppwszHost ? &pwszHost : NULL,
                    ppwszShare ? &pwszShare : NULL,
                    ppwszFile ? &pwszFile : NULL);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszFreeIoPath);

    if (ppwszHost)
    {
        *ppwszHost = pwszHost;
    }
    if (ppwszShare)
    {
        *ppwszShare = pwszShare;
    }
    if (ppwszFile)
    {
        *ppwszFile = pwszFile;
    }

    return status;

error:

    RTL_FREE(&pwszHost);
    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszFile);

    goto cleanup;
}

NTSTATUS
RdrConvertPath(
    PCWSTR pwszIoPath,
    PWSTR* ppwszHost,
    PWSTR* ppwszShare,
    PWSTR* ppwszFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    enum
    {
        STATE_START,
        STATE_HOST,
        STATE_SHARE,
        STATE_FILE
    } state = STATE_START;
    ULONG ulSrcIndex = 0;
    ULONG ulDstIndex = 0;
    ULONG ulHostIndex = 0;
    PWSTR pwszHost = NULL;
    PWSTR pwszShare = NULL;
    PWSTR pwszFile = NULL;


    for (ulSrcIndex = 0; pwszIoPath[ulSrcIndex]; ulSrcIndex++)
    {
        switch (state)
        {
        case STATE_START:
            status = RTL_ALLOCATE(
                &pwszHost,
                WCHAR,
                (LwRtlWC16StringNumChars(pwszIoPath) + 1) * sizeof(WCHAR));
            BAIL_ON_NT_STATUS(status);
            if (!IS_SEPARATOR(pwszIoPath[ulSrcIndex]))
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }

            while (IS_SEPARATOR(pwszIoPath[ulSrcIndex+1]))
            {
                ulSrcIndex++;
            }
            ulDstIndex = 0;
            state = STATE_HOST;
            break;
        case STATE_HOST:
            if (IS_SEPARATOR(pwszIoPath[ulSrcIndex]))
            {
                status = RTL_ALLOCATE(
                    &pwszShare,
                    WCHAR,
                    (LwRtlWC16StringNumChars(pwszHost) +
                     LwRtlWC16StringNumChars(pwszIoPath + ulSrcIndex) + 4) * sizeof(WCHAR));
                BAIL_ON_NT_STATUS(status);
                ulDstIndex = 0;
                pwszShare[ulDstIndex++] = '\\';
                pwszShare[ulDstIndex++] = '\\';
                for (ulHostIndex = 0; pwszHost[ulHostIndex] && pwszHost[ulHostIndex] != '@'; ulHostIndex++)
                {
                    pwszShare[ulDstIndex++] = pwszHost[ulHostIndex];
                }
                pwszShare[ulDstIndex++] = '\\';
                state = STATE_SHARE;
            }
            else
            {
                pwszHost[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        case STATE_SHARE:
            if (IS_SEPARATOR(pwszIoPath[ulSrcIndex]))
            {
                status = RTL_ALLOCATE(
                    &pwszFile,
                    WCHAR,
                    (LwRtlWC16StringNumChars(pwszIoPath + ulSrcIndex) + 1) * sizeof(WCHAR));
                BAIL_ON_NT_STATUS(status);
                ulDstIndex = 0;
                pwszFile[ulDstIndex++] = '\\';
                state = STATE_FILE;
            }
            else
            {
                pwszShare[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        case STATE_FILE:
            if (IS_SEPARATOR(pwszIoPath[ulSrcIndex]))
            {
                if (!IS_SEPARATOR(pwszFile[ulDstIndex-1]))
                {
                    pwszFile[ulDstIndex++] = '\\';
                }
            }
            else
            {
                pwszFile[ulDstIndex++] = pwszIoPath[ulSrcIndex];
            }
            break;
        }
    }

    if (!pwszHost || !pwszShare)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    if (!pwszFile)
    {
        status = LwRtlWC16StringAllocateFromCString(&pwszFile, "\\");
        BAIL_ON_NT_STATUS(status);
    }

    if (ppwszHost)
    {
        *ppwszHost = pwszHost;
    }
    else
    {
        RTL_FREE(&pwszHost);
    }

    if (ppwszShare)
    {
        *ppwszShare = pwszShare;
    }
    else
    {
        RTL_FREE(&pwszShare);
    }

    if (ppwszFile)
    {
        *ppwszFile = pwszFile;
    }
    else
    {
        RTL_FREE(&pwszFile);
    }

cleanup:

    return status;

error:

    RTL_FREE(&pwszHost);
    RTL_FREE(&pwszShare);
    RTL_FREE(&pwszFile);

    goto cleanup;
}

NTSTATUS
RdrConstructCanonicalPath(
    PWSTR pwszShare,
    PWSTR pwszFilename,
    PWSTR* ppwszCanonical
    )
{
    if (pwszFilename[0] == '\\' &&
        pwszFilename[1] == '\0')
    {
        return LwRtlWC16StringDuplicate(ppwszCanonical, pwszShare);
    }
    else
    {
        return LwRtlWC16StringAllocatePrintfW(
            ppwszCanonical,
            L"%ws%ws",
            pwszShare,
            pwszFilename);
    }
}

BOOLEAN
RdrShareIsIpc(
    PWSTR pwszShare
    )
{
    static const WCHAR wszIpcDollar[] = {'I','P','C','$','\0'};
    ULONG ulLen = LwRtlWC16StringNumChars(pwszShare);

    return (ulLen >= 4 && LwRtlWC16StringIsEqual(pwszShare + ulLen - 4, wszIpcDollar, FALSE));
}

