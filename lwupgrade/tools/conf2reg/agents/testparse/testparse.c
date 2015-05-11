/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2009
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
 * Copyright (C) Likewise Software 2009
 * All rights reserved.
 *
 * Authors:
 *
 */
#include "includes.h"

DWORD
TestParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    );

static
DWORD
TestParseConfigSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (bSectionStart)
    {
        fprintf(stdout, "[%s] {\n", pszSectionName);
    }
    else
    {
        fprintf(stdout, "} //%s\n", pszSectionName);
    }
    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
TestParseConfigNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    fprintf(stdout, "\"%s\"=\"%s\"\n", pszName, pszValue);

    *pbContinue = TRUE;

    return dwError;
}


DWORD
TestParseConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;

    dwError = UpParseConfigFile(
                pszConfFile,
                &TestParseConfigSectionHandler,
                &TestParseConfigNameValuePair,
                NULL);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

