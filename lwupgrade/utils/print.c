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

DWORD
UpPrintBoolean(
    FILE *fp,
    PCSTR pszName,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;
    PSTR pszBoolean = NULL;

    dwError = UpFormatBoolean(pszName, bValue, &pszBoolean);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs(pszBoolean, fp) < 0 )
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);


cleanup:
    UpFreeString(pszBoolean);
    return dwError;

error:
    goto cleanup;
}

DWORD
UpPrintDword(
    FILE *fp,
    PCSTR pszName,
    DWORD dwValue
    )
{
    DWORD dwError = 0;
    PSTR pszDword = NULL;

    dwError = UpFormatDword(pszName, dwValue, &pszDword);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs(pszDword, fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    UpFreeString(pszDword);
    return dwError;

error:
    goto cleanup;
}

DWORD
UpPrintString(
    FILE *fp,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszString = NULL;

    dwError = UpFormatString(pszName, pszValue, &pszString);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs(pszString, fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    UpFreeString(pszString);
    return dwError;

error:
    goto cleanup;
}

DWORD
UpPrintMultiString(
    FILE *fp,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszString = NULL;

    dwError = UpFormatMultiString(pszName, pszValue, &pszString);
    BAIL_ON_UP_ERROR(dwError);

    if (fputs(pszString, fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    UpFreeString(pszString);
    return dwError;

error:
    goto cleanup;
}

