/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
