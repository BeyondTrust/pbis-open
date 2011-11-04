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

