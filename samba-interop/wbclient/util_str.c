/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *  Copyright (C) BeyondTrust Software. All rights reserved.
 *
 *  Module Name:
 *
 *     util_str.c
 *
 *  Abstract:
 *
 *        Utility String functions
 *
 *  Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */
#include "lsawbclient_p.h"
#include "util_str.h"
#include <ctype.h>
#include <string.h>

/**********************************************************
 * Convert a string to upper case.  modify the input string
 */

VOID
StrUpper(
    PSTR pszString
    )
{
    CHAR *p = pszString;

    while (p && *p) {
        *p = toupper(*p);
        p++;
    }

    return;
}


/***********************************************************
 * Is pszStr1 a substring of pszStr2 ?
 */

BOOLEAN
StrEqual(
    PCSTR pszStr1,
    PCSTR pszStr2
    )
{
    PSTR pszCopy1 = NULL;
    PSTR pszCopy2 = NULL;
    BOOLEAN bEqual = FALSE;

    /* If same pointer, then must be equal */

    if (pszStr1 == pszStr2)
        return TRUE;

    /* If either is NULL, cannot be substrings */

    if (!pszStr1 || !pszStr2)
        return FALSE;

    /* Check lengths */

    if (strlen(pszStr1) != strlen(pszStr2))
        return FALSE;

    /* Now copy, convert to upper case, and compare */

    pszCopy1 = _wbc_strdup(pszStr1);
    if (!pszCopy1)
    {
        goto cleanup;
    }

    pszCopy2 = _wbc_strdup(pszStr2);
    if (!pszCopy2)
    {
        goto cleanup;
    }

    StrUpper(pszCopy1);
    StrUpper(pszCopy2);

    if (strcmp(pszCopy1, pszCopy2) == 0) {
        bEqual = TRUE;
    }

cleanup:
    _WBC_FREE(pszCopy1);
    _WBC_FREE(pszCopy2);

    return bEqual;
}

/***********************************************************
 */

BOOLEAN
StrnEqual(
    PCSTR pszStr1,
    PCSTR pszStr2,
    DWORD dwChars
    )
{
    DWORD dwLen1, dwLen2;
    PSTR pszCopy1 = NULL;
    PSTR pszCopy2 = NULL;
    BOOLEAN bResult = FALSE;

    /* If same pointer, then must be equal */

    if (pszStr1 == pszStr2)
        return TRUE;

    /* If either is NULL, cannot be substrings */

    if (!pszStr1 || !pszStr2)
        return FALSE;

    dwLen1 = strlen(pszStr1);
    dwLen2 = strlen(pszStr2);

    pszCopy1 = _wbc_strdup(pszStr1);
    if (!pszCopy1)
    {
        goto cleanup;
    }

    pszCopy2 = _wbc_strdup(pszStr2);
    if (!pszCopy2)
    {
        goto cleanup;
    }

    if (dwLen1 > dwChars) {
        *(pszCopy1 + dwChars) = '\0';
    }
    if (dwLen2 > dwChars) {
        *(pszCopy2 + dwChars) = '\0';
    }

    bResult = StrEqual(pszCopy1, pszCopy2);

cleanup:
    _WBC_FREE(pszCopy1);
    _WBC_FREE(pszCopy2);

    return bResult;
}


