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
 * Abstract: Multistring type utilities
 *
 */

#include "includes.h"

/**
* Deep copy a multistring value.
*
* @param[in] pppszNewStrings Pointer to newly allocated string array
* @param[out] ppszOriginalStrings Multistring to copy
*
* @return STATUS_SUCCESS, or appropriate error.
*/
NTSTATUS
LwIoMultiStringCopy(
    PSTR **pppszNewStrings,
    PCSTR *ppszOriginalStrings
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR *ppszNewStrings = NULL;
    DWORD dwIndex = 0;
    DWORD dwSize = 0;

    if (ppszOriginalStrings == NULL)
    {
        goto cleanup;
    }

    /* First pass: Get size of the MultiSz array. */
    for (dwIndex = 0; ppszOriginalStrings[dwIndex] != NULL; dwIndex++);

    dwSize = dwIndex;

    /* Allocate new MultiSz array for dwSize entries. */
    ntError = RTL_ALLOCATE((PVOID*)&ppszNewStrings, VOID,
        sizeof(PSTR) * (dwSize + 1));
    BAIL_ON_NT_STATUS(ntError);

    /* Duplicate each string into the new array. */
    for (dwIndex = 0; dwIndex < dwSize; dwIndex++)
    {
        ntError = RtlCStringDuplicate(&ppszNewStrings[dwIndex],
            ppszOriginalStrings[dwIndex]);
        BAIL_ON_NT_STATUS(ntError);
    }

    *pppszNewStrings = ppszNewStrings;

 cleanup:
    return ntError;

 error:
    LwIoMultiStringFree(&ppszNewStrings);

    goto cleanup;
}

/**
 * Free a multistring value.
 *
 * It is safe to pass NULL to this function.
 *
 * @param[in] ppszStrings Multistring to free
 */
VOID
LwIoMultiStringFree(
    PSTR **pppszStrings
    )
{
    size_t idx = 0;
    if (!pppszStrings || !*pppszStrings)
    {
        return;
    }
    while (*pppszStrings[idx])
    {
        LwRtlMemoryFree(*pppszStrings[idx++]);
    }
    LwRtlMemoryFree(*pppszStrings);
    *pppszStrings = NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
