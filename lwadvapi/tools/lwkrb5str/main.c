/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Advanced API (lwadvapi)
 *
 *        Tool to generate krb5 error strings
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <krb5.h>
#include "lwdef.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwerror.h"
#include "krb5error-table.h"
#include "bail.h"

#define LW_PRINTF_STRING(x) ((x) ? (x) : "<null>")

static
DWORD
FileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size);

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PCSTR pszOutName = "lwerror-table-krb5.h";
    FILE * pFileOut = NULL;
    PCSTR pszKrb5Prefix = "LW_ERROR_KRB5";
    PCSTR pszFormat = NULL;
    PCSTR pszName = NULL;
    PCSTR pszFormat1 = "S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_%s, -1, \"%s\" )\n";
    PCSTR pszFormat2 = "S ( LW_STATUS_MESSAGE_NOT_FOUND, LW_ERROR_KRB5_%s, -1, \"%s\" )\n";
    PCSTR pszWinName = NULL;
    PSTR pszLine = NULL;
    DWORD i = 0;
    krb5_context ctx = NULL;
    PCSTR pszKrb5Error = NULL;

    pFileOut = fopen(pszOutName, "w");
    if (!pFileOut)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_LW_ERROR(dwError);

    pszLine = "/*\n";
    dwError  = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
    BAIL_ON_LW_ERROR(dwError);

    pszLine = " * lwerror-table-krb5.h is generated from krb5error-table.h\n";
    dwError  = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
    BAIL_ON_LW_ERROR(dwError);

    pszLine = " * using lwkrb5str.  Do not modify lwerror-table-krb5.h\n";
    dwError  = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
    BAIL_ON_LW_ERROR(dwError);

    pszLine = " * directly.  Modify krb5error-table.h, then run lwkrb5str.\n";
    dwError  = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
    BAIL_ON_LW_ERROR(dwError);

    pszLine = " */\n\n";
    dwError  = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
    BAIL_ON_LW_ERROR(dwError);
    pszLine = NULL;

    krb5_init_context(&ctx);
    if (!ctx)
    {
        fprintf(stderr, "Failed to obtain Kerberos context.\n");
        goto error;
    }

    for (i = 0 ; krb5err_lwerr_map[i].pszKrb5errStr; i++)
    {
        pszWinName = LwWin32ExtErrorToName(krb5err_lwerr_map[i].lwerr);
        if (pszWinName &&
            (strncmp(pszWinName, pszKrb5Prefix, strlen(pszKrb5Prefix)) != 0))
        {
            // mapped to non-Kerberos error
            continue;
        }

        switch (krb5err_lwerr_map[i].krb5err)
        {
            case KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN:
                pszName = "KRB5_S_PRINCIPAL_UNKNOWN";
                break;
            default:
                pszName = krb5err_lwerr_map[i].pszKrb5errStr;
        }

        if (strncmp(pszName, "KRB5", strlen("KRB5")) == 0)
        {
            pszFormat = pszFormat1;
        }
        else
        {
            pszFormat = pszFormat2;
        }

        pszKrb5Error = krb5_get_error_message(
                            ctx,
                            krb5err_lwerr_map[i].krb5err);
        if (!pszKrb5Error)
        {
            fprintf(stderr, "Failed to obtain string for %s (%d).\n",
                    pszName, krb5err_lwerr_map[i].krb5err);
        }

        dwError = LwAllocateStringPrintf(
                      &pszLine,
                      pszFormat,
                      pszName,
                      pszKrb5Error ? pszKrb5Error : "");
        BAIL_ON_LW_ERROR(dwError);

        dwError = FileStreamWrite(pFileOut, pszLine, strlen(pszLine));
        BAIL_ON_LW_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszLine);
        krb5_free_error_message(ctx, pszKrb5Error);
        pszKrb5Error = NULL;
    }

cleanup:

    if (pszKrb5Error)
    {
        krb5_free_error_message(ctx, pszKrb5Error);
    }
    if (ctx)
    {
        krb5_free_context(ctx);
    }
    if (pFileOut)
    {
        fclose(pFileOut);
    }

    LW_SAFE_FREE_STRING(pszLine);

    return (dwError);

error:

    fprintf(
        stderr,
        "Failed to generate Kerberos error string table.  Error code %u (%s).\n%s\n",
        dwError,
        LW_PRINTF_STRING(LwNtStatusToName(dwError)),
        LW_PRINTF_STRING(LwNtStatusToDescription(dwError)));

    goto cleanup;
}

static
DWORD
FileStreamWrite(
    FILE* handle,
    PCSTR data,
    unsigned int size)
{
    DWORD dwError = ERROR_SUCCESS;

    unsigned int written = 0;

    written = fwrite(data, 1, size, handle);
    if (written < size)
    {
        dwError = LwMapErrnoToLwError(errno);
        fprintf(stderr, "Failed to write string %d: %s\n", dwError, data);
    }

    return dwError;
}

