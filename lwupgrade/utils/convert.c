/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name: convert.c
 *
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

DWORD
UpStringToMultiString(
    PCSTR pszIn,
    PCSTR pszDelims,
    PSTR *ppszOut
    )
{
    DWORD dwError = 0;
    BOOLEAN bPreviousCharacterIsDelimiter = FALSE;
    DWORD i = 0;
    DWORD j = 0;
    PSTR pszCompactIn = NULL;

    // Make a copy of the string, reserving enough space for terminator.
    dwError = LwAllocateMemory(strlen(pszIn) + 2, (PVOID*)&pszCompactIn);
    BAIL_ON_UP_ERROR(dwError);

    memcpy(pszCompactIn, pszIn, strlen(pszIn) + 1);

    // First, remove all whitespace from the string.
    //dwError = LwAllocateString(pszIn, &pszCompactIn);
    //BAIL_ON_UP_ERROR(dwError);


    i = 0;
    j = 0;
    while (pszCompactIn[i])
    {
        if (!isspace((int)pszCompactIn[i]))
        {
            pszCompactIn[j++] = pszCompactIn[i];
        }

        i++;
    }
    pszCompactIn[j] = '\0';

    // Second, remove all 'empty' strings.
    bPreviousCharacterIsDelimiter = FALSE;
    i = 0;
    j = 0;
    while(pszCompactIn[i])
    {
        BOOLEAN bCharacterIsDelimiter = FALSE;

        if (strchr(pszDelims, pszCompactIn[i]))
        {
            bCharacterIsDelimiter = TRUE;
        }

        // Don't want to delimiters in a row.
        if (!(bPreviousCharacterIsDelimiter && bCharacterIsDelimiter))
        {
            pszCompactIn[j++] = pszCompactIn[i];
        }

        bPreviousCharacterIsDelimiter = bCharacterIsDelimiter;
        i++;
    }
    pszCompactIn[j++] = '\0';


    // Finally, replace all delmiters with '\0'.
    i = 0;
    while (pszCompactIn[i])
    {
        if (strchr(pszDelims, pszCompactIn[i]))
        {
            pszCompactIn[i] = '\0';
        }
        i++;
    }
    pszCompactIn[i+1] = '\0';

cleanup:

    *ppszOut = pszCompactIn;
    return dwError;

error:

    LW_SAFE_FREE_STRING(pszCompactIn);
    goto cleanup;
}

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char** endptr,
    int base
    )
{
#if defined(HAVE___STRTOLL)
    return __strtoll(nptr, endptr, base);
#else
#error strtoll support is not available
#endif
}

#endif /* defined(HAVE_STRTOLL) */
