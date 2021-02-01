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
