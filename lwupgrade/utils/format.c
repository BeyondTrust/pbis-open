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
 * Abstract:
 *
 * Authors:
 * 
 */
#include "includes.h"

VOID
UpFreeString(PSTR pszStr)
{
    LwFreeString(pszStr);
}


static
DWORD
UpAllocateEscapedString(PCSTR pszStr, PSTR *ppszEscapedStr)
{
    DWORD dwError = 0;
    const PCSTR pszEscapeChars = "\\\"";
    DWORD i, j;
    PSTR pszEscapedStr = NULL;
    DWORD dwCount = 0;

    if (pszStr == NULL )
        goto cleanup;

    for (i = 0; pszStr[i]; i++)
    {
        if (strchr(pszEscapeChars, pszStr[i]) != NULL)
            dwCount++;
        dwCount++;
    }
    dwCount++;

    dwError = LwAllocateMemory(dwCount, (PVOID)&pszEscapedStr);
    BAIL_ON_UP_ERROR(dwError);

    for (i = 0, j = 0; pszStr[i]; i++)
    {
        if (strchr(pszEscapeChars, pszStr[i]) != NULL)
        {
            pszEscapedStr[j++] = '\\';
        }
        pszEscapedStr[j++] = pszStr[i];
    }
    pszEscapedStr[j] = '\0';

cleanup:
    *ppszEscapedStr = pszEscapedStr;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pszEscapedStr);
    goto cleanup;
}

DWORD
UpFormatBoolean(
    PCSTR pszName,
    BOOLEAN bValue,
    PSTR *ppszBoolean
    )
{
    DWORD dwError = 0;
    PSTR pszBoolean = NULL;
    char szDword[13];

    snprintf(szDword, sizeof(szDword), "%08X", (DWORD) (bValue ? 1 : 0));
    dwError = LwAllocateStringPrintf(
                &pszBoolean,
                "\"%s\"=dword:%s\n",
                pszName,
                szDword);
    BAIL_ON_UP_ERROR(dwError);

    *ppszBoolean = pszBoolean;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszBoolean);
    goto cleanup;    
}

DWORD
UpFormatDword(
    PCSTR pszName,
    DWORD dwValue,
    PSTR *ppszDword
    )
{
    DWORD dwError = 0;
    PSTR pszDword = NULL;

    dwError = LwAllocateStringPrintf(
                &pszDword,
                "\"%s\"=dword:%08X\n",
                pszName,
                dwValue);
    BAIL_ON_UP_ERROR(dwError);

    *ppszDword = pszDword;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDword);
    goto cleanup;
}

DWORD
UpFormatString(
    PCSTR pszName,
    PCSTR pszValue,
    PSTR *ppszString
    )
{
    DWORD dwError = 0;
    PSTR pszString = NULL;
    PSTR pszEscapedString = NULL;

    dwError = UpAllocateEscapedString(pszValue, &pszEscapedString);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                &pszString,
                "\"%s\"=\"%s\"\n",
                pszName,
                pszEscapedString ? pszEscapedString : "");
    BAIL_ON_UP_ERROR(dwError);

    *ppszString = pszString;

cleanup:
    LW_SAFE_FREE_STRING(pszEscapedString);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszString);
    goto cleanup;
}

DWORD
UpFormatMultiString(
    PCSTR pszName,
    PCSTR pszValue,
    PSTR *ppszMultiString
    )
{
    DWORD dwError = 0;
    PCSTR pszMultiStringSegment = NULL;
    PSTR pszEscapedMultiStringSegment = NULL;
    PSTR pszEscapedMultiString = NULL;
    PSTR *ppszEscapedMultiStrings = NULL;
    DWORD dwSegments = 0;
    DWORD i = 0;
    DWORD dwNeededLength = 0;

    // Count components of string.
    pszMultiStringSegment = pszValue;
    while (pszMultiStringSegment && *pszMultiStringSegment)
    {
        dwSegments++;

        pszMultiStringSegment += strlen(pszMultiStringSegment) + 1;
    }

    dwError = LwAllocateMemory(
                sizeof(PSTR*) * dwSegments,
                (PVOID*)&ppszEscapedMultiStrings);
    BAIL_ON_UP_ERROR(dwError);

    pszMultiStringSegment = pszValue;
    i = 0;
    while (pszMultiStringSegment && *pszMultiStringSegment)
    {
        dwError = UpAllocateEscapedString(
                    pszMultiStringSegment,
                    &pszEscapedMultiStringSegment);
        BAIL_ON_UP_ERROR(dwError);

        // 3 = the room for two double quotes and one space.
        dwNeededLength += strlen(pszEscapedMultiStringSegment) + 3;

        ppszEscapedMultiStrings[i++] = pszEscapedMultiStringSegment;
        pszEscapedMultiStringSegment = NULL;

        pszMultiStringSegment += strlen(pszMultiStringSegment) + 1;
    }
    dwNeededLength += strlen("\"") + strlen(pszName) + strlen("\"=sza:\"\"") +
        strlen("\"\"") + strlen("\n") + 1;

    dwError = LwAllocateMemory(dwNeededLength, (PVOID*)&pszEscapedMultiString);
    BAIL_ON_UP_ERROR(dwError);

    strcat(pszEscapedMultiString, "\"");
    strcat(pszEscapedMultiString, pszName);
    strcat(pszEscapedMultiString, "\"=sza:");

    for (i = 0; i < dwSegments; i++)
    {
        strcat(pszEscapedMultiString, "\"");
        strcat(pszEscapedMultiString, ppszEscapedMultiStrings[i]);
        strcat(pszEscapedMultiString, "\" ");
    }

    strcat(pszEscapedMultiString, "\"\"");
    strcat(pszEscapedMultiString, "\n");

cleanup:

    *ppszMultiString = pszEscapedMultiString;

    for (i = 0; i < dwSegments; i++)
    {
        LW_SAFE_FREE_STRING(ppszEscapedMultiStrings[i]);
    }
    LW_SAFE_FREE_MEMORY(ppszEscapedMultiStrings);
    return dwError;

error:

    LW_SAFE_FREE_STRING(pszEscapedMultiStringSegment);
    LW_SAFE_FREE_STRING(pszEscapedMultiString);
    goto cleanup;
}

